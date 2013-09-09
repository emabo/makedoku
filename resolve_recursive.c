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


struct backup_str {
	int x, y;
	unsigned char z[MAX_DIM];
};

inline int compact_number(unsigned char *m)
{
	int z, z_full;

	for (z = 0; z < dim.grid && m[z]; z++);

	for (z_full = z+1; z_full < dim.grid; z_full++) {
		if (m[z_full]) {
			m[z++] = m[z_full];
			m[z_full] = 0;
		}
	}

	return z;
}

int resolve(unsigned char *m, int *xn, int *yn)
{
	int modified, change, res, z, x, y, num, comp;
	unsigned char *p;

	do {
		*xn = *yn = -1;
		modified = 0;
		res = 1;
		for (x = 0; x < dim.grid; x++) {
			for (y = 0; y < dim.grid; y++) {
				p = m+x*dim.number+y*dim.grid;
				if (*p) {
					if (!*(p+1))
						continue;

					res = 0;
					change = 0;
					for (z = 0; z < dim.grid && p[z]; z++) {
						if (!is_valid_number(m, p[z], x, y)) {
							change = 1;
							p[z] = 0;
						}
					}
					if (change) {
						comp = compact_number(p);
						if (comp == 0)
							return -1;
						else if (comp == 1)
							modified = 1;
						else if (*xn < 0) {
							*xn = x;
							*yn = y;
						}
					} else if (*xn < 0) {
						*xn = x;
						*yn = y;
					}
				} else {
					res = 0;
					z = 0;
					for (num = 1; num <= dim.grid; num++) {
						if (is_valid_number(m, num, x, y)) {
							*p++ = num;
							z++;
						}
					}
					if (!z)
						return -1;
					else if (z == 1)
						modified = 1;
					else if (*xn < 0) {
						*xn = x;
						*yn = y;
					}
				}
			}
		}
	}
	while (modified);

	return res;
}

inline void backup_solve_table(struct backup_str *to, unsigned char *from)
{
	int x, y, i = 0, j;

	for (x = 0; x < dim.grid; x++)
		for (y = 0; y < dim.grid; y++) {
			j = x*dim.number+y*dim.grid;
			if (from[j+1]) {
				memcpy(to[i].z, from+j, dim.grid*sizeof(unsigned char));
				to[i].x = x;
				to[i++].y = y;
			}
		}
	to[i].x = -1;
}

inline void restore_solve_table(unsigned char *to, struct backup_str *from)
{
	int i;

	for (i = 0; from[i].x >= 0; i++)
		memcpy(to+from[i].x*dim.number+from[i].y*dim.grid, from[i].z, dim.grid*sizeof(unsigned char));
}

int recursive_resolve(unsigned char *grid_solve, unsigned char *grid, unsigned char *solution, int unique_sol, int depth, int *sol_depth)
{
	struct backup_str grid_backup[MAX_DIM2];
	int res, x, y, z, solved;
	unsigned char *p;

	if (!depth)
		memcpy(grid_solve, grid, dim.extgrid*sizeof(unsigned char));

	solved = resolve(grid_solve, &x, &y);
	if (solved == -1)
		return 0;
	else if (solved == 1) {
		if (!check_solution(grid_solve, grid)) {
			print_table(grid, 0,0);
			print_table(grid_solve, 0,0);
			printf("error: final solution not correct\n");
			return -1;
		}
		memcpy(solution, grid_solve, dim.extgrid*sizeof(unsigned char));
		if (depth > *sol_depth)
			*sol_depth = depth;
#ifdef DEBUG
		print_table(solution, 0, 0);
#endif
		return 1;
	}

	z = 0;
	res = 0;
	p = grid_solve+x*dim.number+y*dim.grid;

	backup_solve_table(grid_backup, grid_solve);

	while (z < dim.grid && p[z]) {
		*p = p[z++];
		memset(p+1, 0, (dim.grid-1)*sizeof(unsigned char));

#ifdef DEBUG
		printf("fork solution depth=%d %d,%d->%d\n", depth+1, x, y, z);
#endif
		res += recursive_resolve(grid_solve, grid, solution, unique_sol, depth+1, sol_depth);

		if (unique_sol && res > 1) {
#ifdef DEBUG
			printf("EXITING: at least two solutions\n");
#endif
			return res;
		}

		if (z < dim.grid)
			restore_solve_table(grid_solve, grid_backup);
	}

	return res;
}
