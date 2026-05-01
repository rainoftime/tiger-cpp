/**
 * @file regalloc.h
 * @brief Register allocation via iterated register coalescing
 *
 * This module implements the register allocator for the Tiger compiler.
 * It uses the **iterated register coalescing** algorithm (Appel & George 1996),
 * which combines graph coloring with aggressive move coalescing.
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Algorithm overview (Appel Chapter 11)
 * ─────────────────────────────────────────────────────────────────────────
 *
 * The allocator operates on the interference graph built by liveness analysis.
 * It iterates the following steps until no more progress can be made:
 *
 *   1. Build        – construct the interference graph (via LiveGraphFactory)
 *   2. MakeWorklist – classify nodes into simplify/freeze/spill worklists
 *   3. Simplify     – remove low-degree, non-move-related nodes
 *   4. Coalesce     – merge move-related pairs (George's or Briggs' test)
 *   5. Freeze       – give up coalescing a low-degree move-related node
 *   6. SelectSpill  – choose a high-degree node to potentially spill
 *   7. AssignColors – assign colors (registers) to nodes on the select stack
 *   8. RewriteProgram – if spills occurred, insert load/store code and restart
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Worklists
 * ─────────────────────────────────────────────────────────────────────────
 * Nodes are classified into mutually exclusive worklists:
 *   precolored_        – machine registers (pre-assigned colors)
 *   initial_           – all other nodes (not yet classified)
 *   simplify_worklist_ – low-degree, non-move-related nodes (ready to simplify)
 *   freeze_worklist_   – low-degree, move-related nodes
 *   spill_worklist_    – high-degree nodes (candidates for spilling)
 *   spilled_nodes_     – nodes selected for spilling
 *   coalesced_nodes_   – nodes that have been coalesced into another
 *   colored_nodes_     – nodes that have been successfully colored
 *   select_stack_      – nodes removed from the graph (in simplification order)
 *
 * Move worklists:
 *   worklist_moves_    – moves that are candidates for coalescing
 *   active_moves_      – moves not yet ready for coalescing
 *   coalesced_moves_   – moves that have been coalesced
 *   constrained_moves_ – moves whose endpoints interfere (cannot coalesce)
 *   frozen_moves_      – moves that will no longer be coalesced
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Coalescing safety tests
 * ─────────────────────────────────────────────────────────────────────────
 * Two nodes u and v can be coalesced if they are connected by a move and
 * do not interfere.  The coalescing is safe if one of these tests passes:
 *
 *   George's test: for every neighbor t of v,
 *     t interferes with u  OR  degree(t) < K
 *
 *   Briggs' test: |{t ∈ adj(u) ∪ adj(v) | degree(t) ≥ K}| < K
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Spill rewriting
 * ─────────────────────────────────────────────────────────────────────────
 * When a node is spilled, RewriteProgram() allocates a new frame slot and
 * replaces each use/def of the spilled temp with fresh temps and
 * load/store instructions:
 *   - Before each use:  movq slot(%rsp), t_new
 *   - After each def:   movq t_new, slot(%rsp)
 * The allocator then restarts from scratch with the rewritten program.
 */

#ifndef TIGER_REGALLOC_REGALLOC_H_
#define TIGER_REGALLOC_REGALLOC_H_

#include <unordered_map>

#include "tiger/codegen/assem.h"
#include "tiger/codegen/codegen.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/regalloc/color.h"
#include "tiger/util/graph.h"

namespace ra {

/**
 * @brief The result of register allocation
 *
 * Contains:
 *   coloring_ – a temp::Map mapping each virtual register to a physical
 *               register name string (e.g., "%rax")
 *   il_       – the final instruction list (with spill code inserted if needed)
 *
 * Non-copyable and non-movable; ownership is transferred via
 * RegAllocator::TransferResult().
 */
class Result {
public:
  temp::Map *coloring_;    ///< Virtual register → physical register name
  assem::InstrList *il_;   ///< Final instruction list (after spill rewriting)

