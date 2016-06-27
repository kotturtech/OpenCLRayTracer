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
 * Part of the OpenCL wrapper framework. Provides 
 * device and platform information retrieval and encapsulations of OpenCL Device and Platform
 *
 */

#ifndef CL_RT_CLINTERFACE_H
#define CL_RT_CLINTERFACE_H

#include <CL/cl.h>
#include <OpenCLUtils\APIErrorCheck.h>
#include <boost/smart_ptr.hpp>
#include <common/Errata.h>

namespace CLRayTracer
{
	namespace OpenCLUtils
	{

		/*************************************************************
		* Internal utilities, forward declarations, etc
		**************************************************************/
		class CLContext;
		class CLPlatform;
		class CLDevice;
		class CLExecutionContext;
		std::ostream& operator<<(std::ostream& o,const CLPlatform& p);
		std::ostream& operator<<(std::ostream& o,const CLDevice& p);

#define DEVICE_STRING_PROPERTY_GETTER(getterName,param) inline Common::Result getterName(std::string& value, Common::Errata& err) const \
		{																									\
		char buf[1024];																					\
		cl_int status = clGetDeviceInfo(_deviceId,param,sizeof(buf),buf,NULL);							\
		if (status != CL_SUCCESS)																		\
		{                                                                                               \
		FILL_ERRATA(err,"Getting device property: "<< std::string(#param) <<" failed!, reason: " << appsdk::getOpenCLErrorCodeStr(status));\
		return Common::Error;                                                                               \
		}																								\
		\
		value = std::string(buf);																		\
		return Common::Success;																					\
		}																									\



#define DEVICE_PROPERTY_GETTER(getterName,param,type) inline Common::Result getterName(type& value, Common::Errata& err) const {char buf[1024];cl_int status = clGetDeviceInfo(_deviceId,param,sizeof(buf),buf,NULL);	if (status != CL_SUCCESS){FILL_ERRATA(err,"Getting device property: "<< std::string(#param) <<" failed!, reason: " << appsdk::getOpenCLErrorCodeStr(status)); return Common::Error;}value = *((type*)buf);return Common::Success;}																	

		/***************************************************************
		* API classes
		****************************************************************/
		/**class CLDevice encapsulates an OpenCL device*/
		class CLDevice
		{
			friend class CLPlatform;
		public:

			/**class ImageSupport provides group of OpenCL device properties related to OpenCL image support*/
			class ImageSupport
			{
			public:
				/**Constructor*/
				ImageSupport(cl_device_id deviceId):_deviceId(deviceId){}
				
				/** Retrieves property of OpenCL Device: Whether Images are supported by device
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_IMAGE_SUPPORT device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getImageSupport,CL_DEVICE_IMAGE_SUPPORT,cl_bool);
				
				/** Retrieves property of OpenCL Device: Max height of 2D image in pixels. 
				* The minimum value is 8192 if CL_DEVICE_IMAGE_SUPPORT is CL_TRUE. 
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_IMAGE2D_MAX_HEIGHT device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getImage2DMaxHeight,CL_DEVICE_IMAGE2D_MAX_HEIGHT,size_t);
				
				/** Retrieves property of OpenCL Device: Max width of 2D image in pixels. 
				*   The minimum value is 8192 if CL_DEVICE_IMAGE_SUPPORT is CL_TRUE.
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_IMAGE2D_MAX_WIDTH device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getImage2DMaxWidth,CL_DEVICE_IMAGE2D_MAX_WIDTH,size_t);
				
				/** Retrieves property of OpenCL Device: Max height of 3D image in pixels.
				* The minimum value is 2048 if CL_DEVICE_IMAGE_SUPPORT is CL_TRUE.
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_IMAGE3D_MAX_HEIGHT device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getImage3DMaxHeight,CL_DEVICE_IMAGE3D_MAX_HEIGHT,size_t);
				
				/** Retrieves property of OpenCL Device:Max width of 3D image in pixels.
				* The minimum value is 2048 if CL_DEVICE_IMAGE_SUPPORT is CL_TRUE.
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_IMAGE3D_MAX_WIDTH device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getImage3DMaxWidth,CL_DEVICE_IMAGE3D_MAX_WIDTH,size_t);
				
				/** Retrieves property of OpenCL Device: Max depth of 3D image in pixels.
				*   The minimum value is 2048 if CL_DEVICE_IMAGE_SUPPORT is CL_TRUE.
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_IMAGE3D_MAX_DEPTH device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getImage3DMaxDepth,CL_DEVICE_IMAGE3D_MAX_DEPTH,size_t);
				
				/** Retrieves property of OpenCL Device: Max number of simultaneous image objects that can be read by a kernel. 
				*   The minimum value is 128 if CL_DEVICE_IMAGE_SUPPORT is CL_TRUE.
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_MAX_READ_IMAGE_ARGS device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getMaxReadImageArgs,CL_DEVICE_MAX_READ_IMAGE_ARGS,cl_uint);
				
				/** Retrieves property of OpenCL Device: Max number of simultaneous image objects that can be written to by a kernel. 
				* The minimum value is 8 if CL_DEVICE_IMAGE_SUPPORT is CL_TRUE.
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_MAX_WRITE_IMAGE_ARGS device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getMaxWriteImageArgs,CL_DEVICE_MAX_WRITE_IMAGE_ARGS,cl_uint);
				
				/** Retrieves property of OpenCL Device: Maximum number of samplers that can be used in a kernel.
				 *  The minimum value is 16 if CL_DEVICE_IMAGE_SUPPORT is CL_TRUE. 
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_MAX_SAMPLERS device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getMaxSamplers,CL_DEVICE_MAX_SAMPLERS,cl_uint);
			private:
				cl_device_id _deviceId;
			};

			/**Class MemoryInfo groups device properties related to memory*/
			class MemoryInfo
			{
			public:
				/**Constructor*/
				MemoryInfo(cl_device_id deviceId):_deviceId(deviceId){}
				/** Retrieves property of OpenCL Device: Size of local memory in bytes.
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_LOCAL_MEM_SIZE device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getLocalMemSize,CL_DEVICE_LOCAL_MEM_SIZE,cl_ulong);
				
				/** Retrieves property of OpenCL Device: Size of global memory in bytes.
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_GLOBAL_MEM_SIZE device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getGlobalMemSize,CL_DEVICE_GLOBAL_MEM_SIZE,cl_ulong);
				
				/** Retrieves property of OpenCL Device: Size of global memory cache in bytes.
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_GLOBAL_MEM_CACHE_SIZE device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getGlobalMemCacheSize,CL_DEVICE_GLOBAL_MEM_CACHE_SIZE,cl_ulong);
				
				/** Retrieves property of OpenCL Device: Max size of memory object allocation in bytes
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_MAX_MEM_ALLOC_SIZE device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getMaxMemAllocSize,CL_DEVICE_MAX_MEM_ALLOC_SIZE,cl_ulong);
				
				/** Retrieves property of OpenCL Device: Size of global memory cache line in bytes.
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getGlobalMemCacheLineSize,CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE,cl_uint);
				
				/** Retrieves property of OpenCL Device: Type of local memory supported.
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_LOCAL_MEM_TYPE device parameter in OpenCL specification
				* @see cl_device_local_mem_type in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getLocalMemType,CL_DEVICE_LOCAL_MEM_TYPE,cl_device_local_mem_type);
				
				/** Retrieves property of OpenCL Device: Type of global memory cache supported.
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_GLOBAL_MEM_CACHE_TYPE device parameter in OpenCL specification
				* @see cl_device_mem_cache_type in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getGlobalMemCacheType,CL_DEVICE_GLOBAL_MEM_CACHE_TYPE,cl_device_mem_cache_type);
				
				/** Retrieves property of OpenCL Device: Is CL_TRUE if the device implements error correction for the memories, caches, registers etc. in the device.
				* Is CL_FALSE if the device does not implement error correction. This can be a requirement for certain clients of OpenCL.
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_ERROR_CORRECTION_SUPPORT device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getErrorCorrectionSupport,CL_DEVICE_ERROR_CORRECTION_SUPPORT,cl_bool);
			private:
				cl_device_id _deviceId;
			};

			/**Class ExecutionInfo groups device properties related to kernel execution*/
			class ExecutionInfo
			{
			public:
				/**Constructor*/
				ExecutionInfo(cl_device_id deviceId):_deviceId(deviceId){}

				/** Retrieves property of OpenCL Device: Max size in bytes of the arguments that can be passed to a kernel
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_MAX_PARAMETER_SIZE device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getMaxParameterSize,CL_DEVICE_MAX_PARAMETER_SIZE,size_t);
				
				/** Retrieves property of OpenCL Device: Resolution of device timer in nanoseconds
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_PROFILING_TIMER_RESOLUTION device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getProfilingTimerResolution,CL_DEVICE_PROFILING_TIMER_RESOLUTION,size_t);
				
				/** Retrieves property of OpenCL Device: Maximum configured clock frequency of the device in MHz.
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_MAX_CLOCK_FREQUENCY device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getMaxClockFrequency,CL_DEVICE_MAX_CLOCK_FREQUENCY,cl_uint);
				
				/** Retrieves property of OpenCL Device: The number of parallel compute cores on the OpenCL device.
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_MAX_COMPUTE_UNITS device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getMaxComputeUnits,CL_DEVICE_MAX_COMPUTE_UNITS,cl_uint);
				
				/** Retrieves property of OpenCL Device: Max number of arguments declared with the __constant qualifier in a kernel
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_MAX_CONSTANT_ARGS device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getMaxConstantArgs,CL_DEVICE_MAX_CONSTANT_ARGS,cl_uint);
				
				/** Retrieves property of OpenCL Device: Describes the alignment in bits of the base address of any allocated memory object.
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_MEM_BASE_ADDR_ALIGN device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getMemBaseAddrAlign,CL_DEVICE_MEM_BASE_ADDR_ALIGN,cl_uint);
				
				/** Retrieves property of OpenCL Device: The smallest alignment in bytes which can be used for any data type
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getMinDataTypeAlignSize,CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE,cl_uint);
				
				/** Retrieves property of OpenCL Device: Max size in bytes of a constant buffer allocation
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getMaxConstantBufferSize,CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE,cl_ulong);
				
				/** Retrieves property of OpenCL Device: Describes the command-queue properties supported by the device.
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_QUEUE_PROPERTIES device parameter in OpenCL specification
				* @see cl_command_queue_properties in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getDeviceQueueProperties,CL_DEVICE_QUEUE_PROPERTIES,cl_command_queue_properties);
			private:
				cl_device_id _deviceId;
			};

			/**Class WorkGroupDimensions groups device properties related to work group dimensions constrainsts on this device*/
			class WorkGroupDimensions
			{
			public:
				/**Constructor*/
				WorkGroupDimensions(cl_device_id deviceId):_deviceId(deviceId){}

				/** Retrieves property of OpenCL Device: Maximum number of work-items in a work-group executing a kernel using the data parallel execution model. 
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_MAX_WORK_GROUP_SIZE device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getMaxWorkGroupSize,CL_DEVICE_MAX_WORK_GROUP_SIZE,size_t);
				
				/** Retrieves property of OpenCL Device: Maximum dimensions that specify the global and local work-item IDs used by the data parallel execution model.
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getMaxWorkItemDimensions,CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,cl_uint);
				
				/** Retrieves property of OpenCL Device: Maximum number of work-items that can be specified in each dimension of the work-group
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_MAX_WORK_ITEM_SIZES device parameter in OpenCL specification
				*/
				inline Common::Result getMaxWorkItemSizes(boost::shared_array<size_t>& value, Common::Errata& err) const
				{
					char buf[1024];
					cl_int status = clGetDeviceInfo(_deviceId,CL_DEVICE_MAX_WORK_ITEM_SIZES,sizeof(buf),buf,NULL);
					if (status != CL_SUCCESS)
					{
						FILL_ERRATA(err,"Getting device property: "<< std::string("CL_DEVICE_MAX_WORK_ITEM_SIZES") <<" failed!, reason: " << appsdk::getOpenCLErrorCodeStr(status)); 
						return Common::Error;
					}
					cl_uint dimensions = 0;
					if (!getMaxWorkItemDimensions(dimensions,err))
						return Common::Error;
					value.reset(new size_t[dimensions]);
					size_t* output = (size_t*)buf;
					for (cl_uint i = 0; i < dimensions; i++)
					{
						value[i] = *output;
						output++;
					}
					return Common::Success;
				}																	

			private:
				cl_device_id _deviceId;
			};

			/**Class FloatingPointConfig groups device properties related to FP config*/
			class FloatingPointConfig
			{
			public:
				/**Constructor*/
				FloatingPointConfig(cl_device_id deviceId):_deviceId(deviceId){}

				/** Retrieves property of OpenCL Device: Describes the OPTIONAL double precision floating-point capability of the OpenCL device
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_DOUBLE_FP_CONFIG device parameter in OpenCL specification
				* @see cl_device_fp_config in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getFPConfigDouble,CL_DEVICE_DOUBLE_FP_CONFIG,cl_device_fp_config);
				
				/** Retrieves property of OpenCL Device: Describes single precision floating-point capability of the device.
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_SINGLE_FP_CONFIG device parameter in OpenCL specification
				* @see cl_device_fp_config in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getFPConfigSingle,CL_DEVICE_SINGLE_FP_CONFIG,cl_device_fp_config);
				
				/** Retrieves property of OpenCL Device: Describes the OPTIONAL half precision floating-point capability of the OpenCL device.
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_HALF_FP_CONFIG device parameter in OpenCL specification
				* @see cl_device_fp_config in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getFPConfigHalf,CL_DEVICE_HALF_FP_CONFIG,cl_device_fp_config);
			private:

				cl_device_id _deviceId;
			};

			/**Class PreferredVectorWidths groups device properties related to vector widths*/
			class PreferredVectorWidths
			{
			public:
				/**Constructor*/
				PreferredVectorWidths(cl_device_id deviceId):_deviceId(deviceId){}
				
				/** Retrieves property of OpenCL Device: Preferred native vector width size for built-in scalar types that can be put into vectors.
				*   The vector width is defined as the number of scalar elements that can be stored in the vector.
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getPreferredVectorWidthChar,CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR,cl_uint);
				
				/** Retrieves property of OpenCL Device: Preferred native vector width size for built-in scalar types that can be put into vectors.
				*   The vector width is defined as the number of scalar elements that can be stored in the vector.
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getPreferredVectorWidthSort,CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT,cl_uint);
				
				/** Retrieves property of OpenCL Device: Preferred native vector width size for built-in scalar types that can be put into vectors.
				*   The vector width is defined as the number of scalar elements that can be stored in the vector.
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getPreferredVectorWidthInt,CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT,cl_uint);
				
				/** Retrieves property of OpenCL Device: Preferred native vector width size for built-in scalar types that can be put into vectors.
				*   The vector width is defined as the number of scalar elements that can be stored in the vector.
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getPreferredVectorWidthDouble,CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE,cl_uint);
				
				/** Retrieves property of OpenCL Device: Preferred native vector width size for built-in scalar types that can be put into vectors.
				*   The vector width is defined as the number of scalar elements that can be stored in the vector.
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getPreferredVectorWidthLong,CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG,cl_uint);
				
				/** Retrieves property of OpenCL Device: Preferred native vector width size for built-in scalar types that can be put into vectors.
				*   The vector width is defined as the number of scalar elements that can be stored in the vector.
				* @param [out]value The value of property
				* @param [out]err Info about error, if occurred
				* @return Result of the operation: Success or Failure
				* @see CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT device parameter in OpenCL specification
				*/
				DEVICE_PROPERTY_GETTER(getPreferredVectorWidthFloat,CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT,cl_uint);
			private:
				cl_device_id _deviceId;
			};

			/** Returns whether the given device is a GPU
			* @return True if the device is a GPU
			*/
			inline bool isGPU() const 
			{ 
				Common::Errata err;
				cl_device_type flags;
				if (getDeviceTypeFlags(flags,err) != Common::Success)
					throw new Common::CLInterfaceException(err);
				cl_uint result = flags & CL_DEVICE_TYPE_GPU;
				return result != 0;
			} 

			/** Returns object that encapsulates Work Dimension related properties of the device
			* @return Object that encapsulates Work Dimension related properties of the device
			*/
			inline const WorkGroupDimensions& getWorkGroupDimensions() const {return _workGroupDimensions;}
			
			/** Returns object that encapsulates Execution Info related properties of the device
			* @return Object that encapsulates Execution Info related properties of the device
			*/
			inline const ExecutionInfo& getExecutionInfo() const {return _executionInfo;}
			
			/** Returns object that encapsulates memory related properties of the device
			* @return Object that encapsulates memory related properties of the device
			*/
			inline const MemoryInfo& getMemoryInfo() const {return _memoryInfo;}

			/**Returns Platform object that contains this device
			* @return  Platform object that contains this device
			*/
			const CLPlatform& getOwnerPlatform() const {return _ownerPlatform;}
			
			/**Returns OpenCL Device ID for this device
			* @return  OpenCL Device ID for this device
			*/
			const cl_device_id& getCLDeviceId() const {return _deviceId;}

			/** Retrieves property of OpenCL Device: Device Name
			* @param [out]value The value of property
			* @param [out]err Info about error, if occurred
			* @return Result of the operation: Success or Failure
			* @see CL_DEVICE_NAME device parameter in OpenCL specification
			*/
			DEVICE_STRING_PROPERTY_GETTER(getDeviceName,CL_DEVICE_NAME);
			
			/** Retrieves property of OpenCL Device: Device Vendor
			* @param [out]value The value of property
			* @param [out]err Info about error, if occurred
			* @return Result of the operation: Success or Failure
			* @see CL_DEVICE_VENDOR device parameter in OpenCL specification
			*/
			DEVICE_STRING_PROPERTY_GETTER(getDeviceVendor,CL_DEVICE_VENDOR);
			
			/** Retrieves property of OpenCL Device: Device Version
			* @param [out]value The value of property
			* @param [out]err Info about error, if occurred
			* @return Result of the operation: Success or Failure
			* @see CL_DEVICE_VERSION device parameter in OpenCL specification
			*/
			DEVICE_STRING_PROPERTY_GETTER(getDeviceCLVersion,CL_DEVICE_VERSION);
			
			/** Retrieves property of OpenCL Device: Device Extensions
			* @param [out]value The value of property
			* @param [out]err Info about error, if occurred
			* @return Result of the operation: Success or Failure
			* @see CL_DEVICE_EXTENSIONS device parameter in OpenCL specification
			*/
			DEVICE_STRING_PROPERTY_GETTER(getDeviceExtensions,CL_DEVICE_EXTENSIONS);
			
			/** Retrieves property of OpenCL Device: Device Profile
			* @param [out]value The value of property
			* @param [out]err Info about error, if occurred
			* @return Result of the operation: Success or Failure
			* @see CL_DEVICE_PROFILE device parameter in OpenCL specification
			*/
			DEVICE_STRING_PROPERTY_GETTER(getDeviceProfile,CL_DEVICE_PROFILE);
			
			/** Retrieves property of OpenCL Device: Device Vendor Id
			* @param [out]value The value of property
			* @param [out]err Info about error, if occurred
			* @return Result of the operation: Success or Failure
			* @see CL_DEVICE_VENDOR_ID device parameter in OpenCL specification
			*/
			DEVICE_PROPERTY_GETTER(getDeviceVendorId,CL_DEVICE_VENDOR_ID,cl_uint);
			
			/** Retrieves property of OpenCL Device: Device Type
			* @param [out]value The value of property
			* @param [out]err Info about error, if occurred
			* @return Result of the operation: Success or Failure
			* @see CL_DEVICE_TYPE device parameter in OpenCL specification
			* @see cl_device_type in OpenCL specification
			*/
			DEVICE_PROPERTY_GETTER(getDeviceTypeFlags,CL_DEVICE_TYPE,cl_device_type);
			
			/** Retrieves property of OpenCL Device: Device Address Bits (64 or 32 bit)
			* @param [out]value The value of property
			* @param [out]err Info about error, if occurred
			* @return Result of the operation: Success or Failure
			* @see CL_DEVICE_ADDRESS_BITS device parameter in OpenCL specification
			*/
			DEVICE_PROPERTY_GETTER(getDeviceAddressBits,CL_DEVICE_ADDRESS_BITS,cl_uint);
			
			/** Retrieves property of OpenCL Device: Whether device is available
			* @param [out]value The value of property
			* @param [out]err Info about error, if occurred
			* @return Result of the operation: Success or Failure
			* @see CL_DEVICE_AVAILABLE device parameter in OpenCL specification
			*/
			DEVICE_PROPERTY_GETTER(getDeviceAvailable,CL_DEVICE_AVAILABLE,cl_bool);
			
			/** Retrieves property of OpenCL Device: Whether device has a compiler available
			* @param [out]value The value of property
			* @param [out]err Info about error, if occurred
			* @return Result of the operation: Success or Failure
			* @see CL_DEVICE_COMPILER_AVAILABLE device parameter in OpenCL specification
			*/
			DEVICE_PROPERTY_GETTER(getDeviceCompilerAvailable,CL_DEVICE_COMPILER_AVAILABLE,cl_bool);
			
			/** Retrieves property of OpenCL Device: Whether device endian-ness is Little Endian
			* @param [out]value The value of property
			* @param [out]err Info about error, if occurred
			* @return Result of the operation: Success or Failure
			* @see CL_DEVICE_ENDIAN_LITTLE device parameter in OpenCL specification
			*/
			DEVICE_PROPERTY_GETTER(getDeviceIsLittleEndian,CL_DEVICE_ENDIAN_LITTLE,cl_bool);
			
			/** Retrieves property of OpenCL Device: Device Name
			* @param [out]value The value of property
			* @param [out]err Info about error, if occurred
			* @return Result of the operation: Success or Failure
			* @see CL_DEVICE_EXECUTION_CAPABILITIES device parameter in OpenCL specification
			* @see cl_device_exec_capabilities in OpenCL specification
			*/
			DEVICE_PROPERTY_GETTER(getDeviceExecCapabilities,CL_DEVICE_EXECUTION_CAPABILITIES,cl_device_exec_capabilities);

		protected:
			CLDevice(cl_device_id deviceId, const CLPlatform& ownerPlatform):_deviceId(deviceId),_ownerPlatform(ownerPlatform),_imageSupport(ImageSupport(deviceId))
				,_memoryInfo(MemoryInfo(deviceId)),_executionInfo(ExecutionInfo(deviceId)),_workGroupDimensions(WorkGroupDimensions(deviceId)),_floatingPointConfig(FloatingPointConfig(deviceId)),
				_preferredVectorWidths(PreferredVectorWidths(deviceId))
			{}
		private:
			const CLPlatform& _ownerPlatform;
			cl_device_id _deviceId;
			ImageSupport _imageSupport;
			MemoryInfo _memoryInfo;
			ExecutionInfo _executionInfo;
			WorkGroupDimensions _workGroupDimensions;
			FloatingPointConfig _floatingPointConfig;
			PreferredVectorWidths _preferredVectorWidths;
		};

		/**class Platform encapsulates a OpenCL platform*/
		class CLPlatform
		{
			friend std::ostream& operator<<(std::ostream& o,const CLPlatform& p);
			friend class CLInterface;
		public:
			/**Returns platform vendor
			* @return Platform Vendor
			*/
			inline std::string getPlatformVendor() const {return _platformVendor;}
			
			/**Returns platform Name
			* @return Platform Name
			*/
			inline std::string getPlatformName() const {return _platformName;}
			
			/**Returns platform Profile
			* @return Platform Profile
			*/
			inline std::string getPlatformProfile() const {return _platformProfile;}
			
			/**Returns platform extensions
			* @return Platform extensions
			*/
			inline std::string getPlatformExtensions() const {return _platformExtensions;}
			
			/**Returns OpenCL version supported by this platform
			* @return OpenCL version supported by this platform
			*/
			inline std::string getPlatformSupportedCLVersion() const {return _supportedCLVersion;}
			
			/**Returns number of devices associated with this platform
			* @return Number of devices associated with this platform
			*/
			inline int getNumOfDevices() const {return _numOfDevices;}

			/**Retrieves device object by device index
			* @param index Desired device index
			* @return Object that encapsulates a Device at the specified index
			*/
			inline const CLDevice* getDeviceByIndex(int index) const
			{
				if (index >= 0 && index < _numOfDevices)
					return _deviceInfo[index].get();
				return NULL;
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
			Common::Result createCLContext(cl_context_properties* props,int deviceIndex,cl_context& outputContext,Common::Errata& err) const;
			
			/**Returns ID of this platform
			* @return Platform ID
			*/
			const cl_platform_id& getCLPlatformId() const {return _platformId;}

			/**Destructor*/
			virtual ~CLPlatform();

		private:
			CLPlatform():_platformId(NULL){}
			Common::Result fillDevices(Common::Errata& err);
			cl_platform_id _platformId;
			std::string _platformVendor;
			std::string _supportedCLVersion;
			std::string _platformName;
			std::string _platformProfile;
			std::string _platformExtensions;
			cl_uint _numOfDevices;
			boost::shared_array<boost::shared_ptr<CLDevice> > _deviceInfo;
			boost::shared_array<cl_device_id> _deviceIds;
		};

		/**class CLInterface provides functionality for initializing OpenCL and retrieval of 
		* OpenCL Platforms and devices*/
		class CLInterface
		{

		public:
			/**Constructor*/
			CLInterface();

			/** Initializes OpenCL. Must be called before any OpenCL operation
			* @param [out]err Error info if occurred
			* @return Result of the operation: Success or Failure	
			*/
			Common::Result initCL(Common::Errata& err);

			/**Returns number of OpenCL platforms on this computer
			* @return Number of OpenCL platforms on this computer
			*/
			cl_uint numOfPlatforms() const { return _numPlatforms;}

			/**Retrieves OpenCL Platform by index
			* @param index Index of the desired platform
			* @return Object that encapsulates OpenCL platform at specified index
			*/
			const CLPlatform* getPlatformByIndex(const int index) const
			{
				if (index > _numPlatforms - 1 || index < 0)
					return NULL;
				return &_availablePlatforms[index];
			}

		private:
			cl_uint _numPlatforms;	
			boost::shared_array<CLPlatform> _availablePlatforms;

		};

		/********************************************************
		* Stream operators for printing information
		********************************************************/
		inline std::ostream& operator<<(std::ostream& o, const CLPlatform& p)
		{
			o << "Platform Name: " << p._platformName << std::endl; 
			o << "Platform Vendor: " << p._platformVendor << std::endl; 
			o << "Supported OpenCL Version: " << p._supportedCLVersion << std::endl; 
			o << "Platform Profile: " << p._platformProfile << std::endl; 
			o << "Platform Extensions: " << p._platformExtensions << std::endl; 
			o << "----------------DEVICES--------------------------" << std::endl;
			for (int i = 0; i < p._numOfDevices; i++)
				o << *(p._deviceInfo[i]);
			o << "----------------END DEVICES--------------------------" << std::endl;
			return o;
		}

		inline std::ostream& operator<<(std::ostream& o, const CLDevice& d)
		{
			std::string val;
			Common::Errata err;
			Common::Result res = d.getDeviceName(val,err);
			o << "Device Name: " << (res != Common::Success ? "[Error getting dev prop]" : val.c_str()) << std::endl;
			res = d.getDeviceVendor(val,err);
			o << "Device Vendor: " << (res != Common::Success ? "[Error getting dev prop]" : val.c_str()) << std::endl;
			res = d.getDeviceCLVersion(val,err);
			o << "Supported CL Version: " << (res != Common::Success ? "[Error getting dev prop]" : val.c_str()) << std::endl;
			o << "GPU: " << (d.isGPU() ? "Yes" : "No") << std::endl;
			/*o << "Device Vendor Id: " << d._deviceVendorId<< std::endl;
			o << "Supported OpenCL Version: " << d._deviceCLVersion<< std::endl;
			o << "Available: " << d._deviceAvailable<< std::endl;
			o << "Is GPU: " << d.isGPU() << std::endl; 
			o << "Device Type Flags: " << d._deviceTypeFlags<< std::endl;
			o << "Device Profile: " << d._deviceProfile<< std::endl;
			o << "Compiler Available: " << d._compilerAvailable<< std::endl;
			o << "Device Extensions: " << d._deviceExtensions<< std::endl;
			o << "Little Endian: " << d._endianLittle<< std::endl;
			o << "Address Bits: " << d._deviceAddressBits<< std::endl;*/
			return o;
		}
	}
}
#endif //CL_RT_CLINTERFACE_H

