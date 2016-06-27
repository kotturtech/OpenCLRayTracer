/**
 * @file CLStructs.h
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
 * Miscellaneous Ray Tracing helper classes and functions
 */
#ifndef CL_RT_CLSTRUCTS
#define CL_RT_CLSTRUCTS

#include <CLData\Transform.h>

/** \addtogroup g1 Ray Tracing Implementation Utilities 
 *  @{
 */

/** \addtogroup g12 General Helper Structures 
 *  @{
 */

/**
* struct Camera - Contains data about camera position, and other data needed for ray generation
*/
struct Camera
{
	CL_FLOAT FOVDistance;
	CL_UINT resX;
	CL_UINT resY;
	CL_UINT supersampilngFactor;
	struct Matrix4 viewTransform;
} ALIGNED(16);

//Macro for easy retrieving of camera position
#define camPosition(cam) getTranslate(&((cam).viewTransform))
//const version of camera position macro
#define camPosition_const(cam) getTranslate_const(&((cam).viewTransform))

#ifdef _WIN32

const float PI = 3.14159265359f;
const float DEG2RAD =  PI /180.0f;

/**
* Calculates Field Of View - Distance between camera eye and view plane from field of view angle
* @param angle Field Of View vertical angle
* @param resx Horizontal camera resolution
* @param resy Vertical camera resolution
* @return Distance between eye and view plane
*/
inline float FOVDistFromAngle(float angle,float resx,float resy)
{
	float a = resy * 0.5f;
	return a/tan(angle * 0.5f * DEG2RAD);
}

#endif


/**
* struct Ray - Contains information about given ray
*/
struct Ray
{
	CL_UINT idx;
	CL_FLOAT3 origin;
	CL_FLOAT3 direction;
}ALIGNED(16);

/**
* Generates ray according to camera settings and pixel index
* @param camera The camera data 
* @param pixelIndex Index of a pixel for which a ray should be generated 
* @return A ray for the pixel at index of a pixel
*/
inline struct Ray generateRay(CL_CONSTANT struct Camera* camera,CL_UINT pixelIndex)
{
	struct Ray ray;
	ray.direction = normalize(
					transformVectorByMatrix_const(&(camera->viewTransform),
					(CL_FLOAT3)combineToVector(-(pixelIndex%camera->resX - camera->resX * 0.5f),
											   pixelIndex/camera->resX - camera->resY * 0.5f,
											   camera->FOVDistance)));
	ray.origin = camPosition_const(*camera);
	ray.idx = pixelIndex;
	return ray;
}


/**
* struct Contact - Contains data about ray/object hit
*/
struct Contact
{
	CL_UINT pixelIndex;
	CL_UINT materialIndex;
	CL_UINT pad[2];
	CL_FLOAT4 normalAndintersectionDistance;
} ALIGNED(16);

//Macro for easy access to distance of the hit from the origin
#define contactDist normalAndintersectionDistance.w

//Constant struct contact that represents that no hit occurred
CL_CONSTANT const struct Contact NO_CONTACT = { 0, 0, {0,0},{0,0,0,0}}; 
												
#endif //CL_RT_CLSTRUCTS

/** @}*/
/** @}*/