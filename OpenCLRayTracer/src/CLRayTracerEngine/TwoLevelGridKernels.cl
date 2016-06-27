/**
 * @file TwoLevelGridKernels.cl
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
 * Kernel functions for Two Level Grid construction and traversal.
 * 
 * This file contains entry points for the kernel - Most of the logic functions are contained in
 * file: TwoLevelGrid.h, for CPU portability
 *
 * @implNote The algorithm implemented according to the article: "Two-Level Grids for Ray Tracing on GPUs"
 *           by Javor Kalojanov, Markus Billeter and Philipp Slusallek
 *           http://www.intel-vci.uni-saarland.de/fileadmin/grafik_uploads/publications/59.pdf
 */

#include "CLData\AccelerationStructs\TwoLevelGrid.h"
#include "CLData\AccelerationStructs\TwoLevelGridData.h"


 /*****************************************************
 * 1. Counts top level pairs
 ******************************************************/
__kernel void prepareDataKernel(CL_GLOBAL char* scene, 
						  CL_CONSTANT struct GridData* grid,
						  CL_GLOBAL CL_UINT* counters)
{
	uint currentIdx = get_global_id(0);
	uint tris = SCENE_HEADER(scene)->totalNumberOfTriangles-1;
	if (currentIdx < tris)
		prepareGridData(scene,currentIdx,grid,counters);
}

/*****************************************************
 * 2. Generates top level pairs
 ******************************************************/
__kernel void writePairsKernel(CL_GLOBAL char* scene, 
					     CL_CONSTANT struct GridData* grid,
					     CL_GLOBAL CL_UINT* prefixSum,
					     CL_GLOBAL CL_UINT* counters,
					     CL_GLOBAL CL_UINT2* pairs)
{
	uint currentIdx = get_global_id(0);
	uint tris = SCENE_HEADER(scene)->totalNumberOfTriangles-1;
	if (currentIdx < tris)
		writePairs(scene,currentIdx,grid,prefixSum,counters,pairs);
}

/*****************************************************
 * 3. Extracts ranges in sorted pairs array
 ******************************************************/
__kernel void extractCellRangesKernel(CL_GLOBAL CL_UINT2* sortedPairs,
						         CL_UINT sortedPairsCount,
						         CL_GLOBAL CL_UINT2* cellRanges,
								 CL_LOCAL CL_UINT2* localBuf,
						         CL_UINT workingBlock)
{
	const uint batchSize = workingBlock;
	const uint batches = ceil((float)sortedPairsCount / (float)batchSize);
	
	uint maxGlobalIdx = sortedPairsCount - 1;
	uint currentLocalIdx = get_global_id(0);
	uint nextLocalIdx;
	long currentGlobalIdx;
	uint nextGlobalIdx;
	for(int i = 0; i < batches; i++)
	{
		//load pairs to shared memory
		nextLocalIdx = currentLocalIdx + 1;
		currentGlobalIdx = i * workingBlock + currentLocalIdx;
		nextGlobalIdx = currentGlobalIdx + 1;
		if (currentGlobalIdx <= maxGlobalIdx)
		{
			localBuf[currentLocalIdx] = sortedPairs[currentGlobalIdx];
			if (currentLocalIdx == workingBlock - 1 && nextGlobalIdx <=maxGlobalIdx) //Last thread writes additional
				localBuf[nextLocalIdx] = sortedPairs[nextGlobalIdx];
		}
		//Synchronize the local memory
		barrier(CLK_LOCAL_MEM_FENCE);
		if (currentGlobalIdx <= maxGlobalIdx && currentLocalIdx < batchSize)
		{
			uint right = localBuf[nextLocalIdx].x;
			uint thisOne = localBuf[currentLocalIdx].x;
			if (right > thisOne)
			{ 
				cellRanges[thisOne].y = nextGlobalIdx;
				cellRanges[right].x = nextGlobalIdx;
			}
		}
	}

	//Post-processing for last index - Closing last pair
	if (currentGlobalIdx == maxGlobalIdx)
	{
		cellRanges[sortedPairs[currentGlobalIdx].x].y = currentGlobalIdx;
	}
}

/*****************************************************
 * 4. Counts leaf cells
 ******************************************************/
__kernel void countLeavesAndFillCellKernel(CL_GLOBAL CL_UINT2* range, CL_GLOBAL CL_UINT* counters,
 CL_GLOBAL struct TopLevelCell* cells, CL_UINT cellsCount,CL_CONSTANT struct GridData* grid)
{
	if (get_global_id(0) < cellsCount)
		fillTopLevelCell(range,counters,cells,grid,get_global_id(0));
}


/*****************************************************
 * 5. Update top leaf cell with beginning of its range
 ******************************************************/
__kernel void updateTopLevelCellsWithLeafRange(CL_GLOBAL struct TopLevelCell* topLevelCells,
											   CL_GLOBAL CL_UINT* leafCountsPerCell,
											   CL_UINT cellCount)
{
	CL_UINT idx = get_global_id(0);
	if (idx > 0)
		topLevelCells[idx].firstLeafIdx = leafCountsPerCell[idx-1];
}

/*****************************************************
 * 6. Count leaf pairs
 ******************************************************/
