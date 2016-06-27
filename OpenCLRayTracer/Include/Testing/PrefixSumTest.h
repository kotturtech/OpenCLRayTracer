/**
 * @file PrefixSumTest.h
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
 * Some testing functions for Prefix Sum algorithm
 * 
 */

#ifndef CL_RT_PREFIX_SUM_TEST 
#define CL_RT_PREFIX_SUM_TEST

#include <iostream>
#include <fstream>
#include <ctime>
#include <CLData\RTKernelUtils.h>

namespace CLRayTracer
{
	namespace Testing
	{
		namespace PrefixSum
		{
			/**
			 * The simple sequential Prefix Sum algorithm
			 * @param output Output array
			 * @param input Input array 
			 * @param length Array size
			 * @return 
			*/
			inline void prefixSumCPU(cl_uint * output,cl_uint * input,const cl_uint length)
			{
				output[0] = 0;
				for(cl_uint i = 1; i < length; ++i)
					output[i] = input[i] + output[i-1];
				
			}

			/**
			 * Tests correctness of Prefix Sum algorithm
			 * @param input Array that was used as input for Prefix Sum algorithm
			 * @param prefixSum Array that contains result of Prefix Sum for input array
			 * @param length Array size
			 * @return True if the results are correct, false otherwise
			*/
			inline bool checkCorrectness(cl_uint* input, cl_uint* prefixSum,cl_uint length)
			{
				float epsilon = 1e-7f;
				bool error = false;
				for(int i = 1; i < length; i++)
				{
					cl_uint sum = input[i] + prefixSum[i-1];
					if (sum != prefixSum[i])
						error = true;
				}

				return !error;
			}

			/**
			 * Compares values in two arrays
			 * @param a1 First array
			 * @param a2 Second array
			 * @param length Array size
			 * @return True if arrays are identical
			*/
			inline bool compareArrays(cl_uint* a1, cl_uint* a2,cl_uint length)
			{
				bool error = false;
				for(int i = 0; i < length; i++)
					if(a1[i] != a2[i])
					{
						//std::cout << "Error at index " << i << "Array 1: " << a1[i] << " Array 2: " << a2[i] << std::endl; 
						error = true;
					}
				return !error;
			}

			/**
			 * Exports Prefix Sum results to file
			 * @param input Array that was used as input to Prefix Sum algorithm
			 * @param prefixSum Array that contains result of Prefix sum calculation on input array
			 * @param length Array size
			 * @return 
			*/
			inline void exportResultsToFile(cl_uint* input, cl_uint* prefixSum,cl_uint length)
			{
				time_t time;
				::time(&time);
				std::stringstream fname;
				fname << "prefixSum_" << time << ".log";
				std::ofstream o(fname.str());
				o << "-------Input-------" << std::endl;
				for (int i = 0; i < length; i++)
					o << "Idx: " << i << " Val: " << input[i] << " Prefix Sum: " << prefixSum[i] << std::endl;
				//o << "total: " <<  prefixSum[length] << std::endl; 
				o.close();
			}
		}
	}
}

#endif