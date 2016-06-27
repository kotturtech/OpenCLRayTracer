/**
 * @file BVH.h
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
 * Utility functions for BVH construction and traversal.
 * 
 * @implNote 
 * The implemented BVH variation is based on radix trees, 
 * adopted from this source:  http://devblogs.nvidia.com/parallelforall/wp-content/uploads/2012/11/karras2012hpg_paper.pdf
 * and related blog entry:    https://devblogs.nvidia.com/parallelforall/thinking-parallel-part-iii-tree-construction-gpu/
 * Handling duplicate Morton codes implemented based on this source: https://github.com/Ooken/NekoEngine
 * 
 * 
 */

#ifndef CL_RT_BVH
#define CL_RT_BVH

#include <CLData/RTKernelUtils.h>
#include <CLData/CLStructs.h>
#include <CLData/AccelerationStructs/BVHData.h>
#include <CLData/Primitives/Triangle.h>

/** Expands a 10-bit integer into 30 bits by inserting 2 zeros after each bit.
*  @param v Integer to expand
*  @return The expanded integer
*/
inline CL_UINT expandBits(CL_UINT v)
{
    v = (v * 0x00010001u) & 0xFF0000FFu;
    v = (v * 0x00000101u) & 0x0F00F00Fu;
    v = (v * 0x00000011u) & 0xC30C30C3u;
    v = (v * 0x00000005u) & 0x49249249u;
    return v;
}

/** Calculates a 30-bit Morton code for the given 3D point located within the unit cube [0,1].
*  @param x X coordinate of a point
*  @param y Y coordinate of a point
*  @param x X coordinate of a point
*  @return The calculated Morton code of a point
*/
inline CL_UINT morton3D(CL_FLOAT x, CL_FLOAT y, CL_FLOAT z)
{
    x = min(max(x * 1024.0f, 0.0f), 1023.0f);
    y = min(max(y * 1024.0f, 0.0f), 1023.0f);
    z = min(max(z * 1024.0f, 0.0f), 1023.0f);
    CL_UINT xx = expandBits((CL_UINT)x);
    CL_UINT yy = expandBits((CL_UINT)y);
    CL_UINT zz = expandBits((CL_UINT)z);
    return xx * 4 + yy * 2 + zz;
}

/** Calculates Morton code for a primitive in the scene, and creates BVH tree node for it
*  @param leavesBuffer Preallocated buffer that will contain the BVH
*  @param mortonCodesToLeaves Key-Value pairs array, that will contain Morton code as key, and primitive index as value
*  @param leafIndex Index of the primitive to process
*  @param scene Buffer that contains the scene
*  @param sceneBB Scene bounding box
*  @return 
*/
inline void calculateMorton(CL_GLOBAL struct BVHNode* leavesBuffer, CL_GLOBAL CL_UINT2* mortonCodesToLeaves, 
							CL_UINT leafIndex, CL_GLOBAL const char* scene)
{
	
	CL_GLOBAL char* buffer;
	CL_UINT3 triangleRef = getTriangleRefByIndex(scene,leafIndex);
	
	buffer = getModelAtIndex(triangleRef.x,scene);
	buffer = getMeshAtIndex(triangleRef.y,buffer);
		
	//Obtaining the triangle and it's centroid
	CL_UINT baseIndex = triangleRef.z * 3;
	VERTEX_TYPE vertex1 = getVertexAt(getIndexAt(baseIndex, buffer),buffer);
	VERTEX_TYPE vertex2 = getVertexAt(getIndexAt(baseIndex + 1,buffer),buffer);
	VERTEX_TYPE vertex3 = getVertexAt(getIndexAt(baseIndex + 2,buffer),buffer);
	struct BVHNode result;
	result.boundingBox = calculateTriangleAABB(vertex1,vertex2,vertex3);
#ifdef _WIN32
	vertex1.x=(vertex1.x + vertex2.x + vertex3.x)/3.0f;
	vertex1.y=(vertex1.y + vertex2.y + vertex3.y)/3.0f;
	vertex1.z=(vertex1.z + vertex2.z + vertex3.z)/3.0f;
#else
	vertex1+= vertex2;
	vertex1+= vertex3;
	vertex1 = vertex1 / 3.0f;
#endif
		
	//Normalize the centroid to scene AABB
	struct AABB sceneBB = SCENE_HEADER(scene)->modelsBoundingBox;
	vertex1 = (VERTEX_TYPE)combineToVector(normalizeScale(sceneBB.bounds[0].x,sceneBB.bounds[1].x,vertex1.x),
										   normalizeScale(sceneBB.bounds[0].y,sceneBB.bounds[1].y,vertex1.y),
										   normalizeScale(sceneBB.bounds[0].z,sceneBB.bounds[1].z,vertex1.z));
	

	//Calculate Morton Code and leaf
	type(result) = LEAF_NODE;
	triangleIndex(result) = triangleRef.z;
	submeshIndex(result) = triangleRef.y;
	modelIndex(result) = triangleRef.x;

	CL_UINT2 mortonToLeaf;
	mortonToLeaf.x =  morton3D(vertex1.x, vertex1.y, vertex1.z);
	mortonToLeaf.y = leafIndex;
	
	//Store
	leavesBuffer[leafIndex] = result;
	mortonCodesToLeaves[leafIndex] = mortonToLeaf;
}

