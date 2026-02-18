/**
 * @file color.h
 * @brief Graph coloring for register allocation in the Tiger compiler
 *
 * This file defines the data structures and interface for the graph-coloring
 * phase of register allocation.  It implements the iterated register
 * coalescing algorithm (Appel & George 1996, Appel Chapter 11).
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Graph coloring concepts
 * ─────────────────────────────────────────────────────────────────────────
 * Register allocation is modelled as graph coloring:
 *   - Each temp (virtual register) is a node in the interference graph.
 *   - Two nodes are connected by an edge if the corresponding temps are
 *     simultaneously live (they cannot share a register).
 *   - The number of colors = number of physical registers (K).
 *   - A valid coloring assigns a color (register) to each node such that
 *     no two adjacent nodes have the same color.
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Iterated register coalescing worklists
 * ─────────────────────────────────────────────────────────────────────────
 * The algorithm maintains several worklists of nodes and moves:
 *
 * Node worklists:
 *   precolored        – machine registers (pre-assigned, infinite degree)
 *   initial           – all other temps (not yet classified)
 *   simplifyWorklist  – low-degree, non-move-related nodes (can be simplified)
 *   freezeWorklist    – low-degree, move-related nodes
 *   spillWorklist     – high-degree nodes (candidates for spilling)
 *   spilledNodes      – nodes selected for spilling in this round
 *   coalescedNodes    – nodes that have been coalesced into another node
 *   coloredNodes      – nodes that have been successfully colored
 *   selectStack       – nodes removed from the graph (in simplification order)
 *
 * Move worklists:
 *   coalescedMoves    – moves that have been coalesced
 *   constrainedMoves  – moves whose operands interfere (cannot coalesce)
 *   frozenMoves       – moves that have been frozen (no longer candidates)
 *   worklistMoves     – moves that are candidates for coalescing
 *   activeMoves       – moves not yet ready for coalescing
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Coalescing safety tests
 * ─────────────────────────────────────────────────────────────────────────
 * Two nodes u and v (connected by a move) can be coalesced if:
 *
 *   George's test: for every neighbor t of v,
 *     t already interferes with u, OR degree(t) < K
 *
 *   Briggs' test: the number of neighbors of (u ∪ v) with degree ≥ K
 *     is less than K
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Result
 * ─────────────────────────────────────────────────────────────────────────
 * After coloring, the Result struct contains:
 *   - coloring_: a temp::Map mapping each temp to its physical register name
 *   - spills_:   a list of temps that could not be colored (must be spilled)
 */

#ifndef TIGER_REGALLOC_COLOR_H_
#define TIGER_REGALLOC_COLOR_H_

#include "tiger/codegen/assem.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/util/graph.h"

namespace col {

/**
 * @brief The result of graph coloring
 *
 * After a successful coloring round:
 *   - coloring_ maps each temp to its assigned physical register name
 *   - spills_ lists temps that could not be colored (need spill code)
 *
 * If spills_ is non-empty, the register allocator rewrites the program
 * (inserting loads/stores) and runs another coloring round.
 */
struct Result {
  temp::Map *coloring_; ///< Temp → physical register name (e.g., "%rax")
  live::INodeList *spills_; ///< Nodes that could not be colored (to be spilled)

  Result() : coloring_(nullptr), spills_(nullptr) {}
  Result(temp::Map *coloring, live::INodeList *spills)
      : coloring_(coloring), spills_(spills) {}
};

/**
 * @brief Perform graph coloring on the interference graph
 *
 * Implements the iterated register coalescing algorithm.  Given the
 * interference graph and move list from liveness analysis, assigns a
 * physical register (color) to each temp.
 *
 * The algorithm proceeds as:
 *   1. MakeWorklist: classify all nodes into simplify/freeze/spill worklists
 *   2. Main loop: repeatedly apply Simplify, Coalesce, Freeze, or SelectSpill
 *      until all worklists are empty
 *   3. AssignColors: assign colors to nodes on the select stack
 *
 * @param ig            The interference graph (from liveness analysis)
 * @param worklist_moves Moves that are candidates for coalescing
 * @param temp_node_map  Maps each temp to its interference graph node
 * @param node_instr_map Maps each node to the instructions that use/def it
 *                       (used for spill rewriting)
 * @return Result containing the coloring map and list of spilled nodes
 */
Result Color(live::IGraph *ig, live::MoveList *worklist_moves,
             tab::Table<temp::Temp, live::INode> *temp_node_map,
             live::NodeInstrMap *node_instr_map);

} // namespace col

#endif // TIGER_REGALLOC_COLOR_H_
