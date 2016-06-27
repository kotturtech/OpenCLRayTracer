/**
 * @file BitonicSort.h
 * @author  Timur Sizov <timorgizer@gmail.com>
 * @version 0.6
 *
 * @section LICENSE
 *
 * Copyright (c) 2016 Timur Sizov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software 
 * without restriction, including without limitation the rights to use, copy, modify, merge, 
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons 
 * to whom the Software is furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all copies or 
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE 
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * @section DESCRIPTION
 *
 * class BitonicSort - The implementation file - Provides interface for GPU implementation of Bitonic Sort
 *
 *  @implNote The GPU kernels were taken from this source: http://www.bealto.com/gpu-sorting_parallel-merge-local.html
 *            and the interface class was implemented according to the source above.
 *
 */

#include <list>
#include <boost\algorithm\string\replace.hpp>
#include <OpenCLUtils\CLExecutionContext.h>
#include <Algorithms\Sorting.h>
#include <CLData\RTKernelUtils.h>

using namespace std;
using namespace CLRayTracer::OpenCLUtils;
using namespace CLRayTracer::Common;

/**String that contains the kernel source*/
extern const char * OpenCLKernelSource;

enum Kernels {
  PARALLEL_BITONIC_B2_KERNEL,
  PARALLEL_BITONIC_B4_KERNEL,
  PARALLEL_BITONIC_B8_KERNEL,
  PARALLEL_BITONIC_B16_KERNEL,
  PARALLEL_BITONIC_C2_KERNEL,
  PARALLEL_BITONIC_C4_KERNEL,
  NB_KERNELS
};

const char * KernelNames[NB_KERNELS+1] = {
  "ParallelBitonic_B2",
  "ParallelBitonic_B4",
  "ParallelBitonic_B8",
  "ParallelBitonic_B16",
  "ParallelBitonic_C2",
  "ParallelBitonic_C4",
  0 };

// Allowed "Bx" kernels (bit mask)
#define ALLOWB (2+4+8)

/**Constructor*/
BitonicSort::BitonicSort(const CLExecutionContext& context,const bool useKeyValue):
	_context(context),_deviceLocalMemory(0),_maxWorkgroupSize(0),_useKeyValue(useKeyValue)
{
	_sortingKernels.reset(new boost::shared_ptr<CLKernel>[NB_KERNELS]);
}

#define KV_TYPE CL_UINT2

/**Initialize
* Performs initialization of a BitonicSort instance. Must be called once per instance
* @param [out] err Error info, in case error occurred
* @return Result, that indicates whether the operation succeeded or failed
*/
Result BitonicSort::initialize(Errata& err)
{
	//Adjust kernel code according to parameters
	stringstream fullKernelCode;
	if (_useKeyValue)
		fullKernelCode << "#define CONFIG_USE_VALUE" << endl;
	fullKernelCode << OpenCLKernelSource;
	
	//Create and compile CL program
	_sortingProgram.reset(new CLProgram(_context));
	if (Success != _sortingProgram->compile(fullKernelCode.str(),err))
		return Error;

	for(int i = 0; i < NB_KERNELS; i++)
	{
		CLKernel* k = NULL;
		if(Success != _sortingProgram->getKernel(KernelNames[i],k,err))
			return Error;
		_sortingKernels[i].reset(k);
	}

	if (Success != _context.getDevice().getWorkGroupDimensions().getMaxWorkGroupSize(_maxWorkgroupSize,err))
		return Error;

	if(Success != _context.getDevice().getMemoryInfo().getLocalMemSize(_deviceLocalMemory,err))
		return Error;
	
	return Success;
}

/**Sorts the input array
* @param input Device pointer to array that shoud be sorted
* @param num_items Number of items in array to be sorted - Must be power of two
* @param [out] err Error info, in case error occurred
* @return Result, that indicates whether the operation succeeded or failed
*/
Result BitonicSort::sort(cl_mem input,size_t num_items,Errata& err)
{
	for (int length=1;length<num_items;length<<=1)
    {
      int inc = length;
      while (inc > 0)
      {
        int ninc = 0;
        int kid;
#if (ALLOWB & 16)  // Allow B16
        if (inc >= 8 && ninc == 0)
        {
          kid = PARALLEL_BITONIC_B16_KERNEL;
          ninc = 4;
        }
#endif
#if (ALLOWB & 8)  // Allow B8
        if (inc >= 4 && ninc == 0)
        {
          kid = PARALLEL_BITONIC_B8_KERNEL;
          ninc = 3;
        }
#endif
#if (ALLOWB & 4)  // Allow B4
        if (inc >= 2 && ninc == 0)
        {
          kid = PARALLEL_BITONIC_B4_KERNEL;
          ninc = 2;
        }
#endif
        // Always allow B2
        if (ninc == 0)
        {
          kid = PARALLEL_BITONIC_B2_KERNEL;
          ninc = 1;
        }
        int nThreads = num_items >> ninc;
		int wg = _maxWorkgroupSize;//c->getMaxWorkgroupSize(targetDevice,kid);
        wg = min(wg,256);
        wg = min(wg,nThreads);
 
		CLKernel& kernel =*_sortingKernels[kid];
		try
		{
			SET_KERNEL_ARGS(kernel,input,inc,length<<1);
		}
		catch(CLInterfaceException e)
		{
			err = Errata(e);
			return Error;
		}
		
		CLEvent evt;
		CLKernelWorkDimension globalDim(1,nThreads);
		CLKernelWorkDimension localDim(1,wg);
		CLKernelExecuteParams sortingKernelExecParams(&globalDim,&localDim,&evt);
		
		//Execute kernel
		if (Success != _context.enqueueKernel(kernel,sortingKernelExecParams,err))
			return Error;
		
		//Flush and make sure that sorting is over before returning control
		if (Success != _context.flushQueue(err))
			return Error;
		
		if (Success != evt.wait(err))
			return Error;
        inc >>= ninc;
      }
    }
	return Success;
}

   

