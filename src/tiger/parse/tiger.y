/**
 * @file tiger.y
 * @brief Parser specification for the Tiger programming language - Lab 3
 *
 * This file contains a Bison (Yacc-compatible) parser specification that defines
 * the grammar for the Tiger programming language. It parses token streams from
 * the lexical analyzer and constructs abstract syntax trees (ASTs).
 *
 * Key features implemented:
 * - Complete Tiger grammar with operator precedence and associativity
 * - AST node construction for all language constructs
 * - Error recovery and reporting
 * - Integration with symbol table and error message systems
 * - Support for mutually recursive type and function declarations
 */

%{
/**
 * @brief Includes and declarations for the parser
 */
#include <string>
#include <iostream>
#include <memory>
#include "tiger/absyn/absyn.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/symbol/symbol.h"

/**
 * @brief External lexer function declaration
 */
extern int yylex();

/**
 * @brief Error reporting function called by the parser on syntax errors
 * @param s Error message string
 */
void yyerror(const char *s);

/**
 * @brief Initialize the lexical analyzer with error handler
 * @param errormsg Error message handler for position tracking
 */
void InitLexer(err::ErrorMsg *errormsg);

/**
 * @brief Global AST tree being constructed by the parser
 */
static std::unique_ptr<absyn::AbsynTree> absyn_tree_;

/**
 * @brief Global error message handler for position tracking
 */
static err::ErrorMsg *errormsg_;

/**
 * @brief Get current token position for AST node construction
 * @return Current token position from error handler
 */
int GetTokPos() {
    return errormsg_->GetTokPos();
}

%}

%code requires {
#include "tiger/absyn/absyn.h"
#include "tiger/symbol/symbol.h"
}

/**
 * @brief Semantic value union for parser attributes
 *
 * Each symbol in the grammar can have associated semantic values
 * of different types depending on what it represents in the AST.
 */
%union {
  int ival;                          /**< Integer literal values */
  std::string* sval;                 /**< String literal values */
  sym::Symbol *sym;                  /**< Symbol table entries */

  absyn::Exp *exp;                   /**< Expression nodes */
  absyn::ExpList *explist;           /**< Expression list nodes */
  absyn::Var *var;                   /**< Variable reference nodes */

  absyn::DecList *declist;           /**< Declaration list nodes */
  absyn::Dec *dec;                   /**< Declaration nodes */
  absyn::EFieldList *efieldlist;     /**< Expression field list nodes */
  absyn::EField *efield;             /**< Expression field nodes */

  absyn::NameAndTyList *tydeclist;    /**< Type declaration list nodes */
  absyn::NameAndTy *tydec;           /**< Type declaration nodes */
  absyn::FieldList *fieldlist;       /**< Field list nodes */
  absyn::Field *field;               /**< Field nodes */

  absyn::FunDecList *fundeclist;     /**< Function declaration list nodes */
  absyn::FunDec *fundec;             /**< Function declaration nodes */
  absyn::Ty *ty;                     /**< Type nodes */
}

/**
 * @brief Token declarations with their semantic value types
 */
%token <sym> ID                        /**< Identifier tokens */
%token <sval> STRING                    /**< String literal tokens */
%token <ival> INT                       /**< Integer literal tokens */

/**
 * @brief Additional token declarations for keywords and operators
 */
%token
  COMMA COLON SEMICOLON LPAREN RPAREN LBRACK RBRACK
  LBRACE RBRACE DOT
  ARRAY IF THEN ELSE WHILE FOR TO DO LET IN END OF
  BREAK NIL
  FUNCTION VAR TYPE

/**
 * @brief Operator precedence and associativity declarations
 *
 * Defines the precedence order (higher precedence binds tighter) and
 * associativity for binary operators in expressions.
 */
%nonassoc ASSIGN                    /**< Assignment (non-associative) */
%left OR                           /**< Logical OR (left-associative) */
%left AND                          /**< Logical AND (left-associative) */
%nonassoc EQ NEQ LT LE GT GE       /**< Comparison operators (non-associative) */
%left PLUS MINUS                   /**< Addition and subtraction (left-associative) */
%left TIMES DIVIDE                 /**< Multiplication and division (left-associative) */

/**
 * @brief Non-terminal symbol type declarations
 *
 * Specifies the semantic value types for each non-terminal symbol
 * in the grammar, enabling type-safe attribute passing.
 */
