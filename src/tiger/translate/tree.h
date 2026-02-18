/**
 * @file tree.h
 * @brief Intermediate Representation (IR) tree for the Tiger compiler
 *
 * This file defines the IR tree used between the front-end (AST translation)
 * and the back-end (canonicalization, code generation).  The design follows
 * Appel's "Modern Compiler Implementation" (the Tiger book), Chapter 7.
 *
 * The IR has two node families:
 *
 *   Stm  – statements: produce side effects but no value
 *   Exp  – expressions: compute a value (and may have side effects)
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Statements (tree::Stm)
 * ─────────────────────────────────────────────────────────────────────────
 *   SeqStm   – sequence two statements: SEQ(s1, s2)
 *   LabelStm – define a label at this point: LABEL(l)
 *   JumpStm  – unconditional jump: JUMP(e, labels)
 *   CjumpStm – conditional jump: CJUMP(op, l, r, t, f)
 *   MoveStm  – assignment: MOVE(dst, src)
 *   ExpStm   – evaluate expression for side effects: EXP(e)
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Expressions (tree::Exp)
 * ─────────────────────────────────────────────────────────────────────────
 *   BinopExp – binary arithmetic/logic: BINOP(op, l, r)
 *   MemExp   – memory load: MEM(e)  (also used as lvalue in MOVE)
 *   TempExp  – virtual register: TEMP(t)
 *   EseqExp  – statement then expression: ESEQ(s, e)
 *   NameExp  – symbolic label address: NAME(l)
 *   ConstExp – integer constant: CONST(i)
 *   CallExp  – function call: CALL(f, args)
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Canonicalization (Canon() / Munch())
 * ─────────────────────────────────────────────────────────────────────────
 * Each node implements two additional operations:
 *
 *   Canon()  – remove ESEQ nodes and ensure CALL results are immediately
 *              moved to a fresh TEMP.  Returns a canonicalized tree.
 *              Used by canon::Canon::Linearize().
 *
 *   Munch()  – maximal-munch instruction selection.  Traverses the tree
 *              and emits x64 assembly instructions into an InstrList.
 *              Returns the TEMP holding the result (for Exp nodes).
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Operator enumerations
 * ─────────────────────────────────────────────────────────────────────────
 *   BinOp  – arithmetic / bitwise operators for BinopExp
 *   RelOp  – relational operators for CjumpStm
 *
 * Helper functions:
 *   NotRel(op)    – logical negation of a relational operator
 *   Commute(op)   – commuted form of a relational operator (swap operands)
 */

#ifndef TIGER_TRANSLATE_TREE_H_
#define TIGER_TRANSLATE_TREE_H_

#include <array>
#include <cassert>
#include <cstdio>
#include <list>
#include <string>

#include "tiger/frame/temp.h"

// Forward Declarations
namespace canon {
class StmAndExp;
class Canon;
} // namespace canon

namespace assem {
class InstrList;
} // namespace assem

namespace frame {
class RegManager;
} // namespace frame

namespace tree {

class Stm;
class Exp;
class NameExp;

class ExpList;
class StmList;

/**
 * @brief Binary arithmetic and bitwise operators for BinopExp
 *
 * Used in BinopExp to specify the operation performed on two integer values.
 * The first four (PLUS..DIV) map directly to Tiger arithmetic operators.
 * The remaining operators are available for low-level code generation.
 */
enum BinOp {
  PLUS_OP,    ///< Integer addition
  MINUS_OP,   ///< Integer subtraction
  MUL_OP,     ///< Integer multiplication
  DIV_OP,     ///< Integer division (signed)
  AND_OP,     ///< Bitwise AND
  OR_OP,      ///< Bitwise OR
  LSHIFT_OP,  ///< Logical left shift
  RSHIFT_OP,  ///< Logical right shift
  ARSHIFT_OP, ///< Arithmetic right shift (sign-extending)
  XOR_OP,     ///< Bitwise XOR
  BIN_OPER_COUNT,
};

/**
 * @brief Relational operators for CjumpStm
 *
 * Used in CjumpStm to specify the comparison between two values.
 * Signed comparisons: EQ, NE, LT, GT, LE, GE.
 * Unsigned comparisons: ULT, ULE, UGT, UGE (for pointer/address comparisons).
 */
enum RelOp {
  EQ_OP,  ///< Equal
  NE_OP,  ///< Not equal
  LT_OP,  ///< Signed less-than
  GT_OP,  ///< Signed greater-than
  LE_OP,  ///< Signed less-than-or-equal
  GE_OP,  ///< Signed greater-than-or-equal
  ULT_OP, ///< Unsigned less-than
  ULE_OP, ///< Unsigned less-than-or-equal
  UGT_OP, ///< Unsigned greater-than
  UGE_OP, ///< Unsigned greater-than-or-equal
  REL_OPER_COUNT,
};

// ═══════════════════════════════════════════════════════════════════════════
// Statement nodes
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Abstract base class for IR statements
 *
 * Statements produce side effects but no value.  Every concrete Stm
 * subclass must implement:
 *   - Print()  – pretty-print for debugging
 *   - Canon()  – canonicalize (remove ESEQ, lift CALL results)
 *   - Munch()  – emit x64 assembly instructions (instruction selection)
 *
 * Static helpers:
 *   - IsNop()     – true if this statement is a no-op (EXP(CONST(0)))
 *   - Seq(x, y)   – build SEQ(x,y) but elide nops
 *   - Commute(s,e)– true if statement s and expression e can be reordered
 */
class Stm {
public:
  virtual ~Stm() = default;

