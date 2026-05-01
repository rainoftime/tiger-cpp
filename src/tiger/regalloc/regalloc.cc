/**
 * @file regalloc.cc
 * @brief Iterated Register Coalescing (IRC) – implementation
 *
 * This file implements the complete register allocation algorithm described in:
 *   "Iterated Register Coalescing" – Appel & George, TOPLAS 1996
 *   (also presented in Appel's "Modern Compiler Implementation", Chapter 11)
 *
 * ═══════════════════════════════════════════════════════════════════════════
 * HIGH-LEVEL ALGORITHM
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Register allocation maps an unlimited set of virtual registers (temporaries)
 * to a limited set of K physical registers.  The problem is equivalent to
 * graph K-coloring: two temporaries that are simultaneously live must receive
 * different colors (registers).
 *
 * The IRC algorithm proceeds as follows:
 *
 *   Build       – construct the interference graph from liveness information
 *   MakeWorklist– classify nodes into simplify / freeze / spill worklists
 *
 *   Repeat until all worklists are empty:
 *     Simplify  – remove a low-degree, non-move-related node
 *     Coalesce  – merge a move-related pair (if safe)
 *     Freeze    – give up coalescing a low-degree move-related node
 *     SelectSpill – choose a high-degree node to potentially spill
 *
 *   AssignColors – assign colors to nodes popped from the select stack
 *
 *   If spills occurred:
 *     RewriteProgram – insert load/store code, restart from Build
 *
 * ═══════════════════════════════════════════════════════════════════════════
 * NODE WORKLISTS (mutually exclusive partitions of all nodes)
 * ═══════════════════════════════════════════════════════════════════════════
 *
 *   precolored_        – machine registers (fixed colors, never simplified)
 *   initial_           – all other nodes before classification
 *   simplify_worklist_ – low-degree (< K), non-move-related
 *   freeze_worklist_   – low-degree (< K), move-related
 *   spill_worklist_    – high-degree (≥ K), spill candidates
 *   spilled_nodes_     – nodes selected for actual spilling
 *   coalesced_nodes_   – nodes merged into another node
 *   colored_nodes_     – nodes that received a color
 *   select_stack_      – nodes removed during simplification (LIFO)
 *
 * ═══════════════════════════════════════════════════════════════════════════
 * MOVE WORKLISTS (mutually exclusive partitions of all moves)
 * ═══════════════════════════════════════════════════════════════════════════
 *
 *   worklist_moves_    – moves that are candidates for coalescing
 *   active_moves_      – moves not yet ready for coalescing
 *   coalesced_moves_   – moves that have been coalesced (eliminated)
 *   constrained_moves_ – moves whose endpoints interfere (cannot coalesce)
 *   frozen_moves_      – moves that will no longer be coalesced
 *
 * ═══════════════════════════════════════════════════════════════════════════
 * COALESCING SAFETY TESTS
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Two nodes u and v connected by a move can be coalesced if they do not
 * already interfere AND one of these tests passes:
 *
 *   George's test (used when u is precolored):
 *     For every neighbor t of v:
 *       t already interferes with u  OR  degree(t) < K
 *     Intuition: coalescing v into u cannot make u harder to color because
 *     every neighbor of v either already conflicts with u or is low-degree.
 *
 *   Briggs' test (used when neither u nor v is precolored):
 *     |{t ∈ adj(u) ∪ adj(v) | degree(t) ≥ K}| < K
 *     Intuition: the combined node has fewer than K high-degree neighbors,
 *     so it can always be colored.
 *
 * ═══════════════════════════════════════════════════════════════════════════
 * SPILL REWRITING
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * When a node cannot be colored (actual spill), RewriteProgram():
 *   1. Allocates a new frame slot for the spilled temporary
 *   2. For each USE of the spilled temp t at instruction i:
 *        Insert before i:  movq slot(%rsp), t_new
 *        Replace t with t_new in i's use list
 *   3. For each DEF of the spilled temp t at instruction i:
 *        Insert after i:   movq t_new, slot(%rsp)
 *        Replace t with t_new in i's def list
 *   4. Restarts the entire allocation with the rewritten program
 *
 * Each new temporary t_new has a very short live range (just one instruction),
 * so it is unlikely to be spilled again.
 */

#include "tiger/regalloc/regalloc.h"

#include "tiger/output/logger.h"

#include <sstream>

extern frame::RegManager *reg_manager;

