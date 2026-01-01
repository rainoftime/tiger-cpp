#include <cstdio>
#include <cstdlib>
#include <map>

#include "tiger/lex/scanner.h"
#include "parse.tab.hh"
#include "tiger/errormsg/errormsg.h"
#include "tiger/symbol/symbol.h"
#include "tiger/frame/frame.h"

// Define here to pass compilation, but no use here
frame::RegManager *reg_manager;
frame::Frags frags;

extern YYSTYPE yylcval;
extern FILE *yyin;

int main(int argc, char **argv) {
  std::map<int, std::string_view> tokname = {{ID, "ID"},
                                             {STRING, "STRING"},
                                             {INT, "INT"},
                                             {COMMA, "COMMA"},
                                             {COLON, "COLON"},
                                             {SEMICOLON, "SEMICOLON"},
                                             {LPAREN, "LPAREN"},
                                             {RPAREN, "RPAREN"},
                                             {LBRACK, "LBRACK"},
                                             {RBRACK, "RBRACK"},
                                             {LBRACE, "LBRACE"},
                                             {RBRACE, "RBRACE"},
                                             {DOT, "DOT"},
                                             {PLUS, "PLUS"},
                                             {MINUS, "MINUS"},
                                             {TIMES, "TIMES"},
                                             {DIVIDE, "DIVIDE"},
                                             {EQ, "EQ"},
                                             {NEQ, "NEQ"},
                                             {LT, "LT"},
                                             {LE, "LE"},
                                             {GT, "GT"},
                                             {GE, "GE"},
                                             {AND, "AND"},
                                             {OR, "OR"},
                                             {ASSIGN, "ASSIGN"},
                                             {ARRAY, "ARRAY"},
                                             {IF, "IF"},
                                             {THEN, "THEN"},
                                             {ELSE, "ELSE"},
                                             {WHILE, "WHILE"},
                                             {FOR, "FOR"},
                                             {TO, "TO"},
                                             {DO, "DO"},
                                             {LET, "LET"},
                                             {IN, "IN"},
                                             {END, "END"},
                                             {OF, "OF"},
                                             {BREAK, "BREAK"},
                                             {NIL, "NIL"},
                                             {FUNCTION, "FUNCTION"},
                                             {VAR, "VAR"},
                                             {TYPE, "TYPE"}};

  if (argc != 2) {
    fprintf(stderr, "usage: a.out filename\n");
    exit(1);
  }

  yyin = fopen(argv[1], "r");
  if (!yyin) {
    fprintf(stderr, "Could not open file %s\n", argv[1]);
    exit(1);
  }

  auto errormsg = std::make_unique<err::ErrorMsg>(std::string(argv[1]));
  InitLexer(errormsg.get());

  while (int tok = yylex()) {
    switch (tok) {
    case ID:
      printf("%10s %4d %s\n", tokname[tok].data(), errormsg->GetTokPos(),
             yylval.sym ? yylval.sym->Name().c_str() : "(null)");
      break;
    case STRING:
      printf("%10s %4d %s\n", tokname[tok].data(), errormsg->GetTokPos(),
             yylval.sval ? yylval.sval->c_str() : "(null)");
      break;
    case INT:
      printf("%10s %4d %d\n", tokname[tok].data(), errormsg->GetTokPos(),
             yylval.ival);
      break;
    default:
      printf("%10s %4d\n", tokname[tok].data(), errormsg->GetTokPos());
    }
  }
  
  fclose(yyin);
  
  // Return non-zero exit code if there were any errors
  return errormsg->AnyErrors() ? 1 : 0;
}
