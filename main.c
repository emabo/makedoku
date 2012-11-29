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

#include <ctype.h>
#include <omp.h>
#include "resolve.h"

#if defined(OPENCL) && defined(_OPENMP)
#error OpenCL and OpenMP cannot live together at the moment
#endif

#define MD_VERSION "0.1.0"
#define MAX_SOLUTION 50
#define MAX_DEPTH 50

#define MAX_OUTPUT_FILES 3
#define DEFAULT_OUTPUT_FILE_FMT "main%d.sud"

// shared vars
//
// options
char *input_filename = NULL;
int random_gen = 0;
int max_sudo = 0;
int initial_rand_num_min = 0;
int initial_rand_num_max = 0;
int iterative = 0;

// files
FILE *output_file[MAX_OUTPUT_FILES] = {NULL};
FILE *input_file = NULL;

// results
int sol_number[MAX_SOLUTION];
int start_number[DIM2+1];
int sol_depth_number[MAX_DEPTH];


/*
 * Files Management
 */
int fread_table(unsigned char *grid)
{
	int x, y, i;
	size_t len = 0;
	char *line = NULL;

	if (getline(&line, &len, input_file) == -1) {
		free(line);
		return 0;
	}

	RESET_GRID(grid, DIM3);

	for (x = 0; x < DIM; x++)
		for (y = 0; y < DIM; y++) {
			i = x*DIM+y;
			GRID(grid,x,y,0) = isdigit(line[i]) ? line[i]-'0' : 0;
		}

	free(line);

	return 1;
}

void fwrite_table(unsigned char *grid, unsigned char *sol, int i)
{
	int x, y;

	for (x = 0; x < DIM; x++)
		for (y = 0; y < DIM; y++)
			fprintf(output_file[i], "%d", GRID(grid,x,y,0));
	fprintf(output_file[i], ">");

	for (x = 0; x < DIM; x++)
		for (y = 0; y < DIM; y++)
			fprintf(output_file[i], "%d", GRID(sol,x,y,0));
	fprintf(output_file[i], "\n");
}

int open_files(void)
{
	int i;
	char str[512];

	if (input_filename && !(input_file = fopen(input_filename, "r")))
		return -1;

	for (i = 0; i < MAX_OUTPUT_FILES; i++) {
		sprintf(str, DEFAULT_OUTPUT_FILE_FMT, i);
		if (!(output_file[i] = fopen(str, "w")))
			return -1;
	}

	return 0;
}

void close_files(void)
{
	int i;

	if (input_file)
		fclose(input_file);

	for (i = 0; i < MAX_OUTPUT_FILES; i++) {
		if (output_file[i])
			fclose(output_file[i]);
	}
}

void init_result(void)
{
	memset(sol_number, 0, sizeof(sol_number));
	memset(sol_depth_number, 0, sizeof(sol_depth_number));
	memset(start_number, 0, sizeof(start_number));
}

void print_result(void)
{
	int i;
	FILE *fo = NULL;

	if (!(fo = fopen("stats.log", "w")))
		return;

	fprintf(fo, "Initial number of givens:\n");
	for (i = 0; i < DIM2+1; i++)
		fprintf(fo, "%d->%d\n", i, start_number[i]);

	fprintf(fo, "Number of final solutions per grid\n");
	for (i = 0; i < MAX_SOLUTION; i++)
		fprintf(fo, "%d->%d\n", i, sol_number[i]);

	fprintf(fo, "Number of cycles or maximum recursion depth reached solving unique solution grids\n");
	for (i = 0; i < MAX_DEPTH; i++)
		fprintf(fo, "%d->%d\n", i, sol_depth_number[i]);

	fclose(fo);
}