namespace ra {

namespace {

bool IsArm64Target() { return frame::IsArm64AppleTarget(); }

temp::TempList *SpillBaseUseList(temp::Temp *extra = nullptr) {
  if (IsArm64Target()) {
    if (extra)
      return new temp::TempList({extra, reg_manager->FramePointer()});
    return new temp::TempList(reg_manager->FramePointer());
  }

  if (extra)
    return new temp::TempList({extra, reg_manager->StackPointer()});
  return new temp::TempList(reg_manager->StackPointer());
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

RegAllocator::RegAllocator(frame::Frame *frame, std::unique_ptr<cg::AssemInstr> assem_instr)
  : frame_(frame), assem_instr_(std::move(assem_instr)) {

  // Build a global temp→name map for debug printing.
  // LayerMap(A, B) looks up in A first, then falls back to B.
  global_map_ = temp::Map::LayerMap(reg_manager->temp_map_, temp::Map::Name());

  // ── Node worklists (all start empty; populated by MakeWorkList) ──────────
  precolored_ = new live::INodeList();

  simplify_worklist_ = new live::INodeList();
  freeze_worklist_ = new live::INodeList();
  spill_worklist_ = new live::INodeList();

  spilled_nodes_ = new live::INodeList();
  coalesced_nodes_ = new live::INodeList();
  colored_nodes_ = new live::INodeList();

  select_stack_ = new live::INodeList();

  // ── Move worklists (all start empty; populated by liveness analysis) ─────
  coalesced_moves_ = new live::MoveList();
  constrained_moves_ = new live::MoveList();
  frozen_moves_ = new live::MoveList();
  worklist_moves_ = new live::MoveList();
  active_moves_ = new live::MoveList();
}

// ─────────────────────────────────────────────────────────────────────────────
// RegAlloc – top-level driver
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Top-level register allocation driver.
 *
 * This function implements the outer loop of the IRC algorithm.  It may be
 * called recursively if actual spills are discovered (after RewriteProgram).
 *
 * Steps:
 *   1. Build the interference graph (BuildIGraph + AssemFlowGraph + Liveness)
 *   2. Initialize precolored nodes and alias map
 *   3. Classify all nodes into worklists (MakeWorkList)
 *   4. Iterate: Simplify → Coalesce → Freeze → SelectSpill until done
 *   5. AssignColors: pop nodes from select_stack_ and assign registers
 *   6a. If spills: RewriteProgram + recursive RegAlloc
 *   6b. If no spills: remove now-redundant move instructions (src == dst)
 */
void RegAllocator::RegAlloc() {

  // ── Step 1: Build interference graph ─────────────────────────────────────
  flow_graph_factory_ = new fg::FlowGraphFactory();
  live_graph_factory_ = new live::LiveGraphFactory();

  // Add all interference graph nodes (precolored registers first, then temps).
  live_graph_factory_->BuildIGraph(assem_instr_.get()->GetInstrList());

  // Build the control flow graph (nodes = instructions, edges = control flow).
  flow_graph_factory_->AssemFlowGraph(assem_instr_.get()->GetInstrList());

  // Run liveness analysis: compute live-in/live-out sets and build interference
  // edges.  Also populates worklist_moves_ with move candidates for coalescing.
  live_graph_factory_->Liveness(flow_graph_factory_->GetFlowGraph(), &worklist_moves_);

  // ── Step 2: Initialize auxiliary maps ────────────────────────────────────
  // Assign colors 0..K-1 to precolored nodes (machine registers).
  InitColor();
  // Initialize alias map: each node is its own alias (no coalescing yet).
  InitAlias();
  // initial_ = all nodes except precolored (these need to be colored).
  initial_ = live_graph_factory_->GetLiveGraph().interf_graph->Nodes()->Diff(precolored_);

  // ── Step 3: Classify nodes into worklists ─────────────────────────────────
  MakeWorkList();

  // ── Step 4: Main loop ─────────────────────────────────────────────────────
  // Priority order: Simplify > Coalesce > Freeze > SelectSpill.
  // This ensures we always make the most conservative progress first.
  do {
    if (!simplify_worklist_->GetList().empty())
      // Remove a low-degree, non-move-related node (safe to color later).
      Simplify();
    else if (!worklist_moves_->GetList().empty())
      // Try to merge a move-related pair (eliminates a move instruction).
      Coalesce();
    else if (!freeze_worklist_->GetList().empty())
      // Give up coalescing a low-degree move-related node.
      Freeze();
    else if (!spill_worklist_->GetList().empty())
      // Optimistically push a high-degree node (may become an actual spill).
      SelectSpill();
  } while (!(simplify_worklist_->GetList().empty()
           && worklist_moves_->GetList().empty()
           && freeze_worklist_->GetList().empty()
           && spill_worklist_->GetList().empty()));

  // ── Step 5: Assign colors ─────────────────────────────────────────────────
  // Pop nodes from select_stack_ and assign the lowest available color.
  // Nodes that cannot be colored are added to spilled_nodes_.
  AssignColors();

  // ── Step 6: Handle spills or finalize ────────────────────────────────────
  if (!spilled_nodes_->GetList().empty()) {
    // Actual spills: insert load/store code and restart allocation.
    RewriteProgram();
    RegAlloc();  // Recursive restart with rewritten program

  } else {
    // No spills: remove move instructions whose source and destination
    // received the same physical register (they are now no-ops).
    assem::InstrList *instr_list = assem_instr_.get()->GetInstrList();
    std::vector<live::InstrPos> delete_moves;
    for (auto instr_it = instr_list->GetList().begin();
         instr_it != instr_list->GetList().end(); instr_it++) {
      assem::Instr *instr = *instr_it;
      if (typeid(*instr) == typeid(assem::MoveInstr)) {
        assem::MoveInstr *move_instr = static_cast<assem::MoveInstr*>(*instr_it);
        temp::Temp *src_reg = move_instr->src_->GetList().front();
        temp::Temp *dst_reg = move_instr->dst_->GetList().front();
        live::INode *src_n = live_graph_factory_->GetTempNodeMap()->Look(src_reg);
        live::INode *dst_n = live_graph_factory_->GetTempNodeMap()->Look(dst_reg);
        // If both ends got the same color, the move is a no-op → delete it.
        if (color_.at(src_n) == color_.at(dst_n))
          delete_moves.push_back(instr_it);
      }
    }

    for (auto it : delete_moves)
      instr_list->Erase(it);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// TransferResult
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Build the final coloring map and transfer ownership to the caller.
 *
 * Iterates over color_ (node → color index) and builds a temp::Map that maps
 * each virtual register (temp::Temp*) to the name string of the physical
 * register it was assigned (e.g., "%rax").
 *
 * The name string is looked up via:
 *   global_map_->Look(reg_manager->Registers()->NthTemp(c))
 * where c is the color index and NthTemp(c) returns the c-th machine register.
 */
std::unique_ptr<Result> RegAllocator::TransferResult() {
  temp::Map *coloring = temp::Map::Empty();
  for (auto node_color : color_) {
    temp::Temp *reg = node_color.first->NodeInfo();  // virtual register
    int c = node_color.second;                        // assigned color index
    // Look up the physical register name for color c
    std::string *str = global_map_->Look(reg_manager->Registers()->NthTemp(c));
    coloring->Enter(reg, str);
  }
  result_ = std::make_unique<Result>(coloring, assem_instr_.get()->GetInstrList());
  return std::move(result_);
}

// ─────────────────────────────────────────────────────────────────────────────
// MakeWorkList – initial node classification
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Classify all non-precolored nodes into one of three worklists:
 *
 *   spill_worklist_    if degree(n) ≥ K  (high-degree, hard to color)
 *   freeze_worklist_   if degree(n) < K  AND  n is move-related
 *   simplify_worklist_ if degree(n) < K  AND  n is NOT move-related
 *
 * K = reg_manager->RegCount() = number of allocatable registers.
 *
 * Move-related means the node is the source or destination of a move that
 * is still a candidate for coalescing (in worklist_moves_ or active_moves_).
 */
void RegAllocator::MakeWorkList() {
  for (live::INode *n : initial_->GetList()) {
    live::INodeList *single_n = new live::INodeList(n);
    if (n->IDegree() >= reg_manager->RegCount())
      // High-degree: may need to spill
      spill_worklist_ = spill_worklist_->Union(single_n);
    else if (MoveRelated(n))
      // Low-degree but involved in a move: defer simplification to allow coalescing
      freeze_worklist_ = freeze_worklist_->Union(single_n);
    else
      // Low-degree, not move-related: safe to simplify immediately
      simplify_worklist_ = simplify_worklist_->Union(single_n);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Helper predicates and accessors
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Return the current neighbors of n in the interference graph.
 *
 * Excludes nodes that have already been removed from the graph:
 *   - select_stack_:    nodes removed during Simplify
 *   - coalesced_nodes_: nodes merged into another node
 *
 * This gives the "effective" adjacency list used by the coloring algorithm.
 */
live::INodeList *RegAllocator::Adjacent(live::INode *n) {
  return n->AdjList()->Diff(select_stack_->Union(coalesced_nodes_));
}

/**
 * Return the moves associated with node n that are still active.
 *
 * A move is "active" for n if it is in active_moves_ or worklist_moves_
 * (i.e., it has not yet been coalesced, constrained, or frozen).
 *
 * Used to determine whether n is move-related and to find coalescing candidates.
 */
live::MoveList *RegAllocator::NodeMoves(live::INode *n) {
  live::MoveList *node_moves = live_graph_factory_->GetLiveGraph().move_list->Look(n);
  // Intersect with (active_moves_ ∪ worklist_moves_) to get only live moves
  return node_moves->Intersect(active_moves_->Union(worklist_moves_));
}

/**
 * Return true if node n is involved in any active or worklist move.
 *
 * Move-related nodes are kept in freeze_worklist_ rather than
 * simplify_worklist_ to give coalescing a chance to eliminate the move.
 */
bool RegAllocator::MoveRelated(live::INode *n) {
  return !NodeMoves(n)->GetList().empty();
}

// ─────────────────────────────────────────────────────────────────────────────
// Simplify – remove a low-degree, non-move-related node
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Remove a low-degree, non-move-related node from the interference graph.
 *
 * The node is pushed onto select_stack_ (LIFO).  When AssignColors() later
 * pops it, the node's neighbors will have been colored, so there will be a
 * free color available (since degree < K).
 *
 * Removing the node effectively decrements the degree of all its neighbors,
 * which may enable them to move from spill_worklist_ to simplify_worklist_
 * or freeze_worklist_.
 */
void RegAllocator::Simplify() {
  live::INode *n = simplify_worklist_->GetList().front();
  simplify_worklist_->DeleteNode(n);
  select_stack_->Prepend(n);  // push onto stack (LIFO order for AssignColors)
  live::INodeList *adj_nodes = Adjacent(n);
  for (live::INode *m : adj_nodes->GetList())
    DecrementDegree(m);  // removing n reduces the effective degree of each neighbor
}

/**
 * Decrement the effective degree of node m by 1.
 *
 * If m's degree drops from K to K-1 (the "threshold"), m transitions from
 * a high-degree node to a low-degree node.  This may enable:
 *   - Moves involving m or its neighbors to be reconsidered for coalescing
 *   - m itself to move from spill_worklist_ to freeze_worklist_ or
 *     simplify_worklist_
 *
 * Precolored nodes are never decremented (their degree is conceptually ∞).
 */
void RegAllocator::DecrementDegree(live::INode *m) {
  if (precolored_->Contain(m))
    return;  // precolored nodes have infinite degree; never decrement
  int d = m->IDegree();
  m->MinusOneIDegree();
  if (d == reg_manager->RegCount()) {
    // m just crossed the threshold from high-degree to low-degree.
    // Enable moves for m and its neighbors (they may now be coalesceable).
    live::INodeList *single_m = new live::INodeList(m);
    EnableMoves(single_m->Union(Adjacent(m)));
    // Move m from spill_worklist_ to the appropriate low-degree worklist.
    spill_worklist_ = spill_worklist_->Diff(single_m);
    if (MoveRelated(m))
      freeze_worklist_ = freeze_worklist_->Union(single_m);
    else
      simplify_worklist_ = simplify_worklist_->Union(single_m);
  }
}

/**
 * Move moves from active_moves_ to worklist_moves_ for the given nodes.
 *
 * When a node's degree drops below K, moves that were previously blocked
 * (in active_moves_) may now be eligible for coalescing.  This function
 * re-enables them by moving them back to worklist_moves_.
 */
void RegAllocator::EnableMoves(live::INodeList *nodes) {
  for (live::INode *n : nodes->GetList()) {
    live::MoveList *moves = NodeMoves(n);
    for (live::Move m : moves->GetList()) {
      if (active_moves_->Contain(m.first, m.second)) {
        live::MoveList *single_move = new live::MoveList(m);
        active_moves_ = active_moves_->Diff(single_move);
        worklist_moves_ = worklist_moves_->Union(single_move);
      }
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Coalesce – attempt to merge a move-related pair
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Try to coalesce the source and destination of a move from worklist_moves_.
 *
 * Coalescing eliminates a move instruction by merging its source and
 * destination into a single node.  This is safe only if the merged node
 * can still be colored (checked by George's or Briggs' test).
 *
 * Algorithm:
 *   1. Pick a move (x, y) from worklist_moves_
 *   2. Resolve aliases: x = GetAlias(x), y = GetAlias(y)
 *   3. Orient so that u is precolored if possible (u = precolored, v = other)
 *   4. Remove the move from worklist_moves_
 *   5. Classify the move:
 *      a. u == v (same node after alias resolution):
 *           → coalesced_moves_; try to simplify u
 *      b. v is precolored OR u and v already interfere:
 *           → constrained_moves_; cannot coalesce
 *      c. George(u,v) passes (u is precolored) OR Briggs(u,v) passes:
 *           → coalesced_moves_; Combine(u, v) merges v into u
 *      d. None of the above:
 *           → active_moves_; try again later
 */
void RegAllocator::Coalesce() {
  live::Move m = worklist_moves_->GetList().front();
  live::MoveList *single_move = new live::MoveList(m);
  // Resolve aliases: follow the coalescing chain to find the canonical node
  live::INode *x = GetAlias(m.first);
  live::INode *y = GetAlias(m.second);
  live::INode *u, *v;

  // Orient: if y is precolored, make u = y (precolored) and v = x (virtual)
  // This ensures George's test is applied when one endpoint is a machine reg.
  if (IsPrecolored(y)) {
    u = y;
    v = x;
  } else {
    u = x;
    v = y;
  }

  worklist_moves_ = worklist_moves_->Diff(single_move);

  if (u == v) {
    // Case (a): both ends are the same node (already coalesced or trivial)
    coalesced_moves_ = coalesced_moves_->Union(single_move);
    AddWorkList(u);  // u may now be non-move-related → move to simplify

  } else if (IsPrecolored(v) || AreAdj(u, v)) {
    // Case (b): both ends are precolored (can't merge two machine regs),
    //           OR the two nodes already interfere (coalescing would be wrong)
    constrained_moves_ = constrained_moves_->Union(single_move);
    AddWorkList(u);
    AddWorkList(v);

  } else if (George(u, v) || Briggs(u, v)) {
    // Case (c): coalescing is safe by George's or Briggs' test
    coalesced_moves_ = coalesced_moves_->Union(single_move);
    Combine(u, v);   // merge v into u
    AddWorkList(u);  // u may now be non-move-related → move to simplify

  } else {
    // Case (d): cannot coalesce yet; put back in active_moves_ for later
    active_moves_ = active_moves_->Union(single_move);
  }
}

/**
 * After coalescing, check if u can now be moved to simplify_worklist_.
 *
 * If u is not precolored, not move-related, and has low degree (< K),
 * it no longer needs to stay in freeze_worklist_ and can be simplified.
 * This is called after a move involving u is resolved (coalesced or constrained).
 */
void RegAllocator::AddWorkList(live::INode *u) {
  if (!IsPrecolored(u) && !MoveRelated(u) && u->IDegree() < reg_manager->RegCount()) {
    live::INodeList *single_u = new live::INodeList(u);
    freeze_worklist_ = freeze_worklist_->Diff(single_u);
    simplify_worklist_ = simplify_worklist_->Union(single_u);
  }
}

/**
 * George's coalescing safety predicate for a single neighbor t of v.
 *
 * Returns true if neighbor t of v satisfies the George condition with
 * respect to u (the precolored node we want to coalesce v into):
 *   degree(t) < K   (t is low-degree, so it won't cause coloring problems)
 *   OR t is precolored  (t already has a fixed color)
 *   OR t already interferes with u  (the edge (t,u) already exists)
 *
 * George's test passes for the whole coalescing if OK(t, u) holds for
 * every neighbor t of v.
 */
bool RegAllocator::OK(live::INode *t, live::INode *r) {
  return r->IDegree() < reg_manager->RegCount() || IsPrecolored(t) || AreAdj(t, r);
}

/**
 * Briggs' coalescing safety test (conservative coalescing).
 *
 * Returns true if the number of high-degree (≥ K) nodes in the combined
 * adjacency set adj(u) ∪ adj(v) is less than K.
 *
 * Intuition: after merging u and v, the combined node has fewer than K
 * high-degree neighbors, so it can always be colored regardless of the
 * order in which other nodes are simplified.
 *
 * @param nodes adj(u) ∪ adj(v) (passed in by Briggs())
 */
bool RegAllocator::Conservative(live::INodeList *nodes) {
  int k = 0;
  for (live::INode *n : nodes->GetList()) {
    if (n->IDegree() >= reg_manager->RegCount())
      k++;
  }
  return k < reg_manager->RegCount();
}

/**
 * Return the canonical representative of node n after coalescing.
 *
 * When node v is coalesced into u, alias_[v] = u.  GetAlias() follows
 * the alias chain recursively until it reaches a node that is not in
 * coalesced_nodes_ (i.e., the root of the coalescing chain).
 *
 * This is used throughout the algorithm to work with canonical nodes
 * rather than stale coalesced nodes.
 */
live::INode *RegAllocator::GetAlias(live::INode *n) {
  if (coalesced_nodes_->Contain(n))
    return GetAlias(alias_.at(n));  // follow the alias chain
  else
    return n;  // n is the canonical representative
}

// ─────────────────────────────────────────────────────────────────────────────
// Combine – merge node v into node u
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Merge node v into node u (coalescing step).
 *
 * After this call:
 *   - v is in coalesced_nodes_ (removed from the graph)
 *   - alias_[v] = u (GetAlias(v) will return u)
 *   - u inherits all of v's moves (for future coalescing)
 *   - All neighbors of v now have an interference edge to u
 *   - Degrees of v's neighbors are decremented (v is removed)
 *   - If u's degree crosses K, u moves from freeze_worklist_ to spill_worklist_
 */
void RegAllocator::Combine(live::INode *u, live::INode *v) {
  live::INodeList *single_v = new live::INodeList(v);

  // Remove v from whichever worklist it is currently in
  if (freeze_worklist_->Contain(v))
    freeze_worklist_ = freeze_worklist_->Diff(single_v);
  else
    spill_worklist_ = spill_worklist_->Diff(single_v);

  // Mark v as coalesced and set its alias to u
  coalesced_nodes_ = coalesced_nodes_->Union(single_v);
  alias_[v] = u;

  // Merge v's move list into u's move list (u inherits all of v's moves)
  live::MoveList *u_moves = live_graph_factory_->GetLiveGraph().move_list->Look(u);
  live::MoveList *v_moves = live_graph_factory_->GetLiveGraph().move_list->Look(v);
  live_graph_factory_->GetLiveGraph().move_list->Enter(u, u_moves->Union(v_moves));
  // Re-enable moves of v (they may now be coalesceable via u)
  EnableMoves(single_v);

  // For each neighbor t of v: add edge (t, u) and decrement t's degree
  // (because v is being removed from the graph)
  live::INodeList *adj_nodes = Adjacent(v);
  for (live::INode *t : adj_nodes->GetList()) {
    live_graph_factory_->GetLiveGraph().interf_graph->AddEdge(t, u);
    DecrementDegree(t);
  }

  // If u's degree just became ≥ K (due to inheriting v's neighbors),
  // move u from freeze_worklist_ to spill_worklist_
  if (u->IDegree() >= reg_manager->RegCount() && freeze_worklist_->Contain(u)) {
    live::INodeList *single_u = new live::INodeList(u);
    freeze_worklist_ = freeze_worklist_->Diff(single_u);
    spill_worklist_ = spill_worklist_->Union(single_u);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// George's and Briggs' coalescing safety tests
// ─────────────────────────────────────────────────────────────────────────────

/**
 * George's coalescing test: safe to coalesce v into precolored u?
 *
 * Applies only when u is precolored.  Returns true if for every neighbor t
 * of v, OK(t, u) holds:
 *   degree(t) < K  OR  t is precolored  OR  t already interferes with u
 *
 * Intuition: every neighbor of v either already conflicts with u (so the
 * edge (t,u) already exists and coalescing doesn't add new constraints) or
 * is low-degree (so it won't prevent u from being colored).
 */
bool RegAllocator::George(live::INode *u, live::INode *v) {
  if (!IsPrecolored(u))
    return false;  // George's test only applies when u is precolored

  live::INodeList *adj_nodes = Adjacent(v);
  for (live::INode *t : adj_nodes->GetList())
    if (!OK(t, u))
      return false;  // found a neighbor that violates the condition

  return true;
}

/**
 * Briggs' coalescing test: safe to coalesce two non-precolored nodes?
 *
 * Applies only when neither u nor v is precolored.  Returns true if the
 * number of high-degree (≥ K) nodes in adj(u) ∪ adj(v) is less than K.
 *
 * Intuition: the combined node will have fewer than K high-degree neighbors,
 * so it can always be colored (it will eventually be simplifiable).
 */
bool RegAllocator::Briggs(live::INode *u, live::INode *v) {
  if (IsPrecolored(u))
    return false;  // Briggs' test only applies when u is not precolored

  live::INodeList *nodes = Adjacent(u);
  nodes = nodes->Union(Adjacent(v));
  return Conservative(nodes);
}

/** @brief Return true if n is a precolored (machine register) node */
bool RegAllocator::IsPrecolored(live::INode *n) {
  return precolored_->Contain(n);
}

/** @brief Return true if nodes u and v have an interference edge */
bool RegAllocator::AreAdj(live::INode *u, live::INode *v) {
  return live_graph_factory_->GetLiveGraph().interf_graph->IAdj(u, v);
}

// ─────────────────────────────────────────────────────────────────────────────
// Freeze – give up coalescing a low-degree move-related node
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Give up coalescing a low-degree move-related node.
 *
 * When no simplification or coalescing is possible, we pick a node from
 * freeze_worklist_ and "freeze" it: we give up trying to coalesce its moves
 * and move it to simplify_worklist_ so it can be simplified.
 *
 * FreezeMoves() marks all moves involving u as frozen (no longer candidates
 * for coalescing), which may enable the other endpoints of those moves to
 * also be simplified.
 */
void RegAllocator::Freeze() {
  live::INode *u = freeze_worklist_->GetList().front();
  live::INodeList *single_u = new live::INodeList(u);
  // Move u from freeze_worklist_ to simplify_worklist_
  freeze_worklist_ = freeze_worklist_->Diff(single_u);
  simplify_worklist_ = simplify_worklist_->Union(single_u);
  // Freeze all moves involving u
  FreezeMoves(u);
}

/**
 * Freeze all moves associated with node u.
 *
 * For each move (u, v) or (v, u) that is still active:
 *   1. Move it from active_moves_ to frozen_moves_
 *   2. If v is now non-move-related and low-degree, move v from
 *      freeze_worklist_ to simplify_worklist_
 *
 * This is called both from Freeze() and from SelectSpill() (to freeze moves
 * of the selected spill candidate before pushing it onto the select stack).
 */
void RegAllocator::FreezeMoves(live::INode *u) {
  live::MoveList *u_moves = NodeMoves(u);
  live::INode *v;

  for (live::Move m : u_moves->GetList()) {
    // Find the other endpoint of the move (not u)
    if (GetAlias(m.second) == GetAlias(u))
      v = GetAlias(m.first);
    else
      v = GetAlias(m.second);

    // Move this move from active_moves_ to frozen_moves_
    live::MoveList *single_m = new live::MoveList(m);
    active_moves_ = active_moves_->Diff(single_m);
    frozen_moves_ = frozen_moves_->Union(single_m);

    // If v is now non-move-related and low-degree, it can be simplified
    if (NodeMoves(v)->GetList().empty() && v->IDegree() < reg_manager->RegCount()) {
      live::INodeList *single_v = new live::INodeList(v);
      freeze_worklist_ = freeze_worklist_->Diff(single_v);
      simplify_worklist_ = simplify_worklist_->Union(single_v);
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// SelectSpill – choose a high-degree node to potentially spill
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Select a node from spill_worklist_ as a potential spill candidate.
 *
 * This is "optimistic" spilling: the selected node is moved to
 * simplify_worklist_ (not immediately spilled).  It may still receive a
 * color during AssignColors() if enough colors are available.  Only if
 * AssignColors() cannot color it does it become an actual spill.
 *
 * The node's moves are frozen (FreezeMoves) since we are giving up on
 * coalescing it.
 */
void RegAllocator::SelectSpill() {
  assert(!spill_worklist_->GetList().empty());
  live::INode *m = HeuristicSelect();  // choose the best candidate to spill

  live::INodeList *single_m = new live::INodeList(m);
  // Move from spill_worklist_ to simplify_worklist_ (optimistic)
  spill_worklist_ = spill_worklist_->Diff(single_m);
  simplify_worklist_ = simplify_worklist_->Union(single_m);
  FreezeMoves(m);  // give up coalescing moves of the spill candidate
}

/**
 * Heuristic for choosing which node to spill.
 *
 * Uses a "furthest next use" heuristic: prefer to spill the temporary whose
 * next use is furthest away from its last definition.  This minimizes the
 * number of load/store instructions inserted by RewriteProgram().
 *
 * Special case: if a temporary is defined but never used, spill it immediately
 * (it is dead code and the spill cost is zero).
 *
 * Note: A more sophisticated heuristic would weight by loop nesting depth
 * (temporaries used inside loops are more expensive to spill).
 *
 * @return The node selected for potential spilling
 */
live::INode *RegAllocator::HeuristicSelect() {
  live::INode *res = nullptr;
  int max_distance = 0;
  assem::InstrList *instr_list = (*assem_instr_).GetInstrList();

  for (live::INode *n : spill_worklist_->GetList()) {
    int pos = 0;
    int start = 0;
    int distance = -1;
    for (assem::Instr *instr : instr_list->GetList()) {
      // Track the most recent definition and measure how far away the next use
      // occurs. Larger gaps make spilling cheaper because a single reload can
      // often cover a long dead region.
      if (instr->Def()->Contain(n->NodeInfo())) {
        start = pos;  // record position of last definition
      }
      if (instr->Use()->Contain(n->NodeInfo())) {
        distance = pos - start;  // distance from def to use
        if (distance > max_distance) {
          max_distance = distance;
          res = n;  // prefer the node with the largest def-to-use distance
        }
      }
      pos++;
    }

    // Defined but never used: spill cost is zero, spill immediately
    if (distance == -1)
      return n;
  }

  return res;
}

// ─────────────────────────────────────────────────────────────────────────────
// AssignColors – assign physical registers to nodes on the select stack
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Assign colors (physical registers) to nodes popped from select_stack_.
 *
 * Nodes were pushed onto select_stack_ in simplification order (LIFO).
 * We pop them in reverse order (the last simplified node is colored first).
 * For each node n:
 *   1. Start with all K colors available
 *   2. For each neighbor w of n (in the original graph):
 *        If GetAlias(w) is already colored, remove its color from available
 *   3. If no colors remain: n is an actual spill → add to spilled_nodes_
 *   4. Otherwise: assign the lowest available color to n
 *
 * After coloring all stack nodes, propagate colors to coalesced nodes:
 *   color[v] = color[GetAlias(v)]  for all v in coalesced_nodes_
 *
 * Note: We use the full AdjList (not Adjacent()) here because we need to
 * consider all original interference edges, including those to nodes that
 * were removed during simplification.
 */
void RegAllocator::AssignColors() {
  while (!select_stack_->GetList().empty()) {
    live::INode *n = select_stack_->GetList().front();
    select_stack_->DeleteNode(n);

    // Start with all colors available
    std::set<int> ok_colors;
    for (int c = 0; c < reg_manager->RegCount(); ++c)
      ok_colors.emplace(c);

    // Remove colors used by already-colored neighbors
    live::INodeList *adj_list = n->AdjList();
    for (live::INode *w : adj_list->GetList()) {
      live::INode *alias = GetAlias(w);
      // Only consider neighbors that have already been colored
      if (colored_nodes_->Union(precolored_)->Contain(alias))
        ok_colors.erase(color_.at(alias));  // this color is taken
    }

    live::INodeList *single_n = new live::INodeList(n);
    if (ok_colors.empty()) {
      // No color available: actual spill
      spilled_nodes_ = spilled_nodes_->Union(single_n);
    } else {
      // Assign the lowest available color
      colored_nodes_ = colored_nodes_->Union(single_n);
      int c = *(ok_colors.begin());
      color_[n] = c;
    }
  }

  // Propagate colors to coalesced nodes (they share the color of their alias)
  for (live::INode *n : coalesced_nodes_->GetList())
    color_[n] = color_[GetAlias(n)];
}

// ─────────────────────────────────────────────────────────────────────────────
// RewriteProgram – insert spill code and reset for next iteration
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Insert load/store code for actual spills and reset state for re-allocation.
 *
 * For each node v in spilled_nodes_:
 *   1. Allocate a new frame slot: acc = frame_->AllocLocal(true)
 *      The slot address is mem_pos = "offset(%rsp)" or similar.
 *   2. For each instruction that USES v:
 *        a. Create a fresh temporary t_new
 *        b. Replace v with t_new in the instruction's use list
 *        c. Insert before the instruction:
 *             movq mem_pos, t_new   (fetch from stack)
 *   3. For each instruction that DEFS v:
 *        a. Create a fresh temporary t_new
 *        b. Replace v with t_new in the instruction's def list
 *        c. Insert after the instruction:
 *             movq t_new, mem_pos   (store to stack)
 *
 * Each t_new has a very short live range (just one instruction), so it is
 * unlikely to be spilled again in the next iteration.
 *
 * After rewriting, all worklists and maps are cleared so RegAlloc() can
 * restart from scratch with the rewritten instruction list.
 */
void RegAllocator::RewriteProgram() {
  live::NodeInstrMap *node_instr_map = live_graph_factory_->GetNodeInstrMap().get();

  for (live::INode *v : spilled_nodes_->GetList()) {
    // Allocate a new frame slot for the spilled temporary
    frame::Access *acc = frame_->AllocLocal(true);
    std::string mem_pos = acc->MunchAccess(frame_);  // e.g., "-8(%rsp)"

    // Get all instructions that use or define this temporary
    auto node_instrs = node_instr_map->at(v);

    for (auto instr_it = node_instrs->begin(); instr_it != node_instrs->end(); instr_it++) {
      std::stringstream instr_ss;
      auto instr_pos = *instr_it;
      assem::Instr *instr = *instr_pos;
      // The instruction list is a std::list, so inserting loads/stores around
      // `instr_pos` does not invalidate the iterator stored in node_instr_map_.

      // Create a fresh temporary for this use/def site
      // (each site gets its own t_new to keep live ranges short)
      temp::Temp *new_reg = temp::TempFactory::NewTemp();

      // ── Handle USE of the spilled temporary ──────────────────────────────
      if (instr->Use()->Contain(v->NodeInfo())) {
        // Replace v with new_reg in the instruction's use list
        for (auto temp_it = instr->Use()->GetList().begin();
             temp_it != instr->Use()->GetList().end(); temp_it++) {
          if (*temp_it == v->NodeInfo()) {
            instr->Use()->Replace(temp_it, new_reg);
            break;
          }
        }

        // Insert a fetch instruction BEFORE the current instruction:
        //   movq mem_pos, new_reg
        if (IsArm64Target())
          instr_ss << "ldur `d0, " << mem_pos;
        else
          instr_ss << "movq " << mem_pos << ", `d0";
        assem::Instr *fetch_instr = new assem::OperInstr(
            instr_ss.str(),
            new temp::TempList(new_reg),                    // def: new_reg
            SpillBaseUseList(),                              // use: frame base
            nullptr);
        assem_instr_.get()->GetInstrList()->Insert(instr_pos, fetch_instr);
        instr_ss.str("");
      }

      // ── Handle DEF of the spilled temporary ──────────────────────────────
      if (instr->Def()->Contain(v->NodeInfo())) {
        // Replace v with new_reg in the instruction's def list
        for (auto temp_it = instr->Def()->GetList().begin();
             temp_it != instr->Def()->GetList().end(); temp_it++) {
          if (*temp_it == v->NodeInfo()) {
            instr->Def()->Replace(temp_it, new_reg);
            break;
          }
        }

        // Insert a store instruction AFTER the current instruction:
        //   movq new_reg, mem_pos
        instr_ss << (IsArm64Target() ? "stur `s0, " : "movq `s0, ") << mem_pos;
        assem::Instr *store_instr = new assem::OperInstr(
            instr_ss.str(),
            nullptr,                                                    // no def
            SpillBaseUseList(new_reg),                                  // use: new_reg, frame base
            nullptr);
        assem_instr_.get()->GetInstrList()->Insert(++instr_pos, store_instr);
      }
    }
  }

  // ── Reset all worklists and maps for the next allocation pass ─────────────
  spilled_nodes_->Clear();
  colored_nodes_->Clear();
  coalesced_nodes_->Clear();

  coalesced_moves_->Clear();
  constrained_moves_->Clear();
  frozen_moves_->Clear();
  worklist_moves_->Clear();
  active_moves_->Clear();

  color_.clear();
  alias_.clear();

  // Destroy the old flow graph and liveness analysis (will be rebuilt)
  delete flow_graph_factory_;
  delete live_graph_factory_;
}

// ─────────────────────────────────────────────────────────────────────────────
// Initialization helpers
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Initialize the color map for precolored (machine register) nodes.
 *
 * Assigns colors 0, 1, ..., K-1 to the K machine registers in the order
 * returned by reg_manager->Registers().  Also populates precolored_ with
 * the corresponding interference graph nodes.
 *
 * Precolored nodes have infinite degree (set by BuildIGraph) so they are
 * never simplified or spilled.
 */
void RegAllocator::InitColor() {
  auto tn_map = live_graph_factory_->GetTempNodeMap();
  int c = 0;
  for (temp::Temp *reg : reg_manager->Registers()->GetList()) {
    live::INode *node = tn_map->Look(reg);
    precolored_->Append(node);   // add to precolored set
    color_[node] = c++;          // assign color index
  }
}

/**
 * Initialize the alias map so each node is its own alias.
 *
 * Before any coalescing, every node n satisfies alias_[n] = n.
 * As coalescing proceeds, alias_[v] = u when v is merged into u.
 * GetAlias() follows the chain to find the canonical representative.
 */
void RegAllocator::InitAlias() {
  live::INodeList *all_nodes = live_graph_factory_->GetLiveGraph().interf_graph->Nodes();
  for (live::INode *n : all_nodes->GetList())
    alias_[n] = n;  // each node is initially its own alias
}

void RegAllocator::PrintMoveList() {

  std::cout << "worklist_moves_: ";
  for (live::Move m : worklist_moves_->GetList()) {
    std::cout <<  *global_map_->Look(m.first->NodeInfo()) << "->" 
              << *global_map_->Look(m.second->NodeInfo()) << " ";
  }
  std::cout << std::endl;

  std::cout << "coalesced_moves_: ";
  for (live::Move m : coalesced_moves_->GetList()) {
    std::cout <<  *global_map_->Look(m.first->NodeInfo()) << "->" 
              << *global_map_->Look(m.second->NodeInfo()) << " ";
  }
  std::cout << std::endl;

  std::cout << "constrained_moves_: ";
  for (live::Move m : constrained_moves_->GetList()) {
    std::cout <<  *global_map_->Look(m.first->NodeInfo()) << "->" 
              << *global_map_->Look(m.second->NodeInfo()) << " ";
  }
  std::cout << std::endl;

  std::cout << "frozen_moves_: ";
  for (live::Move m : frozen_moves_->GetList()) {
    std::cout <<  *global_map_->Look(m.first->NodeInfo()) << "->" 
              << *global_map_->Look(m.second->NodeInfo()) << " ";
  }
  std::cout << std::endl;

  std::cout << "active_moves_: ";
  for (live::Move m : active_moves_->GetList()) {
    std::cout <<  *global_map_->Look(m.first->NodeInfo()) << "->" 
              << *global_map_->Look(m.second->NodeInfo()) << " ";
  }
  std::cout << std::endl;
}

void RegAllocator::PrintAlias() {
  std::cout << "PrintAlias: ";
  auto all_nodes = live_graph_factory_->GetLiveGraph().interf_graph->Nodes();
  for (auto n : all_nodes->GetList())
    std::cout << *global_map_->Look(n->NodeInfo()) << '-'
              << *global_map_->Look(alias_[n]->NodeInfo()) << ' ';
  std::cout << std::endl;
}

void RegAllocator::PrintNodeList() {
  std::cout << "spilled_nodes_: ";
  for (auto n : spilled_nodes_->GetList()) {
    std::cout << *global_map_->Look(n->NodeInfo()) << ' ';
  }
  std::cout << std::endl;

  std::cout << "coalesced_nodes_: ";
  for (auto n : coalesced_nodes_->GetList()) {
    std::cout << *global_map_->Look(n->NodeInfo()) << ' ';
  }
  std::cout << std::endl;

  std::cout << "colored_nodes_: ";
  for (auto n : colored_nodes_->GetList()) {
    std::cout << *global_map_->Look(n->NodeInfo()) << ' ';
  }
  std::cout << std::endl;

  std::cout << "select_stack_: ";
  for (auto n : select_stack_->GetList()) {
    std::cout << *global_map_->Look(n->NodeInfo()) << ' ';
  }
  std::cout << std::endl;
}


} // namespace ra
