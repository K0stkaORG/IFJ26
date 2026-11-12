/**
 * @file optimizer.h
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 12. 11. 2026
 *
 * @brief Header file for the IFJ26 optimizer
 *
 * This file contains the declarations of the functions and data structures
 * used by the IFJ26 optimizer.
 */

#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "ctx.h"

/**
 * @brief Optimizes the intermediate representation of the program
 *
 * @param[in,out] ctx The context of the program
 */
void optimize_ir(Ctx* ctx);

#endif