int main(int argc, char **argv)
{
	int num_sudo = 0, num_threads = 1, shift_argv = 0;

	if (argc < 3) {
		printf("makedoku %s, open source non-interactive sudoku generator and solver.\n", MD_VERSION);
		printf("Usage: %s [options]\n\n", argv[0]);
		printf("-r <min> <max>\trandom generated grids, specify minimum and maximum number of givens.\n");
		printf("-f <filename>\tsolve grids loaded from filename.\n\t\tFilename is a text file where lines represent grids to solve. Each line is a string of 81 characters,\n\t\twhere a given is a digit from 1 to 9 and whatever else character is a solution to find.\n");
		printf("-n <max>\tmaximum number of grids to generate or load from file.\n");
		printf("-i\t\tif this option is present use iterative procedure, otherwise recursive one.\n\t\tWith OpenCL present the procedure is always iterative by default.\n\n");
		printf("NOTE: -r and -n are mutually exclusive, but one of them has to be present. -f is mandatory.\n");
		printf("\tResolved output grids are saved in main0.sud, main1.sud, and main2.sud\n");
		printf("\tdepending on number of cycles or maximum recursion depth reached while solving.\n\n");

		printf("makedoku Copyright (C) 2012 Emanuele Bovisio\n");
		printf("This program comes with ABSOLUTELY NO WARRANTY.\n");
		printf("This is free software, and you are welcome to redistribute it under certain conditions.\n");
		printf("See the GNU General Public License version 3 or later for more details.\n");
		return 1;
	}

	if (!strcmp(argv[1], "-f")) {
		random_gen = 0;
		input_filename = argv[2];
	} else if (!strcmp(argv[1], "-r")) {
		random_gen = 1;
		initial_rand_num_min = atoi(argv[2]);
		initial_rand_num_max = atoi(argv[3]);
		shift_argv = 1;
		printf("Initial random number of givens, min and max: %d, %d\n", initial_rand_num_min, initial_rand_num_max);
	} else {
		printf("error: wrong parameter, first one has to be -r or -f\n");
		return 1;
	}

	if ((argc >= 5+shift_argv) && !strcmp(argv[3+shift_argv], "-n")) {
		max_sudo = atoi(argv[4+shift_argv]);
		printf("Maximum number of grids: %d\n", max_sudo);
	} else {
		printf("error: wrong parameter, there should be -n\n");
		return 1;
	}

#ifdef OPENCL
	iterative = 1;
#else
	if ((argc == 6+shift_argv) && !strcmp(argv[5+shift_argv], "-i"))
		iterative = 1;
	else
		iterative = 0;
#endif

	if (open_files()) {
		printf("error: open files\n");
		goto Error;
	}

	init_result();

#ifdef OPENCL
	init_opencl();
#endif

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)
#endif
	for (num_sudo = 0; num_sudo < max_sudo; num_sudo++) {
		int res = 0, error = 0, init_num = 0, sol_depth;
		unsigned char grid[DIM3], solution[DIM3], grid_solve[DIM3];

#ifdef _OPENMP
		if (num_sudo == 0) {
			int tid = omp_get_thread_num();
			if (tid == 0)
				num_threads = omp_get_num_threads();
		}
#endif

#ifdef _OPENMP
#pragma omp critical(dataread)
#endif
		{
			if (random_gen)
				random_table(grid, initial_rand_num_min, initial_rand_num_max);
			else if (!fread_table(grid))
				error = 1;
		}

		if (!check_initial_grid(grid)) {
			printf("error: initial grid not correct\n");
			print_table(grid, 0,0);
			error = 1;
		}

		if (!error) {
#ifdef DEBUG
			print_table(grid, 0, 0);
#endif

			sol_depth = -1;
			if (iterative)
				res = iterative_resolve(grid, solution, random_gen, &sol_depth);
			else
				res = recursive_resolve(grid_solve, grid, solution, random_gen, 0, &sol_depth);

			if (!((num_sudo+1)%1000))
				printf("# of grids: %d\n", num_sudo+1);

#ifdef DEBUG
			printf("%d: solution found: %d\n", num_sudo+1, res);
			print_table(solution, 1, 0);
#endif

			if (res == 1) {
#ifdef _OPENMP
#pragma omp critical(writeresult)
#endif
				{
					if (sol_depth <= 1)
						fwrite_table(grid, solution, 0);
					else if (sol_depth <= 4)
						fwrite_table(grid, solution, 1);
					else
						fwrite_table(grid, solution, 2);
					sol_depth_number[sol_depth]++;
				}
			}
			else if (res == -1)
				error = 1;

			if (!error && (res == 1 || !random_gen)) {
				init_num = get_initial_number(grid);

#ifdef _OPENMP
#pragma omp critical(dataupdate)
#endif
				{
					start_number[init_num]++;
					sol_number[res]++;
				}
			}
		}
	}

	print_result();

	printf("Number of used threads: %d\n", num_threads);

	close_files();

#ifdef OPENCL
	close_opencl();
#endif

	return 0;

Error:
	close_files();

#ifdef OPENCL
	close_opencl();
#endif

	return 1;
}
