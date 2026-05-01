/**
 * @file liveness.h
 * @brief Liveness analysis and interference graph construction
 *
 * This module implements the liveness analysis and interference graph
 * construction phases that feed into register allocation.
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Liveness analysis
 * ─────────────────────────────────────────────────────────────────────────
 * Liveness analysis is a backward dataflow analysis that computes, for
 * each instruction n, the sets:
 *
 *   live-in[n]  = use[n] ∪ (live-out[n] − def[n])
 *   live-out[n] = ∪ { live-in[s] | s ∈ succ[n] }
 *
 * The analysis iterates until a fixed point is reached (no set changes).
 * Processing nodes in reverse order (backward) accelerates convergence.
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Interference graph (IGraph)
 * ─────────────────────────────────────────────────────────────────────────
 * Two temporaries interfere if they are simultaneously live at some program
 * point.  The interference graph has one node per temporary and an edge
 * between every pair of interfering temporaries.
 *
 * Construction rule:
 *   For each instruction n that defines temp d:
 *     For each temp t in live-out[n]:
 *       Add edge (d, t) to the interference graph
 *   Special case for moves: the source and destination of a move do NOT
 *   interfere (they can potentially share a register via coalescing).
 *
 * Pre-colored nodes (machine registers) are added first with infinite
 * degree so they are never simplified or spilled.
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Move list
 * ─────────────────────────────────────────────────────────────────────────
 * Move instructions are tracked separately in a MoveList.  Each move
 * (src → dst) is a candidate for coalescing: if src and dst do not
 * interfere, they may be merged into a single node.
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Key types
 * ─────────────────────────────────────────────────────────────────────────
 *   INode      – alias for graph::Node<temp::Temp>
 *   INodeList  – alias for graph::NodeList<temp::Temp>
 *   INodePtr   – pointer to INode
 *   INodeListPtr – pointer to INodeList
 *   IGraph     – alias for graph::IGraph (the interference graph class)
 *   IGraphPtr  – pointer to IGraph
 *   MoveList   – a list of (src, dst) move pairs
 *   LiveGraph  – bundles IGraph + per-node MoveList
 *   LiveGraphFactory – builds the live graph from a flow graph
 *
 * Note: graph::IGraph is defined in util/graph.h and extends
 * graph::Graph<temp::Temp> with adjacency-set and degree tracking.
 */

#ifndef TIGER_LIVENESS_LIVENESS_H_
#define TIGER_LIVENESS_LIVENESS_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "tiger/codegen/assem.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/flowgraph.h"
#include "tiger/util/graph.h"

namespace live {

// ── Type aliases for interference graph nodes ────────────────────────────────
/** @brief A node in the interference graph (one per temporary) */
using INode = graph::Node<temp::Temp>;
/** @brief A pointer to an interference graph node */
using INodePtr = graph::Node<temp::Temp> *;
/** @brief A list of interference graph nodes */
using INodeList = graph::NodeList<temp::Temp>;
/** @brief A pointer to a list of interference graph nodes */
using INodeListPtr = graph::NodeList<temp::Temp> *;
/**
 * @brief The interference graph
 *
 * graph::IGraph (defined in util/graph.h) extends graph::Graph<temp::Temp>
 * with:
 *   - An adjacency set for O(1) edge existence queries
 *   - Per-node degree tracking for the coloring algorithm
 *   - Precolored node support (infinite degree, never simplified)
 *
 * Edges are undirected: AddEdge(u, v) adds both (u,v) and (v,u).
 */
using IGraph = graph::IGraph;
/** @brief A pointer to an interference graph */
using IGraphPtr = graph::IGraph *;

/** @brief A move instruction (src → dst) as a pair of interference graph nodes */
using Move = std::pair<INodePtr, INodePtr>;
/** @brief Iterator type for instruction positions in an InstrList */
using InstrPos = std::list<assem::Instr *>::const_iterator;
/** @brief Maps each interference graph node to the instructions that use/def it */
using NodeInstrMap = std::unordered_map<INode *, std::vector<InstrPos> *>;

/**
 * @brief A list of move instructions (for coalescing)
 *
 * Tracks all move instructions in the function.  Each move is a candidate
 * for coalescing: if the source and destination do not interfere, they may
 * be merged into a single node, eliminating the move.
 *
 * Supports set operations (Union, Intersect, Diff) used by the coloring
 * algorithm to manage the move worklists.
 */
class MoveList {
public:
  MoveList() = default;
  explicit MoveList(Move m) : move_list_({m}) {}

  /** @brief Get the underlying list of moves (read-only) */
  [[nodiscard]] const std::list<Move> &GetList() const { return move_list_; }

  /** @brief Append a move to the end of this list */
  void Append(INodePtr src, INodePtr dst) { move_list_.emplace_back(src, dst); }

  /** @brief Prepend a move to the front of this list */
  void Prepend(INodePtr src, INodePtr dst) { move_list_.emplace_front(src, dst); }

