/**
 * @file frame.h
 * @brief Activation record (stack frame) abstraction for the Tiger compiler
 *
 * This file defines the machine-independent interface for activation records
 * (stack frames) and related abstractions.  The concrete implementation for
 * x86-64 is in x64frame.h / x64frame.cc.
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Activation record layout (x64, grows downward)
 * ─────────────────────────────────────────────────────────────────────────
 *
 *   Higher addresses
 *   ┌──────────────────────────────┐  ← caller's SP before call
 *   │  outgoing args beyond 6th    │  (if any)
 *   │  (pushed by caller)          │
 *   ├──────────────────────────────┤  ← SP at function entry (= FP)
 *   │  static link (formal[0])     │  passed in %rdi
 *   │  formal[1]  (%rsi)           │
 *   │  ...                         │
 *   │  formal[5]  (%r9)            │
 *   ├──────────────────────────────┤
 *   │  local[0]  (escaping)        │  ← FP - 8
 *   │  local[1]  (escaping)        │  ← FP - 16
 *   │  ...                         │
 *   ├──────────────────────────────┤
 *   │  outgoing args for callees   │  (max_outgoing_args_ slots)
 *   └──────────────────────────────┘  ← SP during body execution
 *   Lower addresses
 *
 * Frame size = (local_count_ + max_outgoing_args_) * word_size
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Key classes
 * ─────────────────────────────────────────────────────────────────────────
 *
 * RegManager  – abstract interface to machine registers and calling convention
 * Access      – abstract location of a variable (register or frame slot)
 * Frame       – abstract activation record (owns formals and locals)
 * Frag        – a compiled fragment: either a procedure body or a string literal
 * Frags       – the global list of all fragments produced during translation
 *
 * ─────────────────────────────────────────────────────────────────────────
 * View shift
 * ─────────────────────────────────────────────────────────────────────────
 * On function entry, formal parameters arrive in argument registers (or on
 * the stack for the 7th+ argument).  The "view shift" copies each formal
 * from its incoming location to its declared storage location (register or
 * frame slot).  This is recorded as an IR tree in frame_->view_shift and
 * prepended to the function body by ProcEntryExit1.
 */

#ifndef TIGER_FRAME_FRAME_H_
#define TIGER_FRAME_FRAME_H_

#include <list>
#include <string>
#include <vector>

#include "tiger/frame/temp.h"
#include "tiger/translate/tree.h"
#include "tiger/codegen/assem.h"

namespace frame {

class Frame;

/**
 * @brief Abstract interface to machine registers and calling convention
 *
 * Provides a machine-independent view of the register file and the
 * calling convention.  The concrete implementation (X64RegManager) maps
 * each abstract register to a physical x86-64 register.
 *
 * Register categories (x64 System V AMD64 ABI):
 *   Argument registers : %rdi, %rsi, %rdx, %rcx, %r8, %r9  (first 6 args)
 *   Caller-saved       : %rax, %rdi, %rsi, %rdx, %rcx, %r8, %r9, %r10, %r11
 *   Callee-saved       : %rbx, %rbp, %r12–%r15
 *   Return value       : %rax
 *   Stack pointer      : %rsp
 *   Frame pointer      : %rbp  (used as a virtual FP in this compiler)
 */
class RegManager {
public:
  RegManager() : temp_map_(temp::Map::Empty()) {}

  /** @brief Get the physical register temp for register index @p regno */
  temp::Temp *GetRegister(int regno) { return regs_[regno]; }

  /**
   * @brief All general-purpose registers (in calling-convention order)
   * @return TempList of all allocatable registers
   */
  [[nodiscard]] virtual temp::TempList *Registers() = 0;

  /**
   * @brief Registers used to pass the first N arguments (in order)
   *
   * On x64: %rdi, %rsi, %rdx, %rcx, %r8, %r9
   * The static link is passed as the first argument (%rdi).
   *
   * @return TempList of argument registers in calling-convention order
   */
  [[nodiscard]] virtual temp::TempList *ArgRegs() = 0;

