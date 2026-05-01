#include "tiger/codegen/codegen.h"

#include <cstdint>
#include <sstream>

extern frame::RegManager *reg_manager;

namespace {

constexpr int wordsize = 8;

bool IsArm64Target() { return frame::IsArm64AppleTarget(); }

bool FitsArm64Signed9(int value) { return value >= -256 && value <= 255; }

bool FitsArm64AddSubImm(int value) { return value >= 0 && value <= 4095; }

bool FitsArm64ScaledMemOffset(int value) {
  return value >= 0 && value <= 32760 && value % wordsize == 0;
}

void EmitArm64LoadImmediate(assem::InstrList &instr_list, temp::Temp *dst,
                            int value) {
  uint64_t bits = static_cast<uint64_t>(static_cast<int64_t>(value));
  bool emitted = false;

  for (int shift = 0; shift < 64; shift += 16) {
    uint16_t chunk = static_cast<uint16_t>((bits >> shift) & 0xffffu);
    if (!emitted) {
      if (chunk == 0 && shift != 48)
        continue;
      std::stringstream instr_ss;
      instr_ss << "movz `d0, #" << chunk;
      if (shift != 0)
        instr_ss << ", lsl #" << shift;
      instr_list.Append(new assem::OperInstr(
          instr_ss.str(), new temp::TempList(dst), nullptr, nullptr));
      emitted = true;
      continue;
    }

    if (chunk == 0)
      continue;
    std::stringstream instr_ss;
    instr_ss << "movk `d0, #" << chunk;
    if (shift != 0)
      instr_ss << ", lsl #" << shift;
    instr_list.Append(new assem::OperInstr(
        instr_ss.str(), new temp::TempList(dst), new temp::TempList(dst),
        nullptr));
  }

  if (!emitted) {
    instr_list.Append(new assem::OperInstr("movz `d0, #0",
                                           new temp::TempList(dst), nullptr,
                                           nullptr));
  }
}

void EmitArm64MoveReg(assem::InstrList &instr_list, temp::Temp *dst,
                      temp::Temp *src) {
  instr_list.Append(new assem::MoveInstr("mov `d0, `s0",
                                         new temp::TempList(dst),
                                         new temp::TempList(src)));
}

struct Arm64MemFetch {
  std::string fetch_;
  temp::TempList *regs_;
  bool unscaled_;
};

Arm64MemFetch *BuildArm64MemFetch(temp::Temp *base_reg, int offset, int ordinal,
                                  assem::InstrList &instr_list) {
  std::stringstream mem_ss;
  if (offset == 0) {
    mem_ss << "[`s" << ordinal << "]";
    return new Arm64MemFetch{mem_ss.str(), new temp::TempList(base_reg), false};
  }

  if (FitsArm64ScaledMemOffset(offset)) {
    mem_ss << "[`s" << ordinal << ", #" << offset << "]";
    return new Arm64MemFetch{mem_ss.str(), new temp::TempList(base_reg), false};
  }

  if (FitsArm64Signed9(offset)) {
    mem_ss << "[`s" << ordinal << ", #" << offset << "]";
    return new Arm64MemFetch{mem_ss.str(), new temp::TempList(base_reg), true};
  }

  temp::Temp *addr_reg = temp::TempFactory::NewTemp();
  if (offset > 0 && FitsArm64AddSubImm(offset)) {
    std::stringstream instr_ss;
    instr_ss << "add `d0, `s0, #" << offset;
    instr_list.Append(new assem::OperInstr(
        instr_ss.str(), new temp::TempList(addr_reg),
        new temp::TempList(base_reg), nullptr));
  } else if (offset < 0 && FitsArm64AddSubImm(-offset)) {
    std::stringstream instr_ss;
    instr_ss << "sub `d0, `s0, #" << -offset;
    instr_list.Append(new assem::OperInstr(
        instr_ss.str(), new temp::TempList(addr_reg),
        new temp::TempList(base_reg), nullptr));
  } else {
    temp::Temp *offset_reg = temp::TempFactory::NewTemp();
    EmitArm64LoadImmediate(instr_list, offset_reg,
                           offset >= 0 ? offset : -offset);
    std::stringstream instr_ss;
    instr_ss << (offset >= 0 ? "add" : "sub") << " `d0, `s0, `s1";
    instr_list.Append(new assem::OperInstr(
        instr_ss.str(), new temp::TempList(addr_reg),
        new temp::TempList({base_reg, offset_reg}), nullptr));
  }

  mem_ss << "[`s" << ordinal << "]";
  return new Arm64MemFetch{mem_ss.str(), new temp::TempList(addr_reg), false};
}

assem::MemFetch *MunchMemX64(tree::Exp *mem_exp, int ordinal,
                             assem::InstrList &instr_list,
                             std::string_view fs) {
  tree::Exp *exp = static_cast<tree::MemExp *>(mem_exp)->exp_;
  std::stringstream mem_ss;
  if (typeid(*exp) == typeid(tree::BinopExp) &&
      static_cast<tree::BinopExp *>(exp)->op_ == tree::PLUS_OP) {
    tree::BinopExp *bin_exp = static_cast<tree::BinopExp *>(exp);
    if (typeid(*bin_exp->right_) == typeid(tree::ConstExp)) {
      auto *offset_exp = static_cast<tree::ConstExp *>(bin_exp->right_);
      temp::Temp *base_reg = bin_exp->left_->Munch(instr_list, fs);
      if (offset_exp->consti_ == 0)
        mem_ss << "(`s" << ordinal << ")";
      else
        mem_ss << offset_exp->consti_ << "(`s" << ordinal << ")";
      return new assem::MemFetch(mem_ss.str(), new temp::TempList(base_reg));
    }

    if (typeid(*bin_exp->left_) == typeid(tree::ConstExp)) {
      auto *offset_exp = static_cast<tree::ConstExp *>(bin_exp->left_);
      temp::Temp *base_reg = bin_exp->right_->Munch(instr_list, fs);
      if (offset_exp->consti_ == 0)
        mem_ss << "(`s" << ordinal << ")";
      else
        mem_ss << offset_exp->consti_ << "(`s" << ordinal << ")";
      return new assem::MemFetch(mem_ss.str(), new temp::TempList(base_reg));
    }
  }

  temp::Temp *mem_reg = exp->Munch(instr_list, fs);
  mem_ss << "(`s" << ordinal << ")";
  return new assem::MemFetch(mem_ss.str(), new temp::TempList(mem_reg));
}

Arm64MemFetch *MunchMemArm64(tree::Exp *mem_exp, int ordinal,
                             assem::InstrList &instr_list,
                             std::string_view fs) {
  tree::Exp *exp = static_cast<tree::MemExp *>(mem_exp)->exp_;
  if (typeid(*exp) == typeid(tree::BinopExp)) {
    auto *bin_exp = static_cast<tree::BinopExp *>(exp);
    if (bin_exp->op_ == tree::PLUS_OP || bin_exp->op_ == tree::MINUS_OP) {
      tree::Exp *base_exp = nullptr;
      int offset = 0;

      if (typeid(*bin_exp->right_) == typeid(tree::ConstExp)) {
        base_exp = bin_exp->left_;
        offset = static_cast<tree::ConstExp *>(bin_exp->right_)->consti_;
        if (bin_exp->op_ == tree::MINUS_OP)
          offset = -offset;
      } else if (bin_exp->op_ == tree::PLUS_OP &&
                 typeid(*bin_exp->left_) == typeid(tree::ConstExp)) {
        base_exp = bin_exp->right_;
        offset = static_cast<tree::ConstExp *>(bin_exp->left_)->consti_;
      }

      if (base_exp != nullptr) {
        temp::Temp *base_reg = base_exp->Munch(instr_list, fs);
        return BuildArm64MemFetch(base_reg, offset, ordinal, instr_list);
      }
    }
  }

  temp::Temp *addr_reg = exp->Munch(instr_list, fs);
  std::stringstream mem_ss;
  mem_ss << "[`s" << ordinal << "]";
  return new Arm64MemFetch{mem_ss.str(), new temp::TempList(addr_reg), false};
}

void EmitArm64CompareBranch(tree::Exp *left, tree::Exp *right, tree::RelOp op,
                            temp::Label *true_label,
                            assem::InstrList &instr_list,
                            std::string_view fs) {
  if (typeid(*right) == typeid(tree::ConstExp)) {
    int value = static_cast<tree::ConstExp *>(right)->consti_;
    temp::Temp *left_reg = left->Munch(instr_list, fs);
    if (value >= 0 && value <= 4095) {
      std::stringstream instr_ss;
      instr_ss << "cmp `s0, #" << value;
      instr_list.Append(new assem::OperInstr(instr_ss.str(), nullptr,
                                             new temp::TempList(left_reg),
                                             nullptr));
    } else {
      temp::Temp *right_reg = temp::TempFactory::NewTemp();
      EmitArm64LoadImmediate(instr_list, right_reg, value);
      instr_list.Append(new assem::OperInstr(
          "cmp `s0, `s1", nullptr, new temp::TempList({left_reg, right_reg}),
          nullptr));
    }
  } else {
    temp::Temp *left_reg = left->Munch(instr_list, fs);
    temp::Temp *right_reg = right->Munch(instr_list, fs);
    instr_list.Append(new assem::OperInstr(
        "cmp `s0, `s1", nullptr, new temp::TempList({left_reg, right_reg}),
        nullptr));
  }

  std::stringstream branch_ss;
  switch (op) {
  case tree::EQ_OP:
    branch_ss << "b.eq ";
    break;
  case tree::NE_OP:
    branch_ss << "b.ne ";
    break;
  case tree::LT_OP:
    branch_ss << "b.lt ";
    break;
  case tree::GT_OP:
    branch_ss << "b.gt ";
    break;
  case tree::LE_OP:
    branch_ss << "b.le ";
    break;
  case tree::GE_OP:
    branch_ss << "b.ge ";
    break;
  case tree::ULT_OP:
    branch_ss << "b.lo ";
    break;
  case tree::ULE_OP:
    branch_ss << "b.ls ";
    break;
  case tree::UGT_OP:
    branch_ss << "b.hi ";
    break;
  case tree::UGE_OP:
    branch_ss << "b.hs ";
    break;
  default:
    return;
  }
  branch_ss << true_label->Name();
  instr_list.Append(new assem::OperInstr(
      branch_ss.str(), nullptr, nullptr,
      new assem::Targets(new std::vector<temp::Label *>{true_label})));
}

} // namespace

