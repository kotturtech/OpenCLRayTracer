/**
 * @file CLBuffer.cpp
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
 * Implementation of class CLBuffer - Exposes OpenCL device memory operations
 *
 */

#include <Common\Errata.h>
#include <OpenCLUtils\CLBuffer.h>

using namespace CLRayTracer;
using namespace CLRayTracer::OpenCLUtils;
using namespace CLRayTracer::Common;

/**Constructor
* @param context OpenCL execution context
* @param size Size of buffer in bytes
* @param access Access flags: ReadOnly, ReadWrite,etc
*/
CLBuffer::CLBuffer(const CLExecutionContext& context,size_t size,CLBufferFlags::CLBufferAccess access):_context(context),_size(size),_access(access),_actualSize(size)
{
	Errata err;
	if (Success != _context.createDeviceBuffer(access,size,_actualBuffer,err))
		throw new CLInterfaceException(err);
}

/**Constructor
* @param context OpenCL execution context
* @param size Size of buffer in bytes
* @param source Host memory buffer that should be copied to device
* @param access Access flags: ReadOnly, ReadWrite,etc
*/
CLBuffer::CLBuffer(const CLExecutionContext& context,size_t size,void* source,CLBufferFlags::CLBufferAccess access):_context(context),_size(size),_access(access),_actualSize(size)
{
	Errata err;
	if (Success != _context.createDeviceBuffer(access,CLBufferFlags::Copy,size,_actualBuffer,err,source))
		throw new CLInterfaceException(err);
}


/**Resizes the buffer to the new size. The function actually expands the buffer when
* the buffer is smaller than the requested new size, and leaves the memory intact
* when buffer shrinking is requested.
* @param newSize New size in bytes
* @return 
* @see OpenCL buffer access modes enumeration
*/
void CLBuffer::resize(size_t newSize)
{
	if (newSize < _size)
	{
		_size = newSize;
		return;
	}

	if (newSize > _size && newSize < _actualSize)
	{
		_size = newSize;
		return;
	}

	if (_actualSize > 0)
		clReleaseMemObject(_actualBuffer);
	_actualSize = _size = newSize;

    if (_size > 0)
	{
		Errata err;
		if (Success != _context.createDeviceBuffer(_access,_size,_actualBuffer,err))
			throw new CLInterfaceException(err);
	}
}

/**Copies the Device memory associated with this buffer object to host memory
* @param target Target host buffer to copy to
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLBuffer::copyToHost(void* target,Errata& err) const
{
	return copyToHost(target,0,err);
}

/**Copies the Device memory associated with this buffer object to host memory
* @param target Target host buffer to copy to
* @param offset Offset - The starting index at which the copy should start
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result CLBuffer::copyToHost(void* target,size_t offset,Errata& err) const
{
	return _context.enqueueReadBuffer(_actualBuffer,target,offset,_size,err);
}

/**Destructor*/
 CLBuffer::~CLBuffer()
{
	if (_actualSize > 0)
		clReleaseMemObject(_actualBuffer);
}