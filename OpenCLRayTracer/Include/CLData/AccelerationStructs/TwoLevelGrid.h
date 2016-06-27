/**
 * @file TwoLevelGrid.h
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
 * Utility functions for Two Level Grid construction and traversal.
 * 
 * @implNote The algorithm implemented according to the article: "Two-Level Grids for Ray Tracing on GPUs"
 *           by Javor Kalojanov, Markus Billeter and Philipp Slusallek
 *           http://www.intel-vci.uni-saarland.de/fileadmin/grafik_uploads/publications/59.pdf
 */

#ifndef CL_RT_TWOLEVELGRID
#define CL_RT_TWOLEVELGRID

#include <CLData/SceneBufferParser.h>
#include <CLData/RTKernelUtils.h>
#include <CLData/CLStructs.h>
#include <CLData/AccelerationStructs/TwoLevelGridData.h>
#include <CLData/Primitives/Triangle.h>

/*************************************************************
* General utility functions
**************************************************************/

/* Calculates linear cell index from X-Y-Z cell coordinates
*  @param ix X index of the cell
*  @param iy Y index of the cell
*  @param iz Z index of the cell
*  @param rx X component of Grid resolution
*  @param ry Y component of Grid resolution
*  @param rz Z component of Grid resolution
*  @return Linear index of cell at ix,iy,iz
*/
inline CL_UINT getCellIndex(CL_UINT ix, CL_UINT iy, CL_UINT iz,
					 CL_UINT rx, CL_UINT ry, CL_UINT rz)
{
	return iz * rx * ry + iy * rx + ix;
}

/* Calculates X-Y-Z cell coordinates from linear cell index
*  @param idx Linear index of the cell
*  @param rx X component of Grid resolution
*  @param ry Y component of Grid resolution
*  @param rz Z component of Grid resolution
*  @return X-Y-Z cell coordinates of the cell at linear index idx
*/
inline CL_UINT3 getCellRefFromIndex(CL_UINT idx,CL_UINT rx, CL_UINT ry, CL_UINT rz)
{
	CL_UINT a = (rx * ry);
	CL_UINT z = idx / a; 
	CL_UINT b = idx - a * z;
	return (CL_UINT3)combineToVector(b%rx,b/rx,z);
}

/*************************************************************
* Grid Construction Functions
**************************************************************/

/* Counts how many grid cells does a triangle (Precisely, bounding box of a triangle) overlap
*  @param v0 First vertex of a triangle
*  @param v1 Second vertex of a triangle
*  @param v2 Third vertex of atriangle
*  @param grid The grid data
*  @return Count of cells overlapped by triangle bounding box
*/
inline CL_UINT countOverlappingCells(VERTEX_TYPE v0,VERTEX_TYPE v1,VERTEX_TYPE v2,
									 CL_CONSTANT struct GridData* grid)
{
	CL_FLOAT3 lastIndices = (CL_FLOAT3)combineToVector((CL_FLOAT)(grid->resX-1),
													   (CL_FLOAT)(grid->resY-1),
													   (CL_FLOAT)(grid->resZ-1));

	CL_FLOAT3 startCells = MIN3(FLOOR3((MIN3(v0,MIN3(v1,v2)) - (CL_FLOAT3)combineToVector(grid->box.bounds[0].x,grid->box.bounds[0].y,grid->box.bounds[0].z))/(CL_FLOAT3)combineToVector(grid->stepX,grid->stepY,grid->stepZ)),
								lastIndices);
	CL_FLOAT3 endCells = MIN3(FLOOR3((MAX3(v0,MAX3(v1,v2)) - (CL_FLOAT3)combineToVector(grid->box.bounds[0].x,grid->box.bounds[0].y,grid->box.bounds[0].z))/(CL_FLOAT3)combineToVector(grid->stepX,grid->stepY,grid->stepZ)),
								lastIndices);
	CL_FLOAT3 cells = (endCells - startCells) + 1.0f;
	return cells.x * cells.y * cells.z;
}