  virtual void Print(FILE *out, int d) const = 0;
  virtual Stm *Canon() = 0;
  virtual void Munch(assem::InstrList &instr_list, std::string_view fs) = 0;

  /**
   * @brief Test whether this statement is a no-op
   *
   * A statement is a no-op if it is EXP(CONST(n)) for any n.
   * Used by Seq() to simplify the tree.
   */
  bool IsNop();

  /**
   * @brief Build a sequence statement, eliding no-ops
   *
   * Returns x if y is a nop, y if x is a nop, otherwise SEQ(x,y).
   */
  static Stm *Seq(Stm *x, Stm *y);

  /**
   * @brief Test whether statement x and expression y commute
   *
   * Two nodes commute if reordering them cannot change the program's
   * observable behaviour.  Conservative approximation:
   *   - x is a nop, OR
   *   - y is a NAME or CONST (no side effects, no memory access)
   */
  static bool Commute(tree::Stm *x, tree::Exp *y);
};

/**
 * @brief SEQ(left, right) – execute left then right
 *
 * Sequencing node.  After canonicalization, SEQ nodes are eliminated and
 * replaced by a flat list of statements (StmList).
 */
class SeqStm : public Stm {
public:
  Stm *left_;  ///< First statement to execute
  Stm *right_; ///< Second statement to execute

  SeqStm(Stm *left, Stm *right) : left_(left), right_(right) { assert(left); }
  ~SeqStm() override;

  void Print(FILE *out, int d) const override;
  Stm *Canon() override;
  void Munch(assem::InstrList &instr_list, std::string_view fs) override;
};

/**
 * @brief LABEL(l) – define label l at this program point
 *
 * Marks a position in the instruction stream that can be the target of
 * a JumpStm or CjumpStm.  Labels survive canonicalization unchanged.
 */
class LabelStm : public Stm {
public:
  temp::Label *label_; ///< The label being defined

  explicit LabelStm(temp::Label *label) : label_(label) {}
  ~LabelStm() override;

  void Print(FILE *out, int d) const override;
  Stm *Canon() override;
  void Munch(assem::InstrList &instr_list, std::string_view fs) override;
};

/**
 * @brief JUMP(e, labels) – unconditional jump to address e
 *
 * Transfers control to the address computed by expression e.
 * @p jumps lists all possible target labels (needed for dataflow analysis).
 * In practice e is always a NameExp and jumps has exactly one element.
 */
class JumpStm : public Stm {
public:
  NameExp *exp_;                       ///< Target address expression (usually NAME(l))
  std::vector<temp::Label *> *jumps_;  ///< All possible jump targets (for dataflow)

  JumpStm(NameExp *exp, std::vector<temp::Label *> *jumps)
      : exp_(exp), jumps_(jumps) {}
  ~JumpStm() override;

  void Print(FILE *out, int d) const override;
  Stm *Canon() override;
  void Munch(assem::InstrList &instr_list, std::string_view fs) override;
};

/**
 * @brief CJUMP(op, l, r, t, f) – conditional jump
 *
 * Evaluates (l op r).  If true, jumps to true_label_; otherwise to
 * false_label_.  After trace scheduling, false_label_ always immediately
 * follows the CJUMP in the instruction stream (property 7 of canonical form).
 */
class CjumpStm : public Stm {
public:
  RelOp op_;                    ///< Comparison operator
  Exp *left_, *right_;          ///< Operands
  temp::Label *true_label_;     ///< Jump target when condition is true
  temp::Label *false_label_;    ///< Jump target when condition is false

  CjumpStm(RelOp op, Exp *left, Exp *right, temp::Label *true_label,
           temp::Label *false_label)
      : op_(op), left_(left), right_(right), true_label_(true_label),
        false_label_(false_label) {}
  ~CjumpStm() override;

