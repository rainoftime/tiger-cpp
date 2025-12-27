/**
 * @file errormsg.h
 * @brief Error message handling and source position tracking
 * 
 * This module provides error reporting for the Tiger compiler:
 * - Tracks source file position (line number, column)
 * - Formats and outputs error messages with position information
 * - Maintains error count for determining compilation success
 * 
 * The ErrorMsg class is used throughout the compiler pipeline to report
 * errors with accurate source positions.
 */

#ifndef TIGER_ERRORMSG_ERROMSG_H_
#define TIGER_ERRORMSG_ERROMSG_H_

#include <fstream>
#include <list>
#include <string>

/**
 * @brief Forward declaration
 */
class Scanner;

namespace err {

/**
 * @brief Error message handler with position tracking
 * 
 * Tracks the current position in the source file and formats error messages.
 * Position tracking is updated by the lexer as it processes tokens.
 */
class ErrorMsg {
  friend class ::Scanner;

public:
  ErrorMsg() = delete;
  explicit ErrorMsg(std::string_view fname)
      : line_pos_(std::list<int>{0}), file_name_(fname), infile_(fname.data()) {
    if (!infile_.good())
      throw std::invalid_argument("cannot open file");
  }

  /**
   * Add a new line in parser
   */
  void Newline();

  /**
   * Output an error
   * @param pos current position
   * @param message error message
   */
  void Error(int pos, std::string_view message, ...);

  /**
   * Getter for `tok_pos_`
   */
  [[nodiscard]] int GetTokPos() const { return tok_pos_; }

  /**
   * Setter for `tok_pos_`
   */
  void SetTokPos(int pos) { tok_pos_ = pos; }

  /**
   * Getter for `any_errors`
   */
  [[nodiscard]] bool AnyErrors() const { return any_errors_; }

private:
  int tok_pos_ = 1;         // current token position
  bool any_errors_ = false; // flag indicating if any error occurrs
  int line_num_ = 1;        // current line number
  std::list<int> line_pos_; // current token position of a line
  std::string file_name_;   // name of input file
  std::ifstream infile_;    // instream of the input file
};
} // namespace err

#endif // TIGER_ERRORMSG_ERROMSG_H_
