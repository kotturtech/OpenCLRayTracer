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
 * Implementation of fundamental classes of the OpenCL API
 *
 */


#include <boost/format.hpp>
#include <OpenCLUtils\CLExecutionContext.h>
#include <OpenCLUtils\CLInterface.h>

using namespace std;
using namespace CLRayTracer::OpenCLUtils;
using namespace CLRayTracer::Common;


/**********************************************************************
* Implementation of class CLInterface
***********************************************************************/

/**Constructor*/
CLInterface::CLInterface()
{
	
}

/** Initializes OpenCL. Must be called before any OpenCL operation
* @param [out]err Error info if occurred
* @return Result of the operation: Success or Failure	
*/
Result CLInterface::initCL(Errata& err)
{
	//Getting platform count
	cl_int	status = clGetPlatformIDs(0, NULL, &_numPlatforms);
	if (status != CL_SUCCESS)
	{
		FILL_ERRATA(err,"Couldn't get number of platforms: " << appsdk::getOpenCLErrorCodeStr(status) );
		return Error;
	}

	//Getting platform ids
	boost::shared_array<cl_platform_id> availablePlatforms(new cl_platform_id[_numPlatforms]);
	_availablePlatforms.reset(new CLPlatform[_numPlatforms]);

	status = clGetPlatformIDs(_numPlatforms, availablePlatforms.get(),NULL);
	if (status != CL_SUCCESS)
	{

		FILL_ERRATA(err,"Couldn't get number of platforms: " << appsdk::getOpenCLErrorCodeStr(status));
		return Error;
	}

	char buf[1000];
	for (int i = 0; i < _numPlatforms; i++)
	{
		//Set platform ID
		_availablePlatforms[i]._platformId = availablePlatforms[i];
		
		//Set platform params

		//1.Vendor
		status = clGetPlatformInfo(availablePlatforms[i],
                                               CL_PLATFORM_VENDOR,
                                               sizeof(buf),
                                               buf,
                                               NULL);
		if (status != CL_SUCCESS)
		{
			FILL_ERRATA(err,"Couldn't get platform parameter: CL_PLATFORM_VENDOR, reason: " << appsdk::getOpenCLErrorCodeStr(status));
			return Error;
		}
		else
		{
			_availablePlatforms[i]._platformVendor = std::string(buf);
		}

		//2.Supported version
		status = clGetPlatformInfo(availablePlatforms[i],
                                               CL_PLATFORM_VERSION,
                                               sizeof(buf),
                                               buf,
                                               NULL);
		if (status != CL_SUCCESS)
		{
			FILL_ERRATA(err,"Couldn't get platform parameter: CL_PLATFORM_VERSION, reason: " << appsdk::getOpenCLErrorCodeStr(status));
			return Error;
		}
		else
		{
			_availablePlatforms[i]._supportedCLVersion = std::string(buf);
		}

		//3.Platform Name
		status = clGetPlatformInfo(availablePlatforms[i],
                                               CL_PLATFORM_NAME,
                                               sizeof(buf),
                                               buf,
                                               NULL);
		if (status != CL_SUCCESS)
		{
			FILL_ERRATA(err,"Couldn't get platform parameter: CL_PLATFORM_NAME, reason: " << appsdk::getOpenCLErrorCodeStr(status));
			return Error;
		}
		else
		{
			_availablePlatforms[i]._platformName= std::string(buf);
		}

		//4.Platform Profile
		status = clGetPlatformInfo(availablePlatforms[i],
                                               CL_PLATFORM_PROFILE,
                                               sizeof(buf),
                                               buf,
                                               NULL);
		if (status != CL_SUCCESS)
		{
			FILL_ERRATA(err,"Couldn't get platform parameter: CL_PLATFORM_PROFILE, reason: " << appsdk::getOpenCLErrorCodeStr(status));
			return Error;
		}
		else
		{
			_availablePlatforms[i]._platformProfile = std::string(buf);
		}

		//5.Platform Extensions
		status = clGetPlatformInfo(availablePlatforms[i],
                                               CL_PLATFORM_EXTENSIONS,
                                               sizeof(buf),
                                               buf,
                                               NULL);
		if (status != CL_SUCCESS)
		{
			FILL_ERRATA(err,"Couldn't get platform parameter: CL_PLATFORM_EXTENSIONS, reason: " << appsdk::getOpenCLErrorCodeStr(status));
			return Error;
		}
		else
		{
			_availablePlatforms[i]._platformExtensions = std::string(buf);
		}

		//Set up devices per platform
		if (_availablePlatforms[i].fillDevices(err) == Error)
			return Error;
	}

	return Success;

}

