/**
 * @file Transform.h
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
 * Functions and structures related to Transform matrices
 * 
 * @implNote Some matrix operations adaptrd from this source: 
 * https://www.fastgraph.com/makegames/3drotation/3dsrce.html
 */

/** \addtogroup g1 Ray Tracing Implementation Utilities 
 *  @{
 */

/** \addtogroup g12 General Helper Structures 
 *  @{
 */

#ifndef CL_RT_TRANSFORM_H
#define CL_RT_TRANSFORM_H

#include <CLData\RTKernelUtils.h>

/**Helper class for representing orientation*/
struct Quaternion
{
   
     /**
      * Holds the real component of the quaternion.
      */
     CL_FLOAT r;

     /**
      * Holds the first complex component of the
      * quaternion.
      */
     CL_FLOAT i;

     /**
      * Holds the second complex component of the
      * quaternion.
      */
     CL_FLOAT j;

     /**
      * Holds the third complex component of the
      * quaternion.
      */
     CL_FLOAT k;
} ALIGNED(16);

/**Returns a quaternion that represents zero rotation
*@return Zero rotation quaternion
*/
inline struct Quaternion zeroRotation()
{
	struct Quaternion q;
	q.r = 1.0f;
	q.i = q.j = q.k = 0.0f;
	return q;
}
 /**
  * Normalises the quaternion to unit length, making it a valid
  * orientation quaternion.
  * @param q Quaternion to normalize
  */
 inline void normalizeQuaternion(struct Quaternion* q)
 {
     CL_FLOAT d = q->r*q->r+q->i*q->i+q->j*q->j+q->k*q->k;

     // Check for zero length quaternion, and use the no-rotation
     // quaternion in that case.
	 if (d < FLT_EPSILON)
	 {
		 q->i = q->j = q->k = 0.0f;
         q->r = 1;
         return;
     }

     d = ((CL_FLOAT)1.0)/sqrt(d);
     q->r *= d;
     q->i *= d;
     q->j *= d;
     q->k *= d;
 }

 /**
  * Multiplies the quaternion by the given quaternion.
  *
  * @param mult Quaternion to multiply
  * @param mult2 The quaternion by which to multiply.
  * @return Result of multiplication
  */
 inline struct Quaternion mult(REF(struct Quaternion) mult,REF(struct Quaternion) mult2)
 {
	 struct Quaternion result;
     result.r = mult.r*mult2.r - mult.i*mult2.i -
                mult.j*mult2.j - mult.k*mult2.k;
     result.i = mult.r*mult2.i + mult.i*mult2.r +
                mult.j*mult2.k - mult.k*mult2.j;
     result.j = mult.r*mult2.j + mult.j*mult2.r +
                mult.k*mult2.i - mult.i*mult2.k;
     result.k = mult.r*mult2.k + mult.k*mult2.r +
                mult.i*mult2.j - mult.j*mult2.i;
	 return result;
 }

 /**
  * Updates orientation represented by quaternion q, by rotation vector
  * @param q Pointer to quaternion to update
  * @param rot Rotation vector
  */
 inline void rotateByVector(struct Quaternion* o, CL_FLOAT3 rot)
 {
     struct Quaternion q;
	 q.r = 0;
     q.i = rot.x;
     q.j = rot.y;
     q.k = rot.z;
	 q = mult(q,*o);
     o->r+= q.r * 0.5f;
	 o->i+= q.i * 0.5f;
	 o->j+= q.j * 0.5f;
	 o->k+= q.k * 0.5f;

 }


/**
* struct Matrix4 - Stores the transform matrix
*/
struct Matrix4
{
	CL_FLOAT data[12];

} ALIGNED(16);

/**
* Returns identity matrix
* @return Identity matrix
*/
inline struct Matrix4 identityTransform()
{
	struct Matrix4 result;
	result.data[1] = result.data[2] = result.data[3] = result.data[4] = result.data[6] = result.data[7] = result.data[8] = result.data[9] = result.data[11] = 0;
    result.data[0] = result.data[5] = result.data[10] = 1;
	return result;
}

