/**
 * @file CLExecutionContext.cpp
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
 * Implementation of class CLExecutionContext and its related classes.
 *
 */

#include <fstream>
#include <ctime>
#include <OpenCLUtils\CLExecutionContext.h>
#include <OpenCLUtils\CLInterface.h>


using namespace CLRayTracer::OpenCLUtils;
using namespace CLRayTracer::Common;



/*****************************************************************************************
* Implementation of class CLKernelArgument
******************************************************************************************/

/**Dummy arg constant*/
const char* CLKernelArgument::DummyArg = "@@@DUMMY@@@";

/*****************************************************************************************
* Implementation of class CLKernel
******************************************************************************************/

/**Constructor
* @param kernel The raw OpenCL kernel object
*/
CLKernel::CLKernel(cl_kernel kernel):_clKernel(kernel)
{}

/**Destructor*/
CLKernel::~CLKernel()
{
	if (_clKernel != NULL)
		clReleaseKernel(_clKernel);
}

/**Sets argument value for this kernel
* @param argObj Object that encapsulates the argument value
* @param Index at which the argument should be set
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLKernel::setKernelArgument(const CLKernelArgument& argObj,cl_uint index,Errata& err)
{
	cl_uint dataSize = argObj.getDataSize();
	void* data = argObj.getVoidPtr();
	cl_int status = clSetKernelArg(_clKernel, index,dataSize ,data);
	if (status != CL_SUCCESS)
	{
		FILL_ERRATA(err,"Couldn't set kernel argument: " << appsdk::getOpenCLErrorCodeStr(status) );
		return Error;
	}
	return Success;
}

/*****************************************************************************************
* Implementation of class CLProgram
******************************************************************************************/

/* Utility function: convert kernel file into a string */
Result convertToString(const char *filename, std::string& s,Errata& err)
{
	size_t size;
	char*  str;
	std::fstream f(filename, (std::fstream::in | std::fstream::binary));

	if(f.is_open())
	{
		size_t fileSize;
		f.seekg(0, std::fstream::end);
		size = fileSize = (size_t)f.tellg();
		f.seekg(0, std::fstream::beg);
		str = new char[size+1];
		if(!str)
		{
			f.close();
			FILL_ERRATA(err,"Couldn't allocate memory for loading kernel");
			return Error;
		}

		f.read(str, fileSize);
		f.close();
		str[size] = '\0';
		s = str;
		delete[] str;
		return Success;
	}
	FILL_ERRATA(err,"Couldn't load kernel from file:" << filename);
	return Error;
}

/**Constructor
* @param context OpenCL execution context
*/
CLProgram::CLProgram(const CLExecutionContext& context):_context(context),_clProgram(NULL)
{

}

/**Destructor*/
CLProgram::~CLProgram()
{
	if (_clProgram != NULL)
		clReleaseProgram(_clProgram);
}

