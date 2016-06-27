/**
 * @file AABB.h
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
 * Data structures and functions related to Axis Aligned Bounding Box (AABB)
 */

/** \addtogroup g1 Ray Tracing Implementation Utilities 
 *  @{
 */

/** \addtogroup g11 Scene Primitives 
 *  @{
 */

#ifndef CL_RT_AABB_H
#define CL_RT_AABB_H

#include <CLData\RTKernelUtils.h>

//General approach and code adopted from: http://www.cs.utah.edu/~awilliam/box/

/**
 *  struct AABB - Represents Axis Aligned Box
 */
struct AABB
{
	/**Contains the bounds of the box - Min bounds at index 0 and max bounds at index 1*/
	CL_FLOAT4 bounds[2];

} ALIGNED(16);

/**
* Calculates bounding box of a triangle
* @param v1 Vertex 1
* @param v2 Vertex 2
* @param v3 Vertex 3
* @return Bounding box of a triangle defined by input vertices
*/
inline struct AABB calculateTriangleAABB(CL_FLOAT3 v1,CL_FLOAT3 v2, CL_FLOAT3 v3)
{
	struct AABB result;
	result.bounds[0] = (CL_FLOAT4)combineToVector(MIN3(MIN3(v1,v2),v3),0.0f);
	result.bounds[1] = (CL_FLOAT4)combineToVector(MAX3(MAX3(v1,v2),v3),0.0f);

	//Avoiding that the bounding box will be a flat plane
	CL_UINT xFlat = (result.bounds[1].x - result.bounds[0].x) < FLT_EPSILON;
	CL_UINT yFlat = (result.bounds[1].y - result.bounds[0].y) < FLT_EPSILON;
	CL_UINT zFlat = (result.bounds[1].z - result.bounds[0].z) < FLT_EPSILON;
	result.bounds[1].x+=xFlat * FLT_EPSILON; 
	result.bounds[1].y+=yFlat * FLT_EPSILON; 
	result.bounds[1].z+=zFlat * FLT_EPSILON; 
	result.bounds[0].x-=xFlat * FLT_EPSILON; 
	result.bounds[0].y-=yFlat * FLT_EPSILON; 
	result.bounds[0].z-=zFlat * FLT_EPSILON;

	return result;
}

//Default AABB with min maximum bounds and max minimum bounds
#define defaultAABB(aabb) struct AABB aabb; fillVector3(aabb.bounds[0], FLT_MAX,FLT_MAX,FLT_MAX); fillVector3(aabb.bounds[1],FLT_MIN,FLT_MIN,FLT_MIN);
//Zero AABB - All values equal zero
#define zeroAABB(aabb) struct AABB aabb; fillVector3(aabb.bounds[0],0.0f,0.0f,0.0f); fillVector3(aabb.bounds[1],0.0f,0.0f,0.0f);

/**
* Calculate union of two bounding boxes
* @param a Box a
* @param b Box b
* @return Union of bounding boxes
*/
inline struct AABB merge(REF(struct AABB) a, REF(struct AABB) b)
{
	struct AABB result;
	result.bounds[0] = MIN4(a.bounds[0],b.bounds[0]);
	result.bounds[1] = MAX4(a.bounds[1],b.bounds[1]);
	return result;
}

/**
* Calculate union of three bounding boxes
* @param a Box a
* @param b Box b
* @param c Box c
* @return Union of bounding boxes
*/
inline struct AABB merge3(REF(struct AABB) a, REF(struct AABB) b,REF(struct AABB) c)
{
	struct AABB result;
	result.bounds[0] = MIN4(MIN4(a.bounds[0],b.bounds[0]),c.bounds[0]);
	result.bounds[1] = MAX4(MAX4(a.bounds[1],b.bounds[1]),c.bounds[1]);
	return result;
}

#define INV_DIR_X(dir) 1.0f/(dir).x
#define INV_DIR_Y(dir) 1.0f/(dir).y
#define INV_DIR_Z(dir) 1.0f/(dir).z

