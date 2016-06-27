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
 * Part of the OpenCL wrapper framework. CLGLInteropContext exposes functionality 
 * for OpenGL/OpenCL interop
 *
 */


#ifndef CL_RT_CLGLINTEROPAPP_H
#define CL_RT_CLGLINTEROPAPP_H

#include <OpenCLUtils/CLGLExecutionContext.h>
#include <boost/smart_ptr.hpp>

namespace CLRayTracer
{
	
	namespace OpenCLUtils
	{
		class CLPlatform;
	}

	namespace CLGLInterop
	{
		/**class CLGLInteropContext exposes functionality for OpenGL/OpenCL interop*/
		class CLGLInteropContext
		{
		public:

			/** Initializes the object instance
			* @param platform Object that encapsulates the used OpenCL platform 
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result initialize(const OpenCLUtils::CLPlatform& platform,Common::Errata& err);
			
			/** Initializes the object instance. Unlike the function that takes platform as parameter,
			*   this function automatically finds the first platform and device that support OpenGL/OpenCL interop
			*   and use that platform and device
			* @param CLInterface Object that encapsulates OpenCL interface
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			Common::Result initialize(const OpenCLUtils::CLInterface& clInterface,Common::Errata& err);

			/**Retrieves the associated Execution Context object
			* @return The associated Execution Context object
			*/
			inline const CLGLExecutionContext* getExecutionContext() const {return _executionContext.get();}
			
			/**Retrieves the associated OpenCL platform
			* @return The associated OpenCL platform
			*/
			inline const OpenCLUtils::CLPlatform* getInteropPlatform() const {return _interopPlatform;}
			
			/**Retrieves the associated OpenCL device
			* @return The associated OpenCL device
			*/
			inline const OpenCLUtils::CLDevice* getInteropDevice() const {return _interopDeviceIndex >= 0 ? _interopPlatform->getDeviceByIndex(_interopDeviceIndex) : NULL;}
			
			/**Constructor*/
			CLGLInteropContext();

			/**Destructor*/
			virtual ~CLGLInteropContext();
		private:
			boost::shared_ptr<CLGLExecutionContext> _executionContext;
			void cleanup();
			int _interopDeviceIndex;
			const OpenCLUtils::CLPlatform* _interopPlatform;
		};
	}
}

#endif //CL_GL_INTEROPAPP

