/**
 * @file CLGLExecutionContext.h
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
 * Implementation file for class CLGLInteropContext that provides functionality 
 * for OpenGL/OpenCL interop
 *
 */

#include <windows.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <OpenCLUtils/CLInterface.h>
#include <OpenCLUtils/CLGLInteropContext.h>
#include <OpenCLUtils/CLExecutionContext.h>

using namespace CLRayTracer::OpenCLUtils;
using namespace CLRayTracer::Common;
using namespace CLRayTracer::CLGLInterop;

/***********************************************************************************
* Utilities for OpenGL interop
************************************************************************************/
typedef CL_API_ENTRY cl_int (CL_API_CALL *clGetGLContextInfoKHR_fn)(
	const cl_context_properties *properties,
	cl_gl_context_info param_name,
	size_t param_value_size,
	void *param_value,
	size_t *param_value_size_ret);

// Rename references to this dynamically linked function to avoid
// collision with static link version
#define clGetGLContextInfoKHR clGetGLContextInfoKHR_proc
static clGetGLContextInfoKHR_fn clGetGLContextInfoKHR;

/***********************************************************************************
* Implementation of class CLGLInteropContext
************************************************************************************/

/**Constructor*/
CLGLInteropContext::CLGLInteropContext():_executionContext(NULL)
{	
	
}

/**Destructor*/
CLGLInteropContext::~CLGLInteropContext()
{
	cleanup();
}

/** Initializes the object instance. Unlike the function that takes platform as parameter,
*   this function automatically finds the first platform and device that support OpenGL/OpenCL interop
*   and use that platform and device
* @param CLInterface Object that encapsulates OpenCL interface
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLGLInteropContext::initialize(const CLInterface& clInterface,Errata& err)
{
	const int numplatforms = clInterface.numOfPlatforms();
	for (int i = 0; i < numplatforms; i++)
	{
		const CLPlatform* platform = clInterface.getPlatformByIndex(i);
		if (Success == initialize((*platform),err))
			return Success;
	}
	return Error;
}

/** Initializes the object instance
* @param platform Object that encapsulates the used OpenCL platform 
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLGLInteropContext::initialize(const CLPlatform& platform,Errata& err)
{
	_interopDeviceIndex = -1;
	_interopPlatform = &platform;
	bool found = false;

	if (!clGetGLContextInfoKHR)
	{
		clGetGLContextInfoKHR = (clGetGLContextInfoKHR_fn)
			clGetExtensionFunctionAddressForPlatform(platform.getCLPlatformId(), "clGetGLContextInfoKHR");
		if (!clGetGLContextInfoKHR)
		{
			FILL_ERRATA(err, "Failed to query proc address for clGetGLContextInfoKHR");
			return Error;
		}
	}

	cl_context_properties* properties = new cl_context_properties[7];
	properties[0]=CL_CONTEXT_PLATFORM;
	properties[1]=(cl_context_properties) platform.getCLPlatformId();
	properties[2]=CL_GL_CONTEXT_KHR;
	properties[3]=(cl_context_properties) wglGetCurrentContext();
	properties[4]=CL_WGL_HDC_KHR;
	properties[5]=(cl_context_properties) wglGetCurrentDC();
	properties[6]=0;

	//Determining the GPU device within platform
	for (int i = 0; i < platform.getNumOfDevices(); i++)
	{
		if (platform.getDeviceByIndex(i)->isGPU())
		{
			_interopDeviceIndex = i;
			size_t deviceSize = 0;
			int status = clGetGLContextInfoKHR(properties,
											   CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR,
												0,
												NULL,
												&deviceSize);
			if(status != CL_SUCCESS)
			{
				FILL_ERRATA(err,"Couldn't get GL context info: " << appsdk::getOpenCLErrorCodeStr(status) );
				continue;
			}
			if (deviceSize == 0)
				continue;

			found = true;
			break;
		}
	}

	if (_interopDeviceIndex == -1)
	{
		FILL_ERRATA(err,"Platform doesn't include any GPU devices");
		return Error;
	}

	if (!found)
	{
		FILL_ERRATA(err,"Couldn't set up OpenGL Interop for device! Check match between selected platform and primary display device!");
		_interopDeviceIndex = -1;
		return Error;
	}

	const CLDevice* dev = platform.getDeviceByIndex(_interopDeviceIndex);
	cl_device_id interopDevice = dev->getCLDeviceId();
	cl_uint status = clGetGLContextInfoKHR( properties,
		CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR,
		sizeof(cl_device_id),
		&interopDevice,
		NULL);
	if (status != CL_SUCCESS)
	{
		FILL_ERRATA(err,"Couldn't get GL context info: " << appsdk::getOpenCLErrorCodeStr(status) );
		return Error;
	}

	//Initializing execution context
	_executionContext.reset(new CLGLExecutionContext(*dev,properties));
	if (Success != _executionContext->initialize(err))
		return Error;

	return Success;
};

/**Cleanup function for releasing resources*/
void CLGLInteropContext::cleanup()
{
	
}
