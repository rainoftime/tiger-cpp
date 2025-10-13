/**
 * @file scanner.h
 * @brief Interface for the Tiger lexical analyzer - Lab 2
 *
 * This header file provides the interface for the Flex-generated lexical
 * analyzer. It declares the external functions and variables needed to
 * integrate the lexer with the parser and error handling systems.
 */

#ifndef TIGER_LEX_SCANNER_H_
#define TIGER_LEX_SCANNER_H_

#include <string>
#include <iostream>

// Forward declarations
namespace err {
    class ErrorMsg;  /**< Forward declaration for error message handler */
}

// Forward declaration for flex generated functions
extern int yylex();           /**< Main lexing function - returns next token */
extern int yylineno;          /**< Current line number (maintained by flex) */
extern char *yytext;          /**< Text of the current token */
extern int yyleng;            /**< Length of the current token */
extern FILE *yyin;            /**< Input file stream */

/**
 * @brief Initialize the lexical analyzer state
 *
 * Sets up the global state variables for a new lexing session:
 * - Sets the error message handler
 * - Resets character position to 1
 * - Resets comment nesting level to 0
 *
 * @param errormsg Pointer to the error message handler
 */
void InitLexer(err::ErrorMsg *errormsg);

#endif // TIGER_LEX_SCANNER_H_
