/**
 * @file BVHManager.h
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
 * class BVHManager - The host interface to GPU implementation of BVH
 * 
 */

#ifndef CL_RT_BVHBUILDER_H
#define CL_RT_BVHBUILDER_H

#include <Algorithms/AccelerationStructureManager.h>
#include <boost\smart_ptr.hpp>
#include <OpenCLUtils\CLInterface.h>

struct BVHNode;
struct Contact;
struct Camera;

namespace CLRayTracer
{
	namespace Common
	{
		class BitonicSort;
	}

	namespace OpenCLUtils
	{
		class CLBuffer;
	}

	namespace AccelerationStructures
	{
	   /**
		* class BVHManager - The host interface to GPU implementation of BVH
		* 
        */
		class BVHManager: public AccelerationStructureManager
		{
		public:
			/**Constructor*/
			BVHManager(const OpenCLUtils::CLExecutionContext& context,const Scene& scene);
			/***************************************
			* Interface functions
			****************************************/
			/**Performs main initialization of the BVH structure.
			* This initialization shoud be performed just once per instance.
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			virtual Common::Result initialize(Common::Errata& err);
			
			/**Performs initialization of a frame for BVH acceleration structure
			* This initialization shoud be performed before each frame.
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			virtual Common::Result initializeFrame(Common::Errata& err);
			
			/**Constructs BVH acceleration structure, according to associated scene state
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			virtual Common::Result construct(Common::Errata& err);
			
			/**Generates hit data for viewing rays, from the constructed BVH
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			virtual Common::Result generateContacts(Camera& cam,Common::Errata& err);

			/**Generates contacts for rays and fills the contacts array
			* @param rays The rays to be traced - A device memory buffer object that contains rays as struct Ray
			* @param contact The target device memory that will contain the result - For each ray rays[i] the buffer will contain
			*                data about its closest intersection with object in the scene as contacts[i]. The contact data will be stored
			*                as struct Contact
			* @param rayCount The number of rays to trace
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			virtual Common::Result generateContacts(OpenCLUtils::CLBuffer& rays,OpenCLUtils::CLBuffer& contacts, const unsigned int rayCount, Common::Errata& err);
			
			/***************************************
			* Properties and utility functions
			****************************************/
			/**Retrieves the buffer of contacts that were the result of generateContacts functions above*/
			virtual const boost::shared_ptr<OpenCLUtils::CLBuffer> getPrimaryContacts() const { return _primaryContactsArray;}

		protected:
			CL_UINT _mortonBufferItems;
			CL_ULONG _deviceLocalMemory;
			CL_UINT _bvhLeavesCount;
			boost::shared_ptr<Common::BitonicSort> _bitonicSorter;
			boost::shared_ptr<OpenCLUtils::CLBuffer> _bvhNodes;
			boost::shared_ptr<OpenCLUtils::CLBuffer> _sortedMortonCodes;
			boost::shared_ptr<OpenCLUtils::CLBuffer> _nodeVisitCounters;
			boost::shared_ptr<OpenCLUtils::CLBuffer> _primaryContactsArray;
			boost::shared_ptr<OpenCLUtils::CLBuffer> _deviceCamera;
			boost::shared_ptr<OpenCLUtils::CLProgram> _bvhProgram;
			boost::shared_ptr<OpenCLUtils::CLKernel> _mortonCalcKernel;
			boost::shared_ptr<OpenCLUtils::CLKernel> _radixTreeBuildKernel;
			boost::shared_ptr<OpenCLUtils::CLKernel> _bbCalcKernel;
			boost::shared_ptr<OpenCLUtils::CLKernel> _contactGenerateKernel;
			boost::shared_ptr<OpenCLUtils::CLKernel> _contactGenerateKernel2;
		};
	}
}

#endif //CL_RT_BVHBUILDER_H