%type <exp> exp expop expseq                    /**< Expression types */
%type <explist> actuals nonemptyactuals sequencing_exps  /**< Expression list types */
%type <var> lvalue one oneormore               /**< Variable reference types */
%type <declist> decs decs_nonempty             /**< Declaration list types */
%type <dec> decs_nonempty_s vardec             /**< Declaration types */
%type <efieldlist> rec rec_nonempty            /**< Record field list types */
%type <efield> rec_one                         /**< Record field types */
%type <tydeclist> tydec                        /**< Type declaration list types */
%type <tydec> tydec_one                        /**< Type declaration types */
%type <fieldlist> tyfields tyfields_nonempty   /**< Field list types */
%type <field> tyfield                          /**< Field types */
%type <ty> ty                                  /**< Type types */
%type <fundeclist> fundec                      /**< Function declaration list types */
%type <fundec> fundec_one                      /**< Function declaration types */

/**
 * @brief Start symbol declaration
 *
 * Specifies that 'program' is the start symbol of the grammar.
 */
%start program

%%

/**
 * @brief Grammar rules for the Tiger language
 *
 * The following sections define the complete grammar for Tiger:
 * - Program structure
 * - Expressions and operators
 * - Variable references (l-values)
 * - Declarations (types, variables, functions)
 * - Type definitions
 *
 * Each rule constructs appropriate AST nodes with position information
 * for error reporting and semantic analysis.
 */
program:  exp  { absyn_tree_ = std::make_unique<absyn::AbsynTree>($1); }
  ;

/**
 * @brief Expression parsing rules
 *
 * These rules handle all forms of expressions in Tiger:
 * - Literals (integers, strings, nil)
 * - Variable references
 * - Function calls
 * - Arithmetic and logical operations
 * - Record creation
 * - Sequence expressions
 * - Assignments
 * - Conditional expressions
 * - Loops (while, for)
 * - Let expressions with declarations
 * - Array creation
 */

exp:
   INT  { $$ = new absyn::IntExp(GetTokPos(), $1); }
|  STRING  { $$ = new absyn::StringExp(GetTokPos(), $1); }
|  NIL  { $$ = new absyn::NilExp(GetTokPos()); }
|  lvalue  { $$ = new absyn::VarExp(GetTokPos(), $1); }
|  ID LPAREN actuals RPAREN  {
     $$ = new absyn::CallExp(GetTokPos(), $1, $3); }
|  expop  { $$ = $1; }
|  ID LBRACE rec RBRACE  { $$ = new absyn::RecordExp(GetTokPos(), $1, $3); }
|  LPAREN sequencing_exps RPAREN  { $$ = new absyn::SeqExp(GetTokPos(), $2); }
|  lvalue ASSIGN exp  { $$ = new absyn::AssignExp(GetTokPos(), $1, $3); }
|  IF exp THEN exp  { $$ = new absyn::IfExp(GetTokPos(), $2, $4, NULL); }
|  IF exp THEN exp ELSE exp  { $$ = new absyn::IfExp(GetTokPos(), $2, $4, $6); }
|  WHILE exp DO exp  { $$ = new absyn::WhileExp(GetTokPos(), $2, $4); }
|  FOR ID ASSIGN exp TO exp DO exp  { $$ = new absyn::ForExp(GetTokPos(), $2, $4, $6, $8); }
|  BREAK  { $$ = new absyn::BreakExp(GetTokPos()); }
|  LET decs IN expseq END  { $$ = new absyn::LetExp(GetTokPos(), $2, $4); }
|  ID LBRACK exp RBRACK OF exp  { $$ = new absyn::ArrayExp(GetTokPos(), $1, $3, $6); }
|  LPAREN RPAREN  { $$ = new absyn::VoidExp(GetTokPos()); }
|  LPAREN exp RPAREN  { $$ = $2; }
;

expop:
   exp PLUS exp  { $$ = new absyn::OpExp(GetTokPos(), absyn::PLUS_OP, $1, $3); }
