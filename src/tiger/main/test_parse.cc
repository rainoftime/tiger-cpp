#include <cstdio>
#include <fstream>

#include "tiger/absyn/absyn.h"
#include "tiger/parse/parser.h"
#include "tiger/frame/frame.h"

// define here to parse compilation
frame::RegManager *reg_manager;
frame::Frags frags;

int main(int argc, char **argv) {
  std::unique_ptr<absyn::AbsynTree> absyn_tree;

  if (argc < 2) {
    fprintf(stderr, "usage: a.out filename\n");
    exit(1);
  }

  absyn_tree = Parse(std::string(argv[1]));
  if (absyn_tree) {
    absyn_tree->Print(stderr);
    fprintf(stderr, "\n");
  }
  
  // Check for parsing errors and return appropriate exit code
  auto errormsg = GetErrorMsg();
  return (errormsg && errormsg->AnyErrors()) ? 1 : 0;
}
