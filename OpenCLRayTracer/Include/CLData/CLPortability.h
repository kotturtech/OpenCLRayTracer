/**
 * @file CLPortability.h
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
 * Header file that contains macros and functions for portability between OpenCL and Win32 compilers
 */

/** \addtogroup g1 Ray Tracing Implementation Utilities 
 *  @{
 */

/** \addtogroup g15 Misc and Portability Utilities
 *  @{
 */

#ifndef CL_RT_CLPORTABILITY_H
#define CL_RT_CLPORTABILITY_H

//Math headers
#ifdef _WIN32 
#include <CL\cl.h> 
#include <float.h> 
#include <math.h> 
#endif

//Align attribute
#ifndef _WIN32 
#define ALIGNED(X) __attribute__ ((aligned (X))) // !WIN32  
#else
#define ALIGNED(X) __declspec(align(X)) 
#endif

//Data type defines (Add additional types if necessary)
#ifdef _WIN32

#define CL_SHORT cl_short
#define CL_SHORT2 cl_short2
#define CL_SHORT3 cl_short3
#define CL_SHORT4 cl_short4

#define CL_USHORT cl_ushort
#define CL_USHORT2 cl_ushort2
#define CL_USHORT3 cl_ushort3
#define CL_USHORT4 cl_ushort4

#define CL_INT cl_int
#define CL_INT2 cl_int2
#define CL_INT3 cl_int3
#define CL_INT4 cl_int4

#define CL_UINT cl_uint
#define CL_UINT2 cl_uint2
#define CL_UINT3 cl_uint3
#define CL_UINT4 cl_uint4

#define CL_LONG cl_long
#define CL_LONG2 cl_long2
#define CL_LONG3 cl_long3
#define CL_LONG4 cl_long4

#define CL_ULONG cl_ulong
#define CL_ULONG2 cl_ulong2
#define CL_ULONG3 cl_ulong3
#define CL_ULONG4 cl_ulong4

#define CL_FLOAT cl_float
#define CL_FLOAT2 cl_float2
#define CL_FLOAT3 cl_float3
#define CL_FLOAT4 cl_float4

//OpenCL address spaces
#define CL_GLOBAL
#define CL_LOCAL
#define CL_CONSTANT

#else

#define CL_SHORT short
#define CL_SHORT2 short2
#define CL_SHORT3 short3
#define CL_SHORT4 short4

#define CL_USHORT ushort
#define CL_USHORT2 ushort2
#define CL_USHORT3 ushort3
#define CL_USHORT4 ushort4

#define CL_INT int
#define CL_INT2 int2
#define CL_INT3 int3
#define CL_INT4 int4

#define CL_UINT uint
#define CL_UINT2 uint2
#define CL_UINT3 uint3
#define CL_UINT4 uint4

#define CL_LONG long
#define CL_LONG2 long2
#define CL_LONG3 long3
#define CL_LONG4 long4

#define CL_ULONG ulong
#define CL_ULONG2 ulong2
#define CL_ULONG3 ulong3
#define CL_ULONG4 ulong4

#define CL_FLOAT float
#define CL_FLOAT2 float2
#define CL_FLOAT3 float3
#define CL_FLOAT4 float4

//OpenCL address spaces
#define CL_GLOBAL __global
#define CL_LOCAL __local
#define CL_CONSTANT __constant

#endif


//Functions for CPU (Add functions if necessary)
#ifdef _WIN32 

/*This is here to avoid the inclusion of the entire Windows.h here*/

#define min(X, Y) (((X) < (Y)) ? (X) : (Y))
#define max(X, Y) (((X) > (Y)) ? (X) : (Y))

//Normalize the vector
inline cl_float4 cpu_normalize(cl_float4 vec)
{
	float l = 1/sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
	cl_float4 result;
	result.x = vec.x *= l;
	result.y = vec.y *= l;
	result.z = vec.z *= l;
	return result;
}

