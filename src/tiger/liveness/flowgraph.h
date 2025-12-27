/**
 * @file flowgraph.h
 * @brief Control flow graph construction from assembly instructions
 * 
 * This module builds control flow graphs (CFG) from sequences of assembly
 * instructions. The CFG represents the execution flow of a program:
 * - Nodes represent individual instructions
 * - Edges represent possible control flow (sequential execution, jumps, branches)
 * 
 * The flow graph is used by liveness analysis to determine variable lifetimes
 * and build interference graphs for register allocation.
 */

#ifndef TIGER_LIVENESS_FLOWGRAPH_H_
#define TIGER_LIVENESS_FLOWGRAPH_H_

#include "tiger/codegen/assem.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/util/graph.h"

namespace fg {

// Type aliases for flow graph nodes and graphs
using FNode = graph::Node<assem::Instr>;           ///< Flow graph node (instruction)
using FNodePtr = graph::Node<assem::Instr>*;        ///< Pointer to flow graph node
using FNodeListPtr = graph::NodeList<assem::Instr>*; ///< Pointer to node list
using FGraph = graph::Graph<assem::Instr>;          ///< Flow graph type
using FGraphPtr = graph::Graph<assem::Instr>*;      ///< Pointer to flow graph

/**
 * @brief Factory for building control flow graphs
 * 
 * Constructs a control flow graph from a list of assembly instructions.
 * Handles:
 * - Sequential execution (fall-through edges)
 * - Unconditional jumps (jmp instructions)
 * - Conditional jumps (branches with fall-through)
 * - Label targets (for jump destinations)
 */
class FlowGraphFactory {
public:
  /**
   * @brief Construct a new flow graph factory
   * 
   * Initializes an empty flow graph and label mapping table.
   */
  explicit FlowGraphFactory()
      : flowgraph_(new FGraph()),
        label_map_(std::make_unique<tab::Table<temp::Label, FNode>>()) {}
  
  /**
   * @brief Build control flow graph from assembly instructions
   * @param instr_list List of assembly instructions
   * 
   * Creates nodes for each instruction and adds edges representing:
   * - Sequential flow (instruction to next instruction)
   * - Jump targets (jmp/branch instructions to label targets)
   * - Conditional branch fall-through (when branch doesn't jump)
   */
  void AssemFlowGraph(assem::InstrList *instr_list);
  
  /**
   * @brief Get the constructed flow graph
   * @return Pointer to the flow graph
   */
  FGraphPtr GetFlowGraph() { return flowgraph_; }

private:
  FGraphPtr flowgraph_;  ///< The control flow graph
  std::unique_ptr<tab::Table<temp::Label, FNode>> label_map_;  ///< Map labels to nodes (for jump targets)
};

} // namespace fg

#endif