#ifndef TIGER_COMPILER_TARGET_H
#define TIGER_COMPILER_TARGET_H

#include <string>
#include <string_view>
#include <vector>

#include "tiger/codegen/assem.h"
#include "tiger/frame/frame.h"
#include "tiger/translate/tree.h"

namespace frame {

enum class TargetArch {
  X64SystemV,
  Arm64Apple,
};

TargetArch DetectHostTarget();
bool ParseTarget(std::string_view name, TargetArch *target);
std::string TargetName(TargetArch target);

void SetCurrentTarget(TargetArch target);
TargetArch GetCurrentTarget();
bool IsArm64AppleTarget();

std::string MangleExternalSymbol(std::string_view symbol);
temp::Label *NamedCodeLabel(std::string_view symbol);

RegManager *NewRegManagerForTarget(TargetArch target);

Frame *NewFrame(temp::Label *name, std::vector<bool> formals);
tree::Exp *AccessCurrentExp(Access *acc, Frame *frame);
tree::Exp *AccessExp(Access *acc, tree::Exp *fp);
tree::Exp *ExternalCall(std::string s, tree::ExpList *args);
tree::Stm *ProcEntryExit1(Frame *frame, tree::Stm *stm);
assem::InstrList *ProcEntryExit2(assem::InstrList *body);
assem::Proc *ProcEntryExit3(Frame *frame, assem::InstrList *body);

std::string TextSectionDirective();
std::string RodataSectionDirective();
bool EmitsElfFunctionMetadata();

} // namespace frame

#endif // TIGER_COMPILER_TARGET_H