  /**
   * @brief Caller-saved (call-clobbered) registers
   *
   * These registers may be overwritten by a called function.
   * The caller must save them before a call if their values are needed after.
   *
   * @return TempList of caller-saved registers
   */
  [[nodiscard]] virtual temp::TempList *CallerSaves() = 0;

  /**
   * @brief Callee-saved registers
   *
   * These registers must be preserved across function calls.
   * A function that uses them must save and restore them.
   * On x64: %rbx, %rbp, %r12–%r15
   *
   * @return TempList of callee-saved registers
   */
  [[nodiscard]] virtual temp::TempList *CalleeSaves() = 0;

  /**
   * @brief Return-sink registers
   *
   * The set of registers that are "live" at the function return point.
   * Includes callee-saved registers (which must be restored) and %rax
   * (the return value) and %rsp (the stack pointer).
   * Used by ProcEntryExit2 to add a pseudo-instruction that keeps these
   * registers live through the end of the function.
   *
   * @return TempList of return-sink registers
   */
  [[nodiscard]] virtual temp::TempList *ReturnSink() = 0;

  /** @brief Machine word size in bytes (8 on x64) */
  [[nodiscard]] virtual int WordSize() = 0;

  /** @brief Total number of physical registers */
  [[nodiscard]] virtual int RegCount() = 0;

  /** @brief The frame-pointer register (%rbp on x64) */
  [[nodiscard]] virtual temp::Temp *FramePointer() = 0;

  /** @brief The stack-pointer register (%rsp on x64) */
  [[nodiscard]] virtual temp::Temp *StackPointer() = 0;

  /** @brief The return-value register (%rax on x64) */
  [[nodiscard]] virtual temp::Temp *ReturnValue() = 0;

  /**
   * @brief An auxiliary register used for multi-register arithmetic
   *
   * On x64, integer division (idivq) implicitly uses %rdx:%rax as the
   * dividend and leaves the remainder in %rdx.  This register is reserved
   * for such operations.
   *
   * @return %rdx on x64
   */
  [[nodiscard]] virtual temp::Temp *ArithmeticAssistant() = 0;

  /** @brief Map from temp::Temp* to register name string (e.g., "%rax") */
  temp::Map *temp_map_;

protected:
  std::vector<temp::Temp *> regs_; ///< Physical register temps, indexed by enum
};

/**
 * @brief Abstract location of a variable within an activation record
 *
 * A variable can be stored either:
 *   - In a virtual register (InRegAccess): fast, but cannot be accessed
 *     from nested functions (non-escaping variables)
 *   - In the stack frame (InFrameAccess): accessible via frame pointer
 *     arithmetic (escaping variables)
 *
 * MunchAccess() generates the assembly operand string for this location,
 * e.g., "-8(%rsp)" for a frame slot or "%t42" for a register.
 */
class Access {
public:
  /**
   * @brief Generate the assembly operand string for this access
   * @param frame The frame this access belongs to (needed for frame-size label)
   * @return Assembly operand string, e.g., "(framesize-8)(%rsp)"
   */
  virtual std::string MunchAccess(Frame *frame) = 0;
  virtual ~Access() = default;
};

/**
 * @brief Abstract activation record (stack frame) for one function
 *
 * Tracks:
 *   - The function's label (name_)
 *   - Formal parameter accesses (formal_access_)
 *   - Local variable accesses (local_access_)
 *   - The number of escaping locals (local_count_)
 *   - The maximum number of outgoing arguments beyond the 6 register args
 *     (max_outgoing_args_), used to size the outgoing-argument area
 *   - IR trees for the view shift and callee-save save/restore
 *
 * The concrete subclass X64Frame provides the x86-64 implementation.
 */
class Frame {
protected:
  Frame() = default;
  explicit Frame(temp::Label *name)
      : name_(name), local_count_(0), max_outgoing_args_(0) {}
  virtual ~Frame() = default;

  int word_size_; ///< Machine word size (set by subclass)

public:
  /**
   * @brief Allocate a new local variable in this frame
   * @param escape true → allocate in the frame (stack slot);
   *               false → allocate in a fresh virtual register
   * @return Pointer to the new Access object
   */
  virtual Access *AllocLocal(bool escape) = 0;

