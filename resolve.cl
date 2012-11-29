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

#define DIM_SQUARE 3
#define DIM 9 // (DIM_SQUARE*DIM_SQUARE)
#define DIM2 81 // (DIM*DIM)
#define DIM3 729 // (DIM*DIM*DIM)

int is_valid_number_row(__global const uchar *m, uchar num, int x, int y)
{
	for (int j = 0; j < DIM; j++) {
		int index = x*DIM2 + j*DIM;
		if (j != y && m[index] == num && !m[index + 1])
			return 0;
	}

	return 1;
}

int is_valid_number_column(__global const uchar *m, uchar num, int x, int y)
{
	for (int i = 0; i < DIM; i++) {
		int index = i*DIM2 + y*DIM;
		if (i != x && m[index] == num && !m[index + 1])
			return 0;
	}

	return 1;
}

// upper-left number of the square
int is_valid_number_square(__global const uchar *m, uchar num, int x, int y)
{
	int start_i = (x / DIM_SQUARE) * DIM_SQUARE;
	int start_j = (y / DIM_SQUARE) * DIM_SQUARE;

	for (int i = start_i; i < start_i + DIM_SQUARE; i++)
		for (int j = start_j; j < start_j + DIM_SQUARE; j++) {
			int index = i*DIM2 + j*DIM;
			if ((i != x || j != y) && m[index] == num && !m[index + 1])
				return 0;
		}

	return 1;
}

int is_valid_number(__global const uchar *m, uchar num, int x, int y)
{
	return is_valid_number_row(m, num, x, y)
		&& is_valid_number_column(m, num, x, y)
		&& is_valid_number_square(m, num, x, y);
}

__kernel void resolve(__global const uchar* grid_list_in, __global uchar* grid_list_out, int num)
{
	int l = get_global_id(0);
	int x = get_group_id(1);
	int y = get_local_id(1);

	if (x >= DIM || y >= DIM || l >= num)
		return;

	int end = 0, z_out, z;
	int t = l*DIM3;
	int i = t+x*DIM2+y*DIM;

	if (grid_list_in[i]) {
		if (!grid_list_in[i+1]) {
			grid_list_out[i] = grid_list_in[i];
			grid_list_out[i+1] = 0;
		} else if (grid_list_in[i+1] != 255) {
			for (z = 0, z_out = 0; z < DIM && grid_list_in[i+z]; z++)
				if (is_valid_number(&grid_list_in[t], grid_list_in[i+z], x, y)) {
					grid_list_out[i+z_out] = grid_list_in[i+z];
					z_out++;
				}
			if (z_out == 1)
				grid_list_out[i+1] = 255;
			else if (z_out < DIM)
				grid_list_out[i+z_out] = 0;
		} else {
			if (is_valid_number(&grid_list_in[t], grid_list_in[i], x, y)) {
				grid_list_out[i] = grid_list_in[i];
				grid_list_out[i+1] = 0;
			} else
				grid_list_out[i] = 0;
		}
	} else {
		for (z = 1, z_out = 0; z <= DIM; z++)
			if (is_valid_number(&grid_list_in[t], z, x, y)) {
				grid_list_out[i+z_out] = z;
				z_out++;
			}
		if (z_out == 1)
			grid_list_out[i+1] = 255;
		else if (z_out < DIM)
			grid_list_out[i+z_out] = 0;
	}
}

