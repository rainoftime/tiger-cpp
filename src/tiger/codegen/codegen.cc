/**
 * @file codegen.cc
 * @brief x86-64 instruction selection implementation (maximal munch)
 *
 * This file implements the Munch() methods for all IR tree nodes.
 * Each method pattern-matches the IR tree and emits the corresponding
 * x86-64 assembly instructions into the instruction list.
 *
 * The maximal munch algorithm greedily selects the largest matching
 * instruction pattern at each node, minimising the number of instructions.
 *
 * Assembly string format:
 *   `s0, `s1, ... – source (Use) temporaries
 *   `d0, `d1, ... – destination (Def) temporaries
 *
 * All instructions use AT&T syntax (src, dst operand order).
 */

#include "tiger/codegen/codegen.h"

//#include <cassert>
#include <sstream>
#include <iostream>

extern frame::RegManager *reg_manager;

namespace {

constexpr int maxlen = 1024;
/** @brief Machine word size in bytes (x86-64) */
constexpr int wordsize = 8;

} // namespace

namespace cg {

void CodeGen::Codegen() {
  tree::StmList *stm_list = traces_.get()->GetStmList();
  assem::InstrList *instr_list = new assem::InstrList();

  // Translate each IR statement into assembly instructions via maximal munch
  for (auto stm : stm_list->GetList())
    stm->Munch(*instr_list, fs_);

  // Append a return-sink pseudo-instruction to keep callee-saved registers
  // and %rax live at the end of the function (required for liveness analysis)
  instr_list = frame::ProcEntryExit2(instr_list);
  assem_instr_ = std::make_unique<AssemInstr>(instr_list);
}

void AssemInstr::Print(FILE *out, temp::Map *map) const {
  for (auto instr : instr_list_->GetList())
    instr->Print(out, map);
  fprintf(out, "\n");
}
} // namespace cg

namespace tree {
/**
 * Normalize a MEM expression into the addressing form expected by x86-64
 * load/store instructions.
 *
 * The helper recognizes the common base+constant pattern and emits the textual
 * address fragment together with the temp list that must be recorded as Uses.
 * The `ordinal` parameter tells the caller which source slot in the enclosing
 * instruction should correspond to the base register (`s0`, `s1`, ...).
 *
 * If the IR memory expression is not a direct base+constant form, we first
 * materialize the address into a temp and then use the generic `(`sN)` form.
 */
static assem::MemFetch *MunchMem(tree::Exp* mem_exp, int ordinal, 
                                 assem::InstrList &instr_list, std::string_view fs) {
  tree::Exp *exp = static_cast<tree::MemExp *>(mem_exp)->exp_;
  std::stringstream mem_ss;
  if (typeid(*exp) == typeid(tree::BinopExp) 
      && static_cast<tree::BinopExp *>(exp)->op_ == tree::PLUS_OP) {
    tree::BinopExp *bin_exp = static_cast<tree::BinopExp *>(exp);
    if (typeid(*bin_exp->right_) == typeid(tree::ConstExp)) {
      tree::ConstExp *offset_exp = static_cast<tree::ConstExp *>(bin_exp->right_);
      temp::Temp *base_reg = bin_exp->left_->Munch(instr_list, fs);
      if (offset_exp->consti_ == 0)
        mem_ss << "(`s" << ordinal << ")";
      else
        mem_ss << offset_exp->consti_ <<  "(`s" << ordinal << ")";
      return new assem::MemFetch(mem_ss.str(), new temp::TempList(base_reg));

    } else if (typeid(*bin_exp->left_) == typeid(tree::ConstExp)) {
      tree::ConstExp *offset_exp = static_cast<tree::ConstExp *>(bin_exp->left_);
      temp::Temp *base_reg = bin_exp->right_->Munch(instr_list, fs);
      if (offset_exp->consti_ == 0)
        mem_ss << "(`s" << ordinal << ")";
      else
        mem_ss << offset_exp->consti_ <<  "(`s" << ordinal << ")";
      return new assem::MemFetch(mem_ss.str(), new temp::TempList(base_reg));

    } else {
      temp::Temp* mem_reg = exp->Munch(instr_list, fs);
      mem_ss << "(`s" << ordinal << ")";
      return new assem::MemFetch(mem_ss.str(), new temp::TempList(mem_reg));
    }

 } else {
   temp::Temp* mem_reg = exp->Munch(instr_list, fs);
    mem_ss << "(`s" << ordinal << ")";
    return new assem::MemFetch(mem_ss.str(), new temp::TempList(mem_reg));
  }
}

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  left_->Munch(instr_list, fs);
  right_->Munch(instr_list, fs);
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  instr_list.Append(new assem::LabelInstr(label_->Name(), label_));
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  std::string instr_str = "jmp " + exp_->name_->Name();
  instr_list.Append(new assem::OperInstr(instr_str, nullptr, nullptr, 
                      new assem::Targets(jumps_)));
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* `cmpq` in AT&T syntax is "cmp source, dest" and conceptually compares
     dest - source. We therefore emit `cmpq rhs, lhs` so the jump mnemonic
     still corresponds to the original IR predicate `lhs op rhs`. */
  std::stringstream instr_ss;

