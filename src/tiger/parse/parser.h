/**
 * @file parser.h
 * @brief Interface for the Tiger parser - Lab 3
 *
 * This header file provides the interface for the Bison-generated parser.
 * It declares the main parsing function and supporting utilities for
 * parsing Tiger source files into abstract syntax trees.
 */

#ifndef TIGER_PARSE_PARSER_H_
#define TIGER_PARSE_PARSER_H_

#include <iostream>
#include <string>
#include <memory>

// Forward declarations
namespace absyn {
    class AbsynTree;  /**< Forward declaration for AST root class */
}

namespace err {
    class ErrorMsg;   /**< Forward declaration for error message handler */
}

/**
 * @brief Forward declaration of bison generated functions
 */
extern int yyparse();              /**< Main parsing function */
extern void yyerror(const char *s); /**< Error reporting function */

/**
 * @brief Main parsing function for Tiger source files
 *
 * Parses a Tiger source file and constructs an abstract syntax tree.
 * Handles file I/O, lexer initialization, and error reporting.
 *
 * @param fname Path to the Tiger source file to parse
 * @return Unique pointer to the parsed AST, or nullptr if parsing failed
 */
std::unique_ptr<absyn::AbsynTree> Parse(const std::string &fname);

/**
 * @brief Get the current error message handler
 *
 * Returns the error message handler used during parsing for
 * accessing error information and position tracking.
 *
 * @return Pointer to the current error message handler
 */
err::ErrorMsg *GetErrorMsg();

#endif // TIGER_PARSE_PARSER_H_