//Analog to intrinsic CLZ - Count lead zeroes
#include <boost/utility/binary.hpp>
inline cl_uint software_clz(cl_uint x)
{
	cl_uint mask = BOOST_BINARY(1000 0000 0000 0000 0000 0000 0000 0000);
	cl_uint counter = 0;
	while ((x & mask) == 0 && counter < 32)
	{
		mask = mask >> 1;
		counter++;
	}
	return counter;
}

//Analog to intrinsic CLZ - Count lead zeroes
inline cl_uint software_clz(cl_int x)
{
	cl_uint mask = BOOST_BINARY(1000 0000 0000 0000 0000 0000 0000 0000);
	cl_uint counter = 0;
	while ((x & mask) == 0 && counter < 32)
	{
		mask = mask >> 1;
		counter++;
	}
	return counter;
}

//Analog to intrinsic CLZ - Count lead zeroes
inline cl_uint software_clz(cl_ulong x)
{
	cl_ulong mask = BOOST_BINARY(1000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000);
	cl_uint counter = 0;
	while ((x & mask) == 0 && counter < 64)
	{
		mask = mask >> 1;
		counter++;
	}

	return counter;
}

/*Vector and math functions analogs for CPU for testing*/
inline CL_FLOAT3 operator-(const CL_FLOAT3& one, const CL_FLOAT3& two)
{
	CL_FLOAT3 result;
	result.x = one.x - two.x;
	result.y = one.y - two.y;
	result.z = one.z - two.z;
	return result;
}

inline CL_FLOAT3 operator+(const CL_FLOAT3& one, const CL_FLOAT3& two)
{
	CL_FLOAT3 result;
	result.x = one.x + two.x;
	result.y = one.y + two.y;
	result.z = one.z + two.z;
	return result;
}

inline CL_FLOAT3 operator*(const CL_FLOAT3& one, const float& scalar)
{
	CL_FLOAT3 result;
	result.x = one.x * scalar;
	result.y = one.y * scalar;
	result.z = one.z * scalar;
	return result;
}

inline CL_FLOAT3 operator+(const CL_FLOAT3& one, const float& scalar)
{
	CL_FLOAT3 result;
	result.x = one.x + scalar;
	result.y = one.y + scalar;
	result.z = one.z + scalar;
	return result;
}

inline CL_FLOAT3 operator/(const CL_FLOAT3& one, const float& scalar)
{
	CL_FLOAT3 result;
	result.x = one.x / scalar;
	result.y = one.y / scalar;
	result.z = one.z / scalar;
	return result;
}

inline CL_FLOAT3 operator/(const CL_FLOAT3& one, const CL_FLOAT3& two)
{
	CL_FLOAT3 result;
	result.x = one.x / two.x;
	result.y = one.y / two.y;
	result.z = one.z / two.z;
	return result;
}

inline CL_FLOAT3 operator*(const CL_FLOAT3& one, const CL_FLOAT3& two)
{
	CL_FLOAT3 result;
	result.x = one.x * two.x;
	result.y = one.y * two.y;
	result.z = one.z * two.z;
	return result;
}

inline CL_FLOAT3 cross(const CL_FLOAT3& a, const CL_FLOAT3& b)
{
	CL_FLOAT3 result;
	result.x = a.y*b.z - a.z*b.y;
	result.y = a.z*b.x - a.x*b.z;
	result.z = a.x*b.y - a.y*b.x;
	return result;
}

