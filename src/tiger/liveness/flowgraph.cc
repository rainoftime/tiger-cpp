/**
 * @file flowgraph.cc
 * @brief Implementation of control flow graph construction
 * 
 * Implements the algorithm to build control flow graphs from assembly
 * instruction lists. The algorithm:
 * 1. Creates nodes for each instruction
 * 2. Maps labels to nodes for jump resolution
 * 3. Adds edges for sequential flow and jumps
 */

#include "tiger/liveness/flowgraph.h"

namespace fg {

void FlowGraphFactory::AssemFlowGraph(assem::InstrList *instr_list) {
  
  // Step 1: Construct the graph by adding all instructions as nodes
  // Also build label map for jump target resolution
  for (assem::Instr *instr : instr_list->GetList()) {
    FNode *node = flowgraph_->NewNode(instr);
    // Map labels to nodes so we can resolve jump targets
    if (typeid(*instr) == typeid(assem::LabelInstr)) {
      assem::LabelInstr *label_instr = static_cast<assem::LabelInstr *>(instr);
      label_map_.get()->Enter(label_instr->label_, node);
    }
  }

  // Step 2: Add edges to the graph representing control flow
  // Process instructions sequentially, adding edges for:
  // - Sequential execution (fall-through)
  // - Jump targets (unconditional and conditional branches)
  auto node_it = flowgraph_->Nodes()->GetList().begin();
  auto last_node_it = --flowgraph_->Nodes()->GetList().end();
  while (node_it != last_node_it) {
    FNode *node = *node_it;
    assem::Instr *instr = node->NodeInfo();
    
    if (typeid(*instr) == typeid(assem::OperInstr)) {
      assem::OperInstr *oper_instr = static_cast<assem::OperInstr *>(instr);

      // Jump instruction: add edges to all jump targets
      if (oper_instr->jumps_ != nullptr) {
        for (temp::Label *label : *oper_instr->jumps_->labels_) {
          flowgraph_->AddEdge(node, label_map_.get()->Look(label)); 
        }

        // Conditional jump: also falls through to next instruction
        // Unconditional jump (jmp): does not fall through
        if (oper_instr->assem_.find("jmp") == std::string::npos)
          flowgraph_->AddEdge(node, *(++node_it));
        else
          ++node_it; 

      // Non-jump instruction: just falls through
      } else {
        flowgraph_->AddEdge(node, *(++node_it));
      }

    } else {
      // Move or Label instruction: falls through to next
      flowgraph_->AddEdge(node, *(++node_it));
    }
  }
}

} // namespace fg

namespace assem {

/**
 * @brief Get temporaries defined (written) by a label instruction
 * @return Empty list (labels don't define temporaries)
 */
temp::TempList *LabelInstr::Def() const {
  return new temp::TempList();
}

/**
 * @brief Get temporaries defined (written) by a move instruction
 * @return List of destination temporaries, or empty if none
 */
temp::TempList *MoveInstr::Def() const {
  return dst_ == nullptr ? new temp::TempList() : dst_;
}

/**
 * @brief Get temporaries defined (written) by an operation instruction
 * @return List of destination temporaries, or empty if none
 */
temp::TempList *OperInstr::Def() const {
  return dst_ == nullptr ? new temp::TempList() : dst_;
}

/**
 * @brief Get temporaries used (read) by a label instruction
 * @return Empty list (labels don't use temporaries)
 */
temp::TempList *LabelInstr::Use() const {
  return new temp::TempList();
}

/**
 * @brief Get temporaries used (read) by a move instruction
 * @return List of source temporaries, or empty if none
 */
temp::TempList *MoveInstr::Use() const {
  return src_ == nullptr ? new temp::TempList() : src_;
}

/**
 * @brief Get temporaries used (read) by an operation instruction
 * @return List of source temporaries, or empty if none
 */
temp::TempList *OperInstr::Use() const {
  return src_ == nullptr ? new temp::TempList() : src_;
}
} // namespace assem