  /** @brief Test whether move (src, dst) is in this list */
  bool Contain(INodePtr src, INodePtr dst);

  /** @brief Remove move (src, dst) from this list */
  void Delete(INodePtr src, INodePtr dst);

  /** @brief Clear all moves from this list */
  void Clear() { move_list_.clear(); }

  /** @brief Compute the set union of this list and @p list */
  MoveList *Union(MoveList *list);

  /** @brief Compute the set intersection of this list and @p list */
  MoveList *Intersect(MoveList *list);

  /** @brief Compute the set difference: this \ list */
  MoveList *Diff(MoveList *list);

private:
  std::list<Move> move_list_;  ///< List of move pairs (src, dst)
};

/**
 * @brief The live graph: interference graph + move information
 *
 * Bundles together:
 *   - interf_graph: the interference graph (one node per temp)
 *   - move_list:    per-node move list (moves involving each node)
 *
 * The global worklist_moves (passed separately to Liveness()) contains all
 * moves that are candidates for coalescing.
 */
struct LiveGraph {
  IGraphPtr interf_graph;                  ///< The interference graph
  tab::Table<INode, MoveList> *move_list;  ///< Per-node move list (for coalescing)

  LiveGraph(IGraphPtr interf_graph, MoveList *moves)
      : interf_graph(interf_graph),
        move_list(new tab::Table<INode, MoveList>()) {}
};

/**
 * @brief Builds the live graph from a control flow graph
 *
 * Performs liveness analysis and constructs the interference graph.
 * The three main steps are:
 *   1. BuildIGraph: add all temps as nodes (precolored first)
 *   2. LiveMap:     compute live-in and live-out sets (iterative dataflow)
 *   3. InterfGraph: add interference edges based on liveness information
 *
 * Typical usage:
 * @code
 *   live::LiveGraphFactory factory;
 *   factory.BuildIGraph(instr_list);
 *   live::MoveList *worklist_moves = new live::MoveList();
 *   factory.Liveness(flowgraph, &worklist_moves);
 *   live::LiveGraph live_graph = factory.GetLiveGraph();
 * @endcode
 */
class LiveGraphFactory {
public:
  /**
   * @brief Construct a live graph factory
   *
   * Initialises the interference graph with precolored registers.
   */
  LiveGraphFactory();

  /**
   * @brief Step 1: Add all temps as nodes in the interference graph
   *
   * Adds precolored registers (machine registers) first with infinite degree.
   * Then adds all other temps that appear in the instruction list.
   * Also builds the node_instr_map_ (which instructions use/def each node).
   *
   * @param instr_list The instruction list for the function
   */
  void BuildIGraph(assem::InstrList *instr_list);

  /**
   * @brief Steps 2+3: Compute liveness and build interference edges
   *
   * Calls LiveMap() to compute live-in/live-out sets, then InterfGraph()
   * to add interference edges.
   *
   * @param flowgraph      The control flow graph for the function
   * @param worklist_moves Output: moves that are candidates for coalescing
   */
  void Liveness(fg::FGraphPtr flowgraph, MoveList **worklist_moves);

  /** @brief Get the constructed live graph */
  LiveGraph GetLiveGraph() { return live_graph_; }

  /** @brief Get the temp-to-node mapping */
  tab::Table<temp::Temp, INode> *GetTempNodeMap() { return temp_node_map_; }

  /** @brief Get the node-to-instruction mapping */
  std::shared_ptr<NodeInstrMap> GetNodeInstrMap() { return node_instr_map_; }

private:
  LiveGraph live_graph_;                              ///< The live graph being built
  std::unique_ptr<graph::Table<assem::Instr, temp::TempList>> in_;  ///< Live-in sets
  std::unique_ptr<graph::Table<assem::Instr, temp::TempList>> out_; ///< Live-out sets
  tab::Table<temp::Temp, INode> *temp_node_map_;      ///< Temp → INode mapping
  std::shared_ptr<NodeInstrMap> node_instr_map_;      ///< INode → instructions mapping

  /**
   * @brief Step 2: Compute live-in and live-out sets (iterative dataflow)
   *
   * Iterates the dataflow equations until a fixed point:
   *   live-in[n]  = use[n] ∪ (live-out[n] − def[n])
   *   live-out[n] = ∪ { live-in[s] | s ∈ succ[n] }
   *
   * @param flowgraph The control flow graph
   */
  void LiveMap(fg::FGraphPtr flowgraph);

  /**
   * @brief Step 3: Add interference edges based on liveness information
   *
   * For each instruction that defines temp d, adds an interference edge
   * between d and every temp in live-out[n].
   * Move instructions are handled specially: source and destination do not
   * interfere (they are move-related instead).
   *
   * @param flowgraph      The control flow graph
   * @param worklist_moves Output: moves that are candidates for coalescing
   */
  void InterfGraph(fg::FGraphPtr flowgraph, MoveList **worklist_moves);
};

} // namespace live

#endif // TIGER_LIVENESS_LIVENESS_H_