/* Counts how many grid cells does a triangle (Precisely, bounding box of a triangle) overlap, and fills result in counters array
*  @param scene Scene
*  @param triangleIndex Global index of a triangle in the scene
*  @param grid The grid data
*  @param The array of counters
*  @return 
*/
inline void prepareGridData(CL_GLOBAL const char* scene, 
					 CL_UINT triangleIndex, 
					 CL_CONSTANT struct GridData* grid,
					 CL_GLOBAL CL_UINT* counters)
{
	//Getting the references to the triangle
	CL_UINT3 triangleRef = getTriangleRefByIndex(scene,triangleIndex);
	CL_GLOBAL char* submesh = getMeshAtIndex(triangleRef.y,getModelAtIndex(triangleRef.x,scene));
	
	//Now as we have the vertices = Check how many cells does the triangle overlap
	CL_UINT baseIndex = triangleRef.z * 3;
	counters[triangleIndex] = countOverlappingCells(
		 getVertexAt(getIndexAt(baseIndex,submesh),submesh),
		 getVertexAt(getIndexAt(baseIndex + 1,submesh),submesh),
		 getVertexAt(getIndexAt(baseIndex + 2,submesh),submesh),grid); 
}

/* Creates and writes cell-primitive pair for specified triangle at specified index
*  @param v0 First vertex of a triangle
*  @param v1 Second vertex of a triangle
*  @param v2 Third vertex of atriangle
*  @param grid The grid data
*  @param firstPairIdx Index of the first pair related to the given triangle
*  @param triangleIdx Index of given triangle
*  @param pairsArray the target array to write pairs to
*  @return
*/
inline void writeOverlappingPairs(VERTEX_TYPE v0,VERTEX_TYPE v1,VERTEX_TYPE v2,
								  CL_CONSTANT struct GridData* grid,
								  CL_UINT firstPairIdx,
						          CL_UINT triangleIdx,
						          CL_GLOBAL CL_UINT2* pairsArray)
{
	
	CL_FLOAT3 bboxOrigin = (CL_FLOAT3)combineToVector(grid->box.bounds[0].x,grid->box.bounds[0].y,grid->box.bounds[0].z);
	CL_FLOAT3 gridStep = (CL_FLOAT3)combineToVector(grid->stepX,grid->stepY,grid->stepZ);
	CL_UINT3 maxGridIdx = (CL_UINT3)combineToVector(grid->resX-1,grid->resY-1,grid->resZ-1);
	//Calculate coordinates of first cell (The cell in which the min point located)
	CL_UINT3 cell = MIN3(convert_uint3(FLOOR3((MIN3(v0,MIN3(v1,v2)) - bboxOrigin)/gridStep)),maxGridIdx);
	//Calculate extents - over how many cells the triangle BB spans?
	CL_UINT3 cellExtents = MIN3(convert_uint3(FLOOR3((MAX3(v0,MAX3(v1,v2)) - bboxOrigin)/gridStep)),maxGridIdx);
	
	CL_UINT2 pair;
	pair.y = triangleIdx;
	CL_UINT cellCounter = 0;
	//Filling the pairs
	for (CL_UINT z = cell.z; z <= cellExtents.z; z++)
		for (CL_UINT y = cell.y; y <= cellExtents.y; y++)
			for (CL_UINT x = cell.x; x <= cellExtents.x; x++)
			{
				pair.x = getCellIndex(x,y,z,grid->resX,grid->resY,grid->resZ);
				pairsArray[firstPairIdx + cellCounter] = pair;
				cellCounter++;
			}
}

/* Creates and writes cell-primitive pair for specified triangle at specified index
*  @param scene Scene
*  @param triangleIndex Global index of a triangle in the scene
*  @param grid The grid data
*  @param Prefix Sum array of pair counts for each primitive
*  @param Array of counts of pairs per each primitive
*  @param pairsArray the target array to write pairs to
*  @return
*/
inline void writePairs(CL_GLOBAL const char* scene, 
					 CL_UINT triangleIndex, 
					 CL_CONSTANT struct GridData* grid,
					 CL_GLOBAL CL_UINT* prefixSum,
					 CL_GLOBAL CL_UINT* counters,
					 CL_GLOBAL CL_UINT2* pairs)
{
	//Getting the references to the triangle
	CL_UINT3 triangleRef = getTriangleRefByIndex(scene,triangleIndex);
	CL_GLOBAL char* submesh = getMeshAtIndex(triangleRef.y,getModelAtIndex(triangleRef.x,scene));
	CL_UINT myStart = prefixSum[triangleIndex] - counters[triangleIndex]; //The starting space in the pairs array;
	//Prefix sum is the total so far, minus the quiantity of pairs per current triangle
	writeOverlappingPairs(getVertexAt(getIndexAt(triangleRef.z * 3,submesh),submesh),
									  getVertexAt(getIndexAt(triangleRef.z * 3 + 1,submesh),submesh),
									  getVertexAt(getIndexAt(triangleRef.z * 3 + 2,submesh),submesh),
									  grid,myStart,triangleIndex,pairs);
}