/**
* Finds an intersection between ray and AABB
* @param aabb The box to intersect
* @param ro Ray Origin
* @param rd Ray Direction
* @return Ray parameter t, at which the intersection occurs, or 0 in case no intersection found
*/
inline CL_FLOAT AABBIntersect(const REF(struct AABB) aabb,CL_FLOAT3 ro,CL_FLOAT3 rd)
{
	float tmin, tmax, tymin, tymax, tzmin, tzmax;
	tmin = (aabb.bounds[0.0f > (INV_DIR_X(rd))].x - ro.x) * INV_DIR_X(rd);
	tmax = (aabb.bounds[1-(0.0f > (INV_DIR_X(rd)))].x - ro.x) * INV_DIR_X(rd);
	tymin = (aabb.bounds[0.0f > (INV_DIR_Y(rd))].y - ro.y) *  INV_DIR_Y(rd);
	tymax = (aabb.bounds[1-(0.0f > (INV_DIR_Y(rd)))].y - ro.y) * INV_DIR_Y(rd);
	bool flag = !((tmin > tymax) || (tymin > tmax));
	tmin = max(tmin,tymin);
	tmax = min(tmax,tymax);
	tzmin = (aabb.bounds[0.0f > INV_DIR_Z(rd)].z - ro.z) * INV_DIR_Z(rd);
	tzmax = (aabb.bounds[1-(0.0f > INV_DIR_Z(rd))].z - ro.z) * INV_DIR_Z(rd);
	flag&= !((tmin > tzmax) || (tzmin > tmax)); 
	tmin = max(tmin,tzmin);
	tmax = min(tmax,tzmax);
	return tmin * flag;
}

/**
* Finds entry and exit values of ray parameter t, at which the ray enters and exits the box
* @param aabb The box to intersect
* @param ro Ray Origin
* @param rd Ray Direction
* @return Entry and exit values of ray parameter t. In case no intersection found, [0,0] will be returned
*/
inline CL_FLOAT2 findTRange(const REF(struct AABB) aabb,CL_FLOAT3 ro,CL_FLOAT3 rd)
{
	float tmin, tmax, tymin, tymax, tzmin, tzmax;
	tmin = (aabb.bounds[0.0f > (INV_DIR_X(rd))].x - ro.x) * INV_DIR_X(rd);
	tmax = (aabb.bounds[1-(0.0f > (INV_DIR_X(rd)))].x - ro.x) * INV_DIR_X(rd);
	tymin = (aabb.bounds[0.0f > (INV_DIR_Y(rd))].y - ro.y) *  INV_DIR_Y(rd);
	tymax = (aabb.bounds[1-(0.0f > (INV_DIR_Y(rd)))].y - ro.y) * INV_DIR_Y(rd);
	bool flag = !((tmin > tymax) || (tymin > tmax));
	tmin = max(tmin,tymin);
	tmax = min(tmax,tymax);
	tzmin = (aabb.bounds[0.0f > INV_DIR_Z(rd)].z - ro.z) * INV_DIR_Z(rd);
	tzmax = (aabb.bounds[1-(0.0f > INV_DIR_Z(rd))].z - ro.z) * INV_DIR_Z(rd);
	flag&= !((tmin > tzmax) || (tzmin > tmax)); 
	tmin = max(tmin,tzmin);
	tmax = min(tmax,tzmax);
	CL_FLOAT2 result;
	result.x = tmin * flag;
	result.y = tmax * flag;
	return result;
}

/**
* Determines whether a point is inside AABB
* @param aabb The box to test
* @param point The point to test 
* @return True if point is inside the AABB, false otherwise
*/
inline bool isPointInside(const REF(struct AABB) aabb, CL_FLOAT3 point)
{
	bool xIn = containedInRange(aabb.bounds[0].x,aabb.bounds[1].x,point.x);
	bool yIn = containedInRange(aabb.bounds[0].y,aabb.bounds[1].y,point.y);
	bool zIn = containedInRange(aabb.bounds[0].z,aabb.bounds[1].z,point.z);
	return xIn && yIn && zIn;
}

inline bool AABBContains(REF(struct AABB) container, REF(struct AABB) contained)
{
	bool xIn = (containedInRange(container.bounds[0].x,container.bounds[1].x,contained.bounds[0].x))
			   && (containedInRange(container.bounds[0].x,container.bounds[1].x,contained.bounds[1].x));

	bool yIn = (containedInRange(container.bounds[0].y,container.bounds[1].y,contained.bounds[0].y))
			   && (containedInRange(container.bounds[0].y,container.bounds[1].y,contained.bounds[1].y));

	bool zIn = (containedInRange(container.bounds[0].z,container.bounds[1].z,contained.bounds[0].z))
			   && (containedInRange(container.bounds[0].z,container.bounds[1].z,contained.bounds[1].z));
	return zIn && yIn && xIn;
}

/**
* Determines if two AABB overlap
* @param a Box one
* @param b Box two
* @return true whenever boxes a and b overlap, otherwise false
*/
inline bool AABBOverlaps(REF(struct AABB) a, REF(struct AABB) b)
{
	// is minimum of the maximums smaller than maximum of the minimums
	bool xNoOverlap = min(a.bounds[1].x,b.bounds[1].x) < max(a.bounds[0].x,b.bounds[0].x);
	bool yNoOverlap = min(a.bounds[1].y,b.bounds[1].y) < max(a.bounds[0].y,b.bounds[0].y);
	bool zNoOverlap = min(a.bounds[1].z,b.bounds[1].z) < max(a.bounds[0].z,b.bounds[0].z);
	return !(xNoOverlap || yNoOverlap || zNoOverlap);
}

