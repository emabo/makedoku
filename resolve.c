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

#ifdef HAVE_OPENSSL
#include <openssl/rand.h>
#endif
#include "resolve.h"
#include "sudo.h"


void print_table(unsigned char *grid, int single_line, int space)
{
	int x, y;

	for (x = 0; x < dim.grid; x++) {
		for (y = 0; y < dim.grid; y++)
			printf("%c", GRID(grid,x,y,0) ? (GRID(grid,x,y,0) == 255?'*':INT2ASCII(GRID(grid,x,y,0))) : (space?' ':'0'));
		if (!single_line)
			printf("\n");
	}
	printf("\n");
}

void print_grid_table(unsigned char *grid)
{
	int x, y, z;

	for (x = 0; x < dim.grid; x++) {
		for (y = 0; y < dim.grid; y++) {
			for (z = 0; z < dim.grid; z++)
				printf("%c", GRID(grid,x,y,z) ? (GRID(grid,x,y,z) == 255?'*':INT2ASCII(GRID(grid,x,y,z))) : ' ');
			printf(" ");
		}
		printf("\n");
	}
}

// random number between 0 and max (included)
#ifdef HAVE_OPENSSL
int random_int(int max)
{
	unsigned char buf[1];
	static char seed = 0;

	buf[0] = seed++;
	RAND_seed(buf, 1);
	RAND_bytes(buf, 1);
	return (int) ((max + 1) * ((double)buf[0] / 256.0f));
}
#else
int random_int(int max)
{
	return (int) ((max + 1) * (rand() / (RAND_MAX + 1.0)));
}
#endif

int random_start_number(int min, int max)
{
	return (int)(min + random_int(max-min));
}

int random_number(void)
{
	return (int)(1 + random_int(dim.grid-1));
}

int random_index(void)
{
	return random_int(dim.grid-1);
}

void random_table(unsigned char *grid, int num_min, int num_max)
{
#define MAX_ATTEMPT 1000
	int x, y, val, num, i, try, ko;
	int used[MAX_DIM];

	if (num_min < 1 || num_max > dim.number || num_max < num_min)
		return;

	do {
		i = 0;
		try = 0;
		ko = 0;
		RESET_GRID(grid, dim.extgrid);
		memset(used, 0, (MAX_DIM)*sizeof(int));

		num = random_start_number(num_min, num_max);
		while (num) {
			x = random_index();
			y = random_index();
			if (!GRID(grid,x,y,0)) {
				do {
					val = random_number();
				} while (used[val-1] && i < dim.grid-1); 

				if (try++ > MAX_ATTEMPT) {
					ko = 1;
					break;
				}

				if (is_valid_number(grid, val, x, y)) {
					GRID(grid,x,y,0) = val;
					num--;
					i++;
					used[val-1] = 1;
				}
			}
		}
	} while (ko);
}

int get_initial_number(unsigned char *grid)
{
	int x, y, count = 0;

	for (x = 0; x < dim.grid; x++)
		for (y = 0; y < dim.grid; y++)
			if (GRID(grid,x,y,0))
				count++;

	return count;
}

int check_initial_grid(unsigned char *grid)
{
	int x, y;

	for (x = 0; x < dim.grid; x++)
		for (y = 0; y < dim.grid; y++)
			if (GRID(grid,x,y,0) && !GRID(grid,x,y,1)
				&& !is_valid_number(grid, GRID(grid,x,y,0), x, y)) {
				printf("x=%d,y=%d\n", x,y);
				return 0;
			}
	return 1;
}

int check_solution(unsigned char *sol, unsigned char *grid)
{
	int x, y;

	for (x = 0; x < dim.grid; x++)
		for (y = 0; y < dim.grid; y++)
			if (!GRID(sol,x,y,0) || GRID(sol,x,y,1)
				|| (GRID(grid,x,y,0) && GRID(sol,x,y,0) != GRID(grid,x,y,0))
				|| !is_valid_number(sol, GRID(sol,x,y,0), x, y)) {
				printf("x=%d,y=%d\n", x,y);
				return 0;
			}
	return 1;
}