namespace cg {

void CodeGen::Codegen() {
  tree::StmList *stm_list = traces_.get()->GetStmList();
  assem::InstrList *instr_list = new assem::InstrList();

  for (auto stm : stm_list->GetList())
    stm->Munch(*instr_list, fs_);

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

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  left_->Munch(instr_list, fs);
  right_->Munch(instr_list, fs);
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  instr_list.Append(new assem::LabelInstr(label_->Name(), label_));
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  std::string instr_str =
      IsArm64Target() ? "b " + exp_->name_->Name() : "jmp " + exp_->name_->Name();
  instr_list.Append(new assem::OperInstr(
      instr_str, nullptr, nullptr, new assem::Targets(jumps_)));
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  if (IsArm64Target()) {
    EmitArm64CompareBranch(left_, right_, op_, true_label_, instr_list, fs);
    return;
  }

  std::stringstream instr_ss;
  if (typeid(*right_) == typeid(tree::ConstExp)) {
    auto *right_const = static_cast<tree::ConstExp *>(right_);
    temp::Temp *left_reg = left_->Munch(instr_list, fs);
    instr_ss << "cmpq $" << right_const->consti_ << ", `s0";
    instr_list.Append(new assem::OperInstr(instr_ss.str(), nullptr,
                                           new temp::TempList(left_reg),
                                           nullptr));
  } else {
    temp::Temp *left_reg = left_->Munch(instr_list, fs);
    temp::Temp *right_reg = right_->Munch(instr_list, fs);
    instr_list.Append(new assem::OperInstr(
        "cmpq `s0, `s1", nullptr,
        new temp::TempList({right_reg, left_reg}), nullptr));
  }

  instr_ss.str("");
  switch (op_) {
  case EQ_OP:
    instr_ss << "je ";
    break;
  case NE_OP:
    instr_ss << "jne ";
    break;
  case LT_OP:
    instr_ss << "jl ";
    break;
  case GT_OP:
    instr_ss << "jg ";
    break;
  case LE_OP:
    instr_ss << "jle ";
    break;
  case GE_OP:
    instr_ss << "jge ";
    break;
  default:
    return;
  }
  instr_ss << true_label_->Name();
  instr_list.Append(new assem::OperInstr(
      instr_ss.str(), nullptr, nullptr,
      new assem::Targets(new std::vector<temp::Label *>{true_label_})));
}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  if (IsArm64Target()) {
    if (typeid(*dst_) == typeid(tree::MemExp)) {
      temp::Temp *src_reg = src_->Munch(instr_list, fs);
      Arm64MemFetch *fetch = MunchMemArm64(dst_, 1, instr_list, fs);
      std::stringstream instr_ss;
      instr_ss << (fetch->unscaled_ ? "stur " : "str ") << "`s0, "
               << fetch->fetch_;
      temp::TempList *src_regs = new temp::TempList(src_reg);
      src_regs->CatList(fetch->regs_);
      instr_list.Append(new assem::OperInstr(instr_ss.str(), nullptr, src_regs,
                                             nullptr));
      return;
    }

    if (typeid(*src_) == typeid(tree::MemExp)) {
      Arm64MemFetch *fetch = MunchMemArm64(src_, 0, instr_list, fs);
      temp::Temp *dst_reg = dst_->Munch(instr_list, fs);
      std::stringstream instr_ss;
      instr_ss << (fetch->unscaled_ ? "ldur " : "ldr ") << "`d0, "
               << fetch->fetch_;
      instr_list.Append(new assem::OperInstr(
          instr_ss.str(), new temp::TempList(dst_reg), fetch->regs_, nullptr));
      return;
    }

    if (typeid(*src_) == typeid(tree::ConstExp)) {
      temp::Temp *dst_reg = dst_->Munch(instr_list, fs);
      EmitArm64LoadImmediate(
          instr_list, dst_reg, static_cast<tree::ConstExp *>(src_)->consti_);
      return;
    }

    temp::Temp *src_reg = src_->Munch(instr_list, fs);
    temp::Temp *dst_reg = dst_->Munch(instr_list, fs);
    EmitArm64MoveReg(instr_list, dst_reg, src_reg);
    return;
  }

