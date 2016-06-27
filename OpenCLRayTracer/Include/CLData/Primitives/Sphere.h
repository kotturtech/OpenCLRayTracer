/**
 * @file Sphere.h
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
 * Data structures and functions related to representation of a sphere in 3D
 */

/** \addtogroup g1 Ray Tracing Implementation Utilities  
 *  @{
 */

/** \addtogroup g11 Scene Primitives 
 *  @{
 */

#ifndef CL_RT_SPHERE
#define CL_RT_SPHERE

#include <CLData/CLPortability.h>

/**
* struct Sphere - Represents a sphere in 3D
*/
struct Sphere
{
	CL_FLOAT4 data; //First 3 values: Center, fourth: The Radius	
} ALIGNED(16); 

/**
* Calculates intersection data between ray and sphere 
* @param cr Sphere center and radius
* @param ro Ray Origin
* @param rd Ray Direction
* @return In case ray intersects the sphere: Contact normal (x,y,x) and ray parameter t (w) at which the intersection occur.
*         In case there is no intersection w component of the result will be 0.
*/
inline CL_FLOAT4 sphereIntersect(CL_FLOAT4 cr,CL_FLOAT3 ro,CL_FLOAT3 rd)
{
	float a = rd.x * rd.x + rd.y * rd.y + rd.z * rd.z;
	float b = 2.0f * (rd.x * (ro.x - cr.x) + rd.y * (ro.y - cr.y) + rd.z * (ro.z - cr.z));
	float c = ro.x * ro.x - cr.x * (2.0f * ro.x - cr.x) +
		      ro.y * ro.y - cr.y * (2.0f * ro.y - cr.y) + 
			  ro.z * ro.z - cr.z * (2.0f * ro.z - cr.z) - cr.w * cr.w;
	
	//Avoiding branching and ensuring that if there is no solution, the result would be zero, and if there is, the
	//Earliest intersection will be found
	c = sqrt(max((b * b - 4.0f * a * c),0.0f));
	a = min(c,((-b - c) / (2.0f * a))); //distance
	CL_FLOAT4 normal;
	normal.x = rd.x * a - cr.x; //normal - x
	normal.y = rd.y * a - cr.y; //normal - y
	normal.z = rd.z * a - cr.z; //normal - z
	normal.w = 0.0f;
	normal = normalize(normal);
	normal.w = a;
	return normal;
}

#endif //CL_RT_SPHERE

/** @}*/
/** @}*/