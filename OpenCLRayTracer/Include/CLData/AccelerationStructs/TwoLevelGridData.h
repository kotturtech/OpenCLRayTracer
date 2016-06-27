/**
 * @file TwoLevelGridData.h
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
 * Contains data structures and constants for the Two Level Grid acceleration structure
 */


#ifndef CL_RT_TWO_LEVEL_GRID_DATA_H
#define CL_RT_TWO_LEVEL_GRID_DATA_H

#include <CLData/CLPortability.h>
#include <CLData/Primitives/AABB.h>

/*
* struct TopLevelCell - Represents a top level cell in Two Level Grid
*/
struct TopLevelCell
{
	CL_UINT resX;
	CL_UINT resY;
	CL_UINT resZ;
	CL_UINT firstLeafIdx;
} ALIGNED(16);

/*
* struct GridData - Contains general data about Two Level Grid
*/
struct GridData
{
	CL_UINT resX;
	CL_UINT resY;
	CL_UINT resZ;
	CL_FLOAT stepX;
	CL_FLOAT stepY;
	CL_FLOAT stepZ;
	CL_FLOAT leafDensity;
	CL_FLOAT padTo64;
	struct AABB box;
} ALIGNED(16);

/*
* Macros for axis indexes
*/
#define iX 0
#define iY 1
#define iZ 2



#endif //CL_RT_TWO_LEVEL_GRID_DATA_H
