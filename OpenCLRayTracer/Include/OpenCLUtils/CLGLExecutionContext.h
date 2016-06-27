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
 * Part of the OpenCL wrapper framework. CLGLExecutionContext extends the functionality of
 * the CLExecutionContext by adding OpenCL OpenGL interop related functions
 *
 */


#ifndef CL_RT_CLGLEXECUTIONCONTEXT_H
#define CL_RT_CLGLEXECUTIONCONTEXT_H
#include <GL\glew.h>
#include <OpenCLUtils\CLExecutionContext.h>


namespace CLRayTracer
{
	namespace CLGLInterop
	{
		class CLGLExecutionContext;
		/**class CLGLMemoryBuffer encapsulates the VBO and cl memory operations*/
		class CLGLMemoryBuffer
		{
			friend class CLGLExecutionContext;
		public:
			/**Returns the buffer size
			* @return Buffer size in bytes
			*/
			inline size_t getSize() const {return _size;}
			
			/**Returns the OpenGL VBO Id for the buffer
			* @return OpenGL VBO id
			*/
			inline GLint getVBOId() const {return _vboId;}
			
			/**Returns the OpenCL device memory pointer to the buffer
			* @return OpenCL device memory pointer to the buffer
			*/
			inline cl_mem getCLBuffer() const {return _clBuffer;}

			/**Destructor*/
			virtual ~CLGLMemoryBuffer();
		protected:
			CLGLMemoryBuffer(size_t size,GLuint vbo_id,cl_mem clbuffer):_size(size),_vboId(vbo_id),_clBuffer(clbuffer){};
			size_t _size;
			GLuint _vboId;
			cl_mem _clBuffer;
		};

		/**class CLGLExecutionContext Extends CLExecutionContext with OpenCL/OpenGL interop functions.
		*/
		class CLGLExecutionContext : public OpenCLUtils::CLExecutionContext 
		{
		public:
			/**Constructor
			* @param device Object that encapsulate the OpenCL device in use
			* @param contextProperties[optional] The properties of context to be created
			* @see class CLDevice
			* @see cl_context_properties in OpenCL specification
			*/
			CLGLExecutionContext(const OpenCLUtils::CLDevice& device, const cl_context_properties* _contextProperties = NULL)
				:CLExecutionContext(device,_contextProperties){}
			
			/**Destructor*/
			virtual ~CLGLExecutionContext();

			/** Create OpenCL/OpenGL memory object for inter-operation
			* @param data The host data that should be copied to the OpenGL/OpenCL buffer on creation
			* @param size Desired size of the buffer
			* @param [out]output Pointer to the created OpenGL/OpenCL buffer object
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result createCLGLBuffer(void* data,size_t size,boost::shared_ptr<CLGLMemoryBuffer>& out_buffer,Common::Errata& err) const;
			
			/** Create OpenCL/OpenGL memory object for inter-operation
			* @param size Desired size of the buffer
			* @param [out]output Pointer to the created OpenGL/OpenCL buffer object
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result createCLGLBuffer(size_t size,boost::shared_ptr<CLGLMemoryBuffer>& out_buffer,Common::Errata& err) const;
			
			/** Acquire memory object for use with OpenCL memory operations
			* @param object Memory objects list
			* @param evt Events to wait for, before proceeding to this operation
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result enqueueAcquireGLObject(std::vector<cl_mem> objects, OpenCLUtils::CLEvent& evt, Common::Errata& err) const;
			
			/** Acquire memory object for use with OpenCL memory operations
			* @param object Memory objects list
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result enqueueAcquireGLObject(std::vector<cl_mem> objects, Common::Errata& err) const;

			/** Release memory object for use by OpenGL 
			* @param object Memory objects list
			* @param evt Events to wait for, before proceeding to this operation
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result enqueueReleaseGLObject(std::vector<cl_mem> objects, OpenCLUtils::CLEvent& evt, Common::Errata& err) const;
			
			/** Release memory object for use by OpenGL 
			* @param object Memory objects list
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result enqueueReleaseGLObject(std::vector<cl_mem> objects, Common::Errata& err) const;
		};
	}
}

#endif //CL_RT_CLGLEXECUTIONCONTEXT_H

