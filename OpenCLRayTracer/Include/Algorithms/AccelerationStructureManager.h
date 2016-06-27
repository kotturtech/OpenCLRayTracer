/**
 * @file AccelerationStructureManager.h
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
 * class AccelerationStructureManager - The base class for acceleration structures for
 *                                      Ray Tracing
 */

#ifndef CL_RT_ACCELERATION_STRUCTURE_BUILDER_H
#define CL_RT_ACCELERATION_STRUCTURE_BUILDER_H

#include <common\Errata.h>

struct Camera;

namespace CLRayTracer
{
	class Scene;
	namespace OpenCLUtils
	{
		class CLExecutionContext;
		class CLBuffer;
	}

	namespace AccelerationStructures
	{
		/**
		 * class AccelerationStructureManager - The base class for acceleration structures for
		 *                                      Ray Tracing
		 */
		class AccelerationStructureManager
		{
		public:
			
			/**Constructor*/
			AccelerationStructureManager(const OpenCLUtils::CLExecutionContext& context,const Scene& scene)
				:_context(context),_scene(scene){}

			/**General initialization of the acceleration structure managers - Compiles kernels, etc*/
			virtual Common::Result initialize(Common::Errata& err) = 0;
			/**Prepares data for next frame - Memory allocation, etc*/
			virtual Common::Result initializeFrame(Common::Errata& err) = 0;
			/**Constructs the acceleration structure according to the scene*/
			virtual Common::Result construct(Common::Errata& err) = 0;
			/**Generates contacts for viewing rays and fills the primary contacts array*/
			virtual Common::Result generateContacts(Camera& cam,Common::Errata& err) = 0;
			/**Generates contacts for rays and fills the contacts array*/
			virtual Common::Result generateContacts(OpenCLUtils::CLBuffer& rays,OpenCLUtils::CLBuffer& contacts, const unsigned int rayCount, Common::Errata& err) = 0;
			/**Retrieves the buffer of contacts that were the result of generateContacts functions above*/
			virtual const boost::shared_ptr<OpenCLUtils::CLBuffer> getPrimaryContacts() const = 0;
			
			/**Destructor*/
			virtual ~AccelerationStructureManager(){}
		protected:
			const OpenCLUtils::CLExecutionContext& _context;
			const Scene& _scene;
		};
	}
}

#endif