/*---------------------------------------------------------------------------
 * Copyright (C) 2012 - Emanuele Bovisio
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


void resolve_cpu(unsigned char *grid_list_in, unsigned char *grid_list_out, int num)
{
	unsigned char *p_in, *p_out;
	int l, x, y, z, t, z_out;

	for (l = 0; l < num; l++) {
		t = l*DIM3;
		for (x = 0; x < DIM; x++) {
			for (y = 0; y < DIM; y++) {
				p_in = grid_list_in+t+x*DIM2+y*DIM;
				p_out = grid_list_out+t+x*DIM2+y*DIM;
				if (p_in[0]) {
					if (!p_in[1]) {
						p_out[0] = p_in[0];
						p_out[1] = 0;
					} else if (p_in[1] != 255) {
						for (z = 0, z_out = 0; z < DIM && p_in[z]; z++)
							if (is_valid_number(grid_list_in+t, p_in[z], x, y))
								p_out[z_out++] = p_in[z];
						if (z_out == 1)
							p_out[1] = 255;
						else if (z_out < DIM)
							p_out[z_out] = 0;
					} else {
						if (is_valid_number(grid_list_in+t, *p_in, x, y)) {
							p_out[0] = p_in[0];
							p_out[1] = 0;
						} else
							p_out[0] = 0;
					}
				} else {
					for (z = 1, z_out = 0; z <= DIM; z++)
						if (is_valid_number(grid_list_in+t, z, x, y))
							p_out[z_out++] = z;
					if (z_out == 1)
						p_out[1] = 255;
					else if (z_out < DIM)
						p_out[z_out] = 0;
				}
			}
		}
	}
}

int check_grid_list(unsigned char *grid_list, int *xm, int *ym)
{
	unsigned char *p;
	int x, y, res = 1;

	for (x = 0; x < DIM; x++) {
		for (y = 0; y < DIM; y++) {
			p = grid_list+x*DIM2+y*DIM;
			if (!p[0])
				return 0;
			else if (p[1] == 255)
				res = 3;
			else if (p[1] && res == 1) {
				res = 2;
				*xm = x;
				*ym = y;
			}
		}
	}

	return res;
}

int expand(unsigned char *grid_list, unsigned char *grid, int *num, int x, int y)
{
	unsigned char *p_out;
	int z, i;

	i = x*DIM2+y*DIM;

	for (z = 0; z < DIM && grid[i+z]; z++) {
		p_out = grid_list+(*num)*DIM3;
		if (++(*num) >= DIM_LIST) {
			printf("grid list too long\n");
			return -1;
		}
		memcpy(p_out, grid, DIM3*sizeof(unsigned char));
		p_out[i] = grid[i+z];
		p_out[i+1] = 0;
	}

	return 0;
}

int iterative_resolve(unsigned char *grid, unsigned char *solution, int unique_sol, int *sol_depth)
{
	unsigned char grid_list_in[DIM3*DIM_LIST]={0}, grid_list_out[DIM3*DIM_LIST]={0};
	int res, num_in, num_out, num_sol = 0, l, x = 0, y = 0;

	*sol_depth = 0;
	memcpy(grid_list_in, grid, DIM3*sizeof(unsigned char));
	num_in = 1;

	do {
#ifdef OPENCL
		resolve_gpu(grid_list_in, grid_list_out, num_in);
#else
		resolve_cpu(grid_list_in, grid_list_out, num_in);
#endif

		num_out = num_in;
		num_in = 0;
		for (l = 0; l < num_out; l++) {
			if ((res = check_grid_list(grid_list_out+l*DIM3, &x, &y)) == 0)
				continue;
			switch (res) {
				case 1:
					num_sol++;
					if (!check_solution(grid_list_out+l*DIM3, grid)) {
						print_table(grid, 0,0);
						print_table(grid_list_out+l*DIM3, 0,0);
						printf("error: final solution not correct\n");
						return -1;
					}
					if (num_sol > 1 && unique_sol)
						return num_sol;
					else if (num_sol == 1)
						memcpy(solution, grid_list_out+l*DIM3, DIM3*sizeof(unsigned char));
					break;
				case 2:
					if (expand(grid_list_in, grid_list_out+l*DIM3, &num_in, x, y))
						return -1;
					break;
				default:
					memcpy(grid_list_in+num_in*DIM3, grid_list_out+l*DIM3, DIM3*sizeof(unsigned char));
					if (++num_in >= DIM_LIST) {
						printf("grid list too long\n");
						return -1;
					}
					break;
			}
		}
		(*sol_depth)++;
	} while (num_in);

	return num_sol;
}