/** Calculates split value of range of Morton codes
*  @param list Sorted Key-value pairs containing Morton codes as keys
*  @param first Lower bound of range for splitting
*  @param last High bound of range for splitting
*  @return Index of split value in given range
*/
inline int findSplit(CL_GLOBAL CL_UINT2* list, CL_INT first, CL_INT last)
{
  
    CL_UINT firstCode = list[first].x;
    CL_UINT lastCode = list[last].x;
    
    //When morton codes are identical we want to return the first id
    //because if we split it in the middle, both will try to orientate
    //towards the same direction
    
    //to split it in the middle would be the best but since it would be harder
    //for the range detector we do it this way (both ways less instructions needed:
    //here: (first + last) >> 1 to "first" and
    //in determine range we can reduce it to seek forward instead of determining the
    //sub direction and stuff...
    if (firstCode == lastCode)
		return first;
    
    // Calculate the number of highest bits that are the same
    // for all objects, using the count-leading-zeros intrinsic.
    int commonPrefix = CLZ(firstCode ^ lastCode);

    // Use binary search to find where the next bit differs.
    // Specifically, we are looking for the highest object that
    // shares more than commonPrefix bits with the first one.

    int split = first; // initial guess
    int step = last - first;
    do
    {
        step = (step + 1) >> 1; // exponential decrease
        int newSplit = split + step; // proposed new position

        if (newSplit < last)
        {
            unsigned int splitCode = list[newSplit].x;
            int splitPrefix = CLZ(firstCode ^ splitCode);
            if (splitPrefix > commonPrefix)
                split = newSplit; // accept proposal
        }
    }
    while (step > 1);

    return split;
}