/**
* Multiplies matrix A by matrix B
* @param A Pointer to matrix A
* @param B Pointer to matrix B
* @param [out] result The result of multiplication - A X B
* @return 
*/
inline void matrixMultiply(struct Matrix4 *A, struct Matrix4 *B, struct Matrix4 *result)
{
   /* A matrix multiplication (dot product) of two 4x4 matrices.
      Actually, we are only using matrices with 3 rows and 4 columns. */

	result->data[0] = A->data[0]*B->data[0] + A->data[1]*B->data[4] + A->data[2]*B->data[8];
    result->data[1] = A->data[0]*B->data[1] + A->data[1]*B->data[5] + A->data[2]*B->data[9];
    result->data[2] = A->data[0]*B->data[2] + A->data[1]*B->data[6] + A->data[2]*B->data[10];
    result->data[3] = A->data[0]*B->data[3] + A->data[1]*B->data[7] + A->data[2]*B->data[11] + A->data[3];

    result->data[4] = A->data[4]*B->data[0] + A->data[5]*B->data[4] + A->data[6]*B->data[8];
    result->data[5] = A->data[4]*B->data[1] + A->data[5]*B->data[5] + A->data[6]*B->data[9];
    result->data[6] = A->data[4]*B->data[2] + A->data[5]*B->data[6] + A->data[6]*B->data[10];
    result->data[7] = A->data[4]*B->data[3] + A->data[5]*B->data[7] + A->data[6]*B->data[11] + A->data[7];

    result->data[8] = A->data[8]*B->data[0] + A->data[9]*B->data[4] + A->data[10]*B->data[8];
    result->data[9] = A->data[8]*B->data[1] + A->data[9]*B->data[5] + A->data[10]*B->data[9];
    result->data[10] = A->data[8]*B->data[2] + A->data[9]*B->data[6] + A->data[10]*B->data[10];
    result->data[11] = A->data[8]*B->data[3] + A->data[9]*B->data[7] + A->data[10]*B->data[11] + A->data[11];
}

/**
* Fills translation matrix according to vector
* @param A Pointer to matrix to fill
* @param t Translation vector
* @return 
*/
inline void fillTranslate(struct Matrix4 *A, CL_FLOAT3 t)
{
   /* Fill the translation matrix */
   A->data[0] = 1.0f;   A->data[1] = 0.0f;   A->data[2] = 0.0f;   A->data[3] = t.x;
   A->data[4] = 0.0f;   A->data[5] = 1.0f;   A->data[6] = 0.0f;   A->data[7] = t.y;
   A->data[8] = 0.0f;   A->data[9] = 0.0f;   A->data[10]= 1.0f;   A->data[11]= t.z;
}

/**
* Fills rotation matrix according to vector using Euler angles
* @param A Pointer to matrix to fill
* @param t Rotation vector
* @return 
*/
inline void fillRotate(struct Matrix4 *A, CL_FLOAT3 eulerAngles)
{
   /* Fill the rotation matrix, using Euler angles */
	
   struct Matrix4 x,y,z,temp;

   float cx,cy,cz;
   float sx,sy,sz;

   cx = cos(eulerAngles.x);
   cy = cos(eulerAngles.y);
   cz = cos(eulerAngles.z);

   sx = sin(eulerAngles.x);
   sy = sin(eulerAngles.y);
   sz = sin(eulerAngles.z);

   x.data[0]=1;     x.data[1]=0;     x.data[2] =0;     x.data[3] =0;
   x.data[4]=0;     x.data[5]=cx;    x.data[6] =-sx;   x.data[7] =0;
   x.data[8]=0;     x.data[9]=sx;    x.data[10]=cx;    x.data[11]=0;

   y.data[0]=cy;    y.data[1]=0;     y.data[2] =sy;    y.data[3] =0;
   y.data[4]=0;     y.data[5]=1;     y.data[6] =0;     y.data[7] =0;
   y.data[8]=-sy;   y.data[9]=0;     y.data[10]=cy;    y.data[11]=0;

   z.data[0]=cz;    z.data[1]=-sz;   z.data[2] =0;     z.data[3] =0;
   z.data[4]=sz;    z.data[5]=cz;    z.data[6] =0;     z.data[7] =0;
   z.data[8]=0;     z.data[9]=0;     z.data[10]=1;     z.data[11]=0;

   /* Note we are multiplying x*y*z. You can change the order,
      but you will get different results. */

   matrixMultiply(&z,&y,&temp);   // multiply 2 matrices
   matrixMultiply(&temp,&x,A);   // multiply result by 3rd matrix
}

/**
* Transforms vector by transform matrix
* @param matrix Transform matrix
* @param vector Vector to transform
* @return Transformed vector
*/
inline CL_FLOAT3 transformVectorByMatrix(CL_GLOBAL struct Matrix4* matrix, CL_FLOAT3 vector)
{
	return (CL_FLOAT3)combineToVector(vector.x * matrix->data[0] +
                        vector.y * matrix->data[1] +
                        vector.z * matrix->data[2] + matrix->data[3],
				        
						vector.x * matrix->data[4] + 
						vector.y * matrix->data[5] + 
						vector.z * matrix->data[6] + matrix->data[7],
						
						vector.x * matrix->data[8] +
						vector.y * matrix->data[9] +
						vector.z * matrix->data[10] + matrix->data[11]);
}

