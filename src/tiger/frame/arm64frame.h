#ifndef TIGER_COMPILER_ARM64FRAME_H
#define TIGER_COMPILER_ARM64FRAME_H

#include <unordered_map>

#include "tiger/frame/frame.h"

namespace frame {

class Arm64RegManager : public RegManager {
public:
  Arm64RegManager();

  temp::TempList *Registers() override;
  temp::TempList *ArgRegs() override;
  temp::TempList *CallerSaves() override;
  temp::TempList *CalleeSaves() override;
  temp::TempList *ReturnSink() override;
  temp::Temp *FramePointer() override;
  temp::Temp *StackPointer() override;
  temp::Temp *ReturnValue() override;
  temp::Temp *ArithmeticAssistant() override;
  int WordSize() override;
  int RegCount() override;

  enum Register {
    X0,
    X1,
    X2,
    X3,
    X4,
    X5,
    X6,
    X7,
    X9,
    X10,
    X11,
    X12,
    X13,
    X14,
    X15,
    X19,
    X20,
    X21,
    X22,
    X23,
    X24,
    X25,
    X26,
    X27,
    X28,
    FP,
    LR,
    SP,
    REG_COUNT,
  };

  static std::unordered_map<Register, std::string> reg_str;

private:
  static const int WORD_SIZE = 8;
};

Frame *NewArm64Frame(temp::Label *name, std::vector<bool> formals);
tree::Exp *AccessCurrentExpArm64(Access *acc, Frame *frame);
tree::Exp *AccessExpArm64(Access *acc, tree::Exp *fp);
assem::Proc *ProcEntryExit3Arm64(Frame *frame, assem::InstrList *body);

} // namespace frame

#endif // TIGER_COMPILER_ARM64FRAME_H