  Result() : coloring_(nullptr), il_(nullptr) {}
  Result(temp::Map *coloring, assem::InstrList *il)
      : coloring_(coloring), il_(il) {}
  Result(const Result &result) = delete;
  Result(Result &&result) = delete;
  Result &operator=(const Result &result) = delete;
  Result &operator=(Result &&result) = delete;
  ~Result() { delete coloring_; delete il_; }
};

/**
 * @brief Register allocator using iterated register coalescing
 *
 * Performs the complete register allocation pipeline for one function:
 *   1. Build the interference graph (via liveness analysis)
 *   2. Iteratively simplify, coalesce, freeze, and spill
 *   3. Assign colors (physical registers) to virtual registers
 *   4. Rewrite the program if spills occurred, then restart
 *
 * Typical usage:
 * @code
 *   ra::RegAllocator reg_allocator(frame, std::move(assem_instr));
 *   reg_allocator.RegAlloc();
 *   auto result = reg_allocator.TransferResult();
 *   // result->coloring_ maps temps to register names
 *   // result->il_ is the final instruction list
 * @endcode
 */
class RegAllocator {
public:
  /**
   * @brief Construct a register allocator for one function
   * @param frame       The activation record for the function
   * @param assem_instr The abstract assembly instruction list (with virtual regs)
   */
  RegAllocator(frame::Frame *frame, std::unique_ptr<cg::AssemInstr> assem_instr);

  /**
   * @brief Perform register allocation
   *
   * Runs the iterated register coalescing algorithm until no spills remain.
   * After this call, TransferResult() returns the final colored instruction list.
   */
  void RegAlloc();

  /**
   * @brief Transfer ownership of the allocation result to the caller
   * @return Unique pointer to the Result (coloring + final instruction list)
   */
  std::unique_ptr<Result> TransferResult();

private:
  frame::Frame *frame_;                          ///< Activation record for the function
  std::unique_ptr<cg::AssemInstr> assem_instr_;  ///< Abstract assembly (input)
  live::LiveGraphFactory *live_graph_factory_;   ///< Liveness analysis factory
  fg::FlowGraphFactory *flow_graph_factory_;     ///< Control flow graph factory

  // ── Node worklists ──────────────────────────────────────────────────────
  live::INodeList *precolored_;         ///< Machine register nodes (pre-colored)
  live::INodeList *initial_;            ///< Unclassified nodes (not yet in any worklist)
  live::INodeList *select_stack_;       ///< Nodes removed during simplification (LIFO)
  live::INodeList *simplify_worklist_;  ///< Low-degree, non-move-related nodes
  live::INodeList *freeze_worklist_;    ///< Low-degree, move-related nodes
  live::INodeList *spill_worklist_;     ///< High-degree nodes (spill candidates)
  live::INodeList *spilled_nodes_;      ///< Nodes selected for actual spilling
  live::INodeList *coalesced_nodes_;    ///< Nodes merged into another node
  live::INodeList *colored_nodes_;      ///< Nodes that have been assigned a color

  // ── Move worklists ───────────────────────────────────────────────────────
  live::MoveList *coalesced_moves_;     ///< Moves that have been coalesced
  live::MoveList *constrained_moves_;   ///< Moves whose endpoints interfere
  live::MoveList *frozen_moves_;        ///< Moves that will no longer be coalesced
  live::MoveList *worklist_moves_;      ///< Moves that are candidates for coalescing
  live::MoveList *active_moves_;        ///< Moves not yet ready for coalescing

  // ── Auxiliary maps ───────────────────────────────────────────────────────
  std::unordered_map<live::INode*, live::INode*> alias_; ///< Coalescing alias map
  std::unordered_map<live::INode*, int> color_;          ///< Node → color (register index)

  std::unique_ptr<Result> result_;  ///< The allocation result (built by AssignColors)

  temp::Map *global_map_;  ///< Debug: global temp-to-name map

  // ── Initialization ───────────────────────────────────────────────────────
  /** @brief Initialize the color map from precolored registers */
  void InitColor();
  /** @brief Initialize the alias map (each node is its own alias initially) */
  void InitAlias();

  // ── Main algorithm steps ─────────────────────────────────────────────────
  /**
   * @brief Classify all nodes into simplify/freeze/spill worklists
   *
   * A node goes to:
   *   simplify_worklist_ if degree < K and not move-related
   *   freeze_worklist_   if degree < K and move-related
   *   spill_worklist_    if degree >= K
   */
  void MakeWorkList();

  /**
   * @brief Compute the set of non-precolored, non-coalesced neighbors of n
   * @return Adjacency list excluding precolored and coalesced nodes
   */
  live::INodeList *Adjacent(live::INode *n);

  /**
   * @brief Compute the moves associated with node n that are active or in worklist
   * @return Intersection of moves(n) with (active_moves_ ∪ worklist_moves_)
   */
  live::MoveList *NodeMoves(live::INode *n);

  /**
   * @brief Test whether node n is involved in any active or worklist move
   * @return true if NodeMoves(n) is non-empty
   */
  bool MoveRelated(live::INode *n);

  /**
   * @brief Remove a low-degree, non-move-related node from the graph
   *
   * Pushes the node onto select_stack_ and decrements the degree of its
   * neighbors (potentially enabling them for simplification or coalescing).
   */
  void Simplify();