/**
* Calculates diagonal length of AABB
* @param aabb Box
* @return Diagonal length of the input box
*/
inline float diagonalLength(REF(struct AABB) aabb)
{
	float deltaX = aabb.bounds[1].x - aabb.bounds[0].x;
	float deltaY = aabb.bounds[1].y - aabb.bounds[0].y;
	float deltaZ = aabb.bounds[1].z - aabb.bounds[0].z;
	return sqrt(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);
}

/**
* Calculates volume of AABB
* @param aabb Box
* @return Volume of the input box
*/
inline float boxVolume(REF(struct AABB) aabb)
{
	float deltaX = aabb.bounds[1].x - aabb.bounds[0].x;
	float deltaY = aabb.bounds[1].y - aabb.bounds[0].y;
	float deltaZ = aabb.bounds[1].z - aabb.bounds[0].z;
	return deltaX * deltaY * deltaZ;
}

/**
* Calculates centroid of AABB
* @param aabb Box
* @return Centroid of the input box
*/
inline CL_FLOAT3 boxCentroid(REF(struct AABB) aabb)
{
	CL_FLOAT4 temp = ((aabb.bounds[1] - aabb.bounds[0]) * 0.5f) + aabb.bounds[0];
	return (CL_FLOAT3)combineToVector(temp.x,temp.y,temp.z);
}

/**
* Array of unit vectors that correspond to principal axes: Vector at index 0 - Parallel to X axis,
* vector at index 1 - Parallel to Y axis, and vector at index 2 - Parallel to Z axis
*/
const CL_CONSTANT CL_FLOAT3 aabbAxes[3] = { 
								{1.0f,0.0f,0.0f},
							    {0.0f,1.0f,0.0f},
							    {0.0f,0.0f,1.0f} 
							  };

/**
* Projects triangle on an axis
* @param v0 Vertex one
* @param v1 Vertex two
* @param v2 Vertex three
* @param axis Axis on which the triangle should be projected
* @return The range along the input axis, which is a projection of the triangle on that axis
*/
inline CL_FLOAT2 projectTriangle(CL_FLOAT3 v0,CL_FLOAT3 v1, CL_FLOAT3 v2,CL_FLOAT3 axis)
{
	CL_FLOAT2 result;
	result.x = FLT_MAX; //min
	result.y = FLT_MIN; //max

	float val = dot(axis,v0);
	result.x = min(result.x,val);
	result.y = max(result.y,val);
	
	val = dot(axis,v1);
	result.x = min(result.x,val);
	result.y = max(result.y,val);
	
	val = dot(axis,v2);
	result.x = min(result.x,val);
	result.y = max(result.y,val);

	return result;
}

/**
* Projects box on an axis
* @param aabb Box to project
* @param axis Axis on which the boxe should be projected
* @return The range along the input axis, which is a projection of the box on that axis
*/
inline CL_FLOAT2 projectBox(REF(struct AABB) aabb,CL_FLOAT3 axis)
{
	CL_FLOAT2 result;
	result.x = FLT_MAX; //min
	result.y = FLT_MIN; //max

	//Bottom Near Right
	CL_FLOAT3 currentVertex;
	currentVertex.x = aabb.bounds[0].x;
	currentVertex.y = aabb.bounds[0].y;
	currentVertex.z = aabb.bounds[0].z;
	float val = dot(axis,currentVertex);
	result.x = min(result.x,val);
	result.y = max(result.y,val);

	//Bottom Near Left
	currentVertex.x = aabb.bounds[1].x;
	currentVertex.y = aabb.bounds[0].y;
	currentVertex.z = aabb.bounds[0].z;
	val = dot(axis,currentVertex);
	result.x = min(result.x,val);
	result.y = max(result.y,val);

	//Bottom Far Left
	currentVertex.x = aabb.bounds[1].x;
	currentVertex.y = aabb.bounds[0].y;
	currentVertex.z = aabb.bounds[1].z;
	val = dot(axis,currentVertex);
	result.x = min(result.x,val);
	result.y = max(result.y,val);

	//Bottom Far Right
	currentVertex.x = aabb.bounds[0].x;
	currentVertex.y = aabb.bounds[0].y;
	currentVertex.z = aabb.bounds[1].z;
	val = dot(axis,currentVertex);
	result.x = min(result.x,val);
	result.y = max(result.y,val);

	//Top Near Right
	currentVertex.x = aabb.bounds[0].x;
	currentVertex.y = aabb.bounds[1].y;
	currentVertex.z = aabb.bounds[0].z;
	val = dot(axis,currentVertex);
	result.x = min(result.x,val);
	result.y = max(result.y,val);

	//Top Near Left
	currentVertex.x = aabb.bounds[1].x;
	currentVertex.y = aabb.bounds[1].y;
	currentVertex.z = aabb.bounds[0].z;
	val = dot(axis,currentVertex);
	result.x = min(result.x,val);
	result.y = max(result.y,val);

	//Top Far Left
	currentVertex.x = aabb.bounds[1].x;
	currentVertex.y = aabb.bounds[1].y;
	currentVertex.z = aabb.bounds[1].z;
	val = dot(axis,currentVertex);
	result.x = min(result.x,val);
	result.y = max(result.y,val);

	//Top Far Right
	currentVertex.x = aabb.bounds[0].x;
	currentVertex.y = aabb.bounds[1].y;
	currentVertex.z = aabb.bounds[1].z;
	val = dot(axis,currentVertex);
	result.x = min(result.x,val);
	result.y = max(result.y,val);
	
	return result;
}

