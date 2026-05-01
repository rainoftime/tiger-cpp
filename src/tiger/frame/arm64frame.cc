#include "tiger/frame/arm64frame.h"

#include <sstream>

extern frame::RegManager *reg_manager;

namespace frame {

namespace {

constexpr int kSavedFrameRecordBytes = 16;

int Align16(int value) {
  return (value + 15) & ~15;
}

} // namespace

std::unordered_map<Arm64RegManager::Register, std::string>
    Arm64RegManager::reg_str = {
        {X0, "x0"},   {X1, "x1"},   {X2, "x2"},   {X3, "x3"},
        {X4, "x4"},   {X5, "x5"},   {X6, "x6"},   {X7, "x7"},
        {X9, "x9"},   {X10, "x10"}, {X11, "x11"}, {X12, "x12"},
        {X13, "x13"}, {X14, "x14"}, {X15, "x15"}, {X19, "x19"},
        {X20, "x20"}, {X21, "x21"}, {X22, "x22"}, {X23, "x23"},
        {X24, "x24"}, {X25, "x25"}, {X26, "x26"}, {X27, "x27"},
        {X28, "x28"}, {FP, "x29"},  {LR, "x30"},  {SP, "sp"},
};

Arm64RegManager::Arm64RegManager() {
  for (int i = 0; i < REG_COUNT; ++i) {
    temp::Temp *reg = temp::TempFactory::NewTemp();
    regs_.push_back(reg);
    temp_map_->Enter(reg, &reg_str.at(static_cast<Register>(i)));
  }
}

temp::TempList *Arm64RegManager::Registers() {
  temp::TempList *temps = new temp::TempList();
  for (int reg = 0; reg < REG_COUNT; ++reg)
    temps->Append(regs_.at(reg));
  return temps;
}

temp::TempList *Arm64RegManager::ArgRegs() {
  return new temp::TempList({regs_.at(X0), regs_.at(X1), regs_.at(X2),
                             regs_.at(X3), regs_.at(X4), regs_.at(X5),
                             regs_.at(X6), regs_.at(X7)});
}

temp::TempList *Arm64RegManager::CallerSaves() {
  return new temp::TempList({regs_.at(X0), regs_.at(X1), regs_.at(X2),
                             regs_.at(X3), regs_.at(X4), regs_.at(X5),
                             regs_.at(X6), regs_.at(X7), regs_.at(X9),
                             regs_.at(X10), regs_.at(X11), regs_.at(X12),
                             regs_.at(X13), regs_.at(X14), regs_.at(X15),
                             regs_.at(LR)});
}

temp::TempList *Arm64RegManager::CalleeSaves() {
  return new temp::TempList({regs_.at(X19), regs_.at(X20), regs_.at(X21),
                             regs_.at(X22), regs_.at(X23), regs_.at(X24),
                             regs_.at(X25), regs_.at(X26), regs_.at(X27),
                             regs_.at(X28), regs_.at(FP)});
}

temp::TempList *Arm64RegManager::ReturnSink() {
  temp::TempList *temps = CalleeSaves();
  temps->Append(regs_.at(X0));
  temps->Append(regs_.at(SP));
  return temps;
}

temp::Temp *Arm64RegManager::FramePointer() { return regs_.at(FP); }

temp::Temp *Arm64RegManager::StackPointer() { return regs_.at(SP); }

temp::Temp *Arm64RegManager::ReturnValue() { return regs_.at(X0); }

temp::Temp *Arm64RegManager::ArithmeticAssistant() { return regs_.at(X1); }

int Arm64RegManager::WordSize() { return WORD_SIZE; }

int Arm64RegManager::RegCount() { return REG_COUNT; }

class Arm64InFrameAccess : public Access {
public:
  explicit Arm64InFrameAccess(int offset) : offset_(offset) {}

  std::string MunchAccess(Frame *frame) override {
    std::stringstream ss;
    ss << "["
       << *reg_manager->temp_map_->Look(reg_manager->FramePointer()) << ", #-"
       << offset_ << "]";
    return ss.str();
  }

  int offset_;
};

class Arm64InRegAccess : public Access {
public:
  explicit Arm64InRegAccess(temp::Temp *reg) : reg_(reg) {}

  std::string MunchAccess(Frame *frame) override {
    return *temp::Map::Name()->Look(reg_);
  }

  temp::Temp *reg_;
};

class Arm64Frame : public Frame {
public:
  explicit Arm64Frame(temp::Label *name) : Frame(name) { word_size_ = 8; }