  /**
   * @brief Decrement the degree of node n by 1
   *
   * If n's degree drops below K, it may be moved from spill_worklist_ to
   * simplify_worklist_ or freeze_worklist_.
   */
  void DecrementDegree(live::INode *n);

  /**
   * @brief Enable moves associated with the given nodes for coalescing
   *
   * Moves active_moves that involve any node in @p nodes to worklist_moves_.
   */
  void EnableMoves(live::INodeList *nodes);

  /**
   * @brief Attempt to coalesce a move from worklist_moves_
   *
   * Tries to merge the source and destination of a move using George's or
   * Briggs' safety test.  If coalescing is safe, merges the two nodes.
   */
  void Coalesce();

  /**
   * @brief Move node u from freeze_worklist_ to simplify_worklist_ if safe
   *
   * Called after coalescing to check if u is now non-move-related.
   */
  void AddWorkList(live::INode *u);

  /**
   * @brief George's coalescing safety test
   *
   * Returns true if for every neighbor t of v:
   *   t interferes with u  OR  degree(t) < K
   *
   * @param u The node to coalesce into
   * @param v The node to be coalesced
   */
  bool OK(live::INode *t, live::INode *r);

  /**
   * @brief Briggs' coalescing safety test
   *
   * Returns true if the number of high-degree (≥ K) nodes in adj(u) ∪ adj(v)
   * is less than K.
   *
   * @param nodes The union of adjacency lists of the two nodes
   */
  bool Conservative(live::INodeList *nodes);

  /**
   * @brief Get the canonical representative of node n (following alias chain)
   *
   * After coalescing, coalesced nodes point to their representative via alias_.
   * GetAlias() follows the chain to find the root.
   */
  live::INode *GetAlias(live::INode *n);

  /**
   * @brief Merge node v into node u (coalescing)
   *
   * Updates the interference graph, worklists, and alias map.
   */
  void Combine(live::INode *u, live::INode *v);

  /** @brief George's test: check if coalescing u←v is safe for neighbor t */
  bool George(live::INode *u, live::INode *v);
  /** @brief Briggs' test: check if coalescing u and v is safe */
  bool Briggs(live::INode *u, live::INode *v);
  /** @brief Test whether node n is a precolored (machine register) node */
  bool IsPrecolored(live::INode *n);
  /** @brief Test whether nodes u and v are adjacent in the interference graph */
  bool AreAdj(live::INode *u, live::INode *v);

  /**
   * @brief Give up coalescing a low-degree move-related node
   *
   * Moves a node from freeze_worklist_ to simplify_worklist_ and freezes
   * all moves associated with it.
   */
  void Freeze();

  /**
   * @brief Freeze all moves associated with node u
   *
   * Moves the moves from active_moves_/worklist_moves_ to frozen_moves_.
   * May enable the other endpoint of each move for simplification.
   */
  void FreezeMoves(live::INode *u);

  /**
   * @brief Select a node from spill_worklist_ as a potential spill
   *
   * Uses a heuristic (HeuristicSelect) to choose the best candidate.
   * Moves the selected node to simplify_worklist_ (optimistic coloring).
   */
  void SelectSpill();

  /**
   * @brief Heuristic for choosing which node to spill
   *
   * Selects the node whose observed def-to-next-use distance is largest,
   * with an immediate preference for temps that are defined but never used.
   * This is a simple approximation of spill cost rather than a pure
   * degree-based policy.
   *
   * @return The node selected for potential spilling
   */
  live::INode *HeuristicSelect();

  /**
   * @brief Assign colors (physical registers) to nodes on the select stack
   *
   * Pops nodes from select_stack_ and assigns the lowest available color
   * not used by any neighbor.  Nodes that cannot be colored are added to
   * spilled_nodes_.
   */
  void AssignColors();

  /**
   * @brief Rewrite the program to handle actual spills
   *
   * For each node in spilled_nodes_:
   *   1. Allocate a new frame slot
   *   2. Replace each use with: movq slot(%rsp), t_new  (before the instruction)
   *   3. Replace each def with:  movq t_new, slot(%rsp) (after the instruction)
   * Then restarts the allocation from scratch with the rewritten program.
   */
  void RewriteProgram();

  // ── Debug helpers ────────────────────────────────────────────────────────
  void PrintMoveList();  ///< Debug: print all move worklists
  void PrintAlias();     ///< Debug: print the alias map
  void PrintNodeList();  ///< Debug: print all node worklists
};

} // namespace ra

#endif // TIGER_REGALLOC_REGALLOC_H_