  std::stringstream instr_ss;
  if (typeid(*dst_) == typeid(tree::MemExp)) {
    temp::Temp *src_reg = src_->Munch(instr_list, fs);
    assem::MemFetch *fetch = MunchMemX64(dst_, 1, instr_list, fs);
    instr_ss << "movq `s0, " << fetch->fetch_;
    temp::TempList *src_regs = new temp::TempList(src_reg);
    src_regs->CatList(fetch->regs_);
    instr_list.Append(
        new assem::OperInstr(instr_ss.str(), nullptr, src_regs, nullptr));
    return;
  }

  if (typeid(*src_) == typeid(tree::MemExp)) {
    assem::MemFetch *fetch = MunchMemX64(src_, 0, instr_list, fs);
    temp::Temp *dst_reg = dst_->Munch(instr_list, fs);
    instr_ss << "movq " << fetch->fetch_ << ", `d0";
    instr_list.Append(new assem::OperInstr(instr_ss.str(),
                                           new temp::TempList(dst_reg),
                                           fetch->regs_, nullptr));
    return;
  }

  if (typeid(*src_) == typeid(tree::ConstExp)) {
    temp::Temp *dst_reg = dst_->Munch(instr_list, fs);
    instr_ss << "movq $" << static_cast<tree::ConstExp *>(src_)->consti_
             << ", `d0";
    instr_list.Append(new assem::OperInstr(instr_ss.str(),
                                           new temp::TempList(dst_reg),
                                           nullptr, nullptr));
    return;
  }

