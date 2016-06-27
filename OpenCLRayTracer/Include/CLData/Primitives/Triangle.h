/**
 * @file Triangle.h
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
 * Data structures and functions related to representation of a triangle in 3D
 */

/** \addtogroup g1 Ray Tracing Implementation Utilities 
 *  @{
 */

/** \addtogroup g11 Scene Primitives 
 *  @{
 */

#ifndef CL_RT_TRIANGLE
#define CL_RT_TRIANGLE

#include <CLData/CLPortability.h>
#include <CLData/SceneBufferParser.h>
#include <CLData/RTKernelUtils.h>

/**
* struct Triangle - Represents a triangle in 3D space and its related material index
*/
struct Triangle
{
	CL_FLOAT3 vertexes[3]; 
	CL_UINT materialIndex;
} ALIGNED(16);


/**
* Calculates intersection data between ray and triangle
* @implNote Concept adapted from:
*           http://fileadmin.cs.lth.se/cs/Personal/Tomas_Akenine-Moller/raytri/raytri.c
* @param vert0 First vertex of triangle
* @param vert1 Second vertex of triangle
* @param vert2 Third vertex of triangle
* @param orig Ray Origin
* @param dir Ray Direction
* @return In case ray intersects the triangle: Contact normal (x,y,x) and ray parameter t (w) at which the intersection occur.
*         In case there is no intersection w component of the result will be 0.
*/
inline CL_FLOAT4 triangleIntersect(VERTEX_TYPE vert0,VERTEX_TYPE vert1, VERTEX_TYPE vert2, CL_FLOAT3 orig, CL_FLOAT3 dir)
{
   CL_FLOAT3 edge1, edge2, tvec, pvec, qvec;
   float det,inv_det;

   /* find vectors for two edges sharing vert0 */
   edge1=vert1 - vert0;
   edge2=vert2 - vert0;

   /* begin calculating determinant - also used to calculate U parameter */
   pvec = cross(dir,edge2);

   /* if determinant is near zero, ray lies in plane of triangle */
   det = dot(edge1, pvec);

   int flag = fabs(det) >= FLT_EPSILON; //(int)zeroIfSmaller(fabs(det),FLT_EPSILON);
   inv_det = 1.0f / det;

   /* calculate distance from vert0 to ray origin */
   tvec = orig - vert0;

   /* calculate U parameter and test bounds */
   float u = dot(tvec, pvec) * inv_det;
   /*Rewritten for GPU to reduce branching*/
   flag&=(u < 1.0f);
   flag&=(u >= 0.0f);
  
   /* prepare to test V parameter */
   qvec = cross(tvec, edge1);

   /* calculate V parameter and test bounds */
   float v = dot(dir, qvec) * inv_det;
    /*Rewritten for GPU to reduce branching*/
   flag&= ((u+v) <= 1.0f);
   flag&= (v >= 0.0f);

   /* calculate t, ray intersects triangle */
   float t = dot(edge2, qvec) * inv_det * flag; //The flag cancels out t if one of the conditions failed
   CL_FLOAT3 normal = normalize(cross(edge1,edge2));  
   initVector4(result,normal.x,normal.y,normal.z,t);
   return result;
}

/*
* Calculates centroid of a triangle
* @param v0 First vertex of triangle
* @param v1 Second vertex of triangle
* @param v2 Third vertex of triangle
* @return Centroid of the input triangle
*/
inline CL_FLOAT3 triangleCentroid (CL_FLOAT3 v0, CL_FLOAT3 v1, CL_FLOAT3 v2)
{
	return (v0 + v1 + v2) / 3.0f;
}

#endif //CL_RT_TRIANGLE

/** @}*/
/** @}*/