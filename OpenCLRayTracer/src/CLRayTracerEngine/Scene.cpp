/**
 * @file Scene.cpp
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
 * Implementation of class Scene, which is responsible for loading and storing the 3D scene.
 *
 */

#include <fstream>
#include <map>
#include <vector>
#include <iostream>
#include <Windows.h>
#include <boost\algorithm\string.hpp>
#include <boost\smart_ptr.hpp>
#include <Scene\Scene.h>
#include <CLData\SceneBufferParser.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <3rdParty\tiny_obj_loader.h>

using namespace std;
using namespace CLRayTracer;
using namespace CLRayTracer::Common;

/*************************************************************************
* Utility functions and structs
**************************************************************************/

struct ModelData
{
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	unsigned long calculatedDataSize;
};

Result loadMesh(string fileName, ModelData& data, Errata& err)
{
	//REMEMBER - ALL COUNTER-CLOCKWISE!!!!
	string errorMsg;
	size_t slashIdx = fileName.find_last_of('\\');
	size_t rSlashIdx = fileName.find_last_of('/');
	if (!(slashIdx == std::string::npos || rSlashIdx == std::string::npos))
		slashIdx = max(slashIdx,rSlashIdx);
	else
		slashIdx = min(slashIdx,rSlashIdx);

	std::string mtlBase = fileName;
	slashIdx++;
	mtlBase.erase((mtlBase.begin() + slashIdx),mtlBase.end());
	if (!tinyobj::LoadObj(data.shapes,data.materials,errorMsg,fileName.c_str(),mtlBase.c_str()))
	{
		FILL_ERRATA(err,errorMsg);
		return Error;
	}
	return Success;
}

inline unsigned long calculateMeshDataSize(const tinyobj::shape_t& meshShape)
{
	unsigned long bytes = 0;
	bytes+=  meshShape.mesh.indices.size() * sizeof(INDEX_TYPE);
	bytes+= (meshShape.mesh.positions.size()) / 3 * sizeof(VERTEX_TYPE);
	return bytes + MESH_HEADER_SIZE;
}

inline unsigned long calculateModelDataSize(const ModelData& modelData)
{
	unsigned long bytes = 0;
	const int size = modelData.shapes.size();
	for(int i = 0; i < size; i++)
		bytes+=calculateMeshDataSize(modelData.shapes[i]);
	return bytes + MODEL_HEADER_SIZE;
}

inline Material fillMaterial(tinyobj::material_t& material)
{
	Material m;
	m.ambient.x = material.ambient[0];
	m.ambient.y = material.ambient[1];
	m.ambient.z = material.ambient[2];

	m.diffuse.x = material.diffuse[0];
	m.diffuse.y = material.diffuse[1];
	m.diffuse.z = material.diffuse[2];

	m.specular.x = material.specular[0];
	m.specular.y = material.specular[1];
	m.specular.z = material.specular[2];

	m.emission.x = material.emission[0];
	m.emission.y = material.emission[1];
	m.emission.z = material.emission[2];

	m.transmittance.x = material.transmittance[0];
	m.transmittance.y = material.transmittance[1];
	m.transmittance.z = material.transmittance[2];

	m.dissolve = material.dissolve;
	m.illum = material.illum;
	m.ior = material.ior;
	m.shininess = material.shininess;

	return m;
}

/*Adding materials in such way that materials are unique, and adjusting the material indices accordingly*/
inline void processMaterials(ModelData& modelData, vector<Material>& materials)
{
	map<CL_UINT,CL_UINT> indexTransform;
	for(CL_UINT i = 0; i < modelData.materials.size(); i++)
	{
		Material m = fillMaterial(modelData.materials[i]);
		if (materials.empty())
		{
			materials.push_back(m);
			indexTransform[i] = 0;
			continue;
		}
		bool found = false;
		for(CL_UINT j = 0; j < materials.size(); j++)
		{
			if (materialEquals(&m,&materials[j]))
			{
				indexTransform[i]=j;
				found = true;
				break;
			}
		}
		if (!found)
		{
			indexTransform[i] = materials.size();
			materials.push_back(m);
		}
	}

	for(int i = 0; i < modelData.shapes.size(); i++)
	{
		tinyobj::shape_t& shape = modelData.shapes[i];
		for(int j = 0; j < shape.mesh.material_ids.size(); j++)
		{
			CL_UINT originalIndex = shape.mesh.material_ids[j];
			shape.mesh.material_ids[j] = indexTransform[originalIndex];
		}
	}
}