const CL_CONSTANT float oneThird = 1.0f / 3.0f;

/* Calculates leaf cell resolution
*  @param numOfPrims
*  @param grid The grid data
*  @return The grid resolution
*/
inline CL_UINT3 calcLeafCellResolution(CL_UINT numOfPrims,CL_CONSTANT struct GridData* grid)
{
	CL_FLOAT3 cellExtents = (CL_FLOAT3)combineToVector(grid->stepX, grid->stepY,grid->stepZ);
	CL_FLOAT volume = cellExtents.x * cellExtents.y * cellExtents.z;
	CL_FLOAT a = pow(grid->leafDensity * numOfPrims / volume,oneThird);
	return convert_uint3(FLOOR3(cellExtents * a));
}

/* Fills data for top level cell in two level grid
*  @param range Leaf cell range
*  @param leavesCount Leaf cell count per top level cell
*  @param cells Top Level Cells array
*  @param grid Data about the Top Level Grid
*  @param idx Top level cell index
*  @return 
*/
void fillTopLevelCell(CL_GLOBAL CL_UINT2* range, CL_GLOBAL CL_UINT* leavesCount, CL_GLOBAL struct TopLevelCell* cells,CL_CONSTANT struct GridData* grid, CL_UINT idx)
{
	CL_UINT2 rangeItem = range[idx];
	CL_UINT3 res = calcLeafCellResolution(rangeItem.y - rangeItem.x,grid);
	struct TopLevelCell cell;
	cell.resX = res.x;
	cell.resY = res.y;
	cell.resZ = res.z;
	cells[idx] = cell;
	leavesCount[idx] = res.x * res.y * res.z;
}

/* Calculates number of leaf cells according to number of primitives 
*  @param numOfPrims Number of primitives in top level cell
*  @param grid Data about the grid
*  @return Count of leaf cells that top level cell should contain 
*/
inline CL_UINT countLeafCells(CL_UINT numOfPrims,CL_CONSTANT struct GridData* grid)
{
	CL_FLOAT volume = grid->stepX * grid->stepY * grid->stepZ;
	CL_FLOAT a = pow(grid->leafDensity * numOfPrims / volume,oneThird);
	return (a * grid->stepX) * (a * grid->stepY) * (a * grid->stepZ);
}



