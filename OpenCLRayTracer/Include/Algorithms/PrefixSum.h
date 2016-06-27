/**
 * @file PrefixSum.h
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
 * class PrefixSum - Host interface class to GPU implementation of Prefix Sum algorithm
 * 
 */

#ifndef CL_RT_PREFIXSUM
#define CL_RT_PREFIXSUM

#include <boost\smart_ptr.hpp>
#include <OpenCLUtils\CLInterface.h>
#include <CLData\CLPortability.h>

namespace CLRayTracer
{
	namespace OpenCLUtils
	{
		class CLProgram;
		class CLKernel;
	}

	namespace Common
	{
		/**class PrefixSum - Exposes functions for running the GPU parallel Prefix Sum algorithm*/
		class PrefixSum
		{
		public:
			/**constructor*/
			PrefixSum(const OpenCLUtils::CLExecutionContext& context);

			/**Initializes the instance of PrefixSum wrapper
			 * @param [out]err Error info, filled in case there is an error
			 * @return Result, that indicates whether the operation succeeded or failed
			 */
			Common::Result initialize(Common::Errata& err);

			/**Computes prefix sun of device array
			 * By default, this futnction assumes that the values are unsigned ints
			 * @param inputBuffer The input for the algorithm - Array of unsigned integers
			 * @param outputBuffer The output of the algorithm - The prefix sum
			 * @param size Number of items in the input array - Must be power of two
			 * @param [out]err Error info, filled in case there is an error
			 * @return Result, that indicates whether the operation succeeded or failed
			 **/
			Common::Result computePrefixSum(cl_mem inputBuffer,cl_mem outputBuffer,size_t size,Common::Errata& err);

		private:
			Common::Result invokeGroupKernel(CL_UINT offset,CL_UINT length,Common::Errata& err);
			Common::Result invokeGlobalKernel(CL_UINT offset,CL_UINT length,Common::Errata& err);
			const OpenCLUtils::CLExecutionContext& _context;
			boost::shared_ptr<OpenCLUtils::CLProgram> _prefixSumProgram;
			boost::shared_ptr<OpenCLUtils::CLKernel> _groupKernel;
			boost::shared_ptr<OpenCLUtils::CLKernel> _globalKernel;
			CL_ULONG _deviceLocalMemory;
			size_t _deviceProcessors;
			size_t _deviceWavefront;
			size_t _maxWorkgroupSize;
			cl_mem _inputBuffer;
			cl_mem _outputBuffer;
		};
	}
}

#endif //CL_RT_PREFIXSUM
