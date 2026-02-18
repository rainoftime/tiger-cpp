/**
 * @file canon.h
 * @brief IR tree canonicalization for the Tiger compiler
 *
 * This module transforms the IR tree produced by translation into a
 * canonical form suitable for instruction selection.  It implements the
 * three-phase canonicalization algorithm from Appel Chapter 8:
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Phase 1: Linearize
 * ─────────────────────────────────────────────────────────────────────────
 * Removes ESEQ nodes and ensures that CALL results are immediately captured
 * in a fresh TEMP.  The result is a flat list of statements (StmList) with
 * no nested ESEQ nodes.
 *
 * Key transformations:
 *   ESEQ(s, e)  → hoist s before the expression containing ESEQ
 *   CALL(f, a)  → ESEQ(MOVE(TEMP(t), CALL(f, a)), TEMP(t))
 *                 (so that nested calls don't overwrite each other's results)
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Phase 2: BasicBlocks
 * ─────────────────────────────────────────────────────────────────────────
 * Partitions the linearized statement list into basic blocks.  A basic
 * block is a maximal sequence of statements with:
 *   - Exactly one entry point (a LABEL at the beginning)
 *   - Exactly one exit point (a JUMP or CJUMP at the end)
 *
 * Ensures every block starts with a LABEL and ends with a JUMP/CJUMP.
 * Inserts synthetic labels and jumps as needed.
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Phase 3: TraceSchedule
 * ─────────────────────────────────────────────────────────────────────────
 * Orders the basic blocks into traces.  A trace is a sequence of blocks
 * that can be laid out consecutively in memory, minimising the number of
 * unconditional jumps.
 *
 * After trace scheduling, every CJUMP(op, l, r, t, f) is arranged so that
 * the false label f immediately follows the CJUMP in the instruction stream.
 * This allows the false branch to fall through without a jump.
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Classes
 * ─────────────────────────────────────────────────────────────────────────
 *
 *   StmAndExp   – helper pair: (side-effect statement, value expression)
 *                 Used internally during canonicalization of Exp nodes.
 *
 *   Block       – a basic block: a label + the list of all basic blocks
 *                 (used during trace scheduling to look up blocks by label)
 *
 *   StmListList – a list of basic blocks (each block is a StmList)
 *
 *   Traces      – the result of trace scheduling: an ordered StmList
 *                 ready for instruction selection
 *
 *   Canon       – the main canonicalization driver; owns the IR tree and
 *                 drives all three phases
 */

#ifndef TIGER_CANON_CANON_H_
#define TIGER_CANON_CANON_H_

//#include <cstdio>
#include <list>
#include <memory>
//#include <vector>

#include "tiger/frame/temp.h"
#include "tiger/translate/tree.h"

// Forward Declarations
namespace tree {
class StmList;
class Exp;
class Stm;
} // namespace tree

namespace frame {
class ProcFrag;
}

namespace canon {

/**
 * @brief A list of basic blocks
 *
 * Each element is a tree::StmList representing one basic block.
 * Produced by Canon::BasicBlocks() and consumed by Canon::TraceSchedule().
 */
class StmListList {
  friend class Canon;

public:
  StmListList() = default;

  /** @brief Append a basic block to the list */
  void Append(tree::StmList *stmlist) { stmlist_list_.push_back(stmlist); }

  /** @brief Get the list of basic blocks */
  [[nodiscard]] const std::list<tree::StmList *> &GetList() const {
    return stmlist_list_;
  }

private:
  std::list<tree::StmList *> stmlist_list_; ///< The basic blocks
};

/**
 * @brief A basic block descriptor used during trace scheduling
 *
 * Associates the entry label of a basic block with the complete list of
 * all basic blocks (for label-to-block lookup during trace construction).
 */
class Block {
public:
  temp::Label *label_;       ///< Entry label of the current block
  StmListList *stm_lists_;   ///< All basic blocks (for lookup by label)

  Block() : stm_lists_(nullptr), label_(nullptr) {}
  Block(temp::Label *label, StmListList *stm_lists)
      : label_(label), stm_lists_(stm_lists) {}
};

/**
 * @brief A (statement, expression) pair used during canonicalization
 *
 * When canonicalizing an expression, side effects (statements) must be
 * hoisted out.  StmAndExp bundles the hoisted statement with the
 * remaining pure expression.
 *
 * Invariant: s_ contains all side effects; e_ is side-effect-free
 * (no ESEQ, no CALL at the top level).
 *
 * Note: non-copyable to prevent accidental copies of IR tree nodes.
 */
struct StmAndExp {
  tree::Stm *s_; ///< Hoisted side-effect statement (may be a no-op EXP(CONST(0)))
  tree::Exp *e_; ///< Remaining pure expression

  StmAndExp(const StmAndExp &) = delete;
  StmAndExp &operator=(const StmAndExp &) = delete;
};

/**
 * @brief The result of trace scheduling: a flat, ordered statement list
 *
 * After TraceSchedule(), the statements are ordered so that:
 *   - Every CJUMP is immediately followed by its false-label block
 *   - Unnecessary unconditional jumps are minimised
 *
 * This list is consumed by cg::CodeGen for instruction selection.
 *
 * Note: non-copyable and non-movable; ownership is transferred via
 * Canon::TransferTraces() which returns a unique_ptr.
 */
class Traces {
public:
  Traces() = delete;
  Traces(nullptr_t) = delete;

