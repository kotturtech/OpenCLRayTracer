/**
 * @file SortingTest.h
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
 * Some testing functions for Bitonic Sort algorithm
 * 
 */

#ifndef CL_RT_SORTING_TEST 
#define CL_RT_SORTING_TEST

#include <iostream>
#include <map>
#include <boost\smart_ptr.hpp>
#include <CLData\CLPortability.h>


namespace CLRayTracer
{
	namespace Testing
	{
		namespace Sorting
		{
			/**
			 * Check that array of Key-Value pairs is sorted
			 * @param pairs Sorted key-value pairs array to test
			 * @param pairCount Number of elements in the array
			 * @return True if no error was found, false otherwise
			*/
			inline bool testSortingCorrectness(CL_UINT2* pairs,int pairCount)
			{
				CL_UINT firstCode = pairs[0].x;
				bool error = false;
				int inconsistents = 0;
				for(int i = 1; i < pairCount; i++)
				{
					if (firstCode > pairs[i].x)
					{
						inconsistents++;
						error = true;
					}
					firstCode = pairs[i].x;
				}
				
				std::cout << "Sorting Errors: Inconsistencies: "<< inconsistents << std::endl;
				return !error;
			}

			/**
			 * Counts duplicate keys in array of Key-Value pairs
			 * @param pairs Key-value pairs array to test
			 * @param pairCount Number of elements in the array
			 * @return Number of duplicte keys in the array
			*/
			inline CL_UINT countDuplicates(CL_UINT2* pairs,int pairCount)
			{
				CL_UINT duplicates = 0;
				CL_UINT firstCode = pairs[0].x;
				std::map<CL_UINT,CL_UINT> m1;
				
				for(int i = 1; i < pairCount; i++)
				{
					std::map<CL_UINT,CL_UINT>::iterator it = m1.find(pairs[i].x);
					if (it == m1.end())
						m1[pairs[i].x] = 1;
					else it->second++;
				}
				std::map<CL_UINT,CL_UINT>::iterator it = m1.begin();
				for(; it != m1.end(); it++)
				{
					duplicates+=(it->second>1);
				}
				std::cout << "duplicate (Multi Instance Keys) count: "<< duplicates << std::endl;
				return duplicates;
			}

			/**
			 * Tests whether all keys present in one array also present in the other, and their count matches 
			 * @param pairs First array of key-value pairs
			 * @param pairs2 Second array of key-value pairs
			 * @param pairCount Number of pairs in each array
			 * @return True if keys completely match
			*/
			inline bool testKeyIntegrity(CL_UINT2* pairs,CL_UINT2* pairs2,int pairCount)
			{
				bool error = false;
				std::map<CL_UINT,CL_UINT> m1,m2;
				for(int i = 0; i < pairCount; i++)
				{
					/*if (leaves[i].mortonCode != leaves2[i].mortonCode)
						std::cout << "something strange!" << std::endl;*/
					std::map<CL_UINT,CL_UINT>::iterator it = m1.find(pairs[i].x);
					if (it == m1.end())
						m1[pairs[i].x] = 1;
					else it->second++;

					it = m2.find(pairs2[i].x);
					if (it == m2.end())
						m2[pairs2[i].x] = 1;
					else it->second++;
					
				}
				int absent = 0,cnt = 0;
				std::map<CL_UINT,CL_UINT>::iterator it = m1.begin();
				for(; it != m1.end(); it++)
				{
					std::map<CL_UINT,CL_UINT>::iterator it2 = m2.find(it->first); 
					if (it2 == m2.end())
					{
						error = true;
						absent++;
					}
					else
					if (it->second != it2->second)
					{
							error = true;
							cnt++;
					}

				}
				std::cout << "Absent Keys:" << absent << " Mismatching count: " << cnt <<std::endl;
				return !error;
			}

			/**
			 * Sorts array of key-value pairs
			 * @param pairs Array of key-value pairs too sort
			 * @param count Count of pairs in the pairs array
			 * @return True if keys completely match
			*/
			inline void sortKVPairs(CL_UINT2* pairs, const CL_UINT count)
			{
				std::multimap<CL_UINT,CL_UINT> helperMultimap;
				int i = 0;
				for(; i < count; i++)
					helperMultimap.insert(std::make_pair(pairs[i].x,pairs[i].y));
				std::multimap<CL_UINT,CL_UINT>::iterator it = helperMultimap.begin();
				std::multimap<CL_UINT,CL_UINT>::iterator it_end = helperMultimap.end();
				i = 0;
				for(;it != it_end; it++)
				{
					pairs[i].x = it->first;
					pairs[i].y = it->second;
					i++;
				}
			}
			
		}
	}
}

#endif

