#include "tiger/frame/x64frame.h"

#include <iostream>
#include <sstream>

extern frame::RegManager *reg_manager;

namespace frame {
std::unordered_map<X64RegManager::Register, std::string> X64RegManager::reg_str = 
  std::unordered_map<X64RegManager::Register, std::string>{
    { RAX, "%rax" },
    { RBX, "%rbx" },
    { RCX, "%rcx" },
    { RDX, "%rdx" },
    { RSI, "%rsi" },
    { RDI, "%rdi" },
    { R8, "%r8" },
    { R9, "%r9" },
    { R10, "%r10" },
    { R11, "%r11" },
    { R12, "%r12" },
    { R13, "%r13" },
    { R14, "%r14" },
    { R15, "%r15" },
    { RBP, "%rbp" },
    { RSP, "%rsp" },
  };
  
X64RegManager::X64RegManager() {
  for (int i = 0; i < REG_COUNT; ++i) {
    // Machine registers are represented as distinguished temps up front. Later
    // passes treat them as precolored nodes in the interference graph.
    temp::Temp *new_reg = temp::TempFactory::NewTemp();
    regs_.push_back(new_reg);
    temp_map_->Enter(new_reg, &reg_str.at(static_cast<Register>(i)));
  }
}

temp::TempList *X64RegManager::Registers() {
  temp::TempList *temps = new temp::TempList();
  for (int reg = 0; reg < REG_COUNT; ++reg) {
      // Keep the enumeration order stable: color indices are interpreted by
      // register allocation using this exact register list.
      temps->Append(regs_.at(reg));
  }
  return temps;
}

temp::TempList *X64RegManager::ArgRegs() {
  temp::TempList *temps = new temp::TempList({
    regs_.at(RDI),
    regs_.at(RSI),
    regs_.at(RDX),
    regs_.at(RCX),
    regs_.at(R8),
    regs_.at(R9),
  });
  return temps;
}

temp::TempList *X64RegManager::CallerSaves() {
  temp::TempList *temps = new temp::TempList({
    regs_.at(RAX),
    regs_.at(RDI),
    regs_.at(RSI),
    regs_.at(RDX),
    regs_.at(RCX),
    regs_.at(R8),
    regs_.at(R9),
    regs_.at(R10),
    regs_.at(R11),
  });
  return temps;
}

temp::TempList *X64RegManager::CalleeSaves() {
  temp::TempList *temps = new temp::TempList({
    regs_.at(RBX), 
    regs_.at(RBP), 
    regs_.at(R12),
    regs_.at(R13), 
    regs_.at(R14), 
    regs_.at(R15),
  });
  return temps;
}

temp::TempList *X64RegManager::ReturnSink() {
  temp::TempList *temps = CalleeSaves();
  temps->Append(regs_.at(RAX));
  temps->Append(regs_.at(RSP));
  return temps;
}

temp::Temp *X64RegManager::FramePointer() {
  return regs_.at(RBP);
}

temp::Temp *X64RegManager::StackPointer() {
  return regs_.at(RSP);
}

temp::Temp *X64RegManager::ReturnValue() {
  return regs_.at(RAX);
}

temp::Temp *X64RegManager::ArithmeticAssistant() {
  return regs_.at(RDX);
}

int X64RegManager::WordSize() {
  return WORD_SIZE;
}

int X64RegManager::RegCount() {
  return REG_COUNT;
}

class InFrameAccess : public Access {
public:
  int offset;

  explicit InFrameAccess(int offset) : offset(offset) {}
  
  std::string MunchAccess(Frame *frame) override {
    std::stringstream ss;
    // Stack slots are addressed relative to the current `%rsp` plus the
    // assembly-time frame-size constant. This matches FrameAddress() which
    // models FP as SP + framesize throughout the function body.
    ss << "(" << frame->frame_size_->Name() << "-" << offset << ")("
       << *reg_manager->temp_map_->Look(reg_manager->StackPointer()) << ")";
    return ss.str();
  }
};


class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg) : reg(reg) {}

  std::string MunchAccess(Frame *frame) override {
    return *temp::Map::Name()->Look(reg);
  }
};

