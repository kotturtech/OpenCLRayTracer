/**
 * @file CLExecutionContext.h
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
 * Part of the OpenCL wrapper framework. The classes expose simplified functionality
 * for compiling OpenCL programs and executing OpenCL kernels.
 *
 */

#ifndef CL_RT_CLEXECUTIONCONTEXT_H
#define CL_RT_CLEXECUTIONCONTEXT_H

#include <typeinfo>
#include <stdarg.h>
#include <vector>
#include <CL/cl.h>
#include <boost/smart_ptr.hpp>
#include <Common/Errata.h>

namespace CLRayTracer
{
	namespace OpenCLUtils
	{
		class CLPlatform;
		class CLDevice;
		class CLExecutionContext; 

		/**class CLKernelArgument encapsulates an argument to OpenCL kernel*/
		class CLKernelArgument 
		{
		public:
			/**Dummy argument*/
			static const char* DummyArg;

			/**Constructor - Allows implicit initialization with any type*/
			template <class T>
			inline CLKernelArgument(const T& data):_data((void*)(&data)),_dataSize(sizeof(T)),_typeName(typeid(T).name())
			{
				_isDummy = false;
				if (_typeName == typeid(const char*).name())
				{
					if (DummyArg == (*(const char**)(_data)))
						_isDummy = true;
				}
			}

			/**Constructor for local memory allocation*/
			inline explicit CLKernelArgument(cl_uint localMemSize)
			{
				_isDummy = false;
				_data = NULL;
				_dataSize = localMemSize;
				_typeName = "char";//Just at random...
			}

			/**Returns pointer to data encapsulated in the kernel argument wrapper
			* @return Pointer to data encapsulated in the kernel argument wrapper
			*/
			inline void* getVoidPtr() const {return _data;}

			/**Returns argument data size in bytes
			* @return Argument data size in bytes
			*/
			inline size_t getDataSize() const {return _dataSize;}

			/**Indicates whether this argument is a dummy argument
			* @return True if the argument is a Dummy argument
			*/
			inline bool isDummy() const {return _isDummy;}
		private:
			void* _data;
			size_t _dataSize;
			std::string _typeName;
			bool _isDummy;
		};

		/**class CLKernel encapsulates the OpenCL kernel*/
		class CLKernel
		{
			friend class CLExecutionContext;
		public:
			/**Constructor
			* @param kernel The raw OpenCL kernel object
			*/
			CLKernel(cl_kernel kernel);

			/**Sets argument value for this kernel
			* @param argObj Object that encapsulates the argument value
			* @param Index at which the argument should be set
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result setKernelArgument(const CLKernelArgument& argObj,cl_uint index,Common::Errata& err);
			
			/**Destructor*/
			virtual ~CLKernel();
		protected:
			cl_kernel _clKernel;
		};

/**Macro for fast setting of kernel arguments*/
#define SET_KERNEL_ARGS(kernel,...) {CLKernelArgument args[] = {CLKernelArgument::DummyArg,__VA_ARGS__,CLKernelArgument::DummyArg}; CLRayTracer::Common::Result res; CLRayTracer::Common::Errata err; int i = 1; while(!(args[i].isDummy())) { res = kernel.setKernelArgument(args[i],i-1,err); if (res != CLRayTracer::Common::Success) throw new CLRayTracer::Common::CLInterfaceException(err); i++;} }

		/**class CLProgram encapsulates OpenCL program - A compiled OpenCL object that may contain multiple kernels*/
		class CLProgram
		{
		public:
			/**Constructor
			* @param context OpenCL execution context
			*/
			CLProgram(const CLExecutionContext& context);
			
			/**Destructor*/
			virtual ~CLProgram();
			
			/**Loads OpenCL program (cl file) and compiles the source. Creates a log file in case error occurred.
			* @param fileName file name and path
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result loadAndCompile(const std::string& fileName,Common::Errata& err);
			
			/**Loads OpenCL program (cl file) and compiles the source. Creates a log file in case error occurred.
			* @param fileName file name and path
			* @param compilerParams string that contains compiler options for the compiler
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result loadAndCompile(const std::string& fileName,const std::string& compilerParams,Common::Errata& err);
			
			/**Compiles program stored in a string. Creates a log file in case error occurred.
			* @param kernelStr the string that contains the program
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result compile(const std::string& kernelStr,Common::Errata& err);
			
			/**Compiles program stored in a string. Creates a log file in case error occurred
			* @param kernelStr the string that contains the program
			* @param compilerParams string that contains compiler options for the compiler
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result compile(const std::string& kernelStr,const std::string& compilerParams,Common::Errata& err);
			
			/**Creates Kernel object from program, by kernel name
			* @param kernelName Desired kernel name
			* @param [out] kernelObj Resulting kernel object
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result getKernel(const std::string& kernelName, CLKernel*& kernelObj, Common::Errata& err);
		private:
			const CLExecutionContext& _context;
			cl_program _clProgram; 
		};

		namespace CLBufferFlags
		{
			/**Enum Access modes for OpenCL device memory*/
			enum CLBufferAccess {ReadOnly,WriteOnly,ReadWrite};
			/**Enum Host pointer usages for OpenCL device memory allocation
			*  @see OpenCL Host Pointer usage on Device memory allocation
			*/
			enum CLBufferHostPtrOptions {None,Copy,Use,Alloc};
		}

