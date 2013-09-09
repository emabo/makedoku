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

#ifndef _SUDO_H_
#define _SUDO_H_

#define MAX_DIM_SQUARE 4
#define MAX_DIM 16 // (DIM_SQUARE*DIM_SQUARE)
#define MAX_DIM2 256 // (DIM*DIM)
#define MAX_DIM3 4096 // (DIM*DIM*DIM)

#define DIM_LIST 5000

#define GRID(matr,x,y,z) matr[(x)*dim.number+(y)*dim.grid+(z)]

typedef struct {
	int squarex, squarey;
	int grid;
	int number;
	int extgrid;
} dim_t;

extern dim_t dim;

inline int is_valid_number(unsigned char *m, int num, int x, int y);

void compute_dimensions(int sqx, int sqy);

#endif
