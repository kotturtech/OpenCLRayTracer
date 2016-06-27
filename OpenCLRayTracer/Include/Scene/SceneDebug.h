/**
 * @file Scene.h
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
 * Contains some testing helper functions for checking/debugging loading and formatting of scene buffer
 * 
 */

/** \addtogroup g1 Ray Tracing Implementation Utilities 
 *  @{
 */


#ifndef CL_RT_SCENE_BUFFER_DEBUG
#define CL_RT_SCENE_BUFFER_DEBUG

#include <iostream>
#include <vector>
#include <CLData\SceneBufferParser.h>
#include <CLData\Primitives\Triangle.h>

namespace CLRayTracer
{
	namespace SceneDebug
	{

		/**
		* Return random float number
		* @param mn minimum for return value
		* @param mx maximum for return value
		* @return Random float point value in range [mn,mx]
		*/
		float randFloat(float mn, float mx)
		{
			float r = std::rand() / (float) RAND_MAX;
			return mn + (mx-mn)*r;
		}

		/**
		* Prints scene header
		* @param sceneBuffer Buffer that contains the scene
		* @return 
		*/
		inline void printSceneHeader(const char* sceneBuffer)
		{
			SceneHeader* header= SCENE_HEADER(sceneBuffer);
			std::cout << "Scene: "<<std::endl <<
				"Total Data Size: " << header->totalDataSize << std::endl <<
				"Lights: " <<  header->numberOfLights << std::endl <<
				"Spheres: " << header->numberOfSpheres << std::endl <<
				"Models: " << header->numberOfModels << std::endl <<
				"Materials: " << header->numberOfMaterials << std::endl <<
				"Total Primitives: " << header->numberOfPrimitives << std::endl << 
			    "Bounding Box: " << header->modelsBoundingBox.bounds[0].x << ' ' << header->modelsBoundingBox.bounds[1].x << "\n" 
							     << header->modelsBoundingBox.bounds[0].y << ' ' << header->modelsBoundingBox.bounds[1].y << "\n" 
								 << header->modelsBoundingBox.bounds[0].z << ' ' << header->modelsBoundingBox.bounds[1].z << "\n"; 
		}

		/**
		* Prints model data
		* @param sceneBuffer Buffer that contains the scene
		* @return 
		*/
		inline void printModelData(const char* sceneBuffer)
		{
			SceneHeader* scnHeader = SCENE_HEADER(sceneBuffer);
			CL_ULONG numOfModels = scnHeader->numberOfModels;
			std::cout << "Models: " << std::endl << "Models:" << numOfModels << std::endl;
			for (int i = 0; i < numOfModels; i++)
			{
				char* model = getModelAtIndex(0,sceneBuffer);
				CL_ULONG numOfMeshes = MODEL_HEADER(model)->numberOfSubmeshes;
				std::cout << "Model " << i << ": Number Of Meshes: " << numOfMeshes << " Triangles: " << MODEL_HEADER(model)->numberOfTriangles <<std::endl;
				for (int m = 0; m < numOfMeshes; m++)
				{
					char* mesh = getMeshAtIndex(m,model);
					CL_UINT numOfVertices = MESH_HEADER(mesh)->numberOfVertices;
					CL_UINT numOfIndices = MESH_HEADER(mesh)->numberOfIndices;
					std::cout << "Mesh " << m << ": Vertices: " << numOfVertices << " Indices: " << numOfIndices << " Triangles: "<< MESH_HEADER(mesh)->numberOfTriangles <<" Material Index: " << MESH_HEADER(mesh)->materialIndex << std::endl;
					for (int v = 0; v < numOfVertices; v++)
					{
						VERTEX_TYPE vtx = getVertexAt(v,mesh);
						std::cout << "--Vertex " << v << ": "  << vtx.x << ',' << vtx.y << ',' <<vtx.z << std::endl;
					}
					for (int idx = 0; idx < numOfIndices; idx++)
					{
						INDEX_TYPE index = getIndexAt(idx,mesh);
						std::cout << "--Index " << idx << ": " << index << std::endl;
					}
				}
			}
		}

		/**
		* Fills vector with triangles contained in the scene
		* @param tris STL vector of triangles, to fill
		* @param sceneBuffer Buffer that contains the scene
		* @return 
		*/
		inline void fillTriangleVector(std::vector<Triangle>& tris,const char* sceneBuffer)
		{
			CL_ULONG numOfModels = SCENE_HEADER(sceneBuffer)->numberOfModels;
			for (int i = 0; i < numOfModels; i++)
			{
				char* model = getModelAtIndex(0,sceneBuffer);
				CL_ULONG numOfMeshes = MODEL_HEADER(model)->numberOfSubmeshes;
				for (int m = 0; m < numOfMeshes; m++)
				{
					char* mesh = getMeshAtIndex(m,model);
					CL_UINT materialIndex = MESH_HEADER(mesh)->materialIndex;
					CL_UINT numOfIndices = MESH_HEADER(mesh)->numberOfIndices;
					int indexInTriangle = 0;
					Triangle currentTriangle;
					for (int i = 0; i < numOfIndices; i++)
					{
						INDEX_TYPE index = getIndexAt(i,mesh);
						currentTriangle.vertexes[indexInTriangle] = getVertexAt(index,mesh);
						if (indexInTriangle == 2)
						{
							currentTriangle.materialIndex = materialIndex;
							tris.push_back(currentTriangle);
							indexInTriangle = 0;
						}
						else indexInTriangle++;
					}
				}
			}
		}
	}

}

#endif //CL_RT_SCENE_BUFFER_DEBUG

/** @}*/