  if (typeid(*right_) == typeid(tree::ConstExp)) {
    tree::ConstExp *right_const = static_cast<tree::ConstExp *>(right_);
    temp::Temp *left_reg = left_->Munch(instr_list, fs);
    instr_ss << "cmpq $" << right_const->consti_ << ", `s0";
    instr_list.Append(new assem::OperInstr(instr_ss.str(), nullptr, 
                        new temp::TempList(left_reg), nullptr));

  } else {
    temp::Temp *left_reg = left_->Munch(instr_list, fs);
    temp::Temp *right_reg = right_->Munch(instr_list, fs);
    instr_list.Append(new assem::OperInstr("cmpq `s0, `s1", nullptr,
                        new temp::TempList({right_reg, left_reg}), nullptr));
  }

  instr_ss.str("");

  switch (op_) {
  case EQ_OP:
    instr_ss << "je " << true_label_->Name(); break;
  case NE_OP:
    instr_ss << "jne " << true_label_->Name(); break;
  case LT_OP:
    instr_ss << "jl " << true_label_->Name(); break;
  case GT_OP:
    instr_ss << "jg " << true_label_->Name(); break;
  case LE_OP:
    instr_ss << "jle " << true_label_->Name(); break;
  case GE_OP:
    instr_ss << "jge " << true_label_->Name(); break;
  default:
    return;  // error
  }
  instr_list.Append(new assem::OperInstr(instr_ss.str(), nullptr, nullptr,
                      new assem::Targets(new std::vector<temp::Label *>{true_label_})));
                      
}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  std::stringstream instr_ss;

  if (typeid(*dst_) == typeid(tree::MemExp)) {  // movq r, M
    // x86-64 does not support a generic memory-to-memory move here, so the
    // source is always materialized into a temp before we compute the store.
    temp::Temp *src_reg = src_->Munch(instr_list, fs);
    assem::MemFetch *fetch = MunchMem(dst_, 1, instr_list, fs);
    instr_ss << "movq `s0, " << fetch->fetch_;
    temp::TempList *src_regs = new temp::TempList(src_reg);
    src_regs->CatList(fetch->regs_);
    instr_list.Append(new assem::OperInstr(instr_ss.str(), 
                        nullptr, src_regs, nullptr));

  } else {
    if (typeid(*src_) == typeid(tree::MemExp)) {  // movq M, r
      assem::MemFetch *fetch = MunchMem(src_, 0, instr_list, fs);
      temp::Temp *dst_reg = dst_->Munch(instr_list, fs);
      instr_ss << "movq " << fetch->fetch_ << ", `d0";
      instr_list.Append(new assem::OperInstr(instr_ss.str(),
                          new temp::TempList(dst_reg),
                          fetch->regs_, nullptr));

    } else if (typeid(*src_) == typeid(tree::ConstExp)) {  // movq $imm, r
      temp::Temp *dst_reg = dst_->Munch(instr_list, fs);
      instr_ss << "movq $" << static_cast<tree::ConstExp *>(src_)->consti_
               << ", `d0";
      instr_list.Append(new assem::OperInstr(instr_ss.str(), 
                          new temp::TempList(dst_reg),
                          nullptr, nullptr));

    } else {  // movq r1, r2
      temp::Temp *src_reg = src_->Munch(instr_list, fs);
      temp::Temp *dst_reg = dst_->Munch(instr_list, fs);
      instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
                          new temp::TempList(dst_reg),
                          new temp::TempList(src_reg)));
    }
  }
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  exp_->Munch(instr_list, fs);
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  std::string assem_instr;
  std::stringstream instr_ss;

  // leaq fs(r1), r2
  // frame pointer
  if (op_ == PLUS_OP && (typeid(*right_) == typeid(tree::NameExp))
      && static_cast<tree::NameExp *>(right_)->name_->Name() == fs) {
      temp::Temp *left_reg = left_->Munch(instr_list, fs);
      temp::Temp *res_reg = temp::TempFactory::NewTemp();
      instr_ss << "leaq " << fs << "(`s0), `d0";
      instr_list.Append(new assem::OperInstr(instr_ss.str(), 
                          new temp::TempList(res_reg), 
                          new temp::TempList(left_reg), 
                          nullptr));
      return res_reg;
  }

  if (op_ == PLUS_OP || op_ == MINUS_OP) {

    if (op_ == PLUS_OP)
      assem_instr = "addq";
    else
      assem_instr = "subq";
  
    temp::Temp *left_reg = left_->Munch(instr_list, fs);
    temp::Temp* res_reg = temp::TempFactory::NewTemp();

    // x86 arithmetic is destructive on the destination operand. Copy the left
    // operand first so the IR value model stays "expression returns a fresh
    // temp" instead of mutating an existing one in place.
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", 
                        new temp::TempList(res_reg),
                        new temp::TempList(left_reg)));

    if (typeid(*right_) == typeid(tree::ConstExp)) {  // instr $imm, r
      tree::ConstExp *right_const = static_cast<tree::ConstExp *>(right_);
      instr_ss << assem_instr << " $" << right_const->consti_ << ", `d0";
      instr_list.Append(new assem::OperInstr(instr_ss.str(),
                          new temp::TempList({res_reg}),
                          new temp::TempList(res_reg),
                          nullptr));
      return res_reg;

    } else {  // instr r1, r2
      temp::Temp *right_reg = right_->Munch(instr_list, fs);
      instr_ss << assem_instr << " `s1, `d0";
      instr_list.Append(new assem::OperInstr(instr_ss.str(),
                          new temp::TempList({res_reg}),
                          new temp::TempList({res_reg, right_reg}),
                          nullptr));
      return res_reg;
    }

  } else if (op_ == MUL_OP || op_ == DIV_OP) {

    if (op_ == MUL_OP)
      assem_instr = "imulq";
    else
      assem_instr = "idivq";

    temp::Temp *rax = reg_manager->ReturnValue();
    temp::Temp *rdx = reg_manager->ArithmeticAssistant();
    temp::Temp *rax_saver = temp::TempFactory::NewTemp();
    temp::Temp *rdx_saver = temp::TempFactory::NewTemp();

    // `imulq`/`idivq` have fixed register conventions: `%rax` carries the
    // primary operand/result and `%rdx` holds the high half / remainder.
    // We save both machine registers into fresh temps so surrounding code can
    // still treat them like ordinary values before register allocation.
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", 
                        new temp::TempList(rax_saver), 
                        new temp::TempList(rax)));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", 
                        new temp::TempList(rdx_saver), 
                        new temp::TempList(rdx)));

    if (typeid(*left_) == typeid(tree::ConstExp)) {  // movq $imm, %rax
      tree::ConstExp *left_const = static_cast<tree::ConstExp *>(left_);
      instr_ss << "movq $" << left_const->consti_ << ", `d0";
      instr_list.Append(new assem::OperInstr(instr_ss.str(),
                          new temp::TempList(rax), 
                          nullptr, nullptr));

    } else if (typeid(*left_) == typeid(tree::MemExp)) {  // movq M, %rax
      assem::MemFetch *fetch = MunchMem(left_, 0, instr_list, fs);
      instr_ss << "movq " << fetch->fetch_ << ", `d0";
      instr_list.Append(new assem::OperInstr(instr_ss.str(),
                          new temp::TempList(rax),
                          fetch->regs_, nullptr));

    } else {  // movq r, %rax
      temp::Temp *left_reg = left_->Munch(instr_list, fs);
      instr_ss << "movq `s0, `d0";
      instr_list.Append(new assem::MoveInstr(instr_ss.str(),
                          new temp::TempList(rax), 
                          new temp::TempList(left_reg)));
    }

    instr_ss.str("");

    if (op_ == DIV_OP)
      // Signed division consumes the 128-bit dividend in `%rdx:%rax`, so we
      // explicitly sign-extend `%rax` into `%rdx` before issuing `idivq`.
      instr_list.Append(new assem::OperInstr("cqto", 
                          new temp::TempList({rdx, rax, rax_saver, rdx_saver}),
                          new temp::TempList(rax),
                          nullptr));
    
    temp::Temp *right_reg = right_->Munch(instr_list, fs);
    instr_ss << assem_instr << " `s2";
    instr_list.Append(new assem::OperInstr(instr_ss.str(),
                        new temp::TempList({rdx, rax, rax_saver, rdx_saver}),
                        new temp::TempList({rdx, rax, right_reg}),
                        nullptr));

    // Move the result to a new register.
    temp::Temp *res_reg = temp::TempFactory::NewTemp();
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", 
                        new temp::TempList(res_reg), 
                        new temp::TempList(rax)));

    // Restore %rax and %rdx
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", 
                        new temp::TempList(rax), 
                        new temp::TempList(rax_saver)));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", 
                        new temp::TempList(rdx), 
                        new temp::TempList(rdx_saver)));

    return res_reg;
  }

  return temp::TempFactory::NewTemp();  // error

}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  temp::Temp *reg = temp::TempFactory::NewTemp();
  assem::MemFetch* fetch = MunchMem(this, 0, instr_list, fs);
  std::stringstream instr_ss;
  instr_ss << "movq " << fetch->fetch_ << ", `d0";
  instr_list.Append(new assem::OperInstr(instr_ss.str(), 
                      new temp::TempList(reg), 
                      fetch->regs_, nullptr));
  return reg;
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  return temp_;
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  stm_->Munch(instr_list, fs);
  return exp_->Munch(instr_list, fs);
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  temp::Temp *reg = temp::TempFactory::NewTemp();
  std::stringstream instr_ss;
  // load address
  instr_ss << "leaq " << name_->Name() << "(%rip), `d0";
  assem::Instr *instr = new assem::OperInstr(instr_ss.str(), 
                          new temp::TempList(reg), 
                          nullptr, nullptr);
  instr_list.Append(instr);
  return reg;
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  temp::Temp *reg = temp::TempFactory::NewTemp();
  std::stringstream instr_ss;
  instr_ss << "movq $" << consti_ << ", `d0";
  assem::Instr *instr = new assem::OperInstr(instr_ss.str(), 
                          new temp::TempList(reg), 
                          nullptr, nullptr);
  instr_list.Append(instr);
  return reg;
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  temp::Temp *rax = reg_manager->ReturnValue();
  std::stringstream instr_ss;

  if (typeid(*fun_) != typeid(tree::NameExp))  // error
    return rax; 

  // `MunchArgs` performs the System V marshaling (arg registers first, then
  // stack slots) and returns the machine locations that the call should record
  // as Uses for liveness.
  temp::TempList *arg_list = args_->MunchArgs(instr_list, fs);
  temp::TempList *calldefs = reg_manager->CallerSaves();
  calldefs->Append(reg_manager->ReturnValue());

  instr_ss << "callq " << static_cast<tree::NameExp *>(fun_)->name_->Name();
  instr_list.Append(new assem::OperInstr(instr_ss.str(), 
                      calldefs, arg_list, nullptr));
  return rax;
}

temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list, std::string_view fs) {
  temp::TempList *arg_list = new temp::TempList();
  std::stringstream instr_ss;
  int arg_reg_count = reg_manager->ArgRegs()->GetList().size();
  int i = 0;

  for (tree::Exp *arg : this->GetList()) {  // static link is included
    if (i < arg_reg_count) {
      temp::Temp *dst_reg = reg_manager->ArgRegs()->NthTemp(i);
      if (typeid(*arg) == typeid(tree::ConstExp)) {
        tree::ConstExp *const_exp = static_cast<tree::ConstExp *>(arg);
        instr_ss << "movq $" << const_exp->consti_ << ", `d0";
        instr_list.Append(new assem::OperInstr(instr_ss.str(), 
                            new temp::TempList(dst_reg), nullptr, nullptr));
      } else {
        temp::Temp *src_reg = arg->Munch(instr_list, fs);
        instr_ss << "movq `s0, `d0";
        instr_list.Append(new assem::MoveInstr(instr_ss.str(), 
                            new temp::TempList(dst_reg), 
                            new temp::TempList(src_reg)));
      }
      arg_list->Append(dst_reg);

    } else {  // the first one goes to (%rsp)
      // Extra arguments are written into the outgoing-argument area that the
      // frame layer reserved below `%rsp` before the call sequence.
      if (typeid(*arg) == typeid(tree::ConstExp)) {
        tree::ConstExp *const_exp = static_cast<tree::ConstExp *>(arg);
        instr_ss << "movq $" << const_exp->consti_ << ", ";
        if (i != arg_reg_count)
          instr_ss << (i - arg_reg_count) * wordsize;
        instr_ss << "(" << *reg_manager->temp_map_->Look(reg_manager->StackPointer())
                 << ")";
        instr_list.Append(new assem::OperInstr(instr_ss.str(), 
                            nullptr, new temp::TempList(reg_manager->StackPointer()), nullptr));
      } else {
        temp::Temp *src_reg = arg->Munch(instr_list, fs);
        instr_ss << "movq `s0, ";
        if (i != arg_reg_count)
          instr_ss << (i - arg_reg_count) * wordsize;
        instr_ss << "(" << *reg_manager->temp_map_->Look(reg_manager->StackPointer())
                 << ")";
        instr_list.Append(new assem::OperInstr(instr_ss.str(), 
                            nullptr,
                            new temp::TempList({src_reg, reg_manager->StackPointer()}), 
                            nullptr));
      }
    }
    instr_ss.str("");
    ++i;
  }

  // If any stack arguments were emitted, model `%rsp` as a call use as well:
  // the call depends on the outgoing argument area staying intact.
  if (i > arg_reg_count)
    arg_list->Append(reg_manager->StackPointer());

  return arg_list;
}

} // namespace tree