/**********************************************************************
* Implementation of class CLPlatform
***********************************************************************/

#define GET_DEVICE_PROPERTY(param,target_buf,status) {status = clGetDeviceInfo(_deviceId,param,sizeof(target_buf),target_buf,NULL);  } 

/**Utility function to check whether device has extension - TODO - Add to interface, may be useful*/
cl_int hasExtension(const cl_device_id deviceid,const std::string& extension, bool& result)
{
	cl_int status;
	char value[1024];
	const cl_device_id _deviceId = deviceid; //For the macro to be happy...
	result = false;
	//Device Extenstions
	GET_DEVICE_PROPERTY(CL_DEVICE_EXTENSIONS,value,status);
	if (status != CL_SUCCESS)
		return status;
	std::string deviceExtensions = std::string((char*)value);
	if (deviceExtensions.find(extension) != string::npos)
		result = true;
	return status;
}

/** Creates an object that encapsulates OpenCL context for specified device associated with this platform
* @param props Context properties
* @param deviceIndex Index of device within the platform, to create context for
* @param [out]outputContext Created OpenCL context
* @param [out]err Error info if error occurred
* @return Result of the operation: Success or Failure
* @see cl_context in OpenCL specification
* @see cl_context_properties in OpenCL specification
*/
Result CLPlatform::createCLContext(cl_context_properties* props,int deviceIndex,cl_context& outputContext,Errata& err) const
{
	cl_int status;
	if (deviceIndex >= _numOfDevices || deviceIndex < 0)
	{
		FILL_ERRATA(err,"Device Index out of range ");
		return Error;
	}
	outputContext = clCreateContext(props,1,&(_deviceIds[deviceIndex]),NULL,NULL,&status);
	if (status != CL_SUCCESS)
	{
		FILL_ERRATA(err,"Couldn't get context for platform: " << appsdk::getOpenCLErrorCodeStr(status) );
		return Error;
	}
	return Success;
}


/**Private function - Sets up devices in Platform*/
Result CLPlatform::fillDevices(Errata& err)
{
	// Get device count
	
	cl_int status = clGetDeviceIDs(_platformId, CL_DEVICE_TYPE_ALL, 0, NULL, &_numOfDevices);
    if(status != CL_SUCCESS)
    {
		FILL_ERRATA(err,"Couldn't get device ids for platform name:" << _platformName << ", reason: " << appsdk::getOpenCLErrorCodeStr(status));
		  return Error;
    }

	_deviceIds.reset(new cl_device_id[_numOfDevices]);
	_deviceInfo.reset(new boost::shared_ptr<CLDevice>[_numOfDevices]);

	status = clGetDeviceIDs(_platformId, CL_DEVICE_TYPE_ALL, _numOfDevices, _deviceIds.get(),NULL);
    if(status != CL_SUCCESS)
    {
         FILL_ERRATA(err,"Couldn't get device ids for platform name:" << _platformName << ", reason: " << appsdk::getOpenCLErrorCodeStr(status));
		 return Error;
    }

	 ////Setup the device
    for(cl_uint i = 0; i < _numOfDevices; i++)
		_deviceInfo[i].reset(new CLDevice(_deviceIds[i],*this)); //(new CLDevice(_deviceIds[i].get(),_platformId)); 
		
	return Success;
}

/**Destructor*/
CLPlatform::~CLPlatform()
{
	
}


