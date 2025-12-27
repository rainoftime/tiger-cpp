/**
 * @file liveness.h
 * @brief Liveness analysis and interference graph construction
 * 
 * This module performs liveness analysis to determine when variables (temporaries)
 * are live (may be used) at each program point. It then builds an interference
 * graph where:
 * - Nodes represent temporaries (variables/registers)
 * - Edges represent interference (two temporaries can't share a register)
 * 
 * The interference graph is used by register allocation (graph coloring) to
 * assign registers to temporaries. The analysis also identifies move instructions
 * that can be coalesced (source and destination can share a register).
 * 
 * Algorithm:
 * 1. LiveMap: Compute live-in and live-out sets for each instruction (iterative dataflow)
 * 2. InterfGraph: Build interference graph from liveness information
 * 3. Move tracking: Identify move instructions for coalescing
 */

#ifndef TIGER_LIVENESS_LIVENESS_H_
#define TIGER_LIVENESS_LIVENESS_H_

#include "tiger/codegen/assem.h"
#include "tiger/frame/x64frame.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/flowgraph.h"
#include "tiger/util/graph.h"

namespace live {

// Type aliases for interference graph
using INode = graph::Node<temp::Temp>;           ///< Interference graph node (temporary)
using INodePtr = graph::Node<temp::Temp>*;       ///< Pointer to interference node
using INodeList = graph::NodeList<temp::Temp>;   ///< List of interference nodes
using INodeListPtr = graph::NodeList<temp::Temp>*; ///< Pointer to node list
using IGraph = graph::IGraph;                    ///< Interference graph type
using IGraphPtr = graph::IGraph*;               ///< Pointer to interference graph
using Move = std::pair<INodePtr, INodePtr>;     ///< Move instruction (src -> dst)
using InstrPos = std::list<assem::Instr *>::const_iterator;  ///< Instruction position
using NodeInstrMap = std::unordered_map<INode*, std::vector<InstrPos>*>;  ///< Map nodes to instruction positions

/**
 * @brief List of move instructions for coalescing
 * 
 * Tracks move instructions (src -> dst) that can potentially be coalesced
 * during register allocation if source and destination don't interfere.
 */
class MoveList {
public:
  MoveList() = default;
  explicit MoveList(Move m) : move_list_({m}) {}

  [[nodiscard]] const std::list<Move> &GetList() const { return move_list_; }
  void Append(INodePtr src, INodePtr dst) { move_list_.emplace_back(src, dst); }
  bool Contain(INodePtr src, INodePtr dst);
  void Delete(INodePtr src, INodePtr dst);
  void Clear() { move_list_.clear(); }
  void Prepend(INodePtr src, INodePtr dst) {
    move_list_.emplace_front(src, dst);
  }
  MoveList *Union(MoveList *list);
  MoveList *Intersect(MoveList *list);
  MoveList *Diff(MoveList *list);

private:
  std::list<Move> move_list_;  ///< List of move pairs (src, dst)
};

/**
 * @brief Liveness analysis result
 * 
 * Contains the interference graph and move lists computed by liveness analysis.
 * Used by register allocation to assign registers to temporaries.
 */
struct LiveGraph {
  IGraphPtr interf_graph;  ///< Interference graph (temporaries that conflict)
  tab::Table<INode, MoveList> *move_list;  ///< Move instructions per node (for coalescing)

  LiveGraph(IGraphPtr interf_graph, MoveList *moves)
      : interf_graph(interf_graph), 
        move_list(new tab::Table<INode, MoveList>()) {}
};

/**
 * @brief Factory for liveness analysis and interference graph construction
 * 
 * Performs liveness analysis on a control flow graph and builds an interference
 * graph. The analysis uses iterative dataflow to compute live-in and live-out
 * sets for each instruction.
 */
class LiveGraphFactory {
public:
  /**
   * @brief Construct a new liveness analysis factory
   * 
   * Initializes the interference graph with precolored registers (machine registers
   * that are never spilled) and data structures for liveness analysis.
   */
  LiveGraphFactory();
        
  /**
   * @brief Perform liveness analysis and build interference graph
   * @param flowgraph Control flow graph to analyze
   * @param worklist_moves Output: list of move instructions for coalescing
   * 
   * Computes liveness information and builds the interference graph.
   * Identifies move instructions that can be coalesced.
   */
  void Liveness(fg::FGraphPtr flowgraph, MoveList **worklist_moves);
  
  /**
   * @brief Build interference graph nodes from instruction list
   * @param instr_list List of assembly instructions
   * 
   * Creates nodes in the interference graph for all temporaries used
   * or defined in the instructions. Maps temporaries to nodes and tracks
   * which instructions use each temporary.
   */
  void BuildIGraph(assem::InstrList *instr_list);
  
  /**
   * @brief Get the computed live graph
   * @return LiveGraph containing interference graph and move lists
   */
  LiveGraph GetLiveGraph() { return live_graph_; }
  
  /**
   * @brief Get mapping from nodes to instruction positions
   * @return Map from interference nodes to instruction positions
   */
  std::shared_ptr<NodeInstrMap> GetNodeInstrMap() { return node_instr_map_; }
  
  /**
   * @brief Get mapping from temporaries to interference nodes
   * @return Map from temporaries to their interference graph nodes
   */
  tab::Table<temp::Temp, INode> *GetTempNodeMap() { return temp_node_map_; }

private:
  LiveGraph live_graph_;  ///< The computed interference graph and move lists

  // Liveness analysis data structures
  std::unique_ptr<graph::Table<assem::Instr, temp::TempList>> in_;   ///< Live-in sets per instruction
  std::unique_ptr<graph::Table<assem::Instr, temp::TempList>> out_;  ///< Live-out sets per instruction
  tab::Table<temp::Temp, INode> *temp_node_map_;  ///< Map temporaries to interference nodes
  std::shared_ptr<NodeInstrMap> node_instr_map_;   ///< Map nodes to instruction positions

  /**
   * @brief Compute live-in and live-out sets (iterative dataflow)
   * @param flowgraph Control flow graph to analyze
   * 
   * Uses iterative dataflow analysis to compute which temporaries are live
   * at each instruction. A temporary is live if it may be used before being redefined.
   */
  void LiveMap(fg::FGraphPtr flowgraph);
  
  /**
   * @brief Build interference graph from liveness information
   * @param flowgraph Control flow graph
   * @param worklist_moves Output: move instructions for coalescing
   * 
   * Two temporaries interfere if they are both live at the same point.
   * Also identifies move instructions that can be coalesced.
   */
  void InterfGraph(fg::FGraphPtr flowgraph, MoveList **worklist_moves);
};

} // namespace live

#endif