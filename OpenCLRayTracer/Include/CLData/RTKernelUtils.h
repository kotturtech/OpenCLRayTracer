/**
 * @file CLKernelUtils.h
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
 * Utility functions to use in OpenCL kernels and not only
 */

/** \addtogroup g1 Ray Tracing Implementation Utilities 
 *  @{
 */

/** \addtogroup g15 Misc and Portability Utilities
 *  @{
 */

#ifndef CL_RT_RTKERNELUTILS
#define CL_RT_RTKERNELUTILS

#include <CLData\CLPortability.h>

//Macro for calculating ray direction from camera and pixel index
#define rayDirection(rayDirection,pixelIndex,camera) rayDirection.x = (pixelIndex)%(camera).resX - (camera).resX * 0.5f; rayDirection.y = pixelIndex/(camera).resX - (camera).resY * 0.5f; rayDirection.z = (camera).FOVDistance; rayDirection = transformVectorByMatrix(&((camera).viewTransform),rayDirection); rayDirection = normalize(rayDirection); 

//Vector intitialization macros - Prefer using combineToVector instead of these when possible, for GPU
#define initVector4(vec,a,b,c,d) CL_FLOAT4 vec; vec.x = (a); vec.y = (b); vec.z = (c); vec.w = (d);
#define initVector3(vec,a,b,c) CL_FLOAT3 vec; vec.x = (a); vec.y = (b); vec.z = (c);
#define fillVector4(vec,a,b,c,d) vec.x = (a); vec.y = (b); vec.z = (c); vec.w = (d);
#define fillVector3(vec,a,b,c) vec.x = (a); vec.y = (b); vec.z = (c);

//Macro - Determines whether value is inclusively in range
#define containedInRange(lo,hi,value) (lo <= value) && (value <= hi)

/**
* Translates value from old to new scale
* @param oldMin Old scale minimum
* @param oldMax Old scale maximum
* @param value Value in terms of old scale
* @param newMin New scale minimum
* @param newMax New scale maximum
* @return Value in terms of new scale
*/
inline float translateScale(float oldMin,float oldMax, float value, float newMin,float newMax)
{
	float oldLen = oldMax - oldMin;
	float percentage = value / oldLen;
	float newLen = newMax - newMin;
	return percentage * newLen + newMin;
}

/**
* Normalizes value to [0-1] compared to the specified scale
* @param oldMin Old scale minimum
* @param oldMax Old scale maximum
* @param value Value in terms of old scale
* @return Value in terms of new scale [0-1]
*/
inline float normalizeScale(float oldMin,float oldMax,float value)
{
	return translateScale(oldMin,oldMax,value,0.0f,1.0f);
}

/**
* Packs two ints into one long int
* @param a First int
* @param b Second int
* @return Packed value
*/
inline CL_ULONG packIntsToLong(CL_UINT a,CL_UINT b)
{
	return (((CL_ULONG)(a)) << 32) | ((b) & 0xffffffffL); 
}

//64-value stack / Array
#define UINT_STACK_64  {\
		UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,\
		UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,\
		UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,\
		UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,\
		UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,\
		UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,\
		UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,\
		UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX\
	}\

//32-value stack / Array
#define UINT_STACK_32  {\
		UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,\
		UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,\
		UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,\
		UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX,UINT_MAX\
	}\



/***********************/
#ifdef _WIN32 //Functions for use on host

/**
* Check float number equality with epsilon
* @param a First float
* @param b Second float
* @return True if a and b are equal, considering error of size FLT_EPSILON
*/
inline bool floatEquals(CL_FLOAT a,CL_FLOAT b)
{
	float diff = a - b;
	return diff<= FLT_EPSILON && diff >= -FLT_EPSILON;  
}

/**
* Check float number equality with specified epsilon
* @param a First float
* @param b Second float
* @param epsilon Error range
* @return True if a and b are equal, considering error of specified size
*/
inline bool floatEquals(CL_FLOAT a,CL_FLOAT b,CL_FLOAT epsilon)
{
	float diff = a - b;
	return diff<= epsilon && diff >= -epsilon;  
}

/**
* Check float vector equality with specified epsilon
* @param a First float vector
* @param b Second float vector
* @return True if a and b are equal, considering error of size FLT_EPSILON
*/
inline bool float3Equals(const CL_FLOAT3* a, const CL_FLOAT3* b)
{
	if (!floatEquals(a->x,b->x)) return false;
	if (!floatEquals(a->y,b->y)) return false;
	if (!floatEquals(a->z,b->z)) return false;
	return true;
}

/**
* Calculates the smallest number which is a power of two, and is smaller than input value
* @param x Value to calculate for
* @return smallest number which is a power of two, and is larger than input value x
*/
inline CL_ULONG largestPowerOfTwo(CL_ULONG x)
{
	CL_USHORT shift = 64 - CLZ(x);
	return 1L << (shift - 1);
}

/**
* Calculates the smallest number which is a multiple of a specified number, and is larger than input value
* @param number Value to calculate for
* @param multiple Specifies multiple of what number the result should be
* @return Smallest number which is a multiple of "multiple" parameter, and is larger than input value x
*/
inline CL_UINT closestMultipleTo(CL_UINT number,CL_UINT multiple)
{

	if (number % multiple == 0)
		return number;
	return  (number / multiple + 1) * multiple;
}

/**
* Checks whether a number is a power of two
* @param x Number to check
* @return True if input parameter is a power of two
*/
inline bool isPowerOfTwo(CL_ULONG x)
{
	if (x==0 || x==1)
		return true;
	CL_ULONG pow2 = 1;
	
	for(int shift = 1; shift < 64; shift++)
		if ((pow2 << shift) == x)
			return true;

	return false;
}

#endif

#endif //CL_RT_RTKERNELUTILS

/** @}*/
/** @}*/