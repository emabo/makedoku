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


int is_valid_number_row(__global const uchar *m, uchar num, int x, int y, int dim, int dim2)
{
	for (int j = 0; j < dim; j++) {
		int index = x*dim2 + j*dim;
		if (j != y && m[index] == num && !m[index + 1])
			return 0;
	}

	return 1;
}

int is_valid_number_column(__global const uchar *m, uchar num, int x, int y, int dim, int dim2)
{
	for (int i = 0; i < dim; i++) {
		int index = i*dim2 + y*dim;
		if (i != x && m[index] == num && !m[index + 1])
			return 0;
	}

	return 1;
}

// upper-left number of the square
int is_valid_number_square(__global const uchar *m, uchar num, int x, int y, int dimx, int dimy, int dim, int dim2)
{
	int start_i = (x / dimx) * dimx;
	int start_j = (y / dimy) * dimy;

	for (int i = start_i; i < start_i + dimx; i++)
		for (int j = start_j; j < start_j + dimy; j++) {
			int index = i*dim2 + j*dim;
			if ((i != x || j != y) && m[index] == num && !m[index + 1])
				return 0;
		}

	return 1;
}

int is_valid_number(__global const uchar *m, uchar num, int x, int y, int dimx, int dimy)
{
	int dim = dimx * dimy;
	int dim2 = dim * dim;

	return is_valid_number_row(m, num, x, y, dim, dim2)
		&& is_valid_number_column(m, num, x, y, dim, dim2)
		&& is_valid_number_square(m, num, x, y, dimx, dimy, dim, dim2);
}

__kernel void resolve(__global const uchar* grid_list_in, __global uchar* grid_list_out, int num, int dimx, int dimy)
{
	int l = get_global_id(0);
	int x = get_group_id(1);
	int y = get_local_id(1);
	int dim = dimx * dimy;
	int dim2 = dim * dim;
	int dim3 = dim2 * dim;

	if (x >= dim || y >= dim || l >= num)
		return;

	int end = 0, z_out, z;
	int t = l*dim3;
	int i = t+x*dim2+y*dim;

	if (grid_list_in[i]) {
		if (!grid_list_in[i+1]) {
			grid_list_out[i] = grid_list_in[i];
			grid_list_out[i+1] = 0;
		} else if (grid_list_in[i+1] != 255) {
			for (z = 0, z_out = 0; z < dim && grid_list_in[i+z]; z++)
				if (is_valid_number(&grid_list_in[t], grid_list_in[i+z], x, y, dimx, dimy)) {
					grid_list_out[i+z_out] = grid_list_in[i+z];
					z_out++;
				}
			if (z_out == 1)
				grid_list_out[i+1] = 255;
			else if (z_out < dim)
				grid_list_out[i+z_out] = 0;
		} else {
			if (is_valid_number(&grid_list_in[t], grid_list_in[i], x, y, dimx, dimy)) {
				grid_list_out[i] = grid_list_in[i];
				grid_list_out[i+1] = 0;
			} else
				grid_list_out[i] = 0;
		}
	} else {
		for (z = 1, z_out = 0; z <= dim; z++)
			if (is_valid_number(&grid_list_in[t], z, x, y, dimx, dimy)) {
				grid_list_out[i+z_out] = z;
				z_out++;
			}
		if (z_out == 1)
			grid_list_out[i+1] = 255;
		else if (z_out < dim)
			grid_list_out[i+z_out] = 0;
	}
}