//Triangle - AABB Intersection: Method adopted from here: http://fileadmin.cs.lth.se/cs/Personal/Tomas_Akenine-Moller/code/tribox3.txt

/*======================== X-tests ========================*/

#define AXISTEST_X01(a, b, fa, fb)			   \
	p0 = a*v0.y - b*v0.z;			       	   \
	p2 = a*v2.y - b*v2.z;			       	   \
    minimum = min(p0,p2);                      \
    maximum = max(p0,p2);                      \
	rad = fa * boxhalfsize.y + fb * boxhalfsize.z;   \
	yesNoFlag = yesNoFlag && (minimum <= rad && maximum >= -rad);



#define AXISTEST_X2(a, b, fa, fb)			   \
	p0 = a*v0.y - b*v0.z;			           \
	p1 = a*v1.y - b*v1.z;			       	   \
    minimum = min(p0,p1);                      \
    maximum = max(p0,p1);                      \
	rad = fa * boxhalfsize.y + fb * boxhalfsize.z;   \
	yesNoFlag = yesNoFlag && (minimum <= rad && maximum >= -rad);

/*======================== Y-tests ========================*/

#define AXISTEST_Y02(a, b, fa, fb)			   \
	p0 = -a*v0.x + b*v0.z;		      	       \
	p2 = -a*v2.x + b*v2.z;	       	       	   \
    minimum = min(p0,p2);                      \
    maximum = max(p0,p2);                      \
	rad = fa * boxhalfsize.x + fb * boxhalfsize.z;   \
	yesNoFlag = yesNoFlag && (minimum <= rad && maximum >= -rad);



#define AXISTEST_Y1(a, b, fa, fb)			   \
	p0 = -a*v0.x + b*v0.z;		      	       \
	p1 = -a*v1.x + b*v1.z;	     	       	   \
    minimum = min(p0,p1);                      \
    maximum = max(p0,p1);                      \
	rad = fa * boxhalfsize.x + fb * boxhalfsize.z;   \
	yesNoFlag = yesNoFlag && (minimum <= rad && maximum >= -rad);

/*======================== Z-tests ========================*/

#define AXISTEST_Z12(a, b, fa, fb)			   \
	p1 = a*v1.x - b*v1.y;			           \
	p2 = a*v2.x - b*v2.y;			       	   \
    minimum = min(p1,p2);                      \
    maximum = max(p1,p2);                      \
	rad = fa * boxhalfsize.x + fb * boxhalfsize.y;   \
	yesNoFlag = yesNoFlag && (minimum <= rad && maximum >= -rad);

#define AXISTEST_Z0(a, b, fa, fb)		   \
	p0 = a*v0.x - b*v0.y;				   \
	p1 = a*v1.x - b*v1.y;			       \
	minimum = min(p0,p1);                  \
    maximum = max(p0,p1);                  \
	rad = fa * boxhalfsize.x + fb * boxhalfsize.y;   \
	yesNoFlag = yesNoFlag && (minimum <= rad && maximum >= -rad);

