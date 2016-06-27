/**
 * @file TwoLevelGridTest.h
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
 * Some testing functions for Two Level Grid Construction algorithm and its stages
 * 
 */

#ifndef CL_RT_TWO_LEVEL_GRID_TEST 
#define CL_RT_TWO_LEVEL_GRID_TEST

#include <iostream>
#include <fstream>
#include <ctime>
#include <CLData\RTKernelUtils.h>
#include <CLData\AccelerationStructs\TwoLevelGridData.h>

namespace CLRayTracer
{
	namespace Testing
	{
		namespace TwoLevelGrid
		{
			/**
			 * Check if cell-primitive pairs are counted correctly
			 * @param pairsArray Cell-primitive pairs
			 * @param countsArray Counts of cell-primitive pairs, such that for each primitive 
			 *                    index i countsArray[i] contains counts of pairs that involve that primitive
			 * @param pairsCount Number of pairs in array
			 * @param primsCount Number of primitives
			 * @return True if count is correct
			*/
			inline bool checkWhetherPairsCountCorrect(CL_UINT2* pairsArray, CL_UINT* countsArray,CL_UINT pairsCount,CL_UINT primsCount)
			{
				bool error = false;
				for(int i = 0; i < primsCount; i++)
				{
					int hits = 0;
					for (int h = 0; h < pairsCount; h++)
					{
						if (pairsArray[h].y == i)
						hits++;
					}
					if (countsArray[i] != hits)
					{
						std::cout << "Assertion of count of pairs failed for index: " << i << std::endl;
						error = true;
					}
				}
				return !error;
			}

			/* Gets bounding box of a grid cell
			*  @param grid Data about the grid
			*  @param cellCoords X-Y-Z cell coordinates
			*  @param [out] cellBox The resulting cell bounding box
			*  @return 
			*/
			inline void getCellBoundingBox(CL_CONSTANT struct GridData* grid, CL_UINT3 cellCoords, struct AABB* cellBox)
			{
				cellBox->bounds[0].x = cellCoords.x * grid->stepX + grid->box.bounds[0].x;
				cellBox->bounds[0].y = cellCoords.y * grid->stepY + grid->box.bounds[0].y;
				cellBox->bounds[0].z = cellCoords.z * grid->stepZ + grid->box.bounds[0].z;
				cellBox->bounds[1].x = cellBox->bounds[0].x + grid->stepX;
				cellBox->bounds[1].y = cellBox->bounds[0].y + grid->stepY;
				cellBox->bounds[1].z = cellBox->bounds[0].z + grid->stepZ;
			}

			/**
			 * Check if primitives really overlap their assigned cells
			 * @param pairs Cell-primitive pairs
			 * @param pairsCount Number of pairs in array
			 * @param scene Buffer that contains the scene
			 * @param grid The top-level grid data
			 * @return True if all primitives overlap their cells
			*/
			inline bool checkCellsOverlapping(CL_UINT2* pairs,
											  CL_UINT pairsCount,
											  const char* scene,
											  struct GridData& grid)
			{
				bool error = false;
				for (int i = 0; i < pairsCount; i++)
				{
					CL_UINT3 cellCoords = getCellRefFromIndex(pairs[i].x,grid.resX,grid.resY,grid.resZ);
					struct AABB cellBox;
					getCellBoundingBox(&grid,cellCoords,&cellBox);
					struct AABB triangleAABB;
					CL_UINT3 triangleRef = getTriangleRefByIndex(scene,pairs[i].y);
					CL_GLOBAL char* submesh = getMeshAtIndex(triangleRef.y,getModelAtIndex(triangleRef.x,scene));
					VERTEX_TYPE v0 = getVertexAt(getIndexAt(triangleRef.z * 3,submesh),submesh);
					VERTEX_TYPE v1 = getVertexAt(getIndexAt(triangleRef.z * 3 + 1,submesh),submesh);
					VERTEX_TYPE v2 = getVertexAt(getIndexAt(triangleRef.z * 3 + 2,submesh),submesh); 
					triangleAABB = calculateTriangleAABB(v0,v1,v2);
					if (!AABBOverlaps(triangleAABB,cellBox))
					{
						std::cout << "No overlap at index: " << i << std::endl;
						error = true;
					}
				}
				return !error;
			}