/**Loads OpenCL program (cl file) and compiles the source. Creates a log file in case error occurred.
* @param fileName file name and path
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLProgram::loadAndCompile(const std::string& fileName, Errata& err)
{
	return loadAndCompile(fileName,"",err);
}

/**Loads OpenCL program (cl file) and compiles the source. Creates a log file in case error occurred.
* @param fileName file name and path
* @param compilerParams string that contains compiler options for the compiler
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLProgram::loadAndCompile(const std::string& fileName, const std::string& compilerParams, Errata& err)
{
	std::string programSource;
	if (convertToString(fileName.c_str(),programSource,err) != Success)
		return Error;
	return compile(programSource,compilerParams,err);
}

/**Compiles program stored in a string. Creates a log file in case error occurred.
* @param kernelStr the string that contains the program
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLProgram::compile(const std::string& kernel, Errata& err)
{
	return compile(kernel,"",err);
}

/**Compiles program stored in a string. Creates a log file in case error occurred
* @param kernelStr the string that contains the program
* @param compilerParams string that contains compiler options for the compiler
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLProgram::compile(const std::string& kernel, const std::string& compilerParams, Errata& err)
{
	if (!_context._initialized)
	{
		if (_context.initialize(err) != Success)
			return Error;
	}

	const char *source = kernel.c_str();
	size_t sourceSize[] = {strlen(source)};
	cl_int status;
	_clProgram = clCreateProgramWithSource(_context._clContext, 1, &source, sourceSize,&status);
	if (status != CL_SUCCESS)
	{
		FILL_ERRATA(err,"Couldn't create program: " << appsdk::getOpenCLErrorCodeStr(status) );
		return Error;
	}
	
	boost::shared_array<cl_device_id> devices(new cl_device_id[1]);
	devices[0] = _context._device.getCLDeviceId();

	status=clBuildProgram(_clProgram, 1,devices.get(),compilerParams.empty() ? NULL : compilerParams.c_str(),NULL,NULL);
	if (status != CL_SUCCESS)
	{
		size_t len;
		clGetProgramBuildInfo(_clProgram, devices[0], CL_PROGRAM_BUILD_LOG, NULL, NULL, &len);
		boost::shared_array<char> log(new char[len]);
		clGetProgramBuildInfo(_clProgram, devices[0], CL_PROGRAM_BUILD_LOG, len, log.get(), NULL);

		FILL_ERRATA(err,"Couldn't build program: " << appsdk::getOpenCLErrorCodeStr(status) << std::endl << "Build log: " << std::endl << std::string(log.get()));
		time_t time;
		::time(&time);
		std::stringstream fname;
		fname << "build_" << time << ".log";
		std::ofstream o(fname.str());
		o << log.get();
		o.close();
		return Error;
	}

	return Success;
}

/**Creates Kernel object from program, by kernel name
* @param kernelName Desired kernel name
* @param [out] kernelObj Resulting kernel object
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLProgram::getKernel(const std::string& kernelName,CLKernel*& kernelObj, Errata& err)
{
	cl_int status;
	cl_kernel kernel = clCreateKernel(_clProgram,kernelName.c_str(), &status);
	if (status != CL_SUCCESS)
	{
		FILL_ERRATA(err,"Couldn't create kernel object: " << appsdk::getOpenCLErrorCodeStr(status) );
		return Error;
	}
	kernelObj = new CLKernel(kernel);
	return Success;
}

/*****************************************************************************************
* Implementation of class CLExecutionContext
******************************************************************************************/

/**Constructor
* @param device Object that encapsulate the OpenCL device in use
* @param contextProperties[optional] The properties of context to be created
* @see class CLDevice
* @see cl_context_properties in OpenCL specification
*/
CLExecutionContext::CLExecutionContext(const CLDevice& device,const cl_context_properties* props):_platform(device.getOwnerPlatform()),_device(device),_clContext(NULL),_clCommandQueue(NULL),_initialized(false),_contextProperties(props)
{
}

/**Destructor*/
CLExecutionContext::~CLExecutionContext()
{
	if (_clContext != NULL)
		clReleaseContext(_clContext);
	if (_clCommandQueue != NULL)
		clReleaseCommandQueue(_clCommandQueue);
}

/** Initialize the context object - Should be called once per instance
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLExecutionContext::initialize(Errata& err) const
{
	if (_initialized)
	{
		FILL_ERRATA(err,"Object already initialized");
		return Error;
	}

	//get selected device
	int deviceIndex = 0;
	for (; deviceIndex < _platform.getNumOfDevices(); deviceIndex++)
	{
		if (_platform.getDeviceByIndex(deviceIndex) == &_device)
			break;
	}

	//Context
	if (Success != _platform.createCLContext(const_cast<cl_context_properties*>(_contextProperties),deviceIndex,_clContext,err))
		return Error;
	//Command Queue
	cl_int status;
	_clCommandQueue = clCreateCommandQueue(_clContext,_device.getCLDeviceId(), 0, &status);
	if (status != CL_SUCCESS)
	{
		FILL_ERRATA(err,"Couldn't create command queue for device: " << appsdk::getOpenCLErrorCodeStr(status) );
		return Error;
	}
	_initialized = true;
	return Success;
}

/** Create OpenCL memory object
* @param access Access mode
* @param hostPtrOpts Host pointer usage
* @param size Desired size of the buffer
* @param [out]output Pointer to the created device buffer
* @param err Error info
* @param [optional]hostPtr Host pointer, if used
* @return Result of the operation: Success or failure
* @see OpenCL Memory Access modes
* @see OpenCL Host Pointer Usage
*/
Result CLExecutionContext::createDeviceBuffer(CLBufferFlags::CLBufferAccess access,CLBufferFlags::CLBufferHostPtrOptions hostPtrOpts,size_t size,cl_mem& output, Errata& err,void* hostPtr) const
{
	cl_mem_flags accessFlag = access == CLBufferFlags::ReadWrite ? CL_MEM_READ_WRITE : (access == CLBufferFlags::ReadOnly ? CL_MEM_READ_ONLY : CL_MEM_WRITE_ONLY);
	cl_mem_flags hostPtrFlag = 0;
	if (hostPtrOpts != CLBufferFlags::None)
		hostPtrFlag = hostPtrOpts == CLBufferFlags::Use ? CL_MEM_USE_HOST_PTR : (hostPtrOpts == CLBufferFlags::Alloc? CL_MEM_ALLOC_HOST_PTR : CL_MEM_COPY_HOST_PTR);
	if ((hostPtrOpts == CLBufferFlags::Use || hostPtrOpts == CLBufferFlags::Copy) && !hostPtr)
	{
		FILL_ERRATA(err,"Host ptr must not be null with given hostPtrOpts");
		return Error;
	}

	cl_int status = 0;
	output = clCreateBuffer(_clContext, accessFlag|hostPtrFlag,size,hostPtr,&status);
	if (status != CL_SUCCESS)
	{
		FILL_ERRATA(err,"Couldn't create CL Buffer: " << appsdk::getOpenCLErrorCodeStr(status) );
		return Error;
	}

	return Success;
}