/**
* Counts maximum LEAF pairs will be generated for each triangle.
* Still a less precise test based on boundin box - To gain some speed at counting. When pairs will be 
* generated, the precise method should be used
* @param v0 First vertex of triangle
* @param v1 Second vertex of triangle
* @param v2 Third vertex of triangle
* @param topLevelCellIdx Index of top level cell
* @param grid Data about the grid
* @return Maximal number of leaf cell-primitive pairs
*/
inline CL_UINT countOverlappingLeafCells(VERTEX_TYPE v0,VERTEX_TYPE v1,VERTEX_TYPE v2, 
										 CL_GLOBAL struct TopLevelCell* topLevelCell,
										 CL_UINT topLevelCellIdx, 
										 CL_CONSTANT struct GridData* grid)
{
	struct AABB triangleAABB = calculateTriangleAABB(v0,v1,v2);

	CL_FLOAT leafStepX = grid->stepX / topLevelCell->resX;
	CL_FLOAT leafStepY = grid->stepY / topLevelCell->resY;
	CL_FLOAT leafStepZ = grid->stepZ / topLevelCell->resZ;
	CL_UINT3 topLevelCoordinates = getCellRefFromIndex(topLevelCellIdx,grid->resX,grid->resY,grid->resZ);
	CL_FLOAT topBaseX = grid->box.bounds[0].x + topLevelCoordinates.x * grid->stepX;
	CL_FLOAT topBaseY = grid->box.bounds[0].y + topLevelCoordinates.y * grid->stepY;
	CL_FLOAT topBaseZ = grid->box.bounds[0].z + topLevelCoordinates.z * grid->stepZ;

	CL_UINT relevantLeafCount = 0;
	struct AABB leafCell;
	for (CL_UINT z = 0; z < topLevelCell->resZ; z++)
		for (CL_UINT y = 0; y < topLevelCell->resY; y++)
			for (CL_UINT x = 0; x < topLevelCell->resX; x++)
			{
				leafCell.bounds[0].x = topBaseX + x * leafStepX; 
				leafCell.bounds[0].y = topBaseY + y * leafStepY; 
				leafCell.bounds[0].z = topBaseZ + z * leafStepZ; 
				leafCell.bounds[1].x = leafCell.bounds[0].x + leafStepX;
				leafCell.bounds[1].y = leafCell.bounds[0].y + leafStepY;
				leafCell.bounds[1].z = leafCell.bounds[0].z + leafStepZ;
				relevantLeafCount += (CL_UINT)AABBOverlaps(leafCell,triangleAABB);
			}
	return relevantLeafCount;
}

/**
* Counts maximum LEAF pairs will be generated for each triangle.
* Still a less precise test based on boundin box - To gain some speed at counting. When pairs will be 
* generated, the precise method should be used
* @param scene Scene 
* @param topLevelPairs Array of top level pairs
* @param topLevelPairIdx Index of the processed top level pair
* @param grid Data about the grid
* @param array of top level cells
* @return Maximal number of leaf cell-primitive pairs
*/
inline CL_UINT countLeafPairs(CL_GLOBAL const char* scene, 
									 CL_GLOBAL CL_UINT2* topLevelPairs,
									 CL_UINT topLevelPairIdx,
									 CL_CONSTANT struct GridData* grid,
									 CL_GLOBAL struct TopLevelCell* topLevelCells)
{
	//Getting the references to the triangle
	CL_UINT2 topLevelPair = topLevelPairs[topLevelPairIdx];
	CL_UINT3 triangleRef = getTriangleRefByIndex(scene,topLevelPair.y);
	CL_GLOBAL char* submesh = getMeshAtIndex(triangleRef.y,getModelAtIndex(triangleRef.x,scene));
	
	//Now as we have the vertices = Check how many leaf cells does the triangle overlap
	CL_UINT baseIdx = triangleRef.z * 3;
	return countOverlappingLeafCells(
		 getVertexAt(getIndexAt(baseIdx,submesh),submesh),
		 getVertexAt(getIndexAt(baseIdx + 1,submesh),submesh),
		 getVertexAt(getIndexAt(baseIdx + 2,submesh),submesh),
		 (topLevelCells + topLevelPair.x),
		 topLevelPair.x,
		 grid);
}