  void Print(FILE *out, int d) const override;
  Stm *Canon() override;
  void Munch(assem::InstrList &instr_list, std::string_view fs) override;
};

/**
 * @brief MOVE(dst, src) – assignment
 *
 * Evaluates src and stores the result into dst.
 *   - If dst is a TempExp: stores into the virtual register.
 *   - If dst is a MemExp: stores into the memory location.
 *
 * MOVE(TEMP t, CALL(...)) is the canonical form for function calls whose
 * result is used.
 */
class MoveStm : public Stm {
public:
  Exp *dst_; ///< Destination (TempExp or MemExp)
  Exp *src_; ///< Source value

  MoveStm(Exp *dst, Exp *src) : dst_(dst), src_(src) {}
  ~MoveStm() override;

  void Print(FILE *out, int d) const override;
  Stm *Canon() override;
  void Munch(assem::InstrList &instr_list, std::string_view fs) override;
};

/**
 * @brief EXP(e) – evaluate e for its side effects, discard the value
 *
 * Used when an expression is evaluated purely for side effects (e.g., a
 * procedure call whose return value is not needed).
 */
class ExpStm : public Stm {
public:
  Exp *exp_; ///< Expression to evaluate

  explicit ExpStm(Exp *exp) : exp_(exp) {}
  ~ExpStm() override;

  void Print(FILE *out, int d) const override;
  Stm *Canon() override;
  void Munch(assem::InstrList &instr_list, std::string_view fs) override;
};

// ═══════════════════════════════════════════════════════════════════════════
// Expression nodes
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Abstract base class for IR expressions
 *
 * Expressions compute a value and may have side effects.  Every concrete
 * Exp subclass must implement:
 *   - Print()  – pretty-print for debugging
 *   - Canon()  – canonicalize; returns a StmAndExp pair {side-effects, value}
 *   - Munch()  – emit x64 instructions and return the result TEMP
 */
class Exp {
public:
  virtual ~Exp() = default;

  virtual void Print(FILE *out, int d) const = 0;
  virtual canon::StmAndExp Canon() = 0;
  virtual temp::Temp *Munch(assem::InstrList &instr_list, std::string_view fs) = 0;
};

/**
 * @brief BINOP(op, left, right) – binary arithmetic/bitwise operation
 *
 * Computes (left op right) and returns the integer result.
 * Both operands must be integer-valued expressions.
 */
class BinopExp : public Exp {
public:
  BinOp op_;           ///< The binary operator
  Exp *left_, *right_; ///< Left and right operands

  BinopExp(BinOp op, Exp *left, Exp *right)
      : op_(op), left_(left), right_(right) {}
  ~BinopExp() override;

  void Print(FILE *out, int d) const override;
  canon::StmAndExp Canon() override;
  temp::Temp *Munch(assem::InstrList &instr_list, std::string_view fs) override;
};

/**
 * @brief MEM(e) – memory access at address e
 *
 * As an r-value: loads a word from address e.
 * As an l-value (left side of MOVE): stores a word to address e.
 * The word size is machine-dependent (8 bytes on x64).
 */
class MemExp : public Exp {
public:
  Exp *exp_; ///< Address expression

  explicit MemExp(Exp *exp) : exp_(exp) {}
  ~MemExp() override;

  void Print(FILE *out, int d) const override;
  canon::StmAndExp Canon() override;
  temp::Temp *Munch(assem::InstrList &instr_list, std::string_view fs) override;
};

/**
 * @brief TEMP(t) – virtual register t
 *
 * Represents an abstract (virtual) register.  Before register allocation,
 * there are an unlimited number of TEMPs.  After allocation, each TEMP is
 * mapped to a physical machine register (or spilled to memory).
 */
class TempExp : public Exp {
public:
  temp::Temp *temp_; ///< The virtual register

  explicit TempExp(temp::Temp *temp) : temp_(temp) {}
  ~TempExp() override;

  void Print(FILE *out, int d) const override;
  canon::StmAndExp Canon() override;
  temp::Temp *Munch(assem::InstrList &instr_list, std::string_view fs) override;
};

/**
 * @brief ESEQ(s, e) – execute statement s, then evaluate expression e
 *
 * Combines a statement (for side effects) with an expression (for its value).
 * ESEQ nodes are eliminated during canonicalization: the statement is hoisted
 * out and the expression is left in place.
 *
 * Example: ESEQ(MOVE(t, 5), BINOP(+, TEMP(t), CONST(1)))
 *   → after canon: statement MOVE(t,5) followed by expression BINOP(+,t,1)
 */
class EseqExp : public Exp {
public:
  Stm *stm_; ///< Statement to execute first (side effects)
  Exp *exp_; ///< Expression to evaluate after stm_