/** Create OpenCL memory object
* @param access Access mode
* @param size Desired size of the buffer
* @param [out]output Pointer to the created device buffer
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLExecutionContext::createDeviceBuffer(CLBufferFlags::CLBufferAccess access,size_t size,cl_mem& output, Errata& err) const
{
	return createDeviceBuffer(access,CLBufferFlags::None,size,output,err);
}

/**Reads OpenCL device memory contents and writes them into host memory buffer
* @param buffer OpenCL device buffer
* @param outputBuffer Host memory pointer to destination, where memory should be copied
* @param bufferSize The size of the buffer that should be copied
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLExecutionContext::enqueueReadBuffer(const cl_mem& buffer,void* outputBuffer, size_t bufferSize,Errata& err) const
{
	cl_int status = clEnqueueReadBuffer(_clCommandQueue, buffer, CL_TRUE, 0, bufferSize, outputBuffer, 0, NULL, NULL);
	if (status != CL_SUCCESS)
	{
		FILL_ERRATA(err,"clEnqueueReadBuffer: " << appsdk::getOpenCLErrorCodeStr(status) );
		return Error;
	} 
	return Success;
}

/**Reads OpenCL device memory contents and writes them into host memory buffer
* @param buffer OpenCL device buffer
* @param outputBuffer Host memory pointer to destination, where memory should be copied
* @param offset Offset index (In bytes), from which the copying should start
* @param bufferSize The size of the buffer that should be copied
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLExecutionContext::enqueueReadBuffer(const cl_mem& buffer,void* outputBuffer,size_t offset, size_t bufferSize,Errata& err) const
{
	cl_int status = clEnqueueReadBuffer(_clCommandQueue, buffer, CL_TRUE, offset, bufferSize, outputBuffer, 0, NULL, NULL);
	if (status != CL_SUCCESS)
	{
		FILL_ERRATA(err,"clEnqueueReadBuffer: " << appsdk::getOpenCLErrorCodeStr(status) );
		return Error;
	} 
	return Success;
}

/**Fills OpenCL device memory with defined pattern, same principle as Win32 memset
* @param buffer OpenCL device buffer
* @param pattern The pattern that should be filled
* @param bufferSize The size of the buffer that should be copied. Must be a multiple of patternSize
* @param patternSize patternSize The size of pattern - According to OpenCL specs can be {1, 2, 4, 8, 16, 32, 64, 128}
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLExecutionContext::enqueueFillBuffer(const cl_mem& buffer,void* pattern, size_t bufferSize, size_t patternSize, Errata& err) const
{
	CLEvent evt;
	cl_int status = clEnqueueFillBuffer(_clCommandQueue,buffer,pattern,patternSize,0,bufferSize,0,0,&evt.getCLEvent());
	if (status != CL_SUCCESS)
	{
		FILL_ERRATA(err,"clEnqueueFillBuffer: " << appsdk::getOpenCLErrorCodeStr(status) );
		return Error;
	} 

	if (Success != flushQueue(err))
		return Error;

	if (Success != evt.wait(err))
		return Error;

	return Success;
}

/**Fills OpenCL device memory with contents of host memory buffer
* @param buffer Host buffer - The source
* @param outputBuffer OpenCL device buffer - The destination
* @param bufferSize The size of the buffer that should be copied
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLExecutionContext::enqueueWriteBuffer(void* buffer,const cl_mem& outputBuffer, size_t bufferSize,Errata& err) const
{
	cl_int status = clEnqueueWriteBuffer(_clCommandQueue, outputBuffer, CL_TRUE, 0, bufferSize, buffer, 0, NULL, NULL);
	if (status != CL_SUCCESS)
	{
		FILL_ERRATA(err,"clEnqueueWriteBuffer: " << appsdk::getOpenCLErrorCodeStr(status) );
		return Error;
	} 
	return Success;
}

/**Copies OpenCL device memory into destination memory on device
* @param buffer Source OpenCL device buffer
* @param outputBuffer Destination OpenCL device buffer
* @param bufferSize The size of the buffer that should be copied
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLExecutionContext::enqueueCopyBuffer(const cl_mem& buffer,const cl_mem& outputBuffer, size_t bufferSize,Errata& err) const
{
	CLEvent evt;
	cl_int status = clEnqueueCopyBuffer(_clCommandQueue,buffer,outputBuffer,0,0,bufferSize,0,0,&(evt.getCLEvent()));
	if (status != CL_SUCCESS)
	{
		FILL_ERRATA(err,"clEnqueueCopyBuffer: " << appsdk::getOpenCLErrorCodeStr(status) );
		return Error;
	} 

	if (Success != flushQueue(err))
		return Error;

	if (Success != evt.wait(err))
		return Error;

	return Success;
}

/**Enqueues kernel execution on underlying OpenCL command queue - In simple terms, executes the kernel
* @param Object that encapsulates the kernel to execute
* @param Object that encapsulates data about dimensions and syncronization events for kernel execution
* @param err Error info
* @return Result of the operation: Success or failure
*/	
Result CLExecutionContext::enqueueKernel(CLKernel& kernel,CLKernelExecuteParams& params, Errata& err) const
{

	cl_int status = clEnqueueNDRangeKernel(_clCommandQueue,kernel._clKernel,
										   params.globalWorkDimension->workDimensions,
										   params.globalWorkOffset ? params.globalWorkOffset->dimensionValues.get() : NULL,
										   params.globalWorkDimension->dimensionValues.get(),
										   params.localWorkDimension->dimensionValues.get(),
										   params.eventWaitList.size(),
										   params.eventWaitList.size() == 0 ? NULL : &params.eventWaitList[0],
										   params.event == NULL ? NULL :&(params.event->getCLEvent()));
	if (status != CL_SUCCESS)
	{
		FILL_ERRATA(err,"enqueueKernel: " << appsdk::getOpenCLErrorCodeStr(status) );
		return Error;
	} 
	return Success;
}

