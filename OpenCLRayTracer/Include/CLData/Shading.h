/**
 * @file Shading.h
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
 * Functions for shading - CPU testing functions, currently not intended for use on GPU
 * 
 */

#ifndef CL_RT_SHADING
#define CL_RT_SHADING

/** \addtogroup g1 Ray Tracing Implementation Utilities 
 *  @{
 */

/** \addtogroup g16 Utilities for Debugging
 *  @{
 */


#include <CLData/CLStructs.h>
#include <CLData/RTKernelUtils.h>
#include <CLData/SceneBufferParser.h>

#ifdef _WIN32
#include <vector>

/**
* Degenerate shading - Just paint the pixel according to color of material
* @param camera - The camera
* @param contact - Collision data between ray and surface
* @param sceneBuffer - Buffer that contains the scene
* @param [out] resultingColor - The output color - Result of shading
* @return 
*/
inline void cpuShadeDegenerate(Camera* cam, Contact* contact,char* sceneBuffer,CL_FLOAT4& resultingColor)
{
	Material* material = getMaterialAtIndex(sceneBuffer,contact->materialIndex);
	resultingColor.x = material->diffuse.x;
	resultingColor.y = material->diffuse.y;
	resultingColor.z = material->diffuse.z;
	resultingColor.w = 1.0f;
}

/**
* Blinn-Phong shading function
* @param camera - The camera
* @param contact - Collision data between ray and surface
* @param sceneBuffer - Buffer that contains the scene
* @param [out] resultingColor - The output color - Result of shading
* @return 
*/
inline void cpuShadeBlinnPhong(Camera* cam, Contact* contact,char* sceneBuffer,CL_FLOAT4& resultingColor)
{
	Material defaultMaterial;
	defaultMaterial.ambient.x = 0.2f;
	defaultMaterial.ambient.y = 0.2f;
	defaultMaterial.ambient.z = 0.2f;
	defaultMaterial.diffuse.x = 1.0f;
	defaultMaterial.diffuse.y = 1.0f;
	defaultMaterial.diffuse.z = 1.0f;
	defaultMaterial.specular.x = 1.0f;
	defaultMaterial.specular.y = 0.0f;
	defaultMaterial.specular.z = 0.0f;
	defaultMaterial.shininess = 0.4f;

	CL_ULONG numOfLights = SCENE_HEADER(sceneBuffer)->numberOfLights;
	Ray r = generateRay(cam,contact->pixelIndex);
	CL_FLOAT3 intersectionPoint = r.origin + (r.direction * contact->normalAndintersectionDistance.w);
	
	Material* material = NULL; 
	if (contact->materialIndex < SCENE_HEADER(sceneBuffer)->numberOfMaterials)
		material = getMaterialAtIndex(sceneBuffer,contact->materialIndex);
	else 
		material = &defaultMaterial;
	//L - Vector towards light
	//N - Normal
	//V - Viewing vector, towards eye
	//R - Reflection vector
	CL_FLOAT3 V = normalize(camPosition(*cam) - intersectionPoint);
	fillVector4(resultingColor,material->ambient.x,material->ambient.y,material->ambient.z,0.0f); //Initially ambient color and ambient energy
	CL_FLOAT totalEnergy = 0.0f;

	std::vector<Light> lights;
	for(int i = 0; i < numOfLights; i++)
	{
		Light* l = getLightAtIndex(sceneBuffer,i);
		CL_FLOAT3 L = ((CL_FLOAT3)l->posAndEnergy) - intersectionPoint;
		float distanceToLight = LENGTH3(L);
		totalEnergy+=lightEnergyPercentage(distanceToLight,l->posAndEnergy.w) * l->posAndEnergy.w;
		lights.push_back(*l);
	}
	//Adding camera light
	{
		Light cameraLight;
		CL_FLOAT3 position = getTranslate(&(cam->viewTransform));
		cameraLight.posAndEnergy.x = position.x;
		cameraLight.posAndEnergy.y = position.y;
		cameraLight.posAndEnergy.z = position.z;
		cameraLight.posAndEnergy.w = 100000;
		CL_FLOAT3 L = ((CL_FLOAT3)cameraLight.posAndEnergy) - intersectionPoint;
		float distanceToLight = LENGTH3(L);
		totalEnergy+=lightEnergyPercentage(distanceToLight,cameraLight.posAndEnergy.w) * cameraLight.posAndEnergy.w;
		lights.push_back(cameraLight);
	}
	
	for(int i = 0; i < lights.size(); i++)
	{
		Light& l = lights[i];
		CL_FLOAT3 L = ((CL_FLOAT3)l.posAndEnergy) - intersectionPoint;
		float distanceToLight = LENGTH3(L);
		float lightIntensity = lightEnergyPercentage(distanceToLight,l.posAndEnergy.w);
		float percentageOfTotal = (lightIntensity * l.posAndEnergy.w) / totalEnergy;
		float lsEnergy = lightIntensity * percentageOfTotal;
		L = normalize(L);
		float LDotN = dot(L,((CL_FLOAT3)contact->normalAndintersectionDistance));
		LDotN = LDotN > 0.0f? LDotN : 0.0f;
		initVector3(diffuseComponent,
			        material->diffuse.x * LDotN * lsEnergy,
					material->diffuse.y * LDotN * lsEnergy,
					material->diffuse.z * LDotN * lsEnergy);

		resultingColor.x+=diffuseComponent.x;
		resultingColor.y+=diffuseComponent.y;
		resultingColor.z+=diffuseComponent.z;
		CL_FLOAT3 R = normalize((((CL_FLOAT3)(contact->normalAndintersectionDistance)) * (2.0f * LDotN)) - L);
	
		float nSpecular = pow(dot(R,V),material->shininess);
		initVector3(phongComponent,
					material->specular.x * nSpecular * lsEnergy,
					material->specular.y * nSpecular * lsEnergy,
					material->specular.z * nSpecular * lsEnergy);

		resultingColor.x+=max(phongComponent.x,0);
		resultingColor.y+=max(phongComponent.y,0);
		resultingColor.z+=max(phongComponent.z,0);
	
	}
	//Ensuring the color vector is legal
	resultingColor.x = min((resultingColor.x),1.0f);
	resultingColor.y = min((resultingColor.y),1.0f);
	resultingColor.z = min((resultingColor.z),1.0f);
	resultingColor.w = 1.0f; 

}
/** @}*/
/** @}*/
#endif 
#endif //CL_RT_SHADING
