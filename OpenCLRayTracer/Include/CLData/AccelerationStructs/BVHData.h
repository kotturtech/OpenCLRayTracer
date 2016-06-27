/**
 * @file BVHData.h
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
 * Contains data structures and constants for the Bounding Volume Hierarchy acceleration structure
 */

#ifndef CL_RT_BVH_DATA_H
#define CL_RT_BVH_DATA_H

#include <CLData/CLPortability.h>
#include <CLData/Primitives/AABB.h>

/********************************************************************************
* Some constants and macros for easy data access
*********************************************************************************/
#define LEAF_NODE 1
#define INTERNAL_NODE 0

#define PARENT_INDEX_IDX 0

#define TRIANGLE_INDEX_IDX 1
#define SUBMESH_INDEX_IDX 2
#define MODEL_INDEX_IDX 3

#define CHILD_A_IDX 1
#define CHILD_B_IDX 2

#define LEAF_NODE 1
#define INNER_NODE 0

#define triangleIndex(bvhNode) bvhNode.data[TRIANGLE_INDEX_IDX]
#define submeshIndex(bvhNode) bvhNode.data[SUBMESH_INDEX_IDX]
#define modelIndex(bvhNode)  bvhNode.data[MODEL_INDEX_IDX]


#define childA(bvhNode) bvhNode.data[CHILD_A_IDX]
#define childB(bvhNode) bvhNode.data[CHILD_B_IDX]


#define mortonSortingKey data[MORTON_CODE_IDX]

#define parent(bvhNode) bvhNode.data[PARENT_INDEX_IDX]
#define type(bvhNode) bvhNode.boundingBox.bounds[0].w

/*
* struct BVHNode - Represents a node in BVH tree
*/
struct BVHNode
{
	CL_UINT data[4]; 
	struct AABB boundingBox;
} ALIGNED(16);


#define CREATE_DEFAULT_INNER_NODE(varName) struct BVHNode varName; parent(varName) = childA(varName) = childB(varName) = UINT_MAX; type(varName) = INNER_NODE;

#endif //CL_RT_BVH_DATA_H