inline bool parseNumber(const std::string& val, float& outVal)
{
	float f = atof(val.c_str());
	//Check if the string was parsed right
	if (f==0.0)
	{
		std::string::const_iterator it = val.begin();
		std::string::const_iterator it_end = val.end();
		for(;it!=it_end;it++)
			if ((*it) != '0' && (*it) != '.')
				return false;
	}
	outVal = f;
	return true;
}

int stringToFloats(const string& str, float* result,int size)
{
	vector<string> splitVec;
	boost::split(splitVec,str,boost::is_any_of(" "),boost::token_compress_on);
	const int count = min(splitVec.size(),size);
	int index = 0;
	for(int i = 0; i < count; i++)
	{
		float val = 0.0f;
		if (parseNumber(splitVec[i],val))
		{
			result[index] = val;
			index++;
		}
	}
	return index;
}

/*************************************************************************
* API functions
**************************************************************************/

/**Constructor*/
Scene::Scene():_hostSceneData(NULL),_deviceSceneData(NULL),_sceneDataSize(0)
{
	
}

/**Destructor*/
Scene::~Scene()
{
	if (_hostSceneData)
	{
		//Release all host objects
		if (_hostSceneData)
			delete[] _hostSceneData;
		
		_hostSceneData = NULL;
	}
}

