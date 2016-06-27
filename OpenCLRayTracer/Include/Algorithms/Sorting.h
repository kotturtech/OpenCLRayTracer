/**
 * @file Sorting.h
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
 * class BitonicSort - Provides interface for GPU implementation of Bitonic Sort
 *
 *  @implNote The GPU kernels were taken from this source: http://www.bealto.com/gpu-sorting_parallel-merge-local.html
 *            and the interface class was implemented according to the source above.
 *
 */


#ifndef CL_RT_SORTING
#define CL_RT_SORTING

#include <typeinfo>
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
		/** 
		* class BitonicSort Provides host interface for GPU implementation of Bitonic Sort
		*/
		class BitonicSort
		{
		public:
			/**Constructor
			* @param context OpenCL execution context
			* @param useKeyValue Indicates whether the sorted items are Key/Value pairs (CL_UINT2) or 
			*                    single values (CL_UINT)
			*/
			BitonicSort(const OpenCLUtils::CLExecutionContext& context,const bool useKeyValue);
			
			/**Initialize
			* Performs initialization of a BitonicSort instance. Must be called once per instance
			* @param [out] err Error info, in case error occurred
			* @return Result, that indicates whether the operation succeeded or failed
			*/
			Result initialize(Errata& err);
			
			/**Sorts the input array
			* @param input Device pointer to array that shoud be sorted
			* @param num_items Number of items in array to be sorted - Must be power of two
			* @param [out] err Error info, in case error occurred
			* @return Result, that indicates whether the operation succeeded or failed
			*/
			Result sort(cl_mem input,size_t num_items,Errata& err);
		private:
			const OpenCLUtils::CLExecutionContext& _context;
			boost::shared_ptr<OpenCLUtils::CLProgram> _sortingProgram;
			boost::shared_array<boost::shared_ptr<OpenCLUtils::CLKernel> > _sortingKernels;
			size_t _maxWorkgroupSize;
			CL_ULONG _deviceLocalMemory;
			bool _useKeyValue;
		};
	}
}
#endif