|  exp MINUS exp  { $$ = new absyn::OpExp(GetTokPos(), absyn::MINUS_OP, $1, $3); }
|  exp TIMES exp  { $$ = new absyn::OpExp(GetTokPos(), absyn::TIMES_OP, $1, $3); }
|  exp DIVIDE exp  { $$ = new absyn::OpExp(GetTokPos(), absyn::DIVIDE_OP, $1, $3); }
|  exp EQ exp  { $$ = new absyn::OpExp(GetTokPos(), absyn::EQ_OP, $1, $3); }
|  exp NEQ exp  { $$ = new absyn::OpExp(GetTokPos(), absyn::NEQ_OP, $1, $3); }
|  exp LT exp  { $$ = new absyn::OpExp(GetTokPos(), absyn::LT_OP, $1, $3); }
|  exp LE exp  { $$ = new absyn::OpExp(GetTokPos(), absyn::LE_OP, $1, $3); }
|  exp GT exp  { $$ = new absyn::OpExp(GetTokPos(), absyn::GT_OP, $1, $3); }
|  exp GE exp  { $$ = new absyn::OpExp(GetTokPos(), absyn::GE_OP, $1, $3); }
|  exp AND exp  { $$ = new absyn::IfExp(GetTokPos(), $1, $3, new absyn::IntExp(GetTokPos(), 0)); }
|  exp OR exp  { $$ = new absyn::IfExp(GetTokPos(), $1, new absyn::IntExp(GetTokPos(), 1), $3); }
|  MINUS exp  {
     $$ = new absyn::OpExp(GetTokPos(), absyn::MINUS_OP, new absyn::IntExp(GetTokPos(), 0), $2); }
;

// exp1; exp2; ... (0 or more exps)
// Only valid in LET. Returns an Exp.
expseq:
   sequencing_exps  { $$ = new absyn::SeqExp(GetTokPos(), $1); }
|  exp  { $$ = new absyn::SeqExp(GetTokPos(), new absyn::ExpList($1)); }
|  { $$ = new absyn::VoidExp(GetTokPos()); }  // Empty LET body.
;

// exp1; exp2; ... (2 or more exps)
// Intermediate. Returns an ExpList (not Exp).
sequencing_exps:
   exp SEMICOLON exp  { $$ = new absyn::ExpList($3); $$->Prepend($1); }
|  exp SEMICOLON sequencing_exps  { $$ = $3->Prepend($1); }
;

// Actual parameters in a function call.
actuals:
   nonemptyactuals  { $$ = $1; }
|  { $$ = new absyn::ExpList(); }  // The function has no parameters.
;

nonemptyactuals:
   exp COMMA nonemptyactuals  { $$ = $3->Prepend($1); }
|  exp  { $$ = new absyn::ExpList($1); }
;


/**
 * @brief Variable reference (l-value) parsing rules
 *
 * These rules handle variable references and field/array access:
 * - Simple variable names
 * - Field access (record.field)
 * - Array element access (array[index])
 */

lvalue:
   ID  {
     $$ = new absyn::SimpleVar(GetTokPos(), $1); }
|  oneormore  {
     $$ = $1; }
;

oneormore:
   oneormore LBRACK exp RBRACK  { $$ = new absyn::SubscriptVar(GetTokPos(), $1, $3); }
|  oneormore DOT ID  {
     $$ = new absyn::FieldVar(GetTokPos(), $1, $3); }
|  one  { $$ = $1; }
;

one:
   ID LBRACK exp RBRACK  {
     $$ = new absyn::SubscriptVar(GetTokPos(), new absyn::SimpleVar(GetTokPos(), $1), $3); }
|  ID DOT ID  {
     $$ = new absyn::FieldVar(GetTokPos(), new absyn::SimpleVar(GetTokPos(), $1), $3); }
;


/**
 * @brief Type declaration parsing rules
 *
 * These rules handle type definitions in Tiger:
 * - Type aliases (type name = type)
 * - Record types (type name = { field1: type1, field2: type2 })
 * - Array types (type name = array of element_type)
 * - Field definitions for records
 */

// 1 or more type declarations
tydec:
   tydec_one tydec  { $$ = $2->Prepend($1); }
|  tydec_one  { $$ = new absyn::NameAndTyList($1); }
;

tydec_one:
   TYPE ID EQ ty  { $$ = new absyn::NameAndTy($2, $4); }
;

ty:
   ID  { $$ = new absyn::NameTy(GetTokPos(), $1); }
|  LBRACE tyfields RBRACE  { $$ = new absyn::RecordTy(GetTokPos(), $2); }
|  ARRAY OF ID  { $$ = new absyn::ArrayTy(GetTokPos(), $3); }
;

tyfields:
   tyfields_nonempty  { $$ = $1; }
|  { $$ = new absyn::FieldList(); }  // empty
;

