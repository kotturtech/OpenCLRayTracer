/**
 * @file Material.h
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
 * Data structures and functions related to representation of Material of a 3D model
 */

/** \addtogroup g1 Ray Tracing Implementation Utilities  
 *  @{
 */

/** \addtogroup g11 Scene Primitives 
 *  @{
 */

#ifndef CL_RT_MATERIAL
#define CL_RT_MATERIAL

#include <CLData\RTKernelUtils.h>

/**
* struct Material - Contains data about a material for 3D model
*/

struct Material
{
	CL_FLOAT3 ambient;
	CL_FLOAT3 diffuse;
	CL_FLOAT3 specular;
	CL_FLOAT3 transmittance;
	CL_FLOAT3 emission;
	CL_FLOAT shininess;
	CL_FLOAT ior;      // index of refraction
	CL_FLOAT dissolve; // 1 == opaque; 0 == fully transparent
	// illumination model (see http://www.fileformat.info/format/material/)
	CL_FLOAT illum;	
} ALIGNED(16);


#ifdef _WIN32 //This function is not needed for GPU processing

/**
* Determines whether one material equals to the other
* @param a Material one
* @param b Material two
* @return True whether materials a and b are equal
*/
inline bool materialEquals(const struct Material* a, const struct Material* b)
{
	if (!float3Equals(&a->ambient,&b->ambient)) return false;
	if (!float3Equals(&a->diffuse,&b->diffuse)) return false;
	if (!float3Equals(&a->specular,&b->specular)) return false;
	if (!float3Equals(&a->transmittance,&b->transmittance)) return false;
	if (!float3Equals(&a->emission,&b->emission)) return false;
	if (!floatEquals(a->shininess,b->shininess)) return false;
	if (!floatEquals(a->ior,b->ior)) return false;
	if (!floatEquals(a->dissolve,b->dissolve)) return false;
	if (!floatEquals(a->illum,b->illum)) return false;
	return true;
}

#endif

#endif //CL_RT_MATERIAL

/** @}*/
/** @}*/