inline CL_FLOAT dot(const CL_FLOAT3& a, const CL_FLOAT3& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

#define LENGTH3(vec) sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z)

#define sign(n) n >= 0 ? 1 : -1

inline CL_FLOAT2 max2(const CL_FLOAT2& a, const CL_FLOAT2& b)
{
	 CL_FLOAT2 result = { max(a.x,b.x), max(a.y,b.y)}; 
	 return result;
}

inline CL_FLOAT3 max3(const CL_FLOAT3& a, const CL_FLOAT3& b)
{
	 CL_FLOAT3 result = { max(a.x,b.x), max(a.y,b.y),max(a.z,b.z)}; 
	 return result;
}

inline CL_FLOAT4 max4(const CL_FLOAT4& a, const CL_FLOAT4& b)
{
	 CL_FLOAT4 result = { max(a.x,b.x), max(a.y,b.y),max(a.z,b.z),max(a.w,b.w)}; 
	 return result;
}

inline CL_FLOAT2 min2(const CL_FLOAT2& a, const CL_FLOAT2& b)
{
	 CL_FLOAT2 result = {min(a.x,b.x), min(a.y,b.y)}; 
	 return result;
}

inline CL_FLOAT3 min3(const CL_FLOAT3& a, const CL_FLOAT3& b)
{
	 CL_FLOAT3 result = { min(a.x,b.x), min(a.y,b.y),min(a.z,b.z)}; 
	 return result;
}

inline CL_UINT3 min3(const CL_UINT3& a, const CL_UINT3& b)
{
	 CL_UINT3 result = { min(a.x,b.x), min(a.y,b.y),min(a.z,b.z)}; 
	 return result;
}

inline CL_FLOAT4 min4(const CL_FLOAT4& a, const CL_FLOAT4& b)
{
	 CL_FLOAT4 result = { min(a.x,b.x), min(a.y,b.y),min(a.z,b.z),min(a.w,b.w)}; 
	 return result;
}

inline CL_FLOAT4 _combineToVector(const CL_FLOAT3 a,const CL_FLOAT b)
{
		CL_FLOAT4 result;
		result.x = a.x;
		result.y = a.y;
		result.z = a.z;
		result.w = b;
		return result;
}

inline CL_FLOAT4 _combineToVector(const CL_FLOAT a,const CL_FLOAT b,const CL_FLOAT c,const CL_FLOAT d)
{
		CL_FLOAT4 result;
		result.x = a;
		result.y = b;
		result.z = c;
		result.w = d;
		return result;
}

inline CL_UINT2 _combineToVector(const CL_UINT a,const CL_UINT b)
{
		CL_UINT2 result;
		result.x = a;
		result.y = b;
		return result;
}

inline CL_FLOAT3 _combineToVector(const CL_FLOAT a, const CL_FLOAT b, const CL_FLOAT c)
{
		CL_FLOAT3 result;
		result.x = a;
		result.y = b;
		result.z = c;
		return result;
}

inline CL_UINT3 _combineToVector(const CL_UINT a, const CL_UINT b, const CL_UINT c)
{
		CL_UINT3 result;
		result.x = a;
		result.y = b;
		result.z = c;
		return result;
}

inline CL_FLOAT3 floor3(CL_FLOAT3 value)
{
	return _combineToVector(floor(value.x),floor(value.y),floor(value.z));
}

inline CL_UINT3 convert_uint3(CL_FLOAT3 value)
{
	CL_UINT3 result;
	result.x = value.x;
	result.y = value.y;
	result.z = value.z;
	return result;
}

inline CL_FLOAT3 convert_float3(CL_UINT3 value)
{
	CL_FLOAT3 result;
	result.x = value.x;
	result.y = value.y;
	result.z = value.z;
	return result;
}

#endif

//Definition of functions
#ifdef _WIN32
#define CLZ software_clz
#define normalize(vec) cpu_normalize(vec);
#define clamp(v,l,h)  min(max((v), (l)), (h) )
#define MIN2 min2
#define MIN3 min3
#define MIN4 min4
#define MAX2 max2
#define MAX3 max3
#define MAX4 max4
#define FLOOR3 floor3
#define REF(type) type&
#define combineToVector  _combineToVector
#else
#define normalize(vec) fast_normalize(vec);
#define CLZ clz
#define MIN2 min
#define MIN3 min
#define MIN4 min
#define MAX2 max
#define MAX3 max
#define MAX4 max
#define FLOOR3 floor
#define combineToVector
#define REF(type) type
#endif

#endif //CL_RT_CLPORTABILITY_H

/** @}*/
/** @}*/