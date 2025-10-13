/**
 * @file prog1.h
 * @brief Example straightline programs for Lab 1
 *
 * This file declares factory functions that create example SLP (Straightline Program)
 * abstract syntax trees. These programs demonstrate various features of the SLP
 * language including assignments, arithmetic operations, and print statements.
 */

#ifndef STRAIGHTLINE_PROG1_H_
#define STRAIGHTLINE_PROG1_H_

#include "straightline/slp.h"

/**
 * @brief Creates the main example program
 * @return A statement representing the main program
 *
 * This program demonstrates basic SLP features including:
 * - Variable assignment
 * - Arithmetic operations (division)
 * - Print statements with multiple expressions
 */
A::Stm *Prog();

/**
 * @brief Creates a progressively more complex program
 * @return A statement representing the progressive program
 *
 * This program builds on the basic program with more complex expressions.
 */
A::Stm *ProgProg();

/**
 * @brief Creates the most complex example program
 * @return A statement representing the rightmost program
 *
 * This program demonstrates the most advanced SLP features available.
 */
A::Stm *RightProg();

#endif  // STRAIGHTLINE_PROG1_H_