__kernel void prepareGridDataForLeaves(CL_GLOBAL const char* scene, 
									   CL_GLOBAL CL_UINT2* topLevelPairs,
									   CL_UINT topLevelPairsCount,
									   CL_CONSTANT struct GridData* grid,
									   CL_GLOBAL struct TopLevelCell* topLevelCells,
									   CL_GLOBAL CL_UINT* counters)
{
	CL_UINT idx = get_global_id(0); 
	if (idx < topLevelPairsCount)
		counters[idx] = countLeafPairs(scene,topLevelPairs,idx,grid,topLevelCells);
	else
		counters[idx] = 0;
}


/*****************************************************
 * 7. Write leaf pairs
 ******************************************************/
__kernel void writeLeafPairsKernel(CL_GLOBAL const char* scene,
						   CL_GLOBAL CL_UINT2* topLevelPairs,
						   CL_GLOBAL struct TopLevelCell* topLevelCells,
					       CL_CONSTANT struct GridData* grid,
					       CL_GLOBAL CL_UINT* prefixSum,
					       CL_GLOBAL CL_UINT* counters,
					       CL_GLOBAL CL_UINT2* pairs,
						   CL_UINT pairCount)
{
	uint idx = get_global_id(0);
	if(idx < pairCount)
		writeLeafPairs(scene,topLevelPairs,topLevelCells,idx,grid,prefixSum,counters,pairs);
}

/*****************************************************
 * 8. Extract ranges for leaf cells
 ******************************************************/
__kernel void extractLeafCellsKernel(CL_GLOBAL CL_UINT2* sortedLeafPairs,
						   CL_UINT sortedLeafPairsCount,
						   CL_GLOBAL CL_UINT2* leafCellRanges,
						   CL_LOCAL CL_UINT2* localBuf)
{
	const uint batchSize = get_local_size(0);
	const uint batches = ceil((float)get_global_size(0) / (float)batchSize);
	
	uint maxGlobalIdx = sortedLeafPairsCount - 1;
	uint currentLocalIdx = get_global_id(0);
	uint nextLocalIdx;
	long currentGlobalIdx;
	uint nextGlobalIdx;
	for(int i = 0; i < batches; i++)
	{
		//load pairs to shared memory
		nextLocalIdx = currentLocalIdx + 1;
		currentGlobalIdx = i * batchSize + currentLocalIdx;
		nextGlobalIdx = currentGlobalIdx + 1;
		if (currentGlobalIdx <= maxGlobalIdx)
		{
			localBuf[currentLocalIdx] = sortedLeafPairs[currentGlobalIdx];
			if (currentLocalIdx == batchSize - 1 && nextGlobalIdx <=maxGlobalIdx) //Last thread writes additional
				localBuf[nextLocalIdx] = sortedLeafPairs[nextGlobalIdx];
		}
		//Synchronize the local memory
		barrier(CLK_LOCAL_MEM_FENCE);
		if (currentGlobalIdx <= maxGlobalIdx && currentLocalIdx < batchSize)
		{
			uint right = localBuf[nextLocalIdx].x;
			uint thisOne = localBuf[currentLocalIdx].x;
			if (right > thisOne)
			{ 
				leafCellRanges[thisOne].y = nextGlobalIdx;
				leafCellRanges[right].x = nextGlobalIdx;
			}
		}
	}

	//Post-processing for last index - Closing last pair
	if (currentGlobalIdx == maxGlobalIdx)
		leafCellRanges[sortedLeafPairs[currentGlobalIdx].x].y = currentGlobalIdx;
}


/*****************************************************
 * 9. Generate contacts for viewing rays
 ******************************************************/
__kernel __attribute__((work_group_size_hint(1, 1, 64)))
 void generateContactsKernel(CL_CONSTANT struct Camera* camera,
									 CL_GLOBAL char* scene,
									 CL_CONSTANT struct GridData* gridData,
									 CL_GLOBAL struct TopLevelCell* topLevelCells,
									 CL_GLOBAL CL_UINT2* leavesArray,
									 CL_GLOBAL CL_UINT2* pairsRefArray,
									 CL_GLOBAL struct Contact* output)
{
	const CL_UINT maxRays = camera->resX * camera->resY;
	const CL_UINT myIdx = get_global_id(0);
	if (myIdx < maxRays)
	{
			const struct Ray ray = generateRay(camera,myIdx);
			struct Contact result = tlg_generate_contact(ray,scene,gridData,topLevelCells,leavesArray,pairsRefArray);
			result.pixelIndex = myIdx;
			output[myIdx] = result;
	} 
}

/*****************************************************
 * 9. Generate contacts for general rays
 ******************************************************/
__kernel __attribute__((work_group_size_hint(1, 1, 64)))
 void generateContacts2Kernel(CL_GLOBAL struct Ray* rays,
									 uint rayCount,
									 CL_GLOBAL char* scene,
									 CL_CONSTANT struct GridData* gridData,
									 CL_GLOBAL struct TopLevelCell* topLevelCells,
									 CL_GLOBAL CL_UINT2* leavesArray,
									 CL_GLOBAL CL_UINT2* pairsRefArray,
									 CL_GLOBAL struct Contact* output)
{
	const CL_UINT myIdx = get_global_id(0);
	if (myIdx < rayCount)
	{
		struct Contact result = tlg_generate_contact(rays[myIdx],scene,gridData,topLevelCells,leavesArray,pairsRefArray);
		result.pixelIndex = myIdx;
		output[myIdx] = result;
	} 
}














