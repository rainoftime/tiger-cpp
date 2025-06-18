%{
#include <string>
#include <iostream>
#include <memory>
#include "tiger/absyn/absyn.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/symbol/symbol.h"

extern int yylex();
void yyerror(const char *s);
void InitLexer(err::ErrorMsg *errormsg);

static std::unique_ptr<absyn::AbsynTree> absyn_tree_;
static err::ErrorMsg *errormsg_;

int GetTokPos() {
    return errormsg_->GetTokPos();
}

%}

%code requires {
#include "tiger/absyn/absyn.h"
#include "tiger/symbol/symbol.h"
}

%union {
  int ival;
  std::string* sval;
  sym::Symbol *sym;
  absyn::Exp *exp;
  absyn::ExpList *explist;
  absyn::Var *var;
  absyn::DecList *declist;
  absyn::Dec *dec;
  absyn::EFieldList *efieldlist;
  absyn::EField *efield;
  absyn::NameAndTyList *tydeclist;
  absyn::NameAndTy *tydec;
  absyn::FieldList *fieldlist;
  absyn::Field *field;
  absyn::FunDecList *fundeclist;
  absyn::FunDec *fundec;
  absyn::Ty *ty;
}

%token <sym> ID
%token <sval> STRING
%token <ival> INT

%token
  COMMA COLON SEMICOLON LPAREN RPAREN LBRACK RBRACK
  LBRACE RBRACE DOT
  ARRAY IF THEN ELSE WHILE FOR TO DO LET IN END OF
  BREAK NIL
  FUNCTION VAR TYPE

/* token priority */
%nonassoc ASSIGN
%left OR
%left AND
%nonassoc EQ NEQ LT LE GT GE
%left PLUS MINUS
%left TIMES DIVIDE

%type <exp> exp expop expseq
%type <explist> actuals nonemptyactuals sequencing_exps
%type <var> lvalue one oneormore
%type <declist> decs decs_nonempty
%type <dec> decs_nonempty_s vardec
%type <efieldlist> rec rec_nonempty
%type <efield> rec_one
%type <tydeclist> tydec
%type <tydec> tydec_one
%type <fieldlist> tyfields tyfields_nonempty
%type <field> tyfield
%type <ty> ty
%type <fundeclist> fundec
%type <fundec> fundec_one

%start program

%%
program:  exp  { absyn_tree_ = std::make_unique<absyn::AbsynTree>($1); }
  ;

// ------Expressions and List------

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


// ------Variables------

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


// ------Type Declarations and List------

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


// ------Function Declarations and List------

fundec:
   fundec_one fundec  { $$ = $2->Prepend($1); }
|  fundec_one  { $$ = new absyn::FunDecList($1); }
;

fundec_one:
   FUNCTION ID LPAREN tyfields RPAREN EQ exp  { $$ = new absyn::FunDec(GetTokPos(), $2, $4, NULL, $7); }
|  FUNCTION ID LPAREN tyfields RPAREN COLON ID EQ exp  { $$ = new absyn::FunDec(GetTokPos(), $2, $4, $7, $9); }
;


// ------Record Fields and List------

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


// ------Variable Declarations------

vardec:
   VAR ID ASSIGN exp  { $$ = new absyn::VarDec(GetTokPos(), $2, NULL, $4); }
|  VAR ID COLON ID ASSIGN exp  { $$ = new absyn::VarDec(GetTokPos(), $2, $4, $6); }
;


// ------All Declarations------

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

void yyerror(const char *s) {
    errormsg_->Error(GetTokPos(), "%s", s);
}

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
