/**
 * @file CLBuffer.h
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
 * class CLBuffer - Exposes OpenCL device memory operations
 *
 */

#ifndef CL_RT_CLBUFFER_H
#define CL_RT_CLBUFFER_H
#include <OpenCLUtils\CLExecutionContext.h>

namespace CLRayTracer
{
	namespace OpenCLUtils
	{
		/**class CLBuffer Wrapper class for OpenCL Device Memory*/
		class CLBuffer
		{
		public:

			/**Constructor
			* @param context OpenCL execution context
			* @param size Size of buffer in bytes
			* @param access Access flags: ReadOnly, ReadWrite,etc
			*/
			CLBuffer(const CLExecutionContext& context,size_t size, CLBufferFlags::CLBufferAccess access);
			
			/**Constructor
			* @param context OpenCL execution context
			* @param size Size of buffer in bytes
			* @param source Host memory buffer that should be copied to device
			* @param access Access flags: ReadOnly, ReadWrite,etc
			*/
			CLBuffer(const CLExecutionContext& context,size_t size, void* source, CLBufferFlags::CLBufferAccess access);
			
			/**Resizes the buffer to the new size. The function actually expands the buffer when
			* the buffer is smaller than the requested new size, and leaves the memory intact
			* when buffer shrinking is requested.
			* @param newSize New size in bytes
			* @return 
			* @see OpenCL buffer access modes enumeration
			*/
			void resize(size_t newSize);

			/**Returns the used buffer size in bytes
			* @return buffer size in bytes
			*/
			inline size_t getSize() const {return _size;}
			
			/**Returns the size of the actually allocated device memory for the buffer object
			* @return Actual buffer size in bytes
			*/
			inline size_t getActualSize() const {return _actualSize;}
			
			/**Returns the access mode of this buffer
			* @return Access mode of this buffer
			*/
			inline CLBufferFlags::CLBufferAccess getAccess() const {return _access;}
			
			/**Returns the underlying cl_mem object of this buffer
			* @return Underlying cl_mem object of this buffer
			* @see OpenCL buffer access modes enumeration
			*/
			inline const cl_mem& getCLMem() const {return _actualBuffer;}

			/**Copies the Device memory associated with this buffer object to host memory
			* @param target Target host buffer to copy to
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result copyToHost(void* target,Common::Errata& err) const;
			
			/**Copies the Device memory associated with this buffer object to host memory
			* @param target Target host buffer to copy to
			* @param offset Offset - The starting index at which the copy should start
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result copyToHost(void* target,size_t offset, Common::Errata& err) const;
			
			/**Destructor*/
			virtual ~CLBuffer();

		private:
			const CLExecutionContext& _context;
			size_t _size;
			size_t _actualSize;
			CLBufferFlags::CLBufferAccess _access;
			cl_mem _actualBuffer;
		};
	}
}

#endif

