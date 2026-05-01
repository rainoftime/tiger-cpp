#include "tiger/frame/target.h"

#include <utility>

#include "tiger/frame/arm64frame.h"
#include "tiger/frame/x64frame.h"

extern frame::RegManager *reg_manager;

namespace frame {

namespace {

TargetArch current_target = DetectHostTarget();

} // namespace

TargetArch DetectHostTarget() {
#if defined(__APPLE__) && defined(__aarch64__)
  return TargetArch::Arm64Apple;
#else
  return TargetArch::X64SystemV;
#endif
}

bool ParseTarget(std::string_view name, TargetArch *target) {
  if (name == "x64" || name == "x86_64" || name == "x64-sysv") {
    *target = TargetArch::X64SystemV;
    return true;
  }
  if (name == "arm64" || name == "arm64-apple" || name == "apple-arm64" ||
      name == "aarch64-apple") {
    *target = TargetArch::Arm64Apple;
    return true;
  }
  return false;
}

std::string TargetName(TargetArch target) {
  switch (target) {
  case TargetArch::X64SystemV:
    return "x64-sysv";
  case TargetArch::Arm64Apple:
    return "arm64-apple";
  }
  return "unknown";
}

void SetCurrentTarget(TargetArch target) { current_target = target; }

TargetArch GetCurrentTarget() { return current_target; }

bool IsArm64AppleTarget() {
  return GetCurrentTarget() == TargetArch::Arm64Apple;
}

std::string MangleExternalSymbol(std::string_view symbol) {
  if (!IsArm64AppleTarget())
    return std::string(symbol);
  return "_" + std::string(symbol);
}

temp::Label *NamedCodeLabel(std::string_view symbol) {
  return temp::LabelFactory::NamedLabel(MangleExternalSymbol(symbol));
}

RegManager *NewRegManagerForTarget(TargetArch target) {
  switch (target) {
  case TargetArch::X64SystemV:
    return new X64RegManager();
  case TargetArch::Arm64Apple:
    return new Arm64RegManager();
  }
  return new X64RegManager();
}

Frame *NewFrame(temp::Label *name, std::vector<bool> formals) {
  if (IsArm64AppleTarget())
    return NewArm64Frame(name, std::move(formals));
  return NewX64Frame(name, std::move(formals));
}

tree::Exp *AccessCurrentExp(Access *acc, Frame *frame) {
  if (IsArm64AppleTarget())
    return AccessCurrentExpArm64(acc, frame);
  return AccessCurrentExpX64(acc, frame);
}

tree::Exp *AccessExp(Access *acc, tree::Exp *fp) {
  if (IsArm64AppleTarget())
    return AccessExpArm64(acc, fp);
  return AccessExpX64(acc, fp);
}

tree::Exp *ExternalCall(std::string s, tree::ExpList *args) {
  return new tree::CallExp(new tree::NameExp(NamedCodeLabel(s)), args);
}

tree::Stm *ProcEntryExit1(Frame *frame, tree::Stm *stm) {
  stm = new tree::SeqStm(frame->save_callee_saves, stm);
  stm = new tree::SeqStm(frame->view_shift, stm);
  stm = new tree::SeqStm(stm, frame->restore_callee_saves);
  return stm;
}

assem::InstrList *ProcEntryExit2(assem::InstrList *body) {
  assem::Instr *return_sink =
      new assem::OperInstr("", nullptr, reg_manager->ReturnSink(), nullptr);
  body->Append(return_sink);
  return body;
}

assem::Proc *ProcEntryExit3(Frame *frame, assem::InstrList *body) {
  if (IsArm64AppleTarget())
    return ProcEntryExit3Arm64(frame, body);
  return ProcEntryExit3X64(frame, body);
}

std::string TextSectionDirective() { return ".text"; }

std::string RodataSectionDirective() {
  if (IsArm64AppleTarget())
    return ".section __TEXT,__const";
  return ".section .rodata";
}

bool EmitsElfFunctionMetadata() { return !IsArm64AppleTarget(); }

} // namespace frame
