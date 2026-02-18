/**
 * @file x64frame.h
 * @brief x86-64 activation record and register manager for the Tiger compiler
 *
 * This file provides the concrete x86-64 implementation of the machine-
 * independent frame abstractions defined in frame.h.
 *
 * ─────────────────────────────────────────────────────────────────────────
 * X64RegManager
 * ─────────────────────────────────────────────────────────────────────────
 * Implements RegManager for the x86-64 System V AMD64 ABI:
 *
 *   Register   Role
 *   ─────────  ──────────────────────────────────────────────────────────
 *   %rax       Return value; caller-saved; dividend/quotient for idivq
 *   %rbx       Callee-saved (general purpose)
 *   %rcx       4th argument register; caller-saved
 *   %rdx       3rd argument register; caller-saved; remainder for idivq
 *   %rsi       2nd argument register; caller-saved
 *   %rdi       1st argument register (= static link); caller-saved
 *   %rbp       Callee-saved; used as virtual frame pointer
 *   %r8–%r9    5th–6th argument registers; caller-saved
 *   %r10–%r11  Caller-saved (scratch)
 *   %r12–%r15  Callee-saved (general purpose)
 *   %rsp       Stack pointer (not allocatable)
 *
 * ─────────────────────────────────────────────────────────────────────────
 * X64Frame (private, defined in x64frame.cc)
 * ─────────────────────────────────────────────────────────────────────────
 * Concrete activation record for x86-64.  Variables are stored either:
 *   - InRegAccess  : in a virtual register (non-escaping)
 *   - InFrameAccess: at a fixed offset below the frame pointer (escaping)
 *
 * Frame pointer convention used here:
 *   FP = SP + framesize   (computed via "leaq framesize(%rsp), fp")
 *   Local slot k is at FP - k*8  (i.e., (framesize - k*8)(%rsp))
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Helper functions (defined in x64frame.cc)
 * ─────────────────────────────────────────────────────────────────────────
 *
 *   NewFrame(name, formals)
 *     Allocate a new X64Frame.  `formals` is a vector<bool> where each
 *     element indicates whether the corresponding formal parameter escapes.
 *     The first element (index 0) is always `true` for the static link.
 *     Builds the view-shift IR tree and callee-save save/restore trees.
 *
 *   AccessCurrentExp(acc, frame)
 *     Generate an IR expression to access `acc` within `frame` (the current
 *     frame).  For InFrameAccess: MEM(FP - offset).  For InRegAccess: TEMP(r).
 *
 *   AccessExp(acc, fp)
 *     Generate an IR expression to access `acc` given an arbitrary frame-
 *     pointer expression `fp` (used when following static links).
 *
 *   ExternalCall(name, args)
 *     Generate a CALL IR node for a runtime library function (e.g.,
 *     alloc_record, init_array, string_equal).
 *
 *   ProcEntryExit1(frame, stm)
 *     Prepend the view-shift and callee-save trees to `stm`, and append
 *     the callee-restore tree.  Returns the augmented statement.
 *
 *   ProcEntryExit2(body)
 *     Append a pseudo-instruction that marks the return-sink registers as
 *     live at the end of the function (for liveness analysis).
 *
 *   ProcEntryExit3(frame, body)
 *     Generate the function prologue (label, stack allocation) and epilogue
 *     (stack deallocation, retq), wrapping `body` in an assem::Proc.
 */

#ifndef TIGER_COMPILER_X64FRAME_H
#define TIGER_COMPILER_X64FRAME_H

#include <unordered_map>

#include "tiger/frame/frame.h"

/** @brief x86-64 machine word size in bytes */
#define X64_WORD_SIZE 8

namespace frame {

/**
 * @brief x86-64 register manager
 *
 * Manages the 16 general-purpose x86-64 registers and implements the
 * System V AMD64 ABI calling convention.
 *
 * Each register is represented by a temp::Temp* (virtual register) that
 * is pre-colored to the corresponding physical register.  The temp_map_
 * maps each Temp* to its register name string (e.g., "%rax").
 */
class X64RegManager : public RegManager {
public:
  /** @brief Initialise all 16 register temps and populate temp_map_ */
  X64RegManager();

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

  /**
   * @brief Enumeration of x86-64 registers (index into regs_[])
   *
   * The order matches the order in which temps are created in the
   * constructor and must be consistent with reg_str.
   */
  enum Register {
    RAX,  ///< %rax – return value, caller-saved, dividend
    RBX,  ///< %rbx – callee-saved
    RCX,  ///< %rcx – 4th argument, caller-saved
    RDX,  ///< %rdx – 3rd argument, caller-saved, remainder
    RSI,  ///< %rsi – 2nd argument, caller-saved
    RDI,  ///< %rdi – 1st argument (static link), caller-saved
    RBP,  ///< %rbp – callee-saved, virtual frame pointer
    R8,   ///< %r8  – 5th argument, caller-saved
    R9,   ///< %r9  – 6th argument, caller-saved
    R10,  ///< %r10 – caller-saved scratch
    R11,  ///< %r11 – caller-saved scratch
    R12,  ///< %r12 – callee-saved
    R13,  ///< %r13 – callee-saved
    R14,  ///< %r14 – callee-saved
    R15,  ///< %r15 – callee-saved
    RSP,  ///< %rsp – stack pointer (not allocatable)
    REG_COUNT,
  };

