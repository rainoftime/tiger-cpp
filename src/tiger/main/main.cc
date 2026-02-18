/**
 * @file main.cc
 * @brief Tiger compiler driver
 *
 * This is the top-level entry point for the Tiger compiler.  It drives
 * the complete compilation pipeline from source file to x86-64 assembly:
 *
 *   1. Parse          – lex + parse the .tig source file into an AST
 *   2. Semantic analysis – type-check and scope-check the AST
 *   3. Escape analysis  – determine which variables must live in the frame
 *   4. IR translation   – translate the AST to IR tree fragments
 *   5. Assembly output  – canonicalize, select instructions, allocate
 *                         registers, and write the .tig.s output file
 *
 * Global state:
 *   reg_manager – the x86-64 register manager (singleton)
 *   frags       – the global list of compiled fragments (ProcFrag/StringFrag)
 *
 * Usage:
 *   tiger-compiler <file.tig>
 *
 * Output:
 *   <file.tig>.s  – x86-64 assembly (System V AMD64 ABI)
 */

#include "tiger/absyn/absyn.h"
#include "tiger/escape/escape.h"
#include "tiger/frame/x64frame.h"
#include "tiger/output/logger.h"
#include "tiger/output/output.h"
#include "tiger/parse/parser.h"
#include "tiger/translate/translate.h"
#include "tiger/semant/semant.h"

/** @brief Global x86-64 register manager (initialised in main) */
frame::RegManager *reg_manager;
/** @brief Global list of compiled fragments (ProcFrag and StringFrag) */
frame::Frags *frags;

int main(int argc, char **argv) {
  std::string_view fname;
  std::unique_ptr<absyn::AbsynTree> absyn_tree;
  reg_manager = new frame::X64RegManager();
  frags = new frame::Frags();

  if (argc < 2) {
    fprintf(stderr, "usage: tiger-compiler file.tig\n");
    exit(1);
  }

  fname = std::string_view(argv[1]);

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

  return 0;
}
