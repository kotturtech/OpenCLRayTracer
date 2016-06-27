/**
 * @file BVHKernels.cl
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
 * Kernels BVH construction and traversal.
 * This file contains the entry points for kernels, that call actual logic functions contained in the file BVH.h.
 * 
 * @implNote 
 * The implemented BVH variation is based on radix trees, 
 * adopted from this source:  http://devblogs.nvidia.com/parallelforall/wp-content/uploads/2012/11/karras2012hpg_paper.pdf
 * and related blog entry:    https://devblogs.nvidia.com/parallelforall/thinking-parallel-part-iii-tree-construction-gpu/
 * Handling duplicate Morton codes implemented based on this source: https://github.com/Ooken/NekoEngine
 * 
 */
#include "CLData\SceneBufferParser.h"
#include "CLData\Primitives\AABB.h"
#include "CLData\AccelerationStructs\BVH.h"
#include "CLData\AccelerationStructs\BVHData.h"

#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable

/***************************************************
* 1. Calculate Morton code for each primitive
****************************************************/
__kernel void calculateMortonCodes(__global struct BVHNode* leavesBuffer,__global uint2* mortonBuffer, __global const char* scene)
{
	if(get_global_id(0) < SCENE_HEADER(scene)->totalNumberOfTriangles) 
		calculateMorton(leavesBuffer,mortonBuffer,get_global_id(0),scene);
}

/***************************************************
* 2. Builds the actual hierarchy
****************************************************/
__kernel void buildRadixTree(__global struct BVHNode* nodeBuffer,__global uint2* mortonBuffer, uint leafCount)
{
	uint idx = get_global_id(0);
	if (idx < leafCount-2)
		constructNode(nodeBuffer,mortonBuffer,leafCount, get_global_id(0));
} 

/***************************************************************
* 3. Hierarchy post-processing - Calculation of bounding boxes
***************************************************************/
__kernel void computeBoundingBoxes(__global struct BVHNode* nodeBuffer,__global volatile uint* counters, uint leafCount)
{
	CL_UINT current = min((uint)get_global_id(0),(uint)(leafCount-1));
	current = parent(nodeBuffer[current]);
	while(current != UINT_MAX)
	{
		atomic_inc(counters+(current-leafCount));
		if (*(counters+(current-leafCount)) > 1)
		{
			mergeBoundingBox(nodeBuffer,current);
			current = parent(nodeBuffer[current]);
		}
		else break;
	}
}

/***************************************************************
* 4. Generating the contacts for primary rays
***************************************************************/
__kernel void generateContacts(__constant struct Camera* camera,
							      __global struct BVHNode* bvh, 
							      uint rootIdx,
							      const __global char* scene,
							      __global struct Contact* output
								 )
{
		if (get_global_id(0) < (camera->resX * camera->resY))
		{
			struct Ray r = generateRay(camera,get_global_id(0));
			struct Contact c = bvh_generate_contact(r,bvh,rootIdx,scene);
			c.pixelIndex = get_global_id(0);
			output[c.pixelIndex] = c;
		}
}

/***************************************************************
* 5. Generating the contacts for general rays
***************************************************************/
__kernel void generateContacts2(__global struct Ray* rays,
							   uint rayCount,
							   __global struct BVHNode* bvh, 
							   uint rootIdx,
							   const __global char* scene,
							   __global struct Contact* output)
{
		uint idx = get_global_id(0);
		if (idx < rayCount)
		{
			struct Contact c = bvh_generate_contact(rays[idx],bvh,rootIdx,scene);
			c.pixelIndex = idx;
			output[c.pixelIndex] = c;
		}
}