  Access *AllocLocal(bool escape) override;
  tree::Exp *FrameAddress() const override;
  int WordSize() const override;
  std::string GetLabel() const override;
  tree::Exp *StackOffset(int frame_offset) const override;
};

Access *Arm64Frame::AllocLocal(bool escape) {
  Access *access;
  if (escape) {
    local_count_++;
    access = new Arm64InFrameAccess(kSavedFrameRecordBytes +
                                    local_count_ * word_size_);
  } else {
    access = new Arm64InRegAccess(temp::TempFactory::NewTemp());
  }
  local_access_.push_back(access);
  return access;
}

tree::Exp *Arm64Frame::FrameAddress() const {
  return new tree::TempExp(reg_manager->FramePointer());
}

int Arm64Frame::WordSize() const { return word_size_; }

std::string Arm64Frame::GetLabel() const { return name_->Name(); }

tree::Exp *Arm64Frame::StackOffset(int frame_offset) const {
  return new tree::BinopExp(tree::MINUS_OP, FrameAddress(),
                            new tree::ConstExp(frame_offset));
}

Frame *NewArm64Frame(temp::Label *name, std::vector<bool> formals) {
  Frame *frame = new Arm64Frame(name);
  int frame_offset = kSavedFrameRecordBytes + frame->WordSize();
  frame->frame_size_ =
      temp::LabelFactory::NamedLabel(name->Name() + "_framesize");
  frame->view_shift = new tree::ExpStm(new tree::ConstExp(0));

  tree::Exp *dst_exp;
  tree::Stm *single_view_shift;
  int arg_reg_count = reg_manager->ArgRegs()->GetList().size();

  for (int i = 0; i < static_cast<int>(formals.size()); ++i) {
    if (formals.at(i)) {
      frame->formal_access_.push_back(new Arm64InFrameAccess(frame_offset));
      dst_exp = new tree::MemExp(new tree::BinopExp(
          tree::MINUS_OP, frame->FrameAddress(),
          new tree::ConstExp(frame_offset)));
      frame_offset += frame->WordSize();
      frame->local_count_++;
    } else {
      temp::Temp *reg = temp::TempFactory::NewTemp();
      frame->formal_access_.push_back(new Arm64InRegAccess(reg));
      dst_exp = new tree::TempExp(reg);
    }

    if (i < arg_reg_count) {
      single_view_shift = new tree::MoveStm(
          dst_exp, new tree::TempExp(reg_manager->ArgRegs()->NthTemp(i)));
    } else {
      single_view_shift = new tree::MoveStm(
          dst_exp, new tree::MemExp(new tree::BinopExp(
                       tree::PLUS_OP, frame->FrameAddress(),
                       new tree::ConstExp((i - arg_reg_count) *
                                          frame->WordSize()))));
    }
    frame->view_shift = new tree::SeqStm(frame->view_shift, single_view_shift);
  }

  temp::TempList *callee_saves = reg_manager->CalleeSaves();
  temp::Temp *store_reg = temp::TempFactory::NewTemp();
  frame->save_callee_saves = new tree::MoveStm(
      new tree::TempExp(store_reg),
      new tree::TempExp(callee_saves->GetList().front()));
  frame->restore_callee_saves = new tree::MoveStm(
      new tree::TempExp(callee_saves->GetList().front()),
      new tree::TempExp(store_reg));

  for (auto reg_it = ++callee_saves->GetList().begin();
       reg_it != callee_saves->GetList().end(); ++reg_it) {
    store_reg = temp::TempFactory::NewTemp();
    tree::Stm *single_save = new tree::MoveStm(new tree::TempExp(store_reg),
                                               new tree::TempExp(*reg_it));
    tree::Stm *single_restore =
        new tree::MoveStm(new tree::TempExp(*reg_it),
                          new tree::TempExp(store_reg));
    frame->save_callee_saves = new tree::SeqStm(single_save,
                                                frame->save_callee_saves);
    frame->restore_callee_saves = new tree::SeqStm(
        single_restore, frame->restore_callee_saves);
  }

  return frame;
}

tree::Exp *AccessCurrentExpArm64(Access *acc, Frame *frame) {
  if (typeid(*acc) == typeid(Arm64InFrameAccess)) {
    auto *frame_acc = static_cast<Arm64InFrameAccess *>(acc);
    return new tree::MemExp(new tree::BinopExp(
        tree::MINUS_OP, frame->FrameAddress(),
        new tree::ConstExp(frame_acc->offset_)));
  }

  auto *reg_acc = static_cast<Arm64InRegAccess *>(acc);
  return new tree::TempExp(reg_acc->reg_);
}

tree::Exp *AccessExpArm64(Access *acc, tree::Exp *fp) {
  if (typeid(*acc) == typeid(Arm64InFrameAccess)) {
    auto *frame_acc = static_cast<Arm64InFrameAccess *>(acc);
    return new tree::MemExp(new tree::BinopExp(
        tree::MINUS_OP, fp, new tree::ConstExp(frame_acc->offset_)));
  }

  auto *reg_acc = static_cast<Arm64InRegAccess *>(acc);
  return new tree::TempExp(reg_acc->reg_);
}

assem::Proc *ProcEntryExit3Arm64(Frame *frame, assem::InstrList *body) {
  const int local_bytes =
      (frame->local_count_ + frame->max_outgoing_args_) * frame->WordSize();
  const int fs = Align16(local_bytes + 16);

  std::stringstream prologue_ss;
  std::stringstream epilogue_ss;

  prologue_ss << ".p2align 2\n";
  prologue_ss << frame->GetLabel() << ":\n";
  prologue_ss << "sub sp, sp, #" << fs << "\n";
  prologue_ss << "str x29, [sp, #" << (fs - 16) << "]\n";
  prologue_ss << "str x30, [sp, #" << (fs - 8) << "]\n";
  prologue_ss << "add x29, sp, #" << fs << "\n";

  epilogue_ss << "ldr x29, [sp, #" << (fs - 16) << "]\n";
  epilogue_ss << "ldr x30, [sp, #" << (fs - 8) << "]\n";
  epilogue_ss << "add sp, sp, #" << fs << "\n";
  epilogue_ss << "ret\n";

  return new assem::Proc(prologue_ss.str(), body, epilogue_ss.str());
}

} // namespace frame
