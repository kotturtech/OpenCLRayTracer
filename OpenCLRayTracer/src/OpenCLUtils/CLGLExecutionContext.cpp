/**
 * @file CLGLExecutionContext.cpp
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
 * Implementation file of CLGLExecutionContext and its related classes, that extend functionality of
 * the CLExecutionContext by adding OpenCL OpenGL interop related functions
 *
 */

#include <CL\cl_gl.h>
#include <OpenCLUtils/APIErrorCheck.h>
#include <OpenCLUtils/CLGLExecutionContext.h>

using namespace CLRayTracer::Common;
using namespace CLRayTracer::CLGLInterop;
using namespace CLRayTracer::OpenCLUtils;

/*****************************************************************************************
* Implementation of class CLGLExecutionContext
******************************************************************************************/

/**Destructor*/
CLGLExecutionContext::~CLGLExecutionContext()
{
}

/** Create OpenCL/OpenGL memory object for inter-operation
* @param size Desired size of the buffer
* @param [out]output Pointer to the created OpenGL/OpenCL buffer object
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLGLExecutionContext::createCLGLBuffer(size_t size,boost::shared_ptr<CLGLMemoryBuffer>& out_buffer,Errata& err) const
{
	char *data = new char[size];
    memset(data,0,size);
	Result res = createCLGLBuffer(data,size,out_buffer,err);
	delete[] data;
	return res;
}

/** Create OpenCL/OpenGL memory object for inter-operation
* @param data The host data that should be copied to the OpenGL/OpenCL buffer on creation
* @param size Desired size of the buffer
* @param [out]output Pointer to the created OpenGL/OpenCL buffer object
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLGLExecutionContext::createCLGLBuffer(void* data,size_t size,boost::shared_ptr<CLGLMemoryBuffer>& out_buffer,Errata& err) const
{
	//Create VBO
	GLuint glBufferId = 0;
	cl_mem clBuffer = NULL;
	int status = 0;
	GLint bufferSize = 0;
	glGenBuffers(1, &glBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, glBufferId);
	glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
	
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
    if(size != bufferSize)
	{
       glDeleteBuffers(1, &glBufferId);
	   glFinish();
       FILL_ERRATA(err,"Data size mismatch on CL GL Buffer Creation");
	   return Error;
    }

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glFinish();

	clBuffer = clCreateFromGLBuffer(_clContext, CL_MEM_WRITE_ONLY, glBufferId, &status);
	if (CL_SUCCESS != status)
	{
		FILL_ERRATA(err,"Couldn't create CL buffer from GL buffer, reason: "<< appsdk::getOpenCLErrorCodeStr(status));
		return Error;
	}
	out_buffer.reset(new CLGLMemoryBuffer(size,glBufferId,clBuffer));
	return Success;
}

/** Acquire memory object for use with OpenCL memory operations
* @param object Memory objects list
* @param evt Events to wait for, before proceeding to this operation
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLGLExecutionContext::enqueueAcquireGLObject(std::vector<cl_mem> objects, CLEvent& evt, Errata& err) const
{
	cl_int status = clEnqueueAcquireGLObjects(_clCommandQueue,objects.size(),&objects[0],NULL,NULL,&evt.getCLEvent());
	if (CL_SUCCESS != status)
	{
		FILL_ERRATA(err,"Acquire GL Objects failure: "<< appsdk::getOpenCLErrorCodeStr(status));
		return Error;
	}
	return Success;
}

/** Acquire memory object for use with OpenCL memory operations
* @param object Memory objects list
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLGLExecutionContext::enqueueAcquireGLObject(std::vector<cl_mem> objects, Errata& err) const
{
	cl_int status = clEnqueueAcquireGLObjects(_clCommandQueue,objects.size(),&objects[0],NULL,NULL,NULL);
	if (CL_SUCCESS != status)
	{
		FILL_ERRATA(err,"Acquire GL Objects failure: "<< appsdk::getOpenCLErrorCodeStr(status));
		return Error;
	}
	return Success;
}

/** Release memory object for use by OpenGL 
* @param object Memory objects list
* @param evt Events to wait for, before proceeding to this operation
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLGLExecutionContext::enqueueReleaseGLObject(std::vector<cl_mem> objects, CLEvent& evt, Errata& err) const
{
	cl_int status = clEnqueueReleaseGLObjects(_clCommandQueue,objects.size(),&objects[0],NULL,NULL,&evt.getCLEvent());
	if (CL_SUCCESS != status)
	{
		FILL_ERRATA(err,"Release GL Objects failure: "<< appsdk::getOpenCLErrorCodeStr(status));
		return Error;
	}
	return Success;
}

/** Release memory object for use by OpenGL 
* @param object Memory objects list
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLGLExecutionContext::enqueueReleaseGLObject(std::vector<cl_mem> objects, Errata& err) const
{
	cl_int status = clEnqueueReleaseGLObjects(_clCommandQueue,objects.size(),&objects[0],NULL,NULL,NULL);
	if (CL_SUCCESS != status)
	{
		FILL_ERRATA(err,"Release GL Objects failure: "<< appsdk::getOpenCLErrorCodeStr(status));
		return Error;
	}
	return Success;
}

/*****************************************************************************************
* Implementation of class CLGLMemoryBuffer
******************************************************************************************/

/**Destructor*/
CLGLMemoryBuffer::~CLGLMemoryBuffer()
{
	if (_clBuffer)
		clReleaseMemObject(_clBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, _vboId);
	glDeleteBuffers(1, &_vboId);
	
}