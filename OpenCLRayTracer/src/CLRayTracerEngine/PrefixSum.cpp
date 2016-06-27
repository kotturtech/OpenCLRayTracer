/**
 * @file PrefixSum.cpp
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
 * class PrefixSum - Implementation of Host interface class to GPU implementation of Prefix Sum algorithm.
 *
 * @implnotes This class uses kernels from examples included in AMD OpenCL SDK, and implemented according to that example.
 *            Known bug: This class failed to work on Intel graphic cards for undetermined reason -
 *                       Despite of that it compiles fine and run without runtime errors, it gives incorrect results.
 * 
 */

#include <Algorithms\PrefixSum.h>
#include <OpenCLUtils\CLExecutionContext.h>
#include <CLData/RTKernelUtils.h>

using namespace std;
using namespace CLRayTracer::OpenCLUtils;
using namespace CLRayTracer::Common;

/**String that contains the kernel source*/
extern const char * PrefixSumKernelSource;

/**Constructor*/
PrefixSum::PrefixSum(const CLExecutionContext& context):_context(context)
{
	_deviceLocalMemory = 0;
	_deviceProcessors = 0;
	_deviceWavefront = 0;
	_maxWorkgroupSize = 0;
}

/**Initializes the instance of PrefixSum wrapper
 * @param [out]err Error info, filled in case there is an error
 * @return Result, that indicates whether the operation succeeded or failed
 */
Result PrefixSum::initialize(Errata& err)
{
	//Create and compile CL program
	_prefixSumProgram.reset(new CLProgram(_context));
	if (Success != _prefixSumProgram->compile(PrefixSumKernelSource,err))
		return Error;

	CLKernel *k = NULL;
	if (Success != _prefixSumProgram->getKernel("global_prefixSum",k,err))
		return Error;
	_globalKernel.reset(k);

	if (Success != _prefixSumProgram->getKernel("group_prefixSum",k,err))
		return Error;
	_groupKernel.reset(k);

	if(Success != _context.getDevice().getMemoryInfo().getLocalMemSize(_deviceLocalMemory,err))
		return Error;

	if (Success != _context.getMaximalLaunchExecParams(*_groupKernel,_deviceProcessors,_deviceWavefront,err))
		return Error;

	if (Success != _context.getDevice().getWorkGroupDimensions().getMaxWorkGroupSize(_maxWorkgroupSize,err))
		return Error;
	return Success;
}


/**Computes prefix sun of device array
* By default, this futnction assumes that the values are unsigned ints
* @param inputBuffer The input for the algorithm - Array of unsigned integers
* @param outputBuffer The output of the algorithm - The prefix sum
* @param size Number of items in the input array - Must be power of two
* @param [out]err Error info, filled in case there is an error
* @return Result, that indicates whether the operation succeeded or failed
**/
Result PrefixSum::computePrefixSum(cl_mem inputBuffer,cl_mem outputBuffer,size_t size,Errata& err)
{
	if (!isPowerOfTwo(size))
	{
		FILL_ERRATA(err,"Input size must be adjusted to power of 2");
		return Error;
	}

	//Verify that local memory is available
	size_t localDataSize = _maxWorkgroupSize << 1;   // Each thread work on 2 elements
	cl_ulong neededLocalMemory = localDataSize * sizeof(CL_INT);
	if(neededLocalMemory > _deviceLocalMemory)
    {
		FILL_ERRATA(err,"Insufficient Local Memory");
		return Error;
	}

	
	_outputBuffer = outputBuffer;
	_inputBuffer = inputBuffer;
	
	for(size_t offset = 1; offset<size; offset *= localDataSize)
    {
		
        if ((size/offset) > 1)  // Need atlest 2 element for process the kernel
        {
			if (Success != invokeGroupKernel(offset,size,err))
				return Error;
        }

        // Call global_kernel for update all elements
        if(offset > 1)
        {
           	if (Success != invokeGlobalKernel(offset,size,err))
				return Error;
        }
    }

	return Success;
}

Result PrefixSum::invokeGroupKernel(CL_UINT offset,CL_UINT length,Errata& err)
{
	size_t dataSize = length/offset;
	size_t localThreads = _maxWorkgroupSize;
    size_t globalThreads = (dataSize+1) >> 1;    // Actual threads needed
    // Set global thread size multiple of local thread size.
    globalThreads = ((globalThreads + localThreads - 1) / localThreads) *
                    localThreads;

	//Setting kernel arguments
	CLKernelArgument outputBufferArg = _outputBuffer;
	CLKernelArgument inputBufferArg = (offset>1) ? _outputBuffer  : _inputBuffer;
	CL_UINT localMemSize = (localThreads*sizeof(CL_UINT)) << 1;
	CLKernelArgument localMemArg(localMemSize);
	CLKernelArgument offsetArg = offset;
	CLKernelArgument lengthArg = length;

	if (Success != _groupKernel->setKernelArgument(outputBufferArg,0,err))
		return Error;
	if (Success != _groupKernel->setKernelArgument(inputBufferArg,1,err))
		return Error;
	if (Success != _groupKernel->setKernelArgument(offsetArg,2,err))
		return Error;
	if (Success != _groupKernel->setKernelArgument(lengthArg,3,err))
		return Error;
	if (Success != _groupKernel->setKernelArgument(localMemArg,4,err))
		return Error;

	CLEvent evt;
	CLKernelWorkDimension localDim(1,_maxWorkgroupSize);
	CLKernelWorkDimension globalDim(1,globalThreads);
	CLKernelExecuteParams groupKernelExecParams(&globalDim,&localDim,&evt);

	if(Success != _context.enqueueKernel(*_groupKernel,groupKernelExecParams,err))
		return Error;
	if (Success != _context.flushQueue(err))
		return Error;
	if (Success != evt.wait(err))
		return Error;

	return Success;
}

Result PrefixSum::invokeGlobalKernel(CL_UINT offset,CL_UINT length,Errata& err)
{
	size_t localThreads = _maxWorkgroupSize;
    size_t localDataSize = localThreads << 1;   // Each thread work on 2 elements

    // Set number of threads needed for global_kernel.
    size_t globalThreads = length - offset;
    globalThreads -= (globalThreads / (offset * localDataSize)) * offset;

    // Set global thread size multiple of local thread size.
    globalThreads = ((globalThreads + localThreads - 1) / localThreads) *
                    localThreads;

	CLKernelArgument outputBufferArg = _outputBuffer;
	CLKernelArgument offsetArg = offset;
	CLKernelArgument lengthArg = length;
	if (Success != _globalKernel->setKernelArgument(outputBufferArg,0,err))
		return Error;
	if (Success != _globalKernel->setKernelArgument(offsetArg,1,err))
		return Error;
	if (Success != _globalKernel->setKernelArgument(lengthArg,2,err))
		return Error;

	CLEvent evt;
	CLKernelWorkDimension localDim(1,_maxWorkgroupSize);
	CLKernelWorkDimension globalDim(1,globalThreads);
	CLKernelExecuteParams globalKernelExecParams(&globalDim,&localDim,&evt);
	if(Success != _context.enqueueKernel(*_globalKernel,globalKernelExecParams,err))
		return Error;
	if (Success != _context.flushQueue(err))
		return Error;
	if (Success != evt.wait(err))
		return Error;

	return Success;
}