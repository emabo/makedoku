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
#include <CL/opencl.h>
#include "sudo.h"

#define BLOCK_DIM 16

#define oclCheckErrorEX(a, b) __oclCheckErrorEX(a, b, __FILE__ , __LINE__)
#define oclCheckError(a, b) oclCheckErrorEX(a, b)

cl_platform_id cpPlatform;
cl_device_id cdDevice;
cl_context cxGPUContext;
cl_command_queue cqCommandQueue;
cl_program cpProgram;
cl_kernel ckKernel;
cl_mem grid_in, grid_out;
char *cPathAndName = NULL, *cSourceCL = NULL;
size_t szKernelLength;

const char *oclErrorString(cl_int error)
{
    static const char* errorString[] = {
	"CL_SUCCESS",
	"CL_DEVICE_NOT_FOUND",
	"CL_DEVICE_NOT_AVAILABLE",
	"CL_COMPILER_NOT_AVAILABLE",
	"CL_MEM_OBJECT_ALLOCATION_FAILURE",
	"CL_OUT_OF_RESOURCES",
	"CL_OUT_OF_HOST_MEMORY",
	"CL_PROFILING_INFO_NOT_AVAILABLE",
	"CL_MEM_COPY_OVERLAP",
	"CL_IMAGE_FORMAT_MISMATCH",
	"CL_IMAGE_FORMAT_NOT_SUPPORTED",
	"CL_BUILD_PROGRAM_FAILURE",
	"CL_MAP_FAILURE",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"CL_INVALID_VALUE",
	"CL_INVALID_DEVICE_TYPE",
	"CL_INVALID_PLATFORM",
	"CL_INVALID_DEVICE",
	"CL_INVALID_CONTEXT",
	"CL_INVALID_QUEUE_PROPERTIES",
	"CL_INVALID_COMMAND_QUEUE",
	"CL_INVALID_HOST_PTR",
	"CL_INVALID_MEM_OBJECT",
	"CL_INVALID_IMAGE_FORMAT_DESCRIPTOR",
	"CL_INVALID_IMAGE_SIZE",
	"CL_INVALID_SAMPLER",
	"CL_INVALID_BINARY",
	"CL_INVALID_BUILD_OPTIONS",
	"CL_INVALID_PROGRAM",
	"CL_INVALID_PROGRAM_EXECUTABLE",
	"CL_INVALID_KERNEL_NAME",
	"CL_INVALID_KERNEL_DEFINITION",
	"CL_INVALID_KERNEL",
	"CL_INVALID_ARG_INDEX",
	"CL_INVALID_ARG_VALUE",
	"CL_INVALID_ARG_SIZE",
	"CL_INVALID_KERNEL_ARGS",
	"CL_INVALID_WORK_DIMENSION",
	"CL_INVALID_WORK_GROUP_SIZE",
	"CL_INVALID_WORK_ITEM_SIZE",
	"CL_INVALID_GLOBAL_OFFSET",
	"CL_INVALID_EVENT_WAIT_LIST",
	"CL_INVALID_EVENT",
	"CL_INVALID_OPERATION",
	"CL_INVALID_GL_OBJECT",
	"CL_INVALID_BUFFER_SIZE",
	"CL_INVALID_MIP_LEVEL",
	"CL_INVALID_GLOBAL_WORK_SIZE",
    };

    const int errorCount = sizeof(errorString) / sizeof(errorString[0]);

    const int index = -error;

    return (index >= 0 && index < errorCount) ? errorString[index] : "Unspecified Error";
}

inline void __oclCheckErrorEX(cl_int iSample, cl_int iReference, const char* cFile, const int iLine)
{
	if (iReference != iSample) {
		iSample = (iSample == 0) ? -9999 : iSample;

		printf("\n !!! Error # %i (%s) at line %i , in file %s !!!\n\n", iSample, oclErrorString(iSample), iLine, cFile);

		printf("Exiting...\n");

		exit(iSample);
	}
}

static char *oclLoadProgSource(const char* cFilename, const char* cPreamble, size_t* szFinalLength)
{
	FILE *pFileStream = NULL;
	size_t szSourceLength, szPreambleLength;
	char *cSourceString;

#ifdef _WIN32
	if (fopen_s(&pFileStream, cFilename, "rb") != 0)
		return NULL;
#else
	pFileStream = fopen(cFilename, "rb");
	if (pFileStream == 0)
		return NULL;
#endif

	szPreambleLength = strlen(cPreamble);

	// get the length of the source code
	fseek(pFileStream, 0, SEEK_END);
	szSourceLength = ftell(pFileStream);
	fseek(pFileStream, 0, SEEK_SET);

	// allocate a buffer for the source code string and read it in
	cSourceString = (char *)malloc(szSourceLength + szPreambleLength + 1);
	memcpy(cSourceString, cPreamble, szPreambleLength);
	if (fread((cSourceString) + szPreambleLength, szSourceLength, 1, pFileStream) != 1) {
		fclose(pFileStream);
		free(cSourceString);
		return 0;
	}

	// close the file and return the total length of the combined (preamble + source) string
	fclose(pFileStream);
	if(szFinalLength != 0)
		*szFinalLength = szSourceLength + szPreambleLength;
	cSourceString[szSourceLength + szPreambleLength] = '\0';

	return cSourceString;
}

