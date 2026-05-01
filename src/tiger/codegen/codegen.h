/**
 * @file codegen.h
 * @brief x86-64 instruction selection (code generation) for the Tiger compiler
 *
 * This module implements the code generation phase: translating the
 * canonicalized IR tree into a list of abstract assembly instructions.
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Algorithm: Maximal Munch
 * ─────────────────────────────────────────────────────────────────────────
 * Instruction selection uses the "maximal munch" algorithm (Appel Ch. 9):
 * each IR tree node's Munch() method greedily matches the largest applicable
 * instruction pattern and emits the corresponding assembly instruction(s).
 *
 * The Munch() methods are defined on the IR tree nodes (tree.h / codegen.cc):
 *   - Stm::Munch()  – emits instructions for a statement, returns void
 *   - Exp::Munch()  – emits instructions for an expression, returns the
 *                     temp::Temp* holding the result
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Instruction patterns (x86-64 AT&T syntax)
 * ─────────────────────────────────────────────────────────────────────────
 * Key patterns handled:
 *
 *   MoveStm(MemExp(BinopExp(+, r, CONST(k))), src)  → movq `s0, k(`s1)
 *   MoveStm(MemExp(r), src)                          → movq `s0, (`s1)
 *   MoveStm(dst, MemExp(BinopExp(+, r, CONST(k))))  → movq k(`s0), `d0
 *   MoveStm(dst, CONST(k))                           → movq $k, `d0
 *   MoveStm(dst, src)                                → movq `s0, `d0  (MoveInstr)
 *
 *   BinopExp(+/-, r, CONST(k))  → addq/subq $k, `d0
 *   BinopExp(+/-, r1, r2)       → addq/subq `s1, `d0
 *   BinopExp(*, r1, r2)         → imulq `s2  (uses %rax:%rdx)
 *   BinopExp(/, r1, r2)         → cqto; idivq `s2  (uses %rax:%rdx)
 *   BinopExp(+, r, NAME(fs))    → leaq fs(`s0), `d0  (frame pointer)
 *
 *   CjumpStm(op, r, CONST(k))   → cmpq $k, `s0; j<op> label
 *   CjumpStm(op, r1, r2)        → cmpq `s0, `s1; j<op> label
 *
 *   CallExp(NAME(f), args)       → callq f  (args placed in regs/stack first)
 *   NameExp(l)                   → leaq l(%rip), `d0
 *   ConstExp(k)                  → movq $k, `d0
 *   MemExp(BinopExp(+, r, k))    → movq k(`s0), `d0
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Classes
 * ─────────────────────────────────────────────────────────────────────────
 *
 *   AssemInstr  – wraps an InstrList (the output of code generation)
 *   CodeGen     – drives code generation for one function fragment
 */

#ifndef TIGER_CODEGEN_CODEGEN_H_
#define TIGER_CODEGEN_CODEGEN_H_

#include "tiger/canon/canon.h"
#include "tiger/codegen/assem.h"
#include "tiger/frame/target.h"
#include "tiger/translate/tree.h"

// Forward Declarations
namespace frame {
class RegManager;
class Frame;
} // namespace frame

namespace assem {
class Instr;
class InstrList;
} // namespace assem

namespace canon {
class Traces;
} // namespace canon

namespace cg {

/**
 * @brief The output of code generation: a list of abstract assembly instructions
 *
 * Wraps an assem::InstrList produced by CodeGen::Codegen().
 * Passed to register allocation (ra::RegAllocator) and then to
 * ProcEntryExit3 for final assembly output.
 */
class AssemInstr {
public:
  AssemInstr() = delete;
  AssemInstr(nullptr_t) = delete;
  explicit AssemInstr(assem::InstrList *instr_list) : instr_list_(instr_list) {}

  /**
   * @brief Print all instructions with register names from @p map
   * @param out Output file
   * @param map Temp-to-register-name map
   */
  void Print(FILE *out, temp::Map *map) const;

  /** @brief Get the underlying instruction list */
  [[nodiscard]] assem::InstrList *GetInstrList() const { return instr_list_; }

private:
  assem::InstrList *instr_list_; ///< The generated instruction list
};

/**
 * @brief x86-64 code generator for one function fragment
 *
 * Takes the canonicalized IR traces for one function and produces an
 * abstract assembly instruction list via maximal-munch instruction selection.
 *
 * After Codegen():
 *   1. ProcEntryExit2 appends a return-sink pseudo-instruction.
 *   2. The result is passed to ra::RegAllocator for register allocation.
 *   3. ProcEntryExit3 wraps the result with prologue/epilogue strings.
 *
 * Usage:
 * @code
 *   cg::CodeGen code_gen(frame, std::move(traces));
 *   code_gen.Codegen();
 *   auto assem_instr = code_gen.TransferAssemInstr();
 * @endcode
 */
class CodeGen {
public:
  /**
   * @brief Construct a code generator for one function
   * @param frame  The function's activation record (for frame-size label)
   * @param traces The canonicalized IR traces to translate
   */
  CodeGen(frame::Frame *frame, std::unique_ptr<canon::Traces> traces)
      : frame_(frame), traces_(std::move(traces)) {
        fs_ = frame->frame_size_->Name();
      }

  /**
   * @brief Perform instruction selection
   *
   * Iterates over all statements in the trace list, calling Munch() on
   * each to emit the corresponding assembly instructions.  Then calls
   * ProcEntryExit2 to append the return-sink pseudo-instruction.
   */
  void Codegen();

  /**
   * @brief Transfer ownership of the generated instruction list
   * @return Unique pointer to the AssemInstr object
   */
  std::unique_ptr<AssemInstr> TransferAssemInstr() {
    return std::move(assem_instr_);
  }

private:
  frame::Frame *frame_;                      ///< The function's activation record
  std::string fs_;                           ///< Frame-size label name (for leaq patterns)
  std::unique_ptr<canon::Traces> traces_;    ///< Canonicalized IR traces to translate
  std::unique_ptr<AssemInstr> assem_instr_;  ///< Generated instruction list (output)
};

} // namespace cg

#endif // TIGER_CODEGEN_CODEGEN_H_