/** Calculates range of Morton codes for internal BVH node for specified index
*  @param list Sorted Key-value pairs containing Morton codes as keys
*  @param index Index of internal node to process
*  @param size Number of items in Pairs array
*  @return Range of Morton codes in specified list for internal node at specified index.
*/
inline CL_UINT2 determineRange(CL_GLOBAL CL_UINT2* list, CL_INT index,CL_UINT size)
{
  //so we don't have to call it every time
  int lso =size-1;
  //tadaah, it's the root node
  if(index == 0)
  {
	   CL_UINT2 temp;
	   temp.x = 0;
	   temp.y = lso;
	   return temp;
  }

  //direction to walk to, 1 to the right, -1 to the left
  int dir;
  //morton code diff on the outer known side of our range ... diff mc3 diff mc4 ->DIFF<- [mc5 diff mc6 diff ... ] diff .. 
  int d_min;
  int initialindex = index;
  
  CL_UINT minone = list[index-1].x;
  CL_UINT precis = list[index].x;
  CL_UINT pluone = list[index+1].x;
  if((minone == precis && pluone == precis))
  {
    //set the mode to go towards the right, when the left and the right
    //object are being the same as this one, so groups of equal
    //code will be processed from the left to the right
    //and in node order from the top to the bottom, with each node X (ret.x = index)
    //containing Leaf object X and nodes from X+1 (the split func will make this split there)
    //till the end of the groups
    //(if any bit differs... DEP=32) it will stop the search
    while(index > 0 && index < lso)
    {
       //move one step into our direction
       index += 1;
       if(index >= lso || list[index].x != list[index+1].x)
       //we hit the left end of our list, or morton codes differ
			break;
    }
    //return the end of equal grouped codes
	CL_UINT2 res; res.x = initialindex; res.y = index; 
	return res;
	
  }
  else
  {
    //Our codes differ, so we seek for the ranges end in the binary search fashion:
    CL_UINT2 lr;
	lr.x = CLZ(precis ^ minone);
	lr.y = CLZ(precis ^ pluone);
    //now check wich one is higher (codes put side by side and wrote from up to down)
    if(lr.x > lr.y)
    {
		//to the left, set the search-depth to the right depth
		dir = -1;
		d_min = lr.y;
    }
	else
	{
		//to the right, set the search-depth to the left depth
		dir = 1;
		d_min = lr.x;
    }
   }
    //Now look for an range to search in (power of two)
    int l_max = 2;
    //so we don't have to calc it 3x
    int testindex = index + l_max * dir;
    while((testindex<=lso&&testindex>=0)?(CLZ(precis ^ list[testindex].x)>d_min):(false))
    {
		l_max = l_max << 1;
		testindex = index + l_max * dir;
	}
	
	int l = 0;
	//go from l_max/2 ... l_max/4 ... l_max/8 .......... 1 all the way down
	for(int div = 2 ; l_max / div >= 1 ; div = div << 1)
	{
		//calculate the ofset state
		int t = l_max/div;
		//calculate where to test next
		int newTest = index + (l + t)*dir;
		//test if in code range
		if (newTest <= lso && newTest >= 0)
		{
			int splitPrefix = CLZ(precis ^ list[newTest].x);
			//and if the code is higher then our minimum, update the position
			if (splitPrefix > d_min)
				l = l+t;
		}
	}
	return (CL_UINT2)combineToVector(min(index,index + l*dir),max(index,index + l*dir));
}

/** Constructs BVH internal node at specified index and updates the node in hierarchy array
*  @param nodes Array that contains the hierarchy
*  @param mcToLeaves Sorted Key-value pairs containing Morton codes as keys and leaf indices as values
*  @param numLeaves Number of items in Pairs array (Which is also the number of leaf cells)
*  @param idx Index of internal node to process
*  @return 
*/
inline void constructNode(CL_GLOBAL struct BVHNode* nodes, CL_GLOBAL CL_UINT2* mcToLeaves, CL_UINT numLeaves, int idx)
{
	//Range of this node
	CL_UINT2 range = determineRange(mcToLeaves,idx,numLeaves);
	//Split of the range 
	int split = findSplit(mcToLeaves, range.x, range.y);
	CL_UINT internalNodeIndex = idx+numLeaves;

    CL_UINT first = min(range.x,range.y); 
	CL_UINT last = max(range.x,range.y);
	bool aIsLeaf = first == split;
	bool bIsLeaf = (last == (split+1));
	//split == first = 1? then leaf, otherwise inner node
	CL_UINT child_A = mcToLeaves[split].y * aIsLeaf + (split + numLeaves) * !aIsLeaf;
	//split+1 == last = 1? then leaf, otherwise inner node
	CL_UINT child_B = mcToLeaves[split+1].y * bIsLeaf + (split+1 + numLeaves) * !bIsLeaf;
	
	//Some initializations for tracersal and BB calculation that will come afterwards
	fillVector3(nodes[internalNodeIndex].boundingBox.bounds[0],FLT_MAX,FLT_MAX,FLT_MAX);
	fillVector3(nodes[internalNodeIndex].boundingBox.bounds[1],FLT_MIN,FLT_MIN,FLT_MIN);
	type(nodes[internalNodeIndex]) = INNER_NODE;
	if (internalNodeIndex == numLeaves)
		parent(nodes[internalNodeIndex]) = UINT_MAX;

	//Parent and Children Relations
	childA(nodes[internalNodeIndex]) = child_A;
	childB(nodes[internalNodeIndex]) = child_B;
	parent(nodes[child_A]) = internalNodeIndex;
	parent(nodes[child_B]) = internalNodeIndex;
  
}