tyfields_nonempty:
   tyfield COMMA tyfields_nonempty  { $$ = $3->Prepend($1); }
|  tyfield  { $$ = new absyn::FieldList($1); }
;

tyfield:
   ID COLON ID  { $$ = new absyn::Field(GetTokPos(), $1, $3); }
;


/**
 * @brief Function declaration parsing rules
 *
 * These rules handle function definitions in Tiger:
 * - Function name and parameter list
 * - Optional return type specification
 * - Function body expression
 * - Support for procedures (functions with no return value)
 */

fundec:
   fundec_one fundec  { $$ = $2->Prepend($1); }
|  fundec_one  { $$ = new absyn::FunDecList($1); }
;

fundec_one:
   FUNCTION ID LPAREN tyfields RPAREN EQ exp  { $$ = new absyn::FunDec(GetTokPos(), $2, $4, NULL, $7); }
|  FUNCTION ID LPAREN tyfields RPAREN COLON ID EQ exp  { $$ = new absyn::FunDec(GetTokPos(), $2, $4, $7, $9); }
;


/**
 * @brief Record field parsing rules
 *
 * These rules handle record creation and field initialization:
 * - Record field lists (name = value pairs)
 * - Support for empty records
 * - Field name and value expression pairs
 */

rec:
   rec_nonempty  { $$ = $1; }
|  { $$ = new absyn::EFieldList(); }  // empty
;

rec_nonempty:
   rec_one COMMA rec_nonempty  { $$ = $3->Prepend($1); }
|  rec_one  { $$ = new absyn::EFieldList($1); }
;

rec_one:
   ID EQ exp  { $$ = new absyn::EField($1, $3); }
;


/**
 * @brief Variable declaration parsing rules
 *
 * These rules handle variable declarations in Tiger:
 * - Variable name and optional type annotation
 * - Variable initialization expression
 * - Type inference for untyped variables
 */

vardec:
   VAR ID ASSIGN exp  { $$ = new absyn::VarDec(GetTokPos(), $2, NULL, $4); }
|  VAR ID COLON ID ASSIGN exp  { $$ = new absyn::VarDec(GetTokPos(), $2, $4, $6); }
;


/**
 * @brief Declaration list parsing rules
 *
 * These rules handle sequences of declarations in Tiger:
 * - Empty declaration lists
 * - Non-empty declaration lists (type, variable, function declarations)
 * - Proper ordering and sequencing of different declaration types
 */

decs:
   decs_nonempty  { $$ = $1; }
|  { $$ = new absyn::DecList(); }  // empty
;

decs_nonempty:
   decs_nonempty_s decs_nonempty  { $$ = $2->Prepend($1); }
|  decs_nonempty_s  { $$ = new absyn::DecList($1); }
;

decs_nonempty_s:
   tydec  { $$ = new absyn::TypeDec(GetTokPos(), $1); }
|  vardec  { $$ = $1; }
|  fundec  { $$ = new absyn::FunctionDec(GetTokPos(), $1); }
;

%%

/**
 * @brief Error reporting function called by the parser on syntax errors
 * @param s Error message string
 */
void yyerror(const char *s) {
    errormsg_->Error(GetTokPos(), "%s", s);
}

/**
 * @brief Main parsing function for Tiger source files
 *
 * This function orchestrates the complete parsing process:
 * 1. Initializes error handling and lexer state
 * 2. Opens the source file for lexing
 * 3. Invokes the parser (yyparse)
 * 4. Returns the constructed AST if parsing succeeds
 * 5. Handles file I/O and error conditions
 *
 * @param fname Path to the Tiger source file to parse
 * @return Unique pointer to the parsed AST, or nullptr if parsing failed
 */
std::unique_ptr<absyn::AbsynTree> Parse(const std::string &fname) {
    errormsg_ = new err::ErrorMsg(fname);
    InitLexer(errormsg_);
    
    // Open the input file for the lexer
    extern FILE *yyin;
    yyin = fopen(fname.c_str(), "r");
    if (!yyin) {
        errormsg_->Error(0, "Cannot open file %s", fname.c_str());
        return nullptr;
    }
    
    int result = yyparse();
    fclose(yyin);
    
    // If parsing failed or there were errors, don't return the tree
    if (result != 0 || errormsg_->AnyErrors()) {
        return nullptr;
    }
    
    return std::move(absyn_tree_);
}

err::ErrorMsg *GetErrorMsg() {
    return errormsg_;
}
