#ifndef TIGER_LEX_SCANNER_H_
#define TIGER_LEX_SCANNER_H_

#include <string>
#include <iostream>

// Forward declarations
namespace err {
    class ErrorMsg;
}

// Forward declaration for flex generated functions
extern int yylex();
extern int yylineno;
extern char *yytext;
extern int yyleng;
extern FILE *yyin;

// Initialization function
void InitLexer(err::ErrorMsg *errormsg);

#endif // TIGER_LEX_SCANNER_H_