			/**
			 * Check if sorted pairs range per cell calculated correctly
			 * @param sortedPairs Cell-primitive pairs sorted by cell index
			 * @param sortedPairsCount Number of pairs in array
			 * @param cells Top Level Grid cells, represented by range of pairs in the sorted pairs array
			 * @param cellCount Number of cells in cells array
			 * @return True if no errors found
			*/
			inline bool testCellCorrectness(CL_UINT2* sortedPairs,
											CL_UINT sortedPairsCount,
											CL_UINT2* cells,
											CL_UINT cellCount)
			{
				bool error = false;
				for (int cell = 0; cell < cellCount; cell++)
				{
					CL_UINT2 currentCell = cells[cell];
					if (currentCell.x != currentCell.y) //Current cell has range
					{
						for (int pair = 0; pair < sortedPairsCount; pair++)
						{
							if (pair < currentCell.x && sortedPairs[pair].x >= cell) //cell index is out of range from the left,
								error = true;                                        //but cell index in pair array is larger  
							else if (pair >= currentCell.x && pair < currentCell.y && sortedPairs[pair].x != cell) //cell index is in range of this cell
								error = true;                                                                       //but index is not equals to index of this cell
							else if (pair > currentCell.y && sortedPairs[pair].x <= cell)  //cell index is out of range from the right, 
								error = true;                                              //but cell index in pair array
						}
					}
				}
				return !error;
			}

			/**
			 * Export grid data to file
			 * @param sortedPairs Cell-primitive pairs sorted by cell index
			 * @param sortedPairsCount Number of pairs in array
			 * @param cells Top Level Grid cells, represented by range of pairs in the sorted pairs array
			 * @param cellCount Number of cells in cells array
			 * @return
			*/
			inline void exportToFile(CL_UINT2* sortedPairs,
									 CL_UINT sortedPairsCount,
									 CL_UINT2* cells,
									 CL_UINT cellCount)
			{
				time_t time;
				::time(&time);
				std::stringstream fname;
				fname << "twoLevelGrid_" << time << ".log";
				std::ofstream o(fname.str());
				o << "--------Sorted Pairs-------" << std::endl;
				for (int i = 0; i < sortedPairsCount; i++)
					o << "Idx: " << i <<" Key: " << sortedPairs[i].x << " Val: " << sortedPairs[i].y << std::endl;
				o << "--------Cells-------" << std::endl;
				for (int i = 0; i < cellCount; i++)
					o << "Idx: " << i <<" Key: " << cells[i].x << " Val: " << cells[i].y << std::endl;
				o.close();
			}

			/**
			 * Export grid data to file 
			 * @param cells Top Level Grid cells
			 * @param cellCount Number of cells in cells array
			 * @param leafRanges Ranges in the leaf array
			 * @param leafRangesCount Number of items in leafRanges array
			 * @param refPairs Leaf cell-primitive pairs
			 * @param refPairsCount Number of items in refPairs array 
			 * @return
			*/
			inline void exportToFile(TopLevelCell* cells, CL_UINT cellCount,
									 CL_UINT2* leafRanges, CL_UINT leafRangesCount,
									 CL_UINT2* refPairs, CL_UINT refPairsCount)
			{
				time_t time;
				::time(&time);
				std::stringstream fname;
				fname << "twoLevelGrid_final" << time << ".log";
				std::ofstream o(fname.str());
				o << "-----------Top Level Cells-------- " << std::endl;
				for(int i = 0; i < cellCount; i++)
					o << "Cell: " << i << " resX: " << cells[i].resX << " resY: " << cells[i].resY << " resZ: " << cells[i].resZ << " first Leaf: " <<cells[i].firstLeafIdx << std::endl;
				
				o << "--------------Leaf ranges----------------- " << std::endl;
				for(int i = 0; i < leafRangesCount; i++)
					o << "Leaf: " << i << " First ref idx: " << leafRanges[i].x << " Last ref idx: " << leafRanges[i].y << std::endl;

				o << "-------------Reference Array----------" << std::endl;
				for(int i = 0; i < refPairsCount; i++)
					o << "Ref pair: " << i << " Leaf Cell idx: " << refPairs[i].x << " Primitive Idx: " << refPairs[i].y << std::endl;

				o.close();
			}
		}
	}
}

#endif