inline bool planeBoxOverlap(CL_FLOAT3 normal, CL_FLOAT3 vert, CL_FLOAT3 maxbox)	
{
   CL_FLOAT3 vmin,vmax;
 
   //Checking X  				
   vmin.x=-sign(normal.x) * maxbox.x - vert.x;	
   vmax.x= sign(normal.x) * maxbox.x - vert.x;	

    //Checking Y  				
   vmin.y=-sign(normal.y) * maxbox.y - vert.y;	
   vmax.y= sign(normal.y) * maxbox.y - vert.y;	

    //Checking Z  				
   vmin.z=-sign(normal.z) * maxbox.z - vert.z;	
   vmax.z= sign(normal.z) * maxbox.z - vert.z;	
  
   bool yesNoFlag = dot(normal,vmin) <= 0.0f;
   yesNoFlag = yesNoFlag && dot(normal,vmax) >= 0.0f;
   return yesNoFlag;
}

/**
* Determine whether a triangle and a box intersect
* @param boxcenter - The center of the box to test
* @param halfsize - The half-size of the box to test
* @param v0 First vertex of the triangle
* @param v1 Second vertex of the triangle 
* @param v2 Third vertex of the triangle
* @return True if box and triangle do intersect
*/
inline bool AABBTriangleIntersect(CL_FLOAT3 boxcenter,CL_FLOAT3 boxhalfsize, CL_FLOAT3 v0,CL_FLOAT3 v1, CL_FLOAT3 v2)
{
	
	
	/*    use separating axis theorem to test overlap between triangle and box */
    /*    need to test for overlap in these directions: */
    /*    1) the {x,y,z}-directions (actually, since we use the AABB of the triangle */
    /*       we do not even need to test these) */
    /*    2) normal of the triangle */
    /*    3) crossproduct(edge from tri, {x,y,z}-directin) */
    /*       this gives 3x3=9 more tests */

    float minimum,maximum;		
	bool yesNoFlag = true;
	
    /* move everything so that the boxcenter is in (0,0,0) */

    v0 = boxcenter - v0;
	v1 = boxcenter - v1;
	v2 = boxcenter - v2;

   /* Bullet 3:  */
   /*  test the 9 tests first (this was faster) */
   {
		CL_FLOAT3 edge = v1 - v0;
		float p0,p1,p2,rad,fex,fey,fez;
		
		fex = fabs(edge.x);
		fey = fabs(edge.y);
		fez = fabs(edge.z);
		AXISTEST_X01(edge.z, edge.y, fez, fey);
		AXISTEST_Y02(edge.z, edge.x, fez, fex);
		AXISTEST_Z12(edge.y, edge.x, fey, fex);

		edge = v2 - v1; 
		fex = fabs(edge.x);
		fey = fabs(edge.y);
		fez = fabs(edge.z);
		AXISTEST_X01(edge.z, edge.y, fez, fey);
		AXISTEST_Y02(edge.z, edge.x, fez, fex);
		AXISTEST_Z0(edge.y, edge.x, fey, fex);

		edge = v0 - v2; 
		fex = fabs(edge.x);
		fey = fabs(edge.y);
		fez = fabs(edge.z);

		AXISTEST_X2(edge.z, edge.y, fez, fey);
		AXISTEST_Y1(edge.z, edge.x, fez, fex);
		AXISTEST_Z12(edge.y, edge.x, fey, fex);
   }

   /* Bullet 1: */
   /*  first test overlap in the {x,y,z}-directions */
   /*  find min, max of the triangle each direction, and test for overlap in */
   /*  that direction -- this is equivalent to testing a minimal AABB around */
   /*  the triangle against the AABB */

   /* test in X-direction */

   minimum = min(v0.x,min(v1.x,v2.x));
   maximum = max(v0.x,max(v1.x,v2.x));
   yesNoFlag = yesNoFlag && (minimum <= boxhalfsize.x && maximum >= -boxhalfsize.x);

   /* test in Y-direction */
   minimum = min(v0.y,min(v1.y,v2.y));
   maximum = max(v0.y,max(v1.y,v2.y));
   yesNoFlag = yesNoFlag && (minimum <= boxhalfsize.y && maximum >= -boxhalfsize.y);

   /* test in Z-direction */
   minimum = min(v0.z,min(v1.z,v2.z));
   maximum = max(v0.z,max(v1.z,v2.z));
   yesNoFlag = yesNoFlag && (minimum <= boxhalfsize.z && maximum >= -boxhalfsize.z);

   /* Bullet 2: */
   /*  test if the box intersects the plane of the triangle */
   /*  compute plane equation of triangle: normal*x+d=0 */
   CL_FLOAT3 normal = cross(v1 - v0,v2 - v1);
   yesNoFlag = yesNoFlag && planeBoxOverlap(normal,v0,boxhalfsize);
   return yesNoFlag;
}

#endif //CL_RT_AABB_H

/** @}*/
/** @}*/