  /** @brief Map from Register enum to AT&T-syntax register name string */
  static std::unordered_map<Register, std::string> reg_str;

private:
  static const int WORD_SIZE = X64_WORD_SIZE;
};

// ─────────────────────────────────────────────────────────────────────────
// Frame factory and helper functions
// ─────────────────────────────────────────────────────────────────────────

/**
 * @brief Allocate a new x86-64 activation record
 *
 * Creates an X64Frame for a function with the given label and formal
 * parameter escape flags.  The first element of `formals` is always
 * `true` (the static link always escapes).
 *
 * Builds:
 *   - formal_access_: one Access per formal (InRegAccess or InFrameAccess)
 *   - view_shift: IR tree to copy formals from arg regs/stack to their locations
 *   - save_callee_saves / restore_callee_saves: IR trees for callee-save handling
 *
 * @param name    Assembly label for the function
 * @param formals Escape flags for each formal parameter (true = escapes)
 * @return Pointer to the newly allocated Frame
 */
Frame *NewFrame(temp::Label *name, std::vector<bool> formals);

/**
 * @brief Generate an IR expression to access a variable in the current frame
 *
 * Produces:
 *   - MEM(FP - offset)  for InFrameAccess (escaping variable)
 *   - TEMP(r)           for InRegAccess   (non-escaping variable)
 *
 * @param acc   The access descriptor for the variable
 * @param frame The current function's frame (for FP computation)
 * @return IR expression that reads/writes the variable
 */
tree::Exp *AccessCurrentExp(Access *acc, Frame *frame);

/**
 * @brief Generate an IR expression to access a variable given an explicit FP
 *
 * Like AccessCurrentExp but uses an arbitrary frame-pointer expression `fp`
 * instead of the current frame's FP.  Used when following static links to
 * access variables in enclosing functions.
 *
 * @param acc The access descriptor for the variable
 * @param fp  IR expression for the frame pointer of the declaring frame
 * @return IR expression that reads/writes the variable
 */
tree::Exp *AccessExp(Access *acc, tree::Exp *fp);

/**
 * @brief Generate a CALL IR node for a runtime library function
 *
 * Produces CALL(NAME(label), args) where label is looked up by name.
 * Used for Tiger runtime functions: alloc_record, init_array, string_equal,
 * print, flush, getchar, ord, chr, size, substring, concat, not, exit.
 *
 * @param s    Name of the external function
 * @param args Argument list (no static link for external calls)
 * @return CALL IR expression
 */
tree::Exp *ExternalCall(std::string s, tree::ExpList *args);

/**
 * @brief Augment a function body with view shift and callee-save handling
 *
 * Prepends the view-shift IR tree (copy formals to their declared locations)
 * and the callee-save tree, and appends the callee-restore tree to `stm`.
 *
 * Called by tr::ProgTr::Translate() and FunctionDec::Translate() after
 * translating each function body.
 *
 * @param frame The function's activation record
 * @param stm   The translated function body
 * @return Augmented statement: view_shift ; save_callee ; stm ; restore_callee
 */
tree::Stm *ProcEntryExit1(Frame *frame, tree::Stm *stm);

/**
 * @brief Append a return-sink pseudo-instruction to the instruction list
 *
 * Adds an OperInstr with empty assembly string but with ReturnSink() as
 * its source list.  This keeps the callee-saved registers and %rax live
 * at the end of the function, preventing the register allocator from
 * treating them as dead and reusing them.
 *
 * @param body The instruction list for the function body
 * @return The same instruction list with the pseudo-instruction appended
 */
assem::InstrList *ProcEntryExit2(assem::InstrList *body);

/**
 * @brief Generate the function prologue and epilogue
 *
 * Produces an assem::Proc containing:
 *   Prologue:
 *     .set <name>_framesize, <fs>   (define the frame-size constant)
 *     <name>:                        (function label)
 *     subq $<fs>, %rsp               (allocate the frame)
 *   Body: the instruction list
 *   Epilogue:
 *     addq $<fs>, %rsp               (deallocate the frame)
 *     retq                           (return)
 *
 * The frame size fs = (local_count_ + max_outgoing_args_) * word_size.
 *
 * @param frame The function's activation record
 * @param body  The instruction list for the function body
 * @return assem::Proc with prologue, body, and epilogue
 */
assem::Proc *ProcEntryExit3(Frame *frame, assem::InstrList *body);

} // namespace frame

#endif // TIGER_COMPILER_X64FRAME_H
