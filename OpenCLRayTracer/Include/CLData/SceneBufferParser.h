/**
 * @file SceneBufferParser.h
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
 * Functions and structure that handle storing and retrieving scene setup.
 * 
 * The scene is stored in a contiguous buffer, that contains at least the scene header that
 * contains information about how many objects are there in the scene, and of which type.
 * The scene may contain Spheres, Lights, 3D Models, and their materials.
 * Each 3D model contains submeshes, which in turn consist of triangles, stored as vertices and indices.
 *
 * @implNote These functions were implemented to conform with Wavefront OBJ 3D models.
 *           Some retrieval functions are frequently used in GPU algorithms, and optimizing them
 *           could improve performance there and there.
 */

/** \addtogroup g1 Ray Tracing Implementation Utilities 
 *  @{
 */

/** \addtogroup g13 Scene Buffer Utilities
 *  @{
 */

#ifndef CL_RT_SCENEBUFFERPARSER
#define CL_RT_SCENEBUFFERPARSER

//Includes of Primitives
#include "CLData\CLPortability.h"
#include "CLData\Primitives\Sphere.h"
#include "CLData\Primitives\Light.h"
#include "CLData\Primitives\Material.h"
#include "CLData\Primitives\AABB.h"

/**
* struct SceneHeader - Contains basic information about the scene
*/
struct SceneHeader
{
	CL_ULONG totalDataSize; //Data size must include the size of the header!
	CL_ULONG numberOfPrimitives;
	CL_ULONG numberOfLights;
	CL_ULONG numberOfSpheres;
	CL_ULONG numberOfMaterials;
	CL_ULONG modelBufferSize;
	CL_ULONG numberOfModels;
	CL_ULONG totalNumberOfTriangles;
	struct AABB modelsBoundingBox;
} ALIGNED(16);

/***Utility Macros***/
#define SCENE_HEADER_SIZE (sizeof(struct SceneHeader))
#define SCENE_HEADER(buf) ((CL_GLOBAL struct SceneHeader*)(buf))
#define LIGHTS_PTR(buf) (buf + SCENE_HEADER_SIZE)
#define SPHERES_PTR(buf) ((LIGHTS_PTR(buf)) + SCENE_HEADER(buf)->numberOfLights * sizeof(struct Light))
#define MATERIALS_PTR(buf) ((SPHERES_PTR(buf)) + SCENE_HEADER(buf)->numberOfSpheres * sizeof(struct Sphere))
#define MODEL_BUFFER_PTR(buf) ((MATERIALS_PTR(buf)) + SCENE_HEADER(buf)->numberOfMaterials * sizeof(struct Material))

/***Retrieving Functions***/

/**
* Get light from scene buffer, at specified index
* @param buf - Buffer that contains the scene
* @param index - Index of requested light
* @return Light at specified index
*/
inline CL_GLOBAL struct Light* getLightAtIndex(const CL_GLOBAL char* buf,int index)
{
	CL_GLOBAL struct Light* base = (CL_GLOBAL struct Light*)(LIGHTS_PTR(buf));
	return base + index;
}

/**
* Get sphere from scene buffer, at specified index
* @param buf - Buffer that contains the scene
* @param index - Index of requested sphere
* @return Sphere at specified index
*/
inline CL_GLOBAL struct Sphere* getSphereAtIndex(const CL_GLOBAL char* buf,int index)
{
	CL_GLOBAL struct Sphere* base = (CL_GLOBAL struct Sphere*)(SPHERES_PTR(buf));
	return base + index;
}

/**
* Get material from scene buffer, at specified index
* @param buf - Buffer that contains the scene
* @param index - Index of requested material
* @return Material at specified index
*/
inline CL_GLOBAL struct Material* getMaterialAtIndex(const CL_GLOBAL char* buf,int index)
{
	CL_GLOBAL struct Material* base = (CL_GLOBAL struct Material*)(MATERIALS_PTR(buf));
	return base + index;
}

/*******Model Manipulations*******/

/**
* struct ModelHeader - Contains basic information about 3D Model
*
* ---Model Object---
* 8 bytes(long)-Model data size(including header)
* 8 bytes(long)-number of sumbeshes
* ----Mesh Object----
* 8 bytes(long)-buffer size(incuding header)
* 4 bytes(uint)-num of vertices
* 4 bytes(uint)-num of indices
* Dynamic data - * Vertices(float4)
*				 * Indices-(ushort)
*/
struct ModelHeader
{
	CL_ULONG dataSize; //Data size must include the size of the header!
	CL_ULONG numberOfSubmeshes;
	CL_ULONG numberOfTriangles;
	CL_ULONG pad; //Here to verify that GPU and CPU data size match
	struct AABB boundingBox;
} ALIGNED(16);

/**
* struct MeshHeader - Contains basic information about 3D Mesh
*/
struct MeshHeader
{
	CL_ULONG dataSize; //Data size must include the size of the header!
	CL_ULONG numberOfTriangles;
	CL_ULONG numberOfVertices;
	CL_ULONG numberOfIndices;
	CL_ULONG materialIndex;
	CL_ULONG pad; //It is here to verify that GPU and CPU data size match
} ALIGNED(16);


#define MODEL_HEADER_SIZE sizeof(struct ModelHeader)
#define MESH_HEADER_SIZE  sizeof(struct MeshHeader)
#define VERTEX_TYPE CL_FLOAT3
#define VERTEX_SIZE sizeof(VERTEX_TYPE)
#define INDEX_TYPE CL_USHORT
#define INDEX_SIZE sizeof(INDEX_TYPE)