class X64Frame : public Frame {
public:
  X64Frame(temp::Label *name) : Frame(name) {
    word_size_ = X64_WORD_SIZE;
  }

  Access *AllocLocal(bool escape) override;
  tree::Exp *FrameAddress() const override;
  int WordSize() const override;
  std::string GetLabel() const override;
  tree::Exp *StackOffset(int frame_offset) const override;

};

Access *X64Frame::AllocLocal(bool escape) {
  Access *access;
  if (escape) {
    // Escaping locals must survive across nested-function access, so they live
    // in the frame and consume one fixed-width slot each.
    local_count_++;
    access = new InFrameAccess(local_count_ * word_size_);
  } else {
    // Non-escaping locals can stay in fresh virtual temps and may disappear
    // entirely after register allocation.
    access = new InRegAccess(temp::TempFactory::NewTemp());
  }
  local_access_.push_back(access);
  return access;
}

tree::Exp *X64Frame::FrameAddress() const {
  return new tree::BinopExp(tree::PLUS_OP, 
          new tree::TempExp(reg_manager->StackPointer()), new tree::NameExp(frame_size_));
}

int X64Frame::WordSize() const {
  return word_size_;
}

std::string X64Frame::GetLabel() const {
  return name_->Name();
}

tree::Exp *X64Frame::StackOffset(int frame_offset) const {
  return new tree::BinopExp(tree::MINUS_OP, 
          new tree::NameExp(frame_size_), new tree::ConstExp(frame_offset));
}

Frame *NewX64Frame(temp::Label *name, std::vector<bool> formals) {
  Frame* frame = new X64Frame(name);
  int frame_offset = frame->WordSize();
  // The assembler-level frame-size symbol lets both the prologue and every
  // stack-slot reference agree on the final frame size without patching IR.
  frame->frame_size_ = temp::LabelFactory::NamedLabel(name->Name() + "_framesize");

  tree::TempExp *fp_exp = new tree::TempExp(temp::TempFactory::NewTemp());
  // Materialize the "virtual frame pointer" once at function entry so view
  // shift code can address incoming stack arguments consistently.
  frame->view_shift = new tree::MoveStm(fp_exp, frame->FrameAddress());

  tree::Exp *dst_exp;
  tree::Stm *single_view_shift;
  int arg_reg_count = reg_manager->ArgRegs()->GetList().size();
  tree::Exp *fp_exp_copy; 

  // Formals

  if (formals.size() > arg_reg_count) {
    fp_exp_copy = new tree::TempExp(temp::TempFactory::NewTemp());
    // Extra formals arrive in caller-allocated stack slots above the return
    // address, so we keep an extra saved FP expression for those loads.
    frame->view_shift = new tree::SeqStm(frame->view_shift, new tree::MoveStm(fp_exp_copy, frame->FrameAddress()));
  }

  for (int i = 0; i < formals.size(); ++i) {
    if (formals.at(i)) {  // escape
      frame->formal_access_.push_back(new InFrameAccess(frame_offset));
      // Escaping formals are copied into this frame's own slots immediately;
      // nested functions will later reach them through static-link traversal.
      dst_exp = new tree::MemExp(new tree::BinopExp(tree::MINUS_OP, 
                  fp_exp, new tree::ConstExp((i + 1) * frame->WordSize())));
      frame_offset += frame->WordSize();
      frame->local_count_++;
    } else {
      temp::Temp *reg = temp::TempFactory::NewTemp();
      frame->formal_access_.push_back(new InRegAccess(reg));
      dst_exp = new tree::TempExp(reg);
    }

    if (i < arg_reg_count) {
      // Register-passed formal: move from ABI argument register to its home.
      single_view_shift = new tree::MoveStm(dst_exp, new tree::TempExp(reg_manager->ArgRegs()->NthTemp(i)));
    } else {
      // Stack-passed formals start one word above the return address, then
      // continue upward in word-sized slots.
      single_view_shift = new tree::MoveStm(dst_exp, new tree::MemExp(
                            new tree::BinopExp(tree::PLUS_OP, fp_exp_copy, 
                              new tree::ConstExp((i - arg_reg_count + 1) * frame->WordSize()))));
    }
    frame->view_shift = new tree::SeqStm(frame->view_shift, single_view_shift);
  }

  // Save and restore callee-save registers in fresh temps. Those temps then
  // become ordinary values that later passes may either keep in registers or
  // spill, while preserving the ABI-level save/restore semantics.

  temp::TempList *callee_saves = reg_manager->CalleeSaves();
  temp::Temp *store_reg;
  tree::Stm *single_save;
  tree::Stm *single_restore;

  store_reg = temp::TempFactory::NewTemp();
  frame->save_callee_saves = new tree::MoveStm(new tree::TempExp(store_reg), 
                              new tree::TempExp(callee_saves->GetList().front()));
  frame->restore_callee_saves = new tree::MoveStm(new tree::TempExp(callee_saves->GetList().front()),
                                  new tree::TempExp(store_reg));

  for (auto reg_it = ++callee_saves->GetList().begin(); reg_it != callee_saves->GetList().end(); ++reg_it) {
    store_reg = temp::TempFactory::NewTemp();
    single_save = new tree::MoveStm(new tree::TempExp(store_reg), new tree::TempExp(*reg_it));
    single_restore = new tree::MoveStm(new tree::TempExp(*reg_it), new tree::TempExp(store_reg));
    frame->save_callee_saves = new tree::SeqStm(single_save, frame->save_callee_saves);
    frame->restore_callee_saves = new tree::SeqStm(single_restore, frame->restore_callee_saves);
  }

  return frame;
}