/**
* Generates leaf cell - primitive pairs per primitive using a precise test
* @param v0 First vertex of triangle
* @param v1 Second vertex of triangle
* @param v2 Third vertex of triangle
* @param grid Data about the grid
* @param topLevelCell Top level cell
* @param Processed top level pair
* @param startIndex the start index in output array, place to store the generated pairs
* @param pairs Array for output
* @return 
*/
inline void writeOverlappingLeafPairs(VERTEX_TYPE v0,VERTEX_TYPE v1,VERTEX_TYPE v2,
									  CL_CONSTANT struct GridData* grid,
									  struct TopLevelCell topLevelCell,
									  CL_UINT2 topLevelPair,
			                          CL_UINT startIndex,
									  CL_GLOBAL CL_UINT2* pairs)
{
	CL_FLOAT leafStepX = grid->stepX / topLevelCell.resX;
	CL_FLOAT leafStepY = grid->stepY / topLevelCell.resY;
	CL_FLOAT leafStepZ = grid->stepZ / topLevelCell.resZ;
	CL_UINT3 topLevelCoordinates = getCellRefFromIndex(topLevelPair.x,grid->resX,grid->resY,grid->resZ);
	CL_FLOAT topBaseX = grid->box.bounds[0].x + topLevelCoordinates.x * grid->stepX;
	CL_FLOAT topBaseY = grid->box.bounds[0].y + topLevelCoordinates.y * grid->stepY;
	CL_FLOAT topBaseZ = grid->box.bounds[0].z + topLevelCoordinates.z * grid->stepZ;	
	CL_UINT relevantLeafCount = 0;
	CL_UINT2 pair;
	pair.y = topLevelPair.y;
	CL_FLOAT3 leafCellCenter,leafCellHalfSize;
	leafCellHalfSize.x = leafStepX * 0.5f;
	leafCellHalfSize.y = leafStepY * 0.5f;
	leafCellHalfSize.z = leafStepZ * 0.5f;
	for (CL_UINT z = 0; z < topLevelCell.resZ; z++)
		for (CL_UINT y = 0; y < topLevelCell.resY; y++)
			for (CL_UINT x = 0; x < topLevelCell.resX; x++)
			{
				leafCellCenter.x = topBaseX + x * leafStepX + leafCellHalfSize.x; 
				leafCellCenter.y = topBaseY + y * leafStepY + leafCellHalfSize.y; 
				leafCellCenter.z = topBaseZ + z * leafStepZ + leafCellHalfSize.z;	
				if (AABBTriangleIntersect(leafCellCenter,leafCellHalfSize,v0,v1,v2))
				{
					pair.x = getCellIndex(x,y,z,topLevelCell.resX,topLevelCell.resY,topLevelCell.resZ) + topLevelCell.firstLeafIdx;
					pairs[startIndex + relevantLeafCount] = pair;
					relevantLeafCount++;
				}
			}
}

/**
* Generates leaf cell - primitive pairs array
* @param scene Scene 
* @param topLevelPairs Array of top level pairs
* @param topLevelCells array of top level cells
* @param tlpIndex Index of the processed top level pair
* @param grid Data about the grid
* @param prefixSum Array that contains counts of leaf cells so far (Result of prefix-sum)
* @param counters Array of counters of expected leaf-cell-primitive pairs
* @param pairs Array for output
* @return 
*/
inline void writeLeafPairs(CL_GLOBAL const char* scene,
						   CL_GLOBAL CL_UINT2* topLevelPairs,
						   CL_GLOBAL struct TopLevelCell* topLevelCells,
						   CL_UINT tlpIndex, 
					       CL_CONSTANT struct GridData* grid,
					       CL_GLOBAL CL_UINT* prefixSum,
					       CL_GLOBAL CL_UINT* counters,
					       CL_GLOBAL CL_UINT2* pairs)
{
	CL_UINT myStart = prefixSum[tlpIndex] - counters[tlpIndex]; //The starting space in the pairs array;
	//Prefix sum is the total so far, minus the quiantity of pairs per current top level pair
	CL_UINT2 topLevelPair = topLevelPairs[tlpIndex];
	
	CL_UINT3 triangleRef = getTriangleRefByIndex(scene,topLevelPair.y);
	CL_GLOBAL char* submesh = getMeshAtIndex(triangleRef.y,getModelAtIndex(triangleRef.x,scene));

	writeOverlappingLeafPairs(getVertexAt(getIndexAt(triangleRef.z * 3,submesh),submesh),
						  getVertexAt(getIndexAt(triangleRef.z * 3 + 1,submesh),submesh),
						  getVertexAt(getIndexAt(triangleRef.z * 3 + 2,submesh),submesh),
						  grid,
						  topLevelCells[topLevelPair.x],
						  topLevelPair,
						  myStart,pairs);
	 
}

/*************************************************************
* Grid Traversal Functions
**************************************************************/

