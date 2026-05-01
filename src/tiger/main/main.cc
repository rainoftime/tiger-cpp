/**
 * @file main.cc
 * @brief Tiger compiler driver
 *
 * This is the top-level entry point for the Tiger compiler.  It drives
 * the complete compilation pipeline from source file to target assembly:
 *
 *   1. Parse          – lex + parse the .tig source file into an AST
 *   2. Semantic analysis – type-check and scope-check the AST
 *   3. Escape analysis  – determine which variables must live in the frame
 *   4. IR translation   – translate the AST to IR tree fragments
 *   5. Assembly output  – canonicalize, select instructions, allocate
 *                         registers, and write the .tig.s output file
 *
 * Global state:
 *   reg_manager – the active target register manager (singleton)
 *   frags       – the global list of compiled fragments (ProcFrag/StringFrag)
 *
 * Usage:
 *   tiger-compiler [--target <target>] [--emit-binary] [-o output] <file.tig>
 *
 * Output:
 *   <file.tig>.s  – target assembly
 *   <file.tig>.bin – optional linked binary when --emit-binary is used
 */

#include "tiger/absyn/absyn.h"
#include "tiger/escape/escape.h"
#include "tiger/frame/target.h"
#include "tiger/output/logger.h"
#include "tiger/output/output.h"
#include "tiger/parse/parser.h"
#include "tiger/translate/translate.h"
#include "tiger/semant/semant.h"

/** @brief Global target register manager (initialised in main) */
frame::RegManager *reg_manager;
/** @brief Global list of compiled fragments (ProcFrag and StringFrag) */
frame::Frags *frags;

int main(int argc, char **argv) {
  std::string_view fname;
  std::unique_ptr<absyn::AbsynTree> absyn_tree;
  frags = new frame::Frags();
  frame::TargetArch target = frame::DetectHostTarget();
  bool emit_binary = false;
  std::string output_path;

  if (argc < 2) {
    fprintf(stderr,
            "usage: tiger-compiler [--target <target>] [--emit-binary] "
            "[-o output] file.tig\n");
    exit(1);
  }

  for (int i = 1; i < argc; ++i) {
    std::string_view arg(argv[i]);
    if (arg == "--emit-binary") {
      emit_binary = true;
      continue;
    }
    if (arg == "--target") {
      if (i + 1 >= argc ||
          !frame::ParseTarget(std::string_view(argv[i + 1]), &target)) {
        fprintf(stderr, "unknown target: %s\n",
                i + 1 < argc ? argv[i + 1] : "<missing>");
        return 1;
      }
      ++i;
      continue;
    }
    if (arg == "-o") {
      if (i + 1 >= argc) {
        fprintf(stderr, "-o requires an output path\n");
        return 1;
      }
      output_path = argv[++i];
      continue;
    }
    fname = arg;
  }

  if (fname.empty()) {
    fprintf(stderr, "missing Tiger source file\n");
    return 1;
  }

  frame::SetCurrentTarget(target);
  reg_manager = frame::NewRegManagerForTarget(target);

  {
    std::unique_ptr<err::ErrorMsg> errormsg;

    {
      // Lab 3: parsing
      // TigerLog("-------====Parse=====-----\n");
      absyn_tree = Parse(std::string(fname));
      errormsg = std::unique_ptr<err::ErrorMsg>(GetErrorMsg());
    }

    {
      // Lab 4: semantic analysis
      TigerLog("-------====Semantic analysis=====-----\n");
      sem::ProgSem prog_sem(std::move(absyn_tree), std::move(errormsg));
      prog_sem.SemAnalyze();
      absyn_tree = prog_sem.TransferAbsynTree();
      errormsg = prog_sem.TransferErrormsg();
    }

    {
      // Lab 5: escape analysis
      TigerLog("-------====Escape analysis=====-----\n");
      esc::EscFinder esc_finder(std::move(absyn_tree));
      esc_finder.FindEscape();
      absyn_tree = esc_finder.TransferAbsynTree();
    }

    {
      // Lab 5: translate IR tree
      TigerLog("-------====Translate=====-----\n");
      tr::ProgTr prog_tr(std::move(absyn_tree), std::move(errormsg));
      prog_tr.Translate();
      errormsg = prog_tr.TransferErrormsg();
    }

    if (errormsg->AnyErrors())
      return 1; // Don't continue if error occurrs
  }

  {
    // Output assembly
    output::AssemGen assem_gen(fname);
    assem_gen.GenAssem(true);
  }

  if (emit_binary) {
    std::string source = std::string(fname);
    std::string binary = output_path.empty() ? source + ".bin" : output_path;
    std::string command = "clang ";
    if (target == frame::TargetArch::Arm64Apple)
      command += "-arch arm64 ";
    command += source + ".s src/tiger/runtime/runtime.c -o " + binary;
    if (std::system(command.c_str()) != 0) {
      fprintf(stderr, "failed to link binary with command: %s\n",
              command.c_str());
      return 1;
    }
  }

  return 0;
}