/** Merges bounding box of a parent, and its two children
*  @param nodes Array that contains the hierarchy
*  @param idx Index of internal node to process
*  @return 
*/
inline void mergeBoundingBox(CL_GLOBAL struct BVHNode* nodes,CL_UINT currentIdx)
{
	CL_UINT child_A = childA(nodes[currentIdx]);
	CL_UINT child_B = childB(nodes[currentIdx]);

	nodes[currentIdx].boundingBox = merge3(nodes[child_A].boundingBox,nodes[child_B].boundingBox,nodes[currentIdx].boundingBox);
}

/** Performs intersection query for a ray
*  @param ray Ray to query
*  @param bvh Array that contains the BV hierarchy
*  @param rootIdx Index of root of the hierarchy
*  @param scene Buffer that contains the scene
*  @return Contact data that contains ray parameter t at which closest intersection occurs, and intersection normal.
*          In case no intersection found, the t parameter will be 0.
*/
inline struct Contact bvh_generate_contact(struct Ray ray,
								    CL_GLOBAL struct BVHNode* bvh, 
									CL_UINT rootIdx,
									const CL_GLOBAL char* scene)
{
	CL_UINT stack[32];
	CL_UINT stackPointer = 0;
	CL_UINT currentIdx = rootIdx;
	stack[stackPointer++]=UINT_MAX; //Push initial value
	CL_FLOAT4 resContactData;
	resContactData.w = FLT_MAX;
	CL_UINT resMaterialIdx = 0;
	do
    {
		struct BVHNode node = bvh[currentIdx];
		if (type(node) == INNER_NODE)
		{
			CL_UINT child_A_idx = childA(node);
			CL_UINT child_B_idx = childB(node);
			struct AABB child_A_box = bvh[child_A_idx].boundingBox;
			struct AABB child_B_box = bvh[child_B_idx].boundingBox;
			
			float tA = AABBIntersect(child_A_box,ray.origin,ray.direction);
			float tB = AABBIntersect(child_B_box,ray.origin,ray.direction);
			bool aValid = tA > 0 || isPointInside(child_A_box,ray.origin);
			bool bValid = tB > 0 || isPointInside(child_B_box,ray.origin);
			
			if (aValid && bValid)
			{
				currentIdx = child_A_idx;
				stack[stackPointer++] = child_B_idx; // push
			}
			else if (aValid)
				currentIdx = child_A_idx;
			else if (bValid)
				currentIdx = child_B_idx;
			else
				currentIdx = stack[--stackPointer];
		}
		else
		{
			
			CL_GLOBAL char* mesh = getMeshAtIndex(submeshIndex(node),getModelAtIndex(modelIndex(node),scene));
			CL_UINT baseIndex = triangleIndex(node) * 3;
			CL_FLOAT4 contactData = triangleIntersect(
									getVertexAt(getIndexAt(baseIndex,mesh),mesh),
									getVertexAt(getIndexAt(baseIndex + 1,mesh),mesh),
									getVertexAt(getIndexAt(baseIndex + 2,mesh),mesh),ray.origin,ray.direction);
			if (contactData.w > 0 && contactData.w < resContactData.w)
			{
				resContactData = contactData;
				resMaterialIdx = MESH_HEADER(mesh)->materialIndex;	
			} 
			currentIdx = stack[--stackPointer];
		}
	}
	while (currentIdx != UINT_MAX);

	struct Contact result;
	result.normalAndintersectionDistance = resContactData;
	if (result.contactDist == FLT_MAX)
		result.contactDist = 0;
	result.materialIndex = resMaterialIdx;
	return result;
}

#endif

