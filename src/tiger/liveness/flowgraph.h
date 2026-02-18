/**
 * @file flowgraph.h
 * @brief Control flow graph (CFG) construction for the Tiger compiler
 *
 * This module builds a control flow graph (CFG) from an assembly instruction
 * list.  The CFG is used as input to liveness analysis.
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Control flow graph structure
 * ─────────────────────────────────────────────────────────────────────────
 * The CFG is a directed graph where:
 *   - Each node represents one assembly instruction
 *   - Each directed edge (n → m) means instruction m can execute immediately
 *     after instruction n
 *
 * Edge types:
 *   - Sequential (fall-through): from each non-jump instruction to the next
 *   - Jump edges: from a jump/branch instruction to its target label(s)
 *   - Conditional branches add both a jump edge (to the true target) and
 *     a fall-through edge (to the next instruction)
 *   - Unconditional jumps (jmp) add only a jump edge (no fall-through)
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Construction algorithm
 * ─────────────────────────────────────────────────────────────────────────
 * FlowGraphFactory::AssemFlowGraph() builds the CFG in two passes:
 *
 *   Pass 1: Create one node per instruction; map labels to nodes
 *   Pass 2: Add edges:
 *     - For OperInstr with jumps_: add edges to all jump targets
 *       (and a fall-through edge for conditional branches)
 *     - For all other instructions: add a fall-through edge to the next node
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Key types
 * ─────────────────────────────────────────────────────────────────────────
 *   FNode     – a node in the flow graph (wraps graph::Node<assem::Instr>)
 *   FNodeList – a list of flow graph nodes
 *   FGraph    – the flow graph (graph::Graph<assem::Instr>)
 *   FGraphPtr – shared pointer to a FGraph
 *   FlowGraphFactory – builds the flow graph from an instruction list
 */

#ifndef TIGER_LIVENESS_FLOWGRAPH_H_
#define TIGER_LIVENESS_FLOWGRAPH_H_

#include <memory>

#include "tiger/codegen/assem.h"
#include "tiger/util/graph.h"

namespace fg {

/** @brief A node in the control flow graph (one per instruction) */
using FNode = graph::Node<assem::Instr>;
/** @brief A list of flow graph nodes */
using FNodeList = graph::NodeList<assem::Instr>;
/** @brief The control flow graph */
using FGraph = graph::Graph<assem::Instr>;
/** @brief Shared pointer to a control flow graph */
using FGraphPtr = std::shared_ptr<FGraph>;

/**
 * @brief Builds a control flow graph from an assembly instruction list
 *
 * Creates one CFG node per instruction and adds edges for sequential
 * execution and jumps.  Also builds a label-to-node map for resolving
 * jump targets.
 *
 * Typical usage:
 * @code
 *   fg::FlowGraphFactory factory;
 *   factory.AssemFlowGraph(instr_list);
 *   fg::FGraphPtr flowgraph = factory.GetFlowGraph();
 * @endcode
 */
class FlowGraphFactory {
public:
  /**
   * @brief Construct a flow graph factory
   *
   * Initialises an empty flow graph and label mapping table.
   */
  explicit FlowGraphFactory()
      : flowgraph_(new FGraph()),
        label_map_(std::make_unique<tab::Table<temp::Label, FNode>>()) {}

  /**
   * @brief Build the control flow graph from an instruction list
   *
   * Pass 1: Create one node per instruction; map LabelInstr labels to nodes.
   * Pass 2: Add edges:
   *   - OperInstr with jumps_: edges to all jump targets
   *     (+ fall-through for conditional branches)
   *   - All other instructions: fall-through edge to next instruction
   *
   * @param instr_list The instruction list for the function
   */
  void AssemFlowGraph(assem::InstrList *instr_list);

  /** @brief Get the constructed flow graph */
  FGraphPtr GetFlowGraph() { return flowgraph_; }

private:
  FGraphPtr flowgraph_;  ///< The flow graph being built

  /** @brief Maps each label to the flow graph node for its LabelInstr */
  std::unique_ptr<tab::Table<temp::Label, FNode>> label_map_;
};

} // namespace fg

#endif // TIGER_LIVENESS_FLOWGRAPH_H_
