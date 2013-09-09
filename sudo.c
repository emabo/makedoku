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

#include "sudo.h"


dim_t dim = {0};

inline int is_valid_number(unsigned char *m, int num, int x, int y)
{
	int j, i, start_i, start_j;

	for (j = 0; j < dim.grid; j++)
		if (j != y && GRID(m,x,j,0) == num && (!GRID(m,x,j,1) || GRID(m,x,j,1) == 255))
			return 0;

	for (i = 0; i < dim.grid; i++)
		if (i != x && GRID(m,i,y,0) == num && (!GRID(m,i,y,1) || GRID(m,i,y,1) == 255))
			return 0;

	start_i = (x / dim.squarex) * dim.squarex;
	start_j = (y / dim.squarey) * dim.squarey;

	for (i = start_i; i < x; i++)
		for (j = start_j; j < y; j++)
			if (GRID(m,i,j,0) == num && (!GRID(m,i,j,1) || GRID(m,i,j,1) == 255))
				return 0;
	for (i = start_i; i < x; i++)
		for (j = y+1; j < start_j + dim.squarey; j++)
			if (GRID(m,i,j,0) == num && (!GRID(m,i,j,1) || GRID(m,i,j,1) == 255))
				return 0;
	for (i = x+1; i < start_i + dim.squarex; i++)
		for (j = start_j; j < y; j++)
			if (GRID(m,i,j,0) == num && (!GRID(m,i,j,1) || GRID(m,i,j,1) == 255))
				return 0;
	for (i = x+1; i < start_i + dim.squarex; i++)
		for (j = y+1; j < start_j + dim.squarey; j++)
			if (GRID(m,i,j,0) == num && (!GRID(m,i,j,1) || GRID(m,i,j,1) == 255))
				return 0;

	return 1;
}

void compute_dimensions(int sqx, int sqy)
{
	dim.squarex = sqx;
	dim.squarey = sqy;
	dim.grid = sqx * sqy;
	dim.number = dim.grid * dim.grid;
	dim.extgrid = dim.number * dim.grid;
}