static size_t shrRoundUp(int group_size, int global_size)
{
	int r = global_size % group_size;

	if (!r)
		return global_size;
	else
		return global_size + group_size - r;
}

static void create_kernel(void)
{
	cl_int ciErrNum;
	// size of memory required to store the matrix
	const size_t mem_size = sizeof(cl_uchar)*dim.extgrid*DIM_LIST;

	// allocate device memory and copy host to device memory
	grid_in = clCreateBuffer(cxGPUContext, CL_MEM_READ_ONLY, mem_size, NULL, &ciErrNum);
	oclCheckError(ciErrNum, CL_SUCCESS);

	// create buffer to store output
	grid_out = clCreateBuffer(cxGPUContext, CL_MEM_WRITE_ONLY, mem_size, NULL, &ciErrNum);
	oclCheckError(ciErrNum, CL_SUCCESS);

	// create the kernel
	ckKernel = clCreateKernel(cpProgram, "resolve", &ciErrNum);
	oclCheckError(ciErrNum, CL_SUCCESS);
}

static void delete_kernel(void)
{
	cl_int ciErrNum;

	ciErrNum = clReleaseMemObject(grid_in);
	ciErrNum |= clReleaseMemObject(grid_out);
	ciErrNum |= clReleaseKernel(ckKernel);
	oclCheckError(ciErrNum, CL_SUCCESS);
}

void resolve_gpu(unsigned char *grid_list_in, unsigned char *grid_list_out, int num)
{
	cl_int ciErrNum;
	size_t szGlobalWorkSize[2], szLocalWorkSize[2];
	// size of memory required to store the matrix
	const size_t mem_size = sizeof(cl_uchar)*dim.extgrid*num;

	// write to device
	ciErrNum = clEnqueueWriteBuffer(cqCommandQueue, grid_in, CL_FALSE, 0, mem_size, grid_list_in, 0, NULL, NULL);
	oclCheckError(ciErrNum, CL_SUCCESS);

	// set the args values for the naive kernel
	ciErrNum  = clSetKernelArg(ckKernel, 0, sizeof(cl_mem), (void *) &grid_in);
	ciErrNum |= clSetKernelArg(ckKernel, 1, sizeof(cl_mem), (void *) &grid_out);
	ciErrNum |= clSetKernelArg(ckKernel, 2, sizeof(cl_int), &num);
	oclCheckError(ciErrNum, CL_SUCCESS);

	// set up execution configuration
	szLocalWorkSize[0] = BLOCK_DIM;
	szLocalWorkSize[1] = BLOCK_DIM;
	szGlobalWorkSize[0] = shrRoundUp(BLOCK_DIM, num);
	szGlobalWorkSize[1] = dim.grid*BLOCK_DIM;

	// execute the kernel
	ciErrNum = clEnqueueNDRangeKernel(cqCommandQueue, ckKernel, 2, NULL, szGlobalWorkSize, szLocalWorkSize, 0, NULL, NULL);
	oclCheckError(ciErrNum, CL_SUCCESS);

	// Block CPU till GPU is done
	ciErrNum = clFinish(cqCommandQueue);
	oclCheckError(ciErrNum, CL_SUCCESS);

	// Copy back to host
	ciErrNum = clEnqueueReadBuffer(cqCommandQueue, grid_out, CL_TRUE, 0, mem_size, grid_list_out, 0, NULL, NULL);
	oclCheckError(ciErrNum, CL_SUCCESS);
}

void init_opencl(void)
{
	cl_int ciErrNum;

	//Get the NVIDIA platform
	ciErrNum = clGetPlatformIDs(1, &cpPlatform, NULL);
	oclCheckError(ciErrNum, CL_SUCCESS);

	//Get the device
	ciErrNum = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &cdDevice, NULL);
	oclCheckError(ciErrNum, CL_SUCCESS);

	//Create the context
	cxGPUContext = clCreateContext(0, 1, &cdDevice, NULL, NULL, &ciErrNum);
	oclCheckError(ciErrNum, CL_SUCCESS);

	// Create a command-queue
	cqCommandQueue = clCreateCommandQueue(cxGPUContext, cdDevice, 0, &ciErrNum);
	oclCheckError(ciErrNum, CL_SUCCESS);

	// Read the OpenCL kernel in from source file
	cSourceCL = oclLoadProgSource("resolve.cl", "", &szKernelLength);

	// create the program
	cpProgram = clCreateProgramWithSource(cxGPUContext, 1, (const char **)&cSourceCL, &szKernelLength, &ciErrNum);
	oclCheckError(ciErrNum, CL_SUCCESS);

	// build the program
	ciErrNum = clBuildProgram(cpProgram, 0, NULL, "-cl-fast-relaxed-math", NULL, NULL);
	oclCheckError(ciErrNum, CL_SUCCESS);

	create_kernel();
}

void close_opencl(void)
{
	cl_int ciErrNum;

	delete_kernel();

	if (cPathAndName)
		free(cPathAndName);
	if (cSourceCL)
		free(cSourceCL);
	ciErrNum = clReleaseProgram(cpProgram);
	ciErrNum |= clReleaseCommandQueue(cqCommandQueue);
	ciErrNum |= clReleaseContext(cxGPUContext);
	oclCheckError(ciErrNum, CL_SUCCESS);
}