  /**
   * @brief Construct a Traces object from a statement list
   * @param stm_list The trace-scheduled statement list (must not be null)
   * @throws std::invalid_argument if stm_list is null
   */
  explicit Traces(tree::StmList *stm_list) : stm_list_(stm_list) {
    if (stm_list == nullptr)
      throw std::invalid_argument("NULL pointer is not allowed in Traces");
  }
  Traces(const Traces &traces) = delete;
  Traces(Traces &&traces) = delete;
  Traces &operator=(const Traces &traces) = delete;
  Traces &operator=(Traces &&traces) = delete;
  ~Traces();

  /** @brief Get the ordered statement list */
  [[nodiscard]] tree::StmList *GetStmList() const { return stm_list_; }

private:
  tree::StmList *stm_list_; ///< The trace-scheduled statement list
};

/**
 * @brief IR tree canonicalization driver
 *
 * Drives the three-phase canonicalization of one function's IR tree:
 *   1. Linearize()      – remove ESEQ, wrap CALL results in MOVE(TEMP, CALL)
 *   2. BasicBlocks()    – partition into basic blocks
 *   3. TraceSchedule()  – order blocks into traces
 *
 * Typical usage:
 * @code
 *   canon::Canon canon(body_stm);
 *   tree::StmList *linear = canon.Linearize();
 *   canon::StmListList *blocks = canon.BasicBlocks();
 *   tree::StmList *traces = canon.TraceSchedule();
 *   auto result = canon.TransferTraces();
 * @endcode
 *
 * The three phases must be called in order; each phase uses the result
 * of the previous phase.
 */
class Canon {
  friend class frame::ProcFrag;

public:
  Canon() = delete;

  /**
   * @brief Construct a canonicalization driver for one function body
   * @param stm_ir The IR tree for the function body (after ProcEntryExit1)
   */
  explicit Canon(tree::Stm *stm_ir)
      : stm_ir_(stm_ir), stm_canon_(nullptr), block_(),
        block_env_(new sym::Table<tree::StmList>()) {}

  /**
   * @brief Phase 1: Linearize the IR tree
   *
   * Removes all ESEQ nodes by hoisting their statement parts.
   * Wraps every CALL in MOVE(TEMP(t), CALL(...)) to capture the result.
   * Returns a flat StmList with no ESEQ nodes.
   *
   * Properties of the result:
   *   1. No SEQ or ESEQ nodes
   *   2. The parent of every CALL is EXP(..) or MOVE(TEMP t, ..)
   *
   * @return Pointer to the linearized statement list
   */
  tree::StmList *Linearize();

  /**
   * @brief Phase 2: Partition into basic blocks
   *
   * Splits the linearized statement list into maximal basic blocks.
   * Each block starts with a LABEL and ends with a JUMP or CJUMP.
   * Synthetic labels and jumps are inserted as needed.
   *
   * Properties of the result (in addition to 1-2 above):
   *   3. Every block begins with a LABEL
   *   4. A LABEL appears only at the beginning of a block
   *   5. Any JUMP or CJUMP is the last statement in a block
   *   6. Every block ends with a JUMP or CJUMP
   *
   * Must be called after Linearize().
   *
   * @return Pointer to the list of basic blocks
   */
  canon::StmListList *BasicBlocks();

  /**
   * @brief Phase 3: Order blocks into traces
   *
   * Arranges the basic blocks into a linear order (traces) that minimises
   * unconditional jumps.  Ensures every CJUMP is followed by its false label.
   *
   * Properties of the result (in addition to 1-2 above):
   *   7. Every CJUMP(_, t, f) is immediately followed by LABEL f
   *
   * Must be called after BasicBlocks().
   *
   * @return Pointer to the trace-scheduled statement list
   */
  tree::StmList *TraceSchedule();

  /**
   * @brief Transfer ownership of the trace result to the caller
   * @return Unique pointer to the Traces object
   */
  std::unique_ptr<Traces> TransferTraces() { return std::move(traces_); }

private:
  tree::Stm *stm_ir_;                      ///< Input IR tree (from ProcEntryExit1)
  tree::StmList *stm_canon_;               ///< Result of Linearize()
  Block block_;                            ///< Current block descriptor (for TraceSchedule)
  sym::Table<tree::StmList> *block_env_;   ///< Maps labels to basic blocks (for trace lookup)
  std::unique_ptr<Traces> traces_;         ///< Result of TraceSchedule()

  /**
   * @brief Get the next untraced block from the block list
   *
   * Used by TraceSchedule() to find the next block to add to the current trace.
   * Returns the first block in stm_lists_ that has not yet been traced.
   *
   * @return Pointer to the next untraced block, or nullptr if all blocks are traced
   */
  tree::StmList *GetNext();

  /**
   * @brief Build a trace starting from the given statement list
   *
   * Greedily extends the current trace by following fall-through edges.
   * Modifies the block_env_ to mark blocks as traced.
   *
   * @param stms The statement list for the current block (modified in place)
   */
  void Trace(std::list<tree::Stm *> &stms);
};

} // namespace canon

#endif // TIGER_CANON_CANON_H_