/* Work iff frame is the current frame */
tree::Exp *AccessCurrentExpX64(Access *acc, Frame *frame) {
  if (typeid(*acc) == typeid(InFrameAccess)) {
    InFrameAccess *frame_acc = static_cast<InFrameAccess *>(acc);

    // Recompute the current frame's base address instead of assuming a fixed
    // hardware frame pointer register.
    return new tree::MemExp(new tree::BinopExp(tree::MINUS_OP, 
            frame->FrameAddress(), new tree::ConstExp(frame_acc->offset)));

  } else {
    InRegAccess *reg_acc = static_cast<InRegAccess *>(acc);
    return new tree::TempExp(reg_acc->reg);
  }
}

tree::Exp *AccessExpX64(Access *acc, tree::Exp *fp) {
  if (typeid(*acc) == typeid(InFrameAccess)) {
    // `fp` is an explicit frame-pointer expression, usually obtained by
    // following static links through enclosing activation records.
    InFrameAccess *frame_acc = static_cast<InFrameAccess *>(acc);
    return new tree::MemExp(new tree::BinopExp(tree::MINUS_OP, 
            fp, new tree::ConstExp(frame_acc->offset)));
  } else {
    InRegAccess *reg_acc = static_cast<InRegAccess *>(acc);
    return new tree::TempExp(reg_acc->reg);
  }
}

assem::Proc *ProcEntryExit3X64(Frame *frame, assem::InstrList *body) {
  std::stringstream prologue_ss;
  std::stringstream epilogue_ss;

  // Reserve enough space for both escaping locals and the maximum outgoing
  // stack-argument area needed by any call in the function body.
  int fs = (frame->local_count_ + frame->max_outgoing_args_) * frame->WordSize();
  prologue_ss << ".set " << frame->frame_size_->Name() << ", " << fs << "\n"; 

  // Function label
  prologue_ss << frame->GetLabel() << ":\n";

  // Adjust stack pointer (allocate the frame)
  if (fs != 0)
    prologue_ss << "subq $" << fs << ", " 
                << *reg_manager->temp_map_->Look(reg_manager->StackPointer())
                << "\n";
  
  // Reset stack pointer (deallocate the frame)
  if (fs != 0)
    epilogue_ss << "addq $" << fs << ", " 
                << *reg_manager->temp_map_->Look(reg_manager->StackPointer())
                << "\n";

  // Return instruction
  epilogue_ss << "retq\n";

  return new assem::Proc(prologue_ss.str(), body, epilogue_ss.str());
}

} // namespace frame
