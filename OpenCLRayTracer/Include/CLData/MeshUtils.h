/**
 * @file MeshUtils.h
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
 * Utility functions for triangle meshes
 */

/** \addtogroup g1 Ray Tracing Implementation Utilities 
 *  @{
 */

/** \addtogroup g13 Scene Buffer Utilities
 *  @{
 */

#ifndef CL_RT_MESH_UTILS_H
#define CL_RT_MESH_UTILS_H

#include <CLData/SceneBufferParser.h>
#include <CLData/Primitives/AABB.h>

/**
* Calculates bounding box of a triangle mesh
* @param modelBuffer - Buffer that contains the model
* @return Bounding box of input model
*/
inline struct AABB calculateAABB(CL_GLOBAL char* modelBuffer)
{
	CL_FLOAT maxX,maxY,maxZ;
	CL_FLOAT minX,minY,minZ;
	CL_FLOAT sumX,sumY,sumZ;
	maxX = maxY = maxZ = FLT_MIN;
	minX = minY = minZ = FLT_MAX;
	ModelHeader* model = MODEL_HEADER(modelBuffer);
	for (int i = 0; i < model->numberOfSubmeshes; i++)
	{
		char* meshData = getMeshAtIndex(i,modelBuffer);
		MeshHeader* meshHeader = MESH_HEADER(meshData);
		for(int v = 0; v < meshHeader->numberOfVertices; v++)
		{
			VERTEX_TYPE vertex = getVertexAt(v,meshData);
			maxX = max(maxX,vertex.x);
			maxY = max(maxY,vertex.y);
			maxZ = max(maxZ,vertex.z);
			minX = min(minX,vertex.x);
			minY = min(minY,vertex.y);
			minZ = min(minZ,vertex.z);
		}
	}

	AABB result;
	fillVector3(result.bounds[0],minX,minY,minZ);
	fillVector3(result.bounds[1],maxX,maxY,maxZ);
	return result;
}
 
#endif // CL_RT_MESH_UTILS_H

/** @}*/
/** @}*/