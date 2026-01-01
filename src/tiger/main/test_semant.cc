#include <cstdio>
//#include <fstream>
//#include <iostream>

#include "tiger/absyn/absyn.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/parse/parser.h"
#include "tiger/semant/semant.h"
#include "tiger/frame/frame.h"

// define here to pass compilation
frame::RegManager *reg_manager;
frame::Frags *frags;

int main(int argc, char **argv) {
  std::unique_ptr<absyn::AbsynTree> absyn_tree;
  err::ErrorMsg *errormsg;

  if (argc < 2) {
    fprintf(stderr, "usage: a.out filename\n");
    exit(1);
  }

  {
    absyn_tree = Parse(std::string(argv[1]));
    errormsg = GetErrorMsg();
  }

  if (absyn_tree && errormsg) {
    sem::ProgSem program_Semanalyzer(std::move(absyn_tree), std::unique_ptr<err::ErrorMsg>(errormsg));
    program_Semanalyzer.SemAnalyze();
  }
  
  // Check for parsing/semantic errors and return appropriate exit code
  return (errormsg && errormsg->AnyErrors()) ? 1 : 0;
}
