/**
 * @file Light.h
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
 * Data structures and functions related to Light, used for shading
 */

/** \addtogroup g1 Ray Tracing Implementation Utilities 
 *  @{
 */

/** \addtogroup g11 Scene Primitives 
 *  @{
 */

#ifndef CL_RT_POINTLIGHT_H
#define CL_RT_POINTLIGHT_H

#include <CLData/CLPortability.h>
/**
* struct Light - Represents a light in a scene
*/
struct Light
{
	CL_FLOAT4 posAndEnergy; //x,y,z - Location, w - Energy (Distance of effect)
} ALIGNED(16);

/**
* Calculates bounding box of a triangle
* @param distance - The distance to light
* @param lightEnergy - The energy of a light source
* @return Returns percentage of light energy at given distance
*/
inline float lightEnergyPercentage(float distance,float lightEnergy)
{
	return max((1.0f - distance/lightEnergy),0.0f);
}


#endif //CL_RT_POINTLIGHT_H

/** @}*/
/** @}*/