		/**class CLKernelWorkDimension encapsulates the work dimensions for OpenCL kernel execution*/
		class CLKernelWorkDimension
		{
		public:
			/**Constructor 
			* @param workDim Number of work dimensions
			* @param va_arg Size of the work group along corresponding dimension
			*/
			CLKernelWorkDimension(cl_uint workDim,...):workDimensions(workDim),dimensionValues(new size_t[workDim])
			{
				va_list arglist;
				va_start(arglist,workDim);
				for (int i = 0; i < workDim; i++)
					dimensionValues[i] = va_arg(arglist,size_t);
				va_end(arglist);
			}

			/**Counts total work items for Dimension object
			*@return Sum of all items in all dimensions
			*/
			inline unsigned int totalItems() const 
			{
				unsigned int dim = 1;
				for (int i = 0; i < workDimensions; i++)
					dim*=dimensionValues[i];
				return dim;
			}

			/**Number of work dimensions*/
			const cl_uint workDimensions;
			/**Dimension extents*/
			const boost::shared_array<size_t> dimensionValues;
		};

		/**class CLEvent encapsulates OpenCL synchronization event*/
		class CLEvent
		{
		public:
			/**Constructor
			 * @param evt Raw OpenCL event object
			 */
			CLEvent(cl_event evt = NULL):clEvent(evt){}
			
			/**Returns the underlying OpenCL event
			*@return Underlying OpenCL event
			*/
			cl_event& getCLEvent() {return clEvent;} 
			
			/**Resets state of the event, should be called before reusing the event object
			*@return 
			*/
			void reset();

			/** Wait for OpenCL operation that should be synchronized by this event to complete
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result wait(Common::Errata& err);
			
			/** Wait for OpenCL operation that should be synchronized by all events in event list to complete
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			static Common::Result wait(std::vector<boost::shared_ptr<CLEvent> > events, Common::Errata& err);
			
			/**Destructor*/
			virtual ~CLEvent();
		protected:
			cl_event clEvent;
		};

		/**class CLKernelExecutionParams encapsulates the work dimensions for kernel execution*/
		class CLKernelExecuteParams
		{
		public:
			/**Constructor
			* @param globalWD Global work dimension
			* @param localWD Local work dimension
			* @param [optional]evt Synchronization event associated with kernel execution
			*/
			CLKernelExecuteParams(CLKernelWorkDimension* globalWD,CLKernelWorkDimension* localWD, CLEvent* evt = NULL)
				:globalWorkDimension(globalWD),localWorkDimension(localWD),event(evt)
			{
				globalWorkOffset = NULL;
			}
			
			/**Adds the input event to event wait list for kernel execution
			* @param evt Event to add
			* @return
			*/
			void addEventToWaitList(CLEvent evt) {eventWaitList.push_back(evt.getCLEvent());}
			
			/**Event object (This class doesn't have ownership on the object)*/
			CLEvent* event;
			/**Local work dimension (This class doesn't have ownership on the object)*/
			CLKernelWorkDimension* localWorkDimension;
			/**Global work dimension (This class doesn't have ownership on the object)*/
			CLKernelWorkDimension* globalWorkDimension;
			/**Global work offset (This class doesn't have ownership on the object)*/
			CLKernelWorkDimension* globalWorkOffset;
			/**Event wait list for kernel execution*/
			std::vector<cl_event> eventWaitList;
		};

		/**class CLExecutionContext encapsulates operations on OpenCL command queue: Kernel executions, memory
		*                           operations, creating synchronization events, etc. This class encapsulates partial
		*                           set of those operations, and can be extended if needed
		*/
		class CLExecutionContext
		{
			friend class CLProgram;
		public:
			/**Constructor
			* @param device Object that encapsulate the OpenCL device in use
			* @param contextProperties[optional] The properties of context to be created
			* @see class CLDevice
			* @see cl_context_properties in OpenCL specification
			*/
			CLExecutionContext(const CLDevice& device, const cl_context_properties* _contextProperties = NULL);

