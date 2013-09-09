/*---------------------------------------------------------------------------
 * Copyright (C) 2012, 2013 - Emanuele Bovisio
 *
 * This file is part of makedoku.
 *
 * makedoku is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * makedoku is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with makedoku.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU Lesser General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 *---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef OPENCL
#include "resolve_cl.h"
#endif
#include "sudo.h"

#define RESET_GRID(m, dimen) memset(m, 0, (dimen)*sizeof(unsigned char))


inline int get_initial_number(unsigned char *grid);

int check_initial_grid(unsigned char *grid);
int check_solution(unsigned char *sol, unsigned char *grid);

void random_table(unsigned char *grid, int num_min, int num_max);

int iterative_resolve(unsigned char *grid, unsigned char *solution, int unique_sol, int *sol_depth);

int recursive_resolve(unsigned char *grid_solve, unsigned char *grid, unsigned char *solution, int unique_sol, int depth, int *sol_depth);

void print_grid_table(unsigned char *grid);

void print_table(unsigned char *grid, int single_line, int space);

