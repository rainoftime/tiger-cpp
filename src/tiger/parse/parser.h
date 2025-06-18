#ifndef TIGER_PARSE_PARSER_H_
#define TIGER_PARSE_PARSER_H_

#include <iostream>
#include <string>
#include <memory>

// Forward declarations
namespace absyn {
    class AbsynTree;
}

namespace err {
    class ErrorMsg;
}

// Forward declaration of bison generated functions
extern int yyparse();
extern void yyerror(const char *s);

// Parser interface functions
std::unique_ptr<absyn::AbsynTree> Parse(const std::string &fname);
err::ErrorMsg *GetErrorMsg();

#endif // TIGER_PARSE_PARSER_H_