/** Issues all queues operations to the device. Encapsulates the clFlush OpenCL method
* @param err Error info
* @return Result of the operation: Success or failure
* @see clFlush
*/
Result CLExecutionContext::flushQueue(Errata& err) const
{
	cl_int status = clFlush(_clCommandQueue);
	if (status != CL_SUCCESS)
	{
		FILL_ERRATA(err,"clFlush error: " << appsdk::getOpenCLErrorCodeStr(status) );
		return Error;
	} 
	return Success;
}

/** Blocks until all commands on the queue are completed. Encapsulates the clFinish OpenCL method
* @param err Error info
* @return Result of the operation: Success or failure
* @see clFinish
*/
Result CLExecutionContext::finishQueue(Errata& err) const
{
	cl_int status = clFinish(_clCommandQueue);
	if (status != CL_SUCCESS)
	{
		FILL_ERRATA(err,"clFinish error: " << appsdk::getOpenCLErrorCodeStr(status) );
		return Error;
	} 
	return Success;
}

/** Retrieves the work group size and number of work groups for Maximal Launch - that is, the exact local workgroup size
*   and exact number of workgroups to fill the device (Number of multiprocessors). This is useful for Persistent Thread
*   programming style - That's where the term "Maximal Launch" comes from
* @param Kernel The kernel to check
* @param [out]computeDevices Will contain the number of multiprocessors of the used OpenCL device
* @param [out]preferredWorkgroupSize The wavefront size for the used OpenCL device, which is also the preferred
*                                    workgroup size to achieve the Maximal Launch
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLExecutionContext::getMaximalLaunchExecParams(const CLKernel& kernel, size_t& computeDevices, size_t& preferredWorkgroupSize,Errata& err) const
{
	cl_uint numOfComputeDevices;
	if (Success != _device.getExecutionInfo().getMaxComputeUnits(numOfComputeDevices,err))
		return Error;
	preferredWorkgroupSize = 0;
	cl_int status = clGetKernelWorkGroupInfo(kernel._clKernel,_device.getCLDeviceId(),CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,sizeof(size_t),&preferredWorkgroupSize,NULL);
	if (CL_SUCCESS != status)
	{
		FILL_ERRATA(err,"Failed to query kernel for CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE: " <<  appsdk::getOpenCLErrorCodeStr(status) );
		return Error;
	}
	computeDevices = numOfComputeDevices;	
	return Success;
}

/**Retrieves the max allowed workgroup size for the specified kernel
* @param kernel The kernel to check
* @param [out]maxWGSize Will contain the resulting maximal workgroup size for the kernel
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLExecutionContext::getMaxWorkgroupForKernel(const CLKernel& kernel, size_t& maxWGSize,Errata& err) const
{
	 size_t result;
	 cl_int status = clGetKernelWorkGroupInfo(kernel._clKernel,_device.getCLDeviceId(),CL_KERNEL_WORK_GROUP_SIZE,sizeof(size_t),&result,0);
	 if (CL_SUCCESS != status)
	 {
		FILL_ERRATA(err,"Failed to query kernel for CL_KERNEL_WORK_GROUP_SIZE: " <<  appsdk::getOpenCLErrorCodeStr(status) );
		return Error;
	 }
	 maxWGSize = result;
	 return Success;
}

/** Creates object that encapsulates OpenCL synchronization event
* @param [out]outEvt The created event
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLExecutionContext::createEvent(boost::shared_ptr<CLEvent>& outEvt,Errata& err) const
{
	outEvt = NULL;
	cl_int status = 0;
	cl_event evt = clCreateUserEvent(_clContext,&status);
	if (status != CL_SUCCESS)
	{
		FILL_ERRATA(err,"Couldn't create user event: " << appsdk::getOpenCLErrorCodeStr(status) );
		return Error;
	}
	outEvt.reset(new CLEvent(evt));
	return Success;
}

/** Creates multiple objects that encapsulates OpenCL synchronization event
* @param count Count of desired events
* @param events Vector that shall be filled with created events
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLExecutionContext::createEvents(int count,std::vector<boost::shared_ptr<CLEvent> >& events,Errata& err) const
{
	for(int i = 0; i < count; i++)
	{
		boost::shared_ptr<CLEvent> e;
		if (Success != createEvent(e,err))
		{
			events.clear();
			return Error;
		}
		events.push_back(e);
	}
	return Success;
}

/*****************************************************************************************
* Implementation of class CLEvent
******************************************************************************************/