  /**
   * @brief IR expression for the current frame's base address
   *
   * Returns an expression equivalent to the frame pointer at the time
   * the function body executes.  On x64 this is SP + framesize.
   */
  virtual tree::Exp *FrameAddress() const = 0;

  /** @brief Machine word size in bytes */
  virtual int WordSize() const = 0;

  /** @brief The function's assembly label string */
  virtual std::string GetLabel() const = 0;

  /**
   * @brief IR expression for a stack slot at @p frame_offset bytes from FP
   * @param frame_offset Byte offset from the frame base (positive = downward)
   * @return IR expression for the address of that stack slot
   */
  virtual tree::Exp *StackOffset(int frame_offset) const = 0;

  temp::Label *name_;                       ///< Function label
  std::vector<Access *> formal_access_;     ///< Accesses for formal parameters (incl. static link at [0])
  std::vector<Access *> local_access_;      ///< Accesses for local variables
  int local_count_;                         ///< Number of escaping locals allocated so far

  temp::Label *frame_size_;                 ///< Label for the frame-size constant (set by ProcEntryExit3)
  tree::Stm *view_shift;                    ///< IR tree: copy formals from arg regs/stack to their declared locations
  tree::Stm *save_callee_saves;             ///< IR tree: save callee-saved registers to fresh temps
  tree::Stm *restore_callee_saves;          ///< IR tree: restore callee-saved registers from saved temps
  int max_outgoing_args_;                   ///< Max extra stack args needed for any call in this function
};

// ═══════════════════════════════════════════════════════════════════════════
// Fragments
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Abstract base class for compiled fragments
 *
 * A fragment is a self-contained unit of compiled output:
 *   - ProcFrag: a function body (IR tree + frame)
 *   - StringFrag: a string literal (label + content)
 *
 * OutputAssem() writes the appropriate assembly for the given phase.
 */
class Frag {
public:
  virtual ~Frag() = default;

  /** @brief Output phase selector */
  enum OutputPhase {
    Proc,   ///< Emit procedure (function body) assembly
    String, ///< Emit string literal assembly
  };

  /**
   * @brief Write assembly for this fragment
   * @param out     Output file
   * @param phase   Which phase to emit (Proc or String)
   * @param need_ra Whether to perform register allocation before output
   */
  virtual void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const = 0;
};

/**
 * @brief A string literal fragment
 *
 * Represents a Tiger string constant.  In the output assembly, a string
 * is emitted in the .rodata section as:
 *   label:
 *     .long <length>
 *     .string "<content>"
 *
 * The length word precedes the character data so that the runtime
 * string functions can determine the string's length in O(1).
 */
class StringFrag : public Frag {
public:
  temp::Label *label_; ///< Assembly label for this string
  std::string str_;    ///< String content (may contain embedded NULs)

  StringFrag(temp::Label *label, std::string str)
      : label_(label), str_(std::move(str)) {}

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
};

/**
 * @brief A procedure (function body) fragment
 *
 * Represents one compiled Tiger function.  Contains:
 *   - body_: the IR tree for the function body (after ProcEntryExit1)
 *   - frame_: the activation record (for frame size, formal accesses, etc.)
 *
 * OutputAssem() drives the back-end pipeline for this function:
 *   canonicalization → code generation → register allocation → assembly output
 */
class ProcFrag : public Frag {
public:
  tree::Stm *body_;  ///< IR tree for the function body
  Frame *frame_;     ///< Activation record for this function

  ProcFrag(tree::Stm *body, Frame *frame) : body_(body), frame_(frame) {}

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
};

/**
 * @brief The global list of all compiled fragments
 *
 * Accumulated during IR translation (tr::ProgTr::Translate()).
 * Consumed by output::AssemGen::GenAssem() to produce the final assembly file.
 */
class Frags {
public:
  Frags() = default;
  void PushBack(Frag *frag) { frags_.emplace_back(frag); }
  const std::list<Frag*> &GetList() { return frags_; }

private:
  std::list<Frag*> frags_;
};

} // namespace frame

#endif // TIGER_FRAME_FRAME_H_