/**Loads scene from file. The expected scene file format is explained above
* @param filename Scene file to load
* @param [out]err Error info, if error occurred
* @return Result that indicates whether the operation succeeded or failed
*/
Result Scene::load(const char* filename,Errata& err)
{
	map<string,vector<string> > rawSceneData;
	try
	{
		ifstream f(filename);
		string line;
		if (f.is_open())
		{
			while (getline(f,line))
			{
				size_t spaceIndex = line.find_first_of(' ');
				if (spaceIndex == string::npos)
					continue;
				rawSceneData[line.substr(0,spaceIndex)].push_back(line.substr(spaceIndex+1,line.size()));
			}
			f.close();
		}
		else
		{
			FILL_ERRATA(err, "File: " << filename << " couldn't be opened!");
			return Error;
		}
	}
	catch(exception e)
	{
		FILL_ERRATA(err,"Exception on trying to read scene file",e);
		return Error;
	}

	const int SPHERE_ARRAY_LENGTH = 4;
	const int LIGHT_ARRAY_LENGTH = 4;
	const int NUMERIC_ARRAY_LENGTH = 4; //Should be max of all of them
	
	map<string,vector<string> >::iterator it = rawSceneData.begin();
	map<string,vector<string> >::iterator it_end = rawSceneData.end();
	
	//Calculate total scene size
	_sceneDataSize = SCENE_HEADER_SIZE; 
	cl_ulong totalPrimitiveCount = 0;
	cl_ulong totalLightsCount = 0;
	cl_ulong totalSpheresCount = 0;
	cl_ulong totalMeshCount = 0;
	cl_ulong totalModelDataSize = 0;
	vector<ModelData> loadedMeshModels;
	vector<Material> uniqueMaterials;
	
	for(;it!=it_end;it++)
	{
		if ( (*(it)).first == "LIGHT")
		{
			totalLightsCount = (*(it)).second.size();
			_sceneDataSize+=(totalLightsCount * sizeof(Light)); //Size of lights themselves
		}
		else if ( (*(it)).first == "SPHERE")
		{
			totalSpheresCount = (*(it)).second.size();
			_sceneDataSize+=(totalSpheresCount * sizeof(Sphere)); //Size of circles themselves
		}
		else if ( (*(it)).first == "MESH")
		{
			totalMeshCount = (*(it)).second.size();
			vector<string>& stringVector = (*(it)).second;
			loadedMeshModels.reserve(totalMeshCount);
			const unsigned int count = stringVector.size();
			for (int i = 0; i < count; i++)
			{
				ModelData m;
				loadMesh(stringVector[i],m,err);
				m.calculatedDataSize = calculateModelDataSize(m);
				totalModelDataSize+= m.calculatedDataSize; 
				processMaterials(m,uniqueMaterials);
				loadedMeshModels.push_back(m);
			}
			_sceneDataSize+=totalModelDataSize;
		}
		else 
			continue;

		totalPrimitiveCount+=(*(it)).second.size();
		_sceneDataSize+=uniqueMaterials.size() * sizeof(struct Material);
	}

	//Once size calculated - Now can allocate!
	if (_hostSceneData)
		delete [] _hostSceneData;
	_hostSceneData = new char[_sceneDataSize];
	memset(_hostSceneData,0,_sceneDataSize);
	SceneHeader* sceneHeader = SCENE_HEADER(_hostSceneData);
	sceneHeader->totalDataSize = _sceneDataSize;
	sceneHeader->numberOfPrimitives = totalPrimitiveCount;
	sceneHeader->numberOfLights = totalLightsCount;
	sceneHeader->numberOfSpheres = totalSpheresCount;
	sceneHeader->numberOfModels = loadedMeshModels.size();
	sceneHeader->modelBufferSize = totalModelDataSize;
	sceneHeader->numberOfMaterials = uniqueMaterials.size();
	
	//Preallocate numeric buffer
	boost::scoped_array<float> numericData(new float[NUMERIC_ARRAY_LENGTH]);

	//And, parsing and filling the data
	it = rawSceneData.begin();
	for(;it!=it_end;it++)
	{
		if ( (*(it)).first == "LIGHT")//Parsing sphere, if it is a sphere
		{
			vector<string>& stringVector = (*(it)).second;
			const unsigned int count = stringVector.size();
			for (int i = 0; i < count; i++)
			{
				//Parse light
				if(LIGHT_ARRAY_LENGTH != stringToFloats(stringVector[i],numericData.get(),LIGHT_ARRAY_LENGTH))
				{
					FILL_ERRATA(err,"Sphere array contains some invalid values");
					continue;
				}
				//Position
				Light* lt = getLightAtIndex(_hostSceneData,i);
				lt->posAndEnergy.x = numericData[0]; //location x
				lt->posAndEnergy.y = numericData[1]; //location y
				lt->posAndEnergy.z = numericData[2]; //location z
				lt->posAndEnergy.w = numericData[3]; //Energy - Distance lit 
			}
		}
		else if ( (*(it)).first == "SPHERE")//Parsing sphere, if it is a sphere
		{
			vector<string>& stringVector = (*(it)).second;
			const unsigned int count = stringVector.size();
			
			
			for (int i = 0; i < count; i++)
			{
				//Parse sphere
				if(SPHERE_ARRAY_LENGTH != stringToFloats(stringVector[i],numericData.get(),SPHERE_ARRAY_LENGTH))
				{
					FILL_ERRATA(err,"Sphere array contains some invalid values");
					continue;
				}
				//Position
				Sphere* sp = getSphereAtIndex(_hostSceneData,i);
				sp->data.x = numericData[0];
				sp->data.y = numericData[1];
				sp->data.z = numericData[2];
				//Radius
				sp->data.w = numericData[3];
			}
		}
	}

    //Packing the materials
	for(int i = 0; i < uniqueMaterials.size(); i++)
		*getMaterialAtIndex(_hostSceneData,i) = uniqueMaterials[i];

	//Resetting scene models bounding box
	fillVector3(sceneHeader->modelsBoundingBox.bounds[0],FLT_MAX,FLT_MAX,FLT_MAX);
	fillVector3(sceneHeader->modelsBoundingBox.bounds[1],FLT_MIN,FLT_MIN,FLT_MIN);

	//Special case for mesh models - We already loaded them, all that is needed is to pack them into the buffer
	CL_ULONG sceneTriangleCount = 0;
	for(int i = 0; i < totalMeshCount; i++)
	{
		char* modelData = getModelAtIndex(i,_hostSceneData);
		ModelData& model = loadedMeshModels[i];
		ModelHeader* hdr = MODEL_HEADER(modelData);
		hdr->dataSize = model.calculatedDataSize;
		const int numOfMeshes = model.shapes.size();
		hdr->numberOfSubmeshes = numOfMeshes;
		initVector3(minBounds,FLT_MAX,FLT_MAX,FLT_MAX);
		initVector3(maxBounds,FLT_MIN,FLT_MIN,FLT_MIN);
		CL_ULONG modelTriangleCount = 0;
		for(int m = 0; m < numOfMeshes; m++)
		{
			char* meshBuffer = getMeshAtIndex(m,modelData);
			tinyobj::shape_t* shape = &(loadedMeshModels[i].shapes[m]);
			const unsigned int numOfVertices = shape->mesh.positions.size()/3;
			const unsigned int numOfIndices = shape->mesh.indices.size();
			MeshHeader* meshHeader = MESH_HEADER(meshBuffer);
			meshHeader->dataSize = calculateMeshDataSize(*shape);
			meshHeader->numberOfVertices = numOfVertices;
			meshHeader->numberOfIndices = numOfIndices;
			meshHeader->materialIndex = shape->mesh.material_ids[0];
			meshHeader->numberOfTriangles = numOfIndices / 3;
			modelTriangleCount+=meshHeader->numberOfTriangles;
			int currentPos = 0;
			for (int vi = 0; vi < numOfVertices; vi++)
			{
				VERTEX_TYPE vertex;
				vertex.x = shape->mesh.positions[currentPos++];
				vertex.y = shape->mesh.positions[currentPos++];
				vertex.z = shape->mesh.positions[currentPos++];
				minBounds.x = min(minBounds.x,vertex.x);
				minBounds.y = min(minBounds.y,vertex.y);
				minBounds.z = min(minBounds.z,vertex.z);
				maxBounds.x = max(maxBounds.x,vertex.x);
				maxBounds.y = max(maxBounds.y,vertex.y);
				maxBounds.z = max(maxBounds.z,vertex.z);
				setVertexAt(vertex,vi,meshBuffer);
			}
			for(int ii = 0; ii < numOfIndices; ii++)
				setIndexAt(shape->mesh.indices[ii],ii,meshBuffer);

			hdr->boundingBox.bounds[0] = minBounds;
			hdr->boundingBox.bounds[1] = maxBounds;
			sceneHeader->modelsBoundingBox = merge(hdr->boundingBox,sceneHeader->modelsBoundingBox);
			//merge(&(hdr->boundingBox),&(sceneHeader->modelsBoundingBox));

		}
		hdr->numberOfTriangles = modelTriangleCount;
		sceneTriangleCount+=modelTriangleCount;
	}
	sceneHeader->totalNumberOfTriangles = sceneTriangleCount;
	
	return Success;
}

/**Loads scene from host memory to device memory
* @param OpenCL context
* @param [out]err Error info, if error occurred
* @return Result that indicates whether the operation succeeded or failed
*/
Result Scene::loadToGPU(const OpenCLUtils::CLExecutionContext& context,Errata& err)
{
	//Transferring scene buffer to GPU
	_deviceSceneData.reset(new OpenCLUtils::CLBuffer(context,_sceneDataSize,_hostSceneData,OpenCLUtils::CLBufferFlags::ReadOnly));
	return Success;
}

 