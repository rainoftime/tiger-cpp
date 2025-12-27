/**
 * @file output.h
 * @brief Assembly code output generation
 * 
 * This module handles writing the final assembly code to output files.
 * The AssemGen class coordinates the final compilation phases:
 * - Canonicalization
 * - Code generation
 * - Register allocation (optional)
 * 
 * Outputs x64 assembly code to a .s file.
 */

#ifndef TIGER_COMPILER_OUTPUT_H
#define TIGER_COMPILER_OUTPUT_H

#include <list>
#include <memory>
#include <string>

#include "tiger/canon/canon.h"
#include "tiger/codegen/codegen.h"
#include "tiger/frame/frame.h"
#include "tiger/regalloc/regalloc.h"

namespace output {

/**
 * @brief Assembly code generator
 * 
 * Coordinates the final phases of compilation and writes assembly output.
 * Can optionally perform register allocation or output unallocated code.
 */
class AssemGen {
public:
  AssemGen() = delete;
  explicit AssemGen(std::string_view infile) {
    std::string outfile = static_cast<std::string>(infile) + ".s";
    out_ = fopen(outfile.data(), "w");
  }
  AssemGen(const AssemGen &assem_generator) = delete;
  AssemGen(AssemGen &&assem_generator) = delete;
  AssemGen &operator=(const AssemGen &assem_generator) = delete;
  AssemGen &operator=(AssemGen &&assem_generator) = delete;
  ~AssemGen() { fclose(out_); }

  /**
   * @brief Generate assembly code
   * @param need_ra Whether to perform register allocation
   * 
   * Performs canonicalization, code generation, and optionally register
   * allocation, then writes the assembly code to the output file.
   */
  void GenAssem(bool need_ra);

private:
  FILE *out_; // Instream of source file
};

} // namespace output

#endif // TIGER_COMPILER_OUTPUT_H