/**
* Traverses leaf cells in a top level cell in a Two Level Grid
* @param ray Ray to test for hit 
* @param scene Buffer that contains the scene
* @param topLevelCell Top level cell
* @param cellBox Bounding box of top level cell
* @param leavesArray leaf cells array - Represented as ranges in Reference array
* @param pairsRefArray The cell-primitive reference array
* @return Information about closest intersection of ray and primitive within top level cell
*/
struct Contact processTopLevelCell(const struct Ray ray,
									CL_GLOBAL const char* scene,
									struct TopLevelCell topLevelCell,
									struct AABB cellBox,
									CL_GLOBAL CL_UINT2* leavesArray,
									CL_GLOBAL CL_UINT2* pairsRefArray)
{

	float 	next[3];
	int 	step[3];
	int 	stop[3];
	int idx[3];
	float dt[3];
	
	{
		float tx_min, ty_min, tz_min;
		float tx_max, ty_max, tz_max; 

		float a = 1.0f / ray.direction.x;
		tx_min = ((a >= 0.0f ? cellBox.bounds[0].x : cellBox.bounds[1].x) - ray.origin.x) * a;
		tx_max = ((a >= 0.0f ? cellBox.bounds[1].x : cellBox.bounds[0].x) - ray.origin.x) * a;
	
		a = 1.0f / ray.direction.y;
		ty_min = ((a >= 0.0f ? cellBox.bounds[0].y : cellBox.bounds[1].y) - ray.origin.y) * a;
		ty_max = ((a >= 0.0f ? cellBox.bounds[1].y : cellBox.bounds[0].y) - ray.origin.y) * a;
	
		a = 1.0 / ray.direction.z;
		tz_min = ((a >= 0.0f ? cellBox.bounds[0].z : cellBox.bounds[1].z) - ray.origin.z) * a;
		tz_max = ((a >= 0.0f ? cellBox.bounds[1].z : cellBox.bounds[0].z) - ray.origin.z) * a;
	
		CL_FLOAT3 p = isPointInside(cellBox,ray.origin) ? ray.origin : ray.origin + (ray.direction * min(tx_min,min(ty_min,tz_min))); 
	
		idx[iX] = clamp((p.x - cellBox.bounds[0].x) * (float)topLevelCell.resX / (cellBox.bounds[1].x - cellBox.bounds[0].x), 0.0f, (float)topLevelCell.resX - 1);
		idx[iY] = clamp((p.y - cellBox.bounds[0].y) * (float)topLevelCell.resY / (cellBox.bounds[1].y - cellBox.bounds[0].y), 0.0f, (float)topLevelCell.resY - 1);
		idx[iZ] = clamp((p.z - cellBox.bounds[0].z) * (float)topLevelCell.resZ / (cellBox.bounds[1].z - cellBox.bounds[0].z), 0.0f, (float)topLevelCell.resZ - 1);

		// ray parameter increments per cell in the x, y, and z directions
		dt[iX] = (tx_max - tx_min) / topLevelCell.resX;
		dt[iY] = (ty_max - ty_min) / topLevelCell.resY;
		dt[iZ] = (tz_max - tz_min) / topLevelCell.resZ;
		
		if (ray.direction.x > 0.0f) 
		{
			next[iX] = tx_min + (idx[iX] + 1) * dt[iX];
			step[iX] = +1;
			stop[iX] = topLevelCell.resX;
		}
		else 
		{
			next[iX] = ray.direction.x == 0.0f ? FLT_MAX : tx_min + (topLevelCell.resX - idx[iX]) * dt[iX];
			step[iX] = -1;
			stop[iX] = -1;
		}

		if (ray.direction.y > 0) 
		{
			next[iY] = ty_min + (idx[iY] + 1) * dt[iY];
			step[iY] = +1;
			stop[iY] = topLevelCell.resY;
		}
		else 
		{
			next[iY] = ray.direction.y == 0.0f ? FLT_MAX : ty_min + (topLevelCell.resY - idx[iY]) * dt[iY];
			step[iY] = -1;
			stop[iY] = -1;
		}
		
		if (ray.direction.z > 0) 
		{
			next[iZ] = tz_min + (idx[iZ] + 1) * dt[iZ];
			step[iZ] = +1;
			stop[iZ] = topLevelCell.resZ;
		}
		else 
		{
			next[iZ] = ray.direction.z == 0.0f? FLT_MAX : tz_min + (topLevelCell.resZ - idx[iZ]) * dt[iZ];
			step[iZ] = -1;
			stop[iZ] = -1;
		}
	}
	// traverse the grid
	struct Contact result;
	result.normalAndintersectionDistance.x = 0.0f;
	result.normalAndintersectionDistance.y = 0.0f;
	result.normalAndintersectionDistance.z = 0.0f;
	result.normalAndintersectionDistance.w = FLT_MAX;
	bool contactFound = false;

	while (true) 
	{	
		float minimal = min(next[iX],min(next[iY],next[iZ]));
		int axis = 0;
		while(minimal != next[axis])
			axis++;

		//Process the leaf

		int leafIndex = getCellIndex(idx[iX],idx[iY],idx[iZ],
			topLevelCell.resX,topLevelCell.resY,topLevelCell.resZ) + topLevelCell.firstLeafIdx;
		CL_UINT2 leafRange = leavesArray[leafIndex];
		for (;leafRange.x < leafRange.y; leafRange.x++)
		{
			CL_UINT3 triangleRef = getTriangleRefByIndex(scene,pairsRefArray[leafRange.x].y);
			CL_GLOBAL char* submesh = getModelAtIndex(triangleRef.x,scene);
			submesh = getMeshAtIndex(triangleRef.y,submesh);
			CL_FLOAT4 newContact = triangleIntersect(getVertexAt(getIndexAt(triangleRef.z * 3,submesh),submesh),
													 getVertexAt(getIndexAt(triangleRef.z * 3 + 1,submesh),submesh),
													 getVertexAt(getIndexAt(triangleRef.z * 3 + 2,submesh),submesh),
													 ray.origin,ray.direction);
			if (newContact.w > 0 && newContact.w < result.contactDist)
			{
				contactFound = true;
				result.normalAndintersectionDistance = newContact;
				result.materialIndex = MESH_HEADER(submesh)->materialIndex;
			}
		}

		if (contactFound)
			return result;

		next[axis] += dt[axis];
		idx[axis] += step[axis];
								
		if (idx[axis] == stop[axis])
			return NO_CONTACT;
	}
}