/**Destructor*/
CLEvent::~CLEvent()
{
	reset();
}

/**Resets state of the event, should be called before reusing the event object
*@return 
*/
void CLEvent::reset()
{
	if (clEvent != NULL)
		clReleaseEvent(clEvent);
	clEvent = NULL;
}

/** Wait for OpenCL operation that should be synchronized by this event to complete
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLEvent::wait(Errata& err) 
{
	if (clEvent == NULL)
		return Success;
	cl_uint status = clWaitForEvents(1,&clEvent);
	if (status != CL_SUCCESS)
	{
		FILL_ERRATA(err,"Wait for event generated error: " << appsdk::getOpenCLErrorCodeStr(status) );
		return Error;
	}
	reset();
	return Success;
}

/** Wait for OpenCL operation that should be synchronized by all events in event list to complete
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLEvent::wait(std::vector<boost::shared_ptr<CLEvent> > events, Errata& err)
{
	boost::shared_array<cl_event> evts(new cl_event[events.size()]);
	for(int i = 0; i <events.size(); i++)
		evts[i] = events[i]->getCLEvent();
	cl_uint status = clWaitForEvents(events.size(),evts.get());
	if (status != CL_SUCCESS)
	{
		FILL_ERRATA(err,"Wait for event generated error: " << appsdk::getOpenCLErrorCodeStr(status) );
		return Error;
	}
	for(int i = 0; i <events.size(); i++)
		events[i]->reset();

	return Success;
}


	