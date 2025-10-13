/**
 * @file test_slp.cc
 * @brief Test program for the Straightline Program (SLP) interpreter - Lab 1
 *
 * This program tests the SLP interpreter by running example programs and
 * verifying their behavior. It tests:
 * - Maximum argument calculation for each program
 * - Program interpretation and execution
 * - Different execution orders to verify deterministic behavior
 */

#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "straightline/prog1.h"

/**
 * @brief Main test function for SLP interpreter
 *
 * Tests three example programs (Prog, ProgProg, RightProg) in different orders
 * to verify the SLP interpreter works correctly. For each program, it:
 * 1. Calculates the maximum number of arguments needed
 * 2. Interprets and executes the program
 * 3. Prints output to verify correct behavior
 *
 * @param argc Number of command line arguments
 * @param argv Command line arguments - expects exactly one argument (test case number)
 * @return 0 on success, -1 on error
 */
int main(int argc, char **argv) {
  int args;
  int test_num;

  assert(argc == 2);
  test_num = atoi(argv[1]);

  switch (test_num) {
    case 0:
      // Test case 0: Run Prog first, then ProgProg
      printf("Prog\n");
      args = Prog()->MaxArgs();
      printf("args: %d\n", args);
      Prog()->Interp(nullptr);

      printf("ProgProg\n");
      args = ProgProg()->MaxArgs();
      printf("args: %d\n", args);
      ProgProg()->Interp(nullptr);
      break;

    case 1:
      // Test case 1: Run ProgProg first, then Prog (reverse order)
      printf("ProgProg\n");
      args = ProgProg()->MaxArgs();
      printf("args: %d\n", args);
      ProgProg()->Interp(nullptr);

      printf("Prog\n");
      args = Prog()->MaxArgs();
      printf("args: %d\n", args);
      Prog()->Interp(nullptr);
      break;

    default:
      printf("unexpected case\n");
      exit(-1);
  }
  printf("RightProg\n");
  args = RightProg()->MaxArgs();
  printf("args: %d\n", args);
  RightProg()->Interp(nullptr);

  return 0;
}