/**
* Traverses Two Level Grid
* @param ray Ray to test for hit 
* @param scene Buffer that contains the scene
* @param gridData data about the grid
* @param topLevelCells Array of top level cells
* @param leavesArray leaf cells array - Represented as ranges in Reference array
* @param pairsRefArray The cell-primitive reference array
* @return Information about closest intersection of ray and primitive within the grid
*/
struct Contact tlg_generate_contact(const struct Ray ray,
									CL_GLOBAL const char* scene,
									CL_CONSTANT struct GridData* gridData,
									CL_GLOBAL struct TopLevelCell* topLevelCells,
									CL_GLOBAL CL_UINT2* leavesArray,
									CL_GLOBAL CL_UINT2* pairsRefArray)
{
	
	//Calculating the entry and exit t values for each axis
	float dt[3];
	int idx[3];
	float 	next[3];
	int 	step[3];
	int 	stop[3];

	{
		float tx_min, ty_min, tz_min;
		float tx_max, ty_max, tz_max; 

		float a = 1.0f / ray.direction.x;
		tx_min = ((a >= 0.0f ? gridData->box.bounds[0].x : gridData->box.bounds[1].x) - ray.origin.x) * a;
		tx_max = ((a >= 0.0f ? gridData->box.bounds[1].x : gridData->box.bounds[0].x) - ray.origin.x) * a;
	
		a = 1.0f / ray.direction.y;
		ty_min = ((a >= 0.0f ? gridData->box.bounds[0].y : gridData->box.bounds[1].y) - ray.origin.y) * a;
		ty_max = ((a >= 0.0f ? gridData->box.bounds[1].y : gridData->box.bounds[0].y) - ray.origin.y) * a;
	
		a = 1.0 / ray.direction.z;
		tz_min = ((a >= 0.0f ? gridData->box.bounds[0].z : gridData->box.bounds[1].z) - ray.origin.z) * a;
		tz_max = ((a >= 0.0f ? gridData->box.bounds[1].z : gridData->box.bounds[0].z) - ray.origin.z) * a;
	
		float t0 = min(tx_min,min(ty_min,tz_min));
		
		if (t0 > max(tx_max,max(ty_max,tz_max)))
			return NO_CONTACT;
	
		//Calculating the coordinates of the cell at which traversal begins
		CL_FLOAT3 p = isPointInside(gridData->box,ray.origin) ? ray.origin : ray.origin + (ray.direction * t0); 
		
		idx[iX] = clamp((p.x - gridData->box.bounds[0].x) * (float)gridData->resX / (gridData->box.bounds[1].x - gridData->box.bounds[0].x), 0.0f, (float)gridData->resX - 1);
		idx[iY] = clamp((p.y - gridData->box.bounds[0].y) * (float)gridData->resY / (gridData->box.bounds[1].y - gridData->box.bounds[0].y), 0.0f, (float)gridData->resY - 1);
		idx[iZ] = clamp((p.z - gridData->box.bounds[0].z) * (float)gridData->resZ / (gridData->box.bounds[1].z - gridData->box.bounds[0].z), 0.0f, (float)gridData->resZ - 1);
		
		// ray parameter increments per cell in the x, y, and z directions
	
		dt[iX] = (tx_max - tx_min) / gridData->resX;
		dt[iY] = (ty_max - ty_min) / gridData->resY;
		dt[iZ] = (tz_max - tz_min) / gridData->resZ;
	
		if (ray.direction.x > 0.0f) 
		{
			next[iX] = tx_min + (idx[iX] + 1) * dt[iX];
			step[iX] = +1;
			stop[iX] = gridData->resX;
		}
		else 
		{
			next[iX] = ray.direction.x == 0.0f ? FLT_MAX : tx_min + (gridData->resX - idx[iX]) * dt[iX];
			step[iX] = -1;
			stop[iX] = -1;
		}

		if (ray.direction.y > 0) 
		{
			next[iY] = ty_min + (idx[iY] + 1) * dt[iY];
			step[iY] = +1;
			stop[iY] = gridData->resY;
		}
		else 
		{
			next[iY] = ray.direction.y == 0.0f ? FLT_MAX : ty_min + (gridData->resY - idx[iY]) * dt[iY];
			step[iY] = -1;
			stop[iY] = -1;
		}
		
		if (ray.direction.z > 0) 
		{
			next[iZ] = tz_min + (idx[iZ] + 1) * dt[iZ];
			step[iZ] = +1;
			stop[iZ] = gridData->resZ;
		}
		else 
		{
			next[iZ] = ray.direction.z == 0.0f? FLT_MAX : tz_min + (gridData->resZ - idx[iZ]) * dt[iZ];
			step[iZ] = -1;
			stop[iZ] = -1;
		}
	}
	
	// traverse the grid
	while (true) 
	{	
		struct TopLevelCell cell = topLevelCells[getCellIndex(idx[iX],idx[iY],idx[iZ],
			gridData->resX,gridData->resY,gridData->resZ)];

		float minimal = min(next[iX],min(next[iY],next[iZ]));
		int axis = 0;
		while(minimal != next[axis])
			axis++;

		if (!(cell.resX == 0 || cell.resY == 0 || cell.resZ == 0))
		{
			//Process the cell
			struct AABB cellBox;
			cellBox.bounds[0].x = gridData->box.bounds[0].x + idx[iX] * gridData->stepX;
			cellBox.bounds[0].y = gridData->box.bounds[0].y + idx[iY] * gridData->stepY;
			cellBox.bounds[0].z = gridData->box.bounds[0].z + idx[iZ] * gridData->stepZ;
			cellBox.bounds[1].x = cellBox.bounds[0].x + gridData->stepX;
			cellBox.bounds[1].y = cellBox.bounds[0].y +	gridData->stepY;
			cellBox.bounds[1].z = cellBox.bounds[0].z +	gridData->stepZ;
			struct Contact result = processTopLevelCell(ray,scene,cell,cellBox,leavesArray,pairsRefArray);
			if (result.contactDist > 0.0f)
				return result;
		}

		next[axis] += dt[axis];
		idx[axis] += step[axis];
								
		if (idx[axis] == stop[axis])
			return NO_CONTACT;
	}
}

#endif //CL_RT_TWOLEVELGRID