  temp::Temp *src_reg = src_->Munch(instr_list, fs);
  temp::Temp *dst_reg = dst_->Munch(instr_list, fs);
  instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
                                         new temp::TempList(dst_reg),
                                         new temp::TempList(src_reg)));
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  exp_->Munch(instr_list, fs);
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  if (IsArm64Target()) {
    if (op_ == PLUS_OP || op_ == MINUS_OP) {
      temp::Temp *left_reg = left_->Munch(instr_list, fs);
      temp::Temp *res_reg = temp::TempFactory::NewTemp();
      EmitArm64MoveReg(instr_list, res_reg, left_reg);

      if (typeid(*right_) == typeid(tree::ConstExp)) {
        int value = static_cast<tree::ConstExp *>(right_)->consti_;
        if (value >= 0 && FitsArm64AddSubImm(value)) {
          std::stringstream instr_ss;
          instr_ss << (op_ == PLUS_OP ? "add " : "sub ") << "`d0, `s0, #"
                   << value;
          instr_list.Append(new assem::OperInstr(
              instr_ss.str(), new temp::TempList(res_reg),
              new temp::TempList(res_reg), nullptr));
          return res_reg;
        }
      }

      temp::Temp *right_reg = right_->Munch(instr_list, fs);
      std::stringstream instr_ss;
      instr_ss << (op_ == PLUS_OP ? "add " : "sub ") << "`d0, `s0, `s1";
      instr_list.Append(new assem::OperInstr(
          instr_ss.str(), new temp::TempList(res_reg),
          new temp::TempList({res_reg, right_reg}), nullptr));
      return res_reg;
    }

    if (op_ == MUL_OP || op_ == DIV_OP) {
      temp::Temp *left_reg = left_->Munch(instr_list, fs);
      temp::Temp *right_reg = right_->Munch(instr_list, fs);
      temp::Temp *res_reg = temp::TempFactory::NewTemp();
      std::stringstream instr_ss;
      instr_ss << (op_ == MUL_OP ? "mul " : "sdiv ") << "`d0, `s0, `s1";
      instr_list.Append(new assem::OperInstr(
          instr_ss.str(), new temp::TempList(res_reg),
          new temp::TempList({left_reg, right_reg}), nullptr));
      return res_reg;
    }

    return temp::TempFactory::NewTemp();
  }

  std::string assem_instr;
  std::stringstream instr_ss;
  if (op_ == PLUS_OP && (typeid(*right_) == typeid(tree::NameExp)) &&
      static_cast<tree::NameExp *>(right_)->name_->Name() == fs) {
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
    assem_instr = op_ == PLUS_OP ? "addq" : "subq";
    temp::Temp *left_reg = left_->Munch(instr_list, fs);
    temp::Temp *res_reg = temp::TempFactory::NewTemp();
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
                                           new temp::TempList(res_reg),
                                           new temp::TempList(left_reg)));

    if (typeid(*right_) == typeid(tree::ConstExp)) {
      auto *right_const = static_cast<tree::ConstExp *>(right_);
      instr_ss << assem_instr << " $" << right_const->consti_ << ", `d0";
      instr_list.Append(new assem::OperInstr(
          instr_ss.str(), new temp::TempList({res_reg}),
          new temp::TempList(res_reg), nullptr));
      return res_reg;
    }

    temp::Temp *right_reg = right_->Munch(instr_list, fs);
    instr_ss << assem_instr << " `s1, `d0";
    instr_list.Append(new assem::OperInstr(
        instr_ss.str(), new temp::TempList({res_reg}),
        new temp::TempList({res_reg, right_reg}), nullptr));
    return res_reg;
  }

  if (op_ == MUL_OP || op_ == DIV_OP) {
    assem_instr = op_ == MUL_OP ? "imulq" : "idivq";
    temp::Temp *rax = reg_manager->ReturnValue();
    temp::Temp *rdx = reg_manager->ArithmeticAssistant();
    temp::Temp *rax_saver = temp::TempFactory::NewTemp();
    temp::Temp *rdx_saver = temp::TempFactory::NewTemp();

    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
                                           new temp::TempList(rax_saver),
                                           new temp::TempList(rax)));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
                                           new temp::TempList(rdx_saver),
                                           new temp::TempList(rdx)));

    if (typeid(*left_) == typeid(tree::ConstExp)) {
      auto *left_const = static_cast<tree::ConstExp *>(left_);
      instr_ss << "movq $" << left_const->consti_ << ", `d0";
      instr_list.Append(new assem::OperInstr(instr_ss.str(),
                                             new temp::TempList(rax), nullptr,
                                             nullptr));
    } else if (typeid(*left_) == typeid(tree::MemExp)) {
      assem::MemFetch *fetch = MunchMemX64(left_, 0, instr_list, fs);
      instr_ss << "movq " << fetch->fetch_ << ", `d0";
      instr_list.Append(new assem::OperInstr(instr_ss.str(),
                                             new temp::TempList(rax),
                                             fetch->regs_, nullptr));
    } else {
      temp::Temp *left_reg = left_->Munch(instr_list, fs);
      instr_ss << "movq `s0, `d0";
      instr_list.Append(new assem::MoveInstr(instr_ss.str(),
                                             new temp::TempList(rax),
                                             new temp::TempList(left_reg)));
    }

    instr_ss.str("");
    if (op_ == DIV_OP) {
      instr_list.Append(new assem::OperInstr(
          "cqto", new temp::TempList({rdx, rax, rax_saver, rdx_saver}),
          new temp::TempList(rax), nullptr));
    }

    temp::Temp *right_reg = right_->Munch(instr_list, fs);
    instr_ss << assem_instr << " `s2";
    instr_list.Append(new assem::OperInstr(
        instr_ss.str(), new temp::TempList({rdx, rax, rax_saver, rdx_saver}),
        new temp::TempList({rdx, rax, right_reg}), nullptr));

    temp::Temp *res_reg = temp::TempFactory::NewTemp();
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
                                           new temp::TempList(res_reg),
                                           new temp::TempList(rax)));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
                                           new temp::TempList(rax),
                                           new temp::TempList(rax_saver)));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
                                           new temp::TempList(rdx),
                                           new temp::TempList(rdx_saver)));
    return res_reg;
  }

  return temp::TempFactory::NewTemp();
}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  temp::Temp *reg = temp::TempFactory::NewTemp();
  if (IsArm64Target()) {
    Arm64MemFetch *fetch = MunchMemArm64(this, 0, instr_list, fs);
    std::stringstream instr_ss;
    instr_ss << (fetch->unscaled_ ? "ldur " : "ldr ") << "`d0, "
             << fetch->fetch_;
    instr_list.Append(new assem::OperInstr(instr_ss.str(),
                                           new temp::TempList(reg),
                                           fetch->regs_, nullptr));
    return reg;
  }

  assem::MemFetch *fetch = MunchMemX64(this, 0, instr_list, fs);
  std::stringstream instr_ss;
  instr_ss << "movq " << fetch->fetch_ << ", `d0";
  instr_list.Append(new assem::OperInstr(instr_ss.str(), new temp::TempList(reg),
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
  if (IsArm64Target()) {
    std::stringstream instr_ss;
    instr_ss << "adrp `d0, " << name_->Name() << "@PAGE";
    instr_list.Append(new assem::OperInstr(instr_ss.str(),
                                           new temp::TempList(reg), nullptr,
                                           nullptr));
    instr_ss.str("");
    instr_ss << "add `d0, `s0, " << name_->Name() << "@PAGEOFF";
    instr_list.Append(new assem::OperInstr(instr_ss.str(),
                                           new temp::TempList(reg),
                                           new temp::TempList(reg), nullptr));
    return reg;
  }

  std::stringstream instr_ss;
  instr_ss << "leaq " << name_->Name() << "(%rip), `d0";
  instr_list.Append(new assem::OperInstr(instr_ss.str(), new temp::TempList(reg),
                                         nullptr, nullptr));
  return reg;
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  temp::Temp *reg = temp::TempFactory::NewTemp();
  if (IsArm64Target()) {
    EmitArm64LoadImmediate(instr_list, reg, consti_);
    return reg;
  }

  std::stringstream instr_ss;
  instr_ss << "movq $" << consti_ << ", `d0";
  instr_list.Append(new assem::OperInstr(instr_ss.str(), new temp::TempList(reg),
                                         nullptr, nullptr));
  return reg;
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  temp::Temp *ret = reg_manager->ReturnValue();
  if (typeid(*fun_) != typeid(tree::NameExp))
    return ret;

  temp::TempList *arg_list = args_->MunchArgs(instr_list, fs);
  temp::TempList *calldefs = reg_manager->CallerSaves();
  calldefs->Append(reg_manager->ReturnValue());

  std::stringstream instr_ss;
  instr_ss << (IsArm64Target() ? "bl " : "callq ")
           << static_cast<tree::NameExp *>(fun_)->name_->Name();
  instr_list.Append(
      new assem::OperInstr(instr_ss.str(), calldefs, arg_list, nullptr));
  return ret;
}

temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list,
                                   std::string_view fs) {
  temp::TempList *arg_list = new temp::TempList();
  std::stringstream instr_ss;
  int arg_reg_count = reg_manager->ArgRegs()->GetList().size();
  int i = 0;

  for (tree::Exp *arg : GetList()) {
    if (i < arg_reg_count) {
      temp::Temp *dst_reg = reg_manager->ArgRegs()->NthTemp(i);
      if (typeid(*arg) == typeid(tree::ConstExp)) {
        int value = static_cast<tree::ConstExp *>(arg)->consti_;
        if (IsArm64Target()) {
          EmitArm64LoadImmediate(instr_list, dst_reg, value);
        } else {
          instr_ss << "movq $" << value << ", `d0";
          instr_list.Append(new assem::OperInstr(
              instr_ss.str(), new temp::TempList(dst_reg), nullptr, nullptr));
        }
      } else {
        temp::Temp *src_reg = arg->Munch(instr_list, fs);
        if (IsArm64Target())
          EmitArm64MoveReg(instr_list, dst_reg, src_reg);
        else
          instr_list.Append(new assem::MoveInstr(
              "movq `s0, `d0", new temp::TempList(dst_reg),
              new temp::TempList(src_reg)));
      }
      arg_list->Append(dst_reg);
      instr_ss.str("");
      ++i;
      continue;
    }

    int stack_offset = (i - arg_reg_count) * wordsize;
    if (IsArm64Target()) {
      temp::Temp *src_reg = nullptr;
      if (typeid(*arg) == typeid(tree::ConstExp)) {
        src_reg = temp::TempFactory::NewTemp();
        EmitArm64LoadImmediate(instr_list, src_reg,
                               static_cast<tree::ConstExp *>(arg)->consti_);
      } else {
        src_reg = arg->Munch(instr_list, fs);
      }
      std::stringstream store_ss;
      store_ss << "str `s0, [sp";
      if (stack_offset != 0)
        store_ss << ", #" << stack_offset;
      store_ss << "]";
      instr_list.Append(new assem::OperInstr(
          store_ss.str(), nullptr,
          new temp::TempList({src_reg, reg_manager->StackPointer()}), nullptr));
    } else {
      if (typeid(*arg) == typeid(tree::ConstExp)) {
        instr_ss << "movq $" << static_cast<tree::ConstExp *>(arg)->consti_
                 << ", ";
        if (stack_offset != 0)
          instr_ss << stack_offset;
        instr_ss << "("
                 << *reg_manager->temp_map_->Look(reg_manager->StackPointer())
                 << ")";
        instr_list.Append(new assem::OperInstr(
            instr_ss.str(), nullptr,
            new temp::TempList(reg_manager->StackPointer()), nullptr));
      } else {
        temp::Temp *src_reg = arg->Munch(instr_list, fs);
        instr_ss << "movq `s0, ";
        if (stack_offset != 0)
          instr_ss << stack_offset;
        instr_ss << "("
                 << *reg_manager->temp_map_->Look(reg_manager->StackPointer())
                 << ")";
        instr_list.Append(new assem::OperInstr(
            instr_ss.str(), nullptr,
            new temp::TempList({src_reg, reg_manager->StackPointer()}),
            nullptr));
      }
    }

    instr_ss.str("");
    ++i;
  }

  if (i > arg_reg_count)
    arg_list->Append(reg_manager->StackPointer());
  return arg_list;
}

} // namespace tree