  EseqExp(Stm *stm, Exp *exp) : stm_(stm), exp_(exp) {}
  ~EseqExp() override;

  void Print(FILE *out, int d) const override;
  canon::StmAndExp Canon() override;
  temp::Temp *Munch(assem::InstrList &instr_list, std::string_view fs) override;
};

/**
 * @brief NAME(l) – the address of label l
 *
 * Evaluates to the machine address of the label l.  Used as the target of
 * JumpStm and as the function address in CallExp.
 */
class NameExp : public Exp {
public:
  temp::Label *name_; ///< The label whose address is taken

  explicit NameExp(temp::Label *name) : name_(name) {}
  ~NameExp() override;

  void Print(FILE *out, int d) const override;
  canon::StmAndExp Canon() override;
  temp::Temp *Munch(assem::InstrList &instr_list, std::string_view fs) override;
};

/**
 * @brief CONST(i) – integer constant i
 *
 * Evaluates to the integer value i.  Constants are never spilled and
 * can often be folded into instruction immediates during code generation.
 */
class ConstExp : public Exp {
public:
  int consti_; ///< The integer constant value

  explicit ConstExp(int consti) : consti_(consti) {}
  ~ConstExp() override;

  void Print(FILE *out, int d) const override;
  canon::StmAndExp Canon() override;
  temp::Temp *Munch(assem::InstrList &instr_list, std::string_view fs) override;
};

/**
 * @brief CALL(f, args) – function call
 *
 * Calls the function at address f with argument list args.
 * The return value is left in the machine's return-value register (RAX on x64).
 *
 * After canonicalization, every CALL is wrapped in
 *   MOVE(TEMP(t), CALL(...))
 * so that the result is captured before any subsequent call can overwrite it.
 *
 * The first argument in args is always the static link (for nested functions)
 * or a dummy value (for top-level / external functions).
 */
class CallExp : public Exp {
public:
  Exp *fun_;      ///< Function address (usually NAME(label))
  ExpList *args_; ///< Argument list (static link first, then user arguments)

  CallExp(Exp *fun, ExpList *args) : fun_(fun), args_(args) {}
  ~CallExp() override;

  void Print(FILE *out, int d) const override;
  canon::StmAndExp Canon() override;
  temp::Temp *Munch(assem::InstrList &instr_list, std::string_view fs) override;
};

// ═══════════════════════════════════════════════════════════════════════════
// List containers
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief An ordered list of IR expressions
 *
 * Used for function call argument lists and for building ESEQ chains
 * during translation.
 */
class ExpList {
public:
  ExpList() = default;
  ExpList(std::initializer_list<Exp *> list) : exp_list_(list) {}

  void Append(Exp *exp) { exp_list_.push_back(exp); }
  void Insert(Exp *exp) { exp_list_.push_front(exp); }
  std::list<Exp *> &GetNonConstList() { return exp_list_; }
  const std::list<Exp *> &GetList() { return exp_list_; }

  /**
   * @brief Emit instructions to evaluate all arguments and place them in
   *        the correct argument registers / stack slots (x64 calling convention)
   * @param instr_list Instruction list to append to
   * @param fs         Frame-size label string (for frame-pointer arithmetic)
   * @return TempList of the argument registers actually used
   */
  temp::TempList *MunchArgs(assem::InstrList &instr_list, std::string_view fs);

private:
  std::list<Exp *> exp_list_;
};

/**
 * @brief An ordered list of IR statements
 *
 * Produced by canonicalization (Linearize()) and consumed by BasicBlocks()
 * and TraceSchedule().  Also used as the output of TraceSchedule() for
 * instruction selection.
 */
class StmList {
  friend class canon::Canon;

public:
  StmList() = default;

  const std::list<Stm *> &GetList() { return stm_list_; }

  /**
   * @brief Flatten a (possibly nested) SEQ tree into this list
   *
   * Recursively decomposes SeqStm nodes and appends the leaf statements
   * to stm_list_ in left-to-right order.
   */
  void Linear(Stm *stm);

  void Print(FILE *out) const;

private:
  std::list<Stm *> stm_list_;
};

// ═══════════════════════════════════════════════════════════════════════════
// Operator utilities
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Return the logical negation of a relational operator
 *
 * NotRel(op) gives the operator op' such that
 *   (a op b) == !(a op' b)
 * Used when inverting a CJUMP during trace scheduling.
 */
RelOp NotRel(RelOp);

/**
 * @brief Return the commuted form of a relational operator
 *
 * Commute(op) gives the operator op' such that
 *   (a op b) == (b op' a)
 * Used when swapping operands of a CJUMP.
 */
RelOp Commute(RelOp);

} // namespace tree

#endif // TIGER_TRANSLATE_TREE_H_