			/** Initialize the context object - Should be called once per instance
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result initialize(Common::Errata& err) const;
			
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
			Common::Result createDeviceBuffer(CLBufferFlags::CLBufferAccess access,CLBufferFlags::CLBufferHostPtrOptions hostPtrOpts,size_t size,cl_mem& output, Common::Errata& err,void* hostPtr = NULL) const;
			
			/** Create OpenCL memory object
			* @param access Access mode
			* @param size Desired size of the buffer
			* @param [out]output Pointer to the created device buffer
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result createDeviceBuffer(CLBufferFlags::CLBufferAccess access,size_t size,cl_mem& output, Common::Errata& err) const;
			
			/** Creates object that encapsulates OpenCL synchronization event
			* @param [out]outEvt The created event
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result createEvent(boost::shared_ptr<CLEvent>& outEvt,Common::Errata& err) const;
			
			/** Creates multiple objects that encapsulates OpenCL synchronization event
			* @param count Count of desired events
			* @param events Vector that shall be filled with created events
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result createEvents(int count,std::vector<boost::shared_ptr<CLEvent> >& events,Common::Errata& err) const;
			
			/**Reads OpenCL device memory contents and writes them into host memory buffer
			* @param buffer OpenCL device buffer
			* @param outputBuffer Host memory pointer to destination, where memory should be copied
			* @param bufferSize The size of the buffer that should be copied
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result enqueueReadBuffer(const cl_mem& buffer,void* outputBuffer, size_t bufferSize,Common::Errata& err) const;
			
			/**Reads OpenCL device memory contents and writes them into host memory buffer
			* @param buffer OpenCL device buffer
			* @param outputBuffer Host memory pointer to destination, where memory should be copied
			* @param offset Offset index (In bytes), from which the copying should start
			* @param bufferSize The size of the buffer that should be copied
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result enqueueReadBuffer(const cl_mem& buffer,void* outputBuffer,size_t offset, size_t bufferSize,Common::Errata& err) const;
			
			/**Fills OpenCL device memory with defined pattern, same principle as Win32 memset
			* @param buffer OpenCL device buffer
			* @param pattern The pattern that should be filled
			* @param bufferSize The size of the buffer that should be copied. Must be a multiple of patternSize
			* @param patternSize patternSize The size of pattern - According to OpenCL specs can be {1, 2, 4, 8, 16, 32, 64, 128}
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result enqueueFillBuffer(const cl_mem& buffer,void* pattern, size_t bufferSize, size_t patternSize, Common::Errata& err) const;
			
			/**Fills OpenCL device memory with contents of host memory buffer
			* @param buffer Host buffer - The source
			* @param outputBuffer OpenCL device buffer - The destination
			* @param bufferSize The size of the buffer that should be copied
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result enqueueWriteBuffer(void* buffer, const cl_mem& outputBuffer, size_t bufferSize,Common::Errata& err) const;
			
			/**Copies OpenCL device memory into destination memory on device
			* @param buffer Source OpenCL device buffer
			* @param outputBuffer Destination OpenCL device buffer
			* @param bufferSize The size of the buffer that should be copied
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result enqueueCopyBuffer(const cl_mem& buffer,const cl_mem& outputBuffer, size_t bufferSize,Common::Errata& err) const;
			
			/**Enqueues kernel execution on underlying OpenCL command queue - In simple terms, executes the kernel
			* @param Object that encapsulates the kernel to execute
			* @param Object that encapsulates data about dimensions and syncronization events for kernel execution
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result enqueueKernel(CLKernel& kernel,CLKernelExecuteParams& params, Common::Errata& err) const;
			
			/** Issues all queues operations to the device. Encapsulates the clFlush OpenCL method
			* @param err Error info
			* @return Result of the operation: Success or failure
			* @see clFlush
			*/
			Common::Result flushQueue(Common::Errata& err) const;
			
			/** Blocks until all commands on the queue are completed. Encapsulates the clFinish OpenCL method
			* @param err Error info
			* @return Result of the operation: Success or failure
			* @see clFinish
			*/
			Common::Result finishQueue(Common::Errata& err) const;
			
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
			Common::Result getMaximalLaunchExecParams(const CLKernel& kernel, size_t& computeDevices, size_t& preferredWorkgroupSize,Common::Errata& err) const;
			
			/**Retrieves the max allowed workgroup size for the specified kernel
			* @param kernel The kernel to check
			* @param [out]maxWGSize Will contain the resulting maximal workgroup size for the kernel
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result getMaxWorkgroupForKernel(const CLKernel& kernel, size_t& maxWGSize,Common::Errata& err) const;
			
			/**Retrieves the object that encapsulates the used OpenCL device
			* @return Object that encapsulates the used OpenCL device
			*/
			inline const CLDevice& getDevice() const {return _device;};

			/**Destructor*/
			virtual ~CLExecutionContext();
		protected:
			const CLPlatform& _platform;
			const CLDevice& _device;
			const cl_context_properties* _contextProperties;
			mutable cl_context _clContext;
			mutable cl_command_queue _clCommandQueue;
			mutable bool _initialized;

		};
	}
}
#endif //CL_RT_CLEXECUTIONCONTEXT_H

 