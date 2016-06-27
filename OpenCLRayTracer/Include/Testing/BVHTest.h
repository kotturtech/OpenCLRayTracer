/**
 * @file BVHTest.h
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
 * Some testing functions for BVH construction
 * 
 */


#ifndef CL_RT_BVH_TEST 
#define CL_RT_BVH_TEST

#include <iostream>
#include <CLData\AccelerationStructs\BVHData.h>
#include <boost\smart_ptr.hpp>
#include <CLData\CLPortability.h>


namespace CLRayTracer
{
	namespace Testing
	{
		namespace BVH
		{
			/**
			 * Tests correctness of hierarchy - That path upwards from the leaf nodes leads to root
			 * @param hierarchy - A buffer that contains the nodes
			 * @param leafCount - Number of leaves in hierarchy
			 * @return True if no errors found, otherwise false
			*/
			bool testHierarchyCorrectness(BVHNode* hierarchy,int leafCount)
			{
				int expectedRoot = leafCount+1;
				bool error = false;
				for(int i = 0; i < leafCount; i++)
				{
					CL_UINT p= parent(hierarchy[i]);
					CL_UINT prev = i;
					int counter = 0;
					while(p != UINT_MAX)
					{
						prev = p;
						p = parent(hierarchy[p]);
						counter++;
						if (counter > leafCount)
						{
							std::cout << "Cycle detected from leaf " << i << std::endl;
							return false;
						}
						
					}
					if (prev != leafCount)
					{
						std::cout << "Leaf " << i <<" :Upward path doesn't end at root!" << std::endl;
						error = true;
					}
				}
				return !error;
			}

			/**
			 * Tests correctness of hierarchy - Tests that the parent node box contains or at least overlaps with boxes of it's children
			 * @param hierarchy - A buffer that contains the nodes
			 * @param leafCount - Number of leaves in hierarchy
			 * @return True if no errors found, otherwise false
			*/
			bool testBoundingBoxCorrectness(BVHNode* hierarchy,int leafCount)
			{
				int expectedRoot = leafCount+1;
				bool error = false;
				for(int i = 0; i < leafCount; i++)
				{
					CL_UINT current = i;
					while(current != UINT_MAX)
					{
						CL_UINT next = parent(hierarchy[current]);
						if (next == UINT_MAX)
							break;
						if (!AABBContains((hierarchy[next].boundingBox),(hierarchy[current].boundingBox)))
						{
							std::cout << "BB error, path from leaf: " << i << " parent: " << next << " current: " << current << std::endl;
							if (!AABBOverlaps((hierarchy[next].boundingBox),(hierarchy[current].boundingBox)))
							{
								std::cout << "BB not-even-overlap error, path from leaf: " << i << " parent: " << next << " current: " << current << std::endl;
							}
							error = true;
						}
						current = next;
					}
				}
				return !error;
			}
		}

	}
}

#endif