/**
* Transforms vector by transform matrix - Constant pointer version
* @param matrix Transform matrix
* @param vector Vector to transform
* @return Transformed vector
*/
inline CL_FLOAT3 transformVectorByMatrix_const(CL_CONSTANT struct Matrix4* matrix, CL_FLOAT3 vector)
{
	return (CL_FLOAT3)combineToVector(vector.x * matrix->data[0] +
                        vector.y * matrix->data[1] +
                        vector.z * matrix->data[2] + matrix->data[3],
				        
						vector.x * matrix->data[4] + 
						vector.y * matrix->data[5] + 
						vector.z * matrix->data[6] + matrix->data[7],
						
						vector.x * matrix->data[8] +
						vector.y * matrix->data[9] +
						vector.z * matrix->data[10] + matrix->data[11]);
}

/**
* Retrieves translation vector from transform matrix
* @param transform Transform matrix
* @return Translate component vector
*/
inline CL_FLOAT3 getTranslate(CL_GLOBAL struct Matrix4* transform)
{
	return (CL_FLOAT3)combineToVector(transform->data[3],transform->data[7],transform->data[11]);
}

/**
* Retrieves translation vector from transform matrix - Constant pointer version
* @param transform Transform matrix
* @return Translate component vector
*/
inline CL_FLOAT3 getTranslate_const(CL_CONSTANT struct Matrix4* transform)
{
	  initVector3(result,transform->data[3],transform->data[7],transform->data[11]);
	  return result;
}

/**
* Set translation component of transform matrix
* @param transform Transform matrix
* @param pos Position vector - value to set in the matrix

*/
inline void setTranslate(struct Matrix4* transform, CL_FLOAT3 pos)
{
	  transform->data[3] = pos.x;
	  transform->data[7] = pos.y;
	  transform->data[11] = pos.z;
}

/** Obtains the forward vector of the transform
*   @param transform Transform matrix
*   @return Unit forward vector of the specified transform
*/
inline CL_FLOAT3 forward(REF(struct Matrix4) transform)
{
	CL_FLOAT3 result;
	result.x = transform.data[2];
	result.y = transform.data[6];
	result.z = transform.data[10];
	return result;
}

/** Obtains the up vector of the transform
*   @param transform Transform matrix
*   @return Unit up vector of the specified transform
*/
inline CL_FLOAT3 up(REF(struct Matrix4) transform)
{
	CL_FLOAT3 result;
	result.x = transform.data[1];
	result.y = transform.data[5];
	result.z = transform.data[9];
	return result;
}

/** Obtains the side vector of the transform
*   @param transform Transform matrix
*   @return Unit side vector of the specified transform
*/
inline CL_FLOAT3 side(REF(struct Matrix4) transform)
{
	CL_FLOAT3 result;
	result.x = transform.data[0];
	result.y = transform.data[4];
	result.z = transform.data[8];
	return result;
}

/** Sets forward vector for input transform and adjusts the transform accordingly,for orthonormal basis
*   @param forward New forward vector
*   @param transform Transform matrix
*/
inline void setForward(CL_FLOAT3 forward, struct Matrix4* transform)
{
	forward = normalize(forward);
	CL_FLOAT3 side = cross(forward,up(*transform));
	CL_FLOAT3 up = cross(side,forward);
	transform->data[0] = side.x;
	transform->data[4] = side.y;
	transform->data[8] = side.z;
	transform->data[1] = up.x;
	transform->data[5] = up.y;
	transform->data[9] = up.z;
	transform->data[2] = forward.x;
	transform->data[6] = forward.y;
	transform->data[10] = forward.z;
}

/**
* Sets this matrix to be the rotation matrix corresponding to
* the given quaternion and position vector.
* @param transform The matrix to update
* @param q Quaternion that represents orientation
* @param pos Position vector
*/
inline void setOrientationAndPos(struct Matrix4* transform,REF(struct Quaternion) q, CL_FLOAT3 pos)
{
    transform->data[0] = 1 - (2*q.j*q.j + 2*q.k*q.k);
     transform->data[1] = 2*q.i*q.j + 2*q.k*q.r;
     transform->data[2] = 2*q.i*q.k - 2*q.j*q.r;
     transform->data[3] = pos.x;
	 
     transform->data[4] = 2*q.i*q.j - 2*q.k*q.r;
     transform->data[5] = 1 - (2*q.i*q.i  + 2*q.k*q.k);
     transform->data[6] = 2*q.j*q.k + 2*q.i*q.r;
     transform->data[7] = pos.y;

     transform->data[8] = 2*q.i*q.k + 2*q.j*q.r;
     transform->data[9] = 2*q.j*q.k - 2*q.i*q.r;
     transform->data[10] = 1 - (2*q.i*q.i  + 2*q.j*q.j);
     transform->data[11] = pos.z;
}

#endif //CL_RT_TRANSFORM_H

/** @}*/
/** @}*/