#define MODEL_HEADER(buf) ((CL_GLOBAL struct ModelHeader*)(buf))
#define MESH_HEADER(buf) ((CL_GLOBAL struct MeshHeader*)(buf))

#define VERTEX_BASE(meshBuf) ((CL_GLOBAL VERTEX_TYPE*)(meshBuf + MESH_HEADER_SIZE))
#define INDEX_BASE(meshBuf) ((CL_GLOBAL INDEX_TYPE*)(&(VERTEX_BASE(meshBuf)[MESH_HEADER(meshBuf)->numberOfVertices])))

/**
* Get model from scene buffer, at specified index
* @param index - Index of requested model
* @param sceneBuffer - Buffer that contains the scene
* @return Buffer that contains Model at specified index
*/
inline CL_GLOBAL char* getModelAtIndex(CL_UINT index, const CL_GLOBAL char* sceneBuffer)
{
	CL_GLOBAL char* modelDataPtr = ((CL_GLOBAL char*)(MODEL_BUFFER_PTR(sceneBuffer)));
	for (int i = 0; i < index; i++)
		modelDataPtr+=MODEL_HEADER(modelDataPtr)->dataSize;
	return modelDataPtr;
}

/**
* Get mesh from model buffer, at specified index
* @param index - Index of requested mesh
* @param sceneBuffer - Buffer that contains the model
* @return Buffer that contains Mesh at specified index
*/
inline CL_GLOBAL char* getMeshAtIndex(CL_UINT index, const CL_GLOBAL char* modelBuffer)
{
	CL_GLOBAL char* meshDataPtr = (CL_GLOBAL char*)(modelBuffer + MODEL_HEADER_SIZE);
	 for(int i = 0; i < index; i++)
		 meshDataPtr+=MESH_HEADER(meshDataPtr)->dataSize;
	 return meshDataPtr;
}

/**
* Get vertex from mesh buffer, at specified index
* @param index - Index of requested index
* @param meshBuffer - Buffer that contains the mesh
* @return Vertex at specified index
*/
inline VERTEX_TYPE getVertexAt(CL_UINT index, const CL_GLOBAL char* meshBuffer)
{
	return VERTEX_BASE(meshBuffer)[index];
}

/**
* Get index from mesh buffer, at specified index
* @param index - Index of requested index
* @param meshBuffer - Buffer that contains the mesh
* @return index at specified index
*/
inline INDEX_TYPE getIndexAt(CL_UINT index, const CL_GLOBAL char* meshBuffer)
{
	return INDEX_BASE(meshBuffer)[index];
}

/***Setters***/

/**
* Set vertex in mesh buffer, at specified index
* @param value - Vertex value
* @param index - Index at which the vertex should be set
* @param meshBuffer - Buffer that contains the mesh
* @return 
*/
inline void setVertexAt(VERTEX_TYPE value, CL_UINT index, const CL_GLOBAL char* meshBuffer)
{
	VERTEX_BASE(meshBuffer)[index] = value;
}

/**
* Set index in mesh buffer, at specified index
* @param value - Index value
* @param index - Index at which the index should be set
* @param meshBuffer - Buffer that contains the mesh
* @return 
*/
inline void setIndexAt(INDEX_TYPE value, CL_UINT index, const CL_GLOBAL char* meshBuffer)
{
	INDEX_BASE(meshBuffer)[index] = value;
}

/***Utilities***/

/**
* Gets triangle reference by index: Model,Submesh, and index of triangle within submesh
* @param scene - Buffer that contains the scene
* @param triangleIndex - Global index of triangle
* @return  Vector where: x=Model index, y=Submesh index, z=Index of triangle within submesh
*/
inline CL_UINT3 getTriangleRefByIndex(CL_GLOBAL const char* scene,CL_UINT triangleIndex)
{
	int currentModelIndex = -1;
	int currentSubmeshIndex = -1;
	int currentTriangleIndex = 0;
	int accumulatedTriangles = 0;
	
	//Calculate Model Index
	bool flag = false;
	CL_GLOBAL char* currentModel;
	while (!flag)
	{
		currentModelIndex++;
		currentModel = getModelAtIndex(currentModelIndex,scene);
		accumulatedTriangles+=MODEL_HEADER(currentModel)->numberOfTriangles;
		flag = accumulatedTriangles > triangleIndex;
	}
	accumulatedTriangles-=MODEL_HEADER(currentModel)->numberOfTriangles;

	//Calculate Submesh Index
	flag = false;
	CL_GLOBAL char* currentMesh;
	while (!flag)
	{
		currentSubmeshIndex++;
		currentMesh = getMeshAtIndex(currentSubmeshIndex,currentModel);
		accumulatedTriangles+=MESH_HEADER(getMeshAtIndex(currentSubmeshIndex,currentModel))->numberOfTriangles;
		flag = accumulatedTriangles > triangleIndex;
	}

	accumulatedTriangles-=MESH_HEADER(currentMesh)->numberOfTriangles;

	//Calculate triangle within the submesh
	currentTriangleIndex = max((CL_UINT)0,(CL_UINT)(triangleIndex - accumulatedTriangles));

	CL_UINT3 result;
	result.x = currentModelIndex;
	result.y = currentSubmeshIndex;
	result.z = currentTriangleIndex;
	return result;
}

#endif//CL_RT_SCENEBUFFERPARSER

/** @}*/
/** @}*/