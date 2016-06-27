/**
 * @file TwoLevelGridManager.h
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
 * class TwoLevelGridManager - Provides interface for GPU implementation of acceleration structure:
 *							   Two Level Grid
 *
 */

#ifndef CL_RT_TWO_LEVEL_GRID_BUILDER_H
#define CL_RT_TWO_LEVEL_GRID_BUILDER_H

#include <Algorithms/AccelerationStructureManager.h>
#include <CLData\AccelerationStructs\TwoLevelGridData.h>
#include <boost\smart_ptr.hpp>
#include <OpenCLUtils\CLInterface.h>

struct Contact;
struct Camera;
struct AABB;

namespace CLRayTracer
{
	namespace Common
	{
		class BitonicSort;
		class PrefixSum;
	}

	namespace OpenCLUtils
	{
		class CLBuffer;
	}

	namespace AccelerationStructures
	{
		/** class TwoLevelGridManager - Provides interface for GPU implementation of acceleration structure:
		*							    Two Level Grid
		*/
		class TwoLevelGridManager: public AccelerationStructureManager
		{
		public:
			/**Constructor*/
			TwoLevelGridManager(const OpenCLUtils::CLExecutionContext& context,const Scene& scene);
		    /***************************************
			* Interface functions
			****************************************/

			/**Performs main initialization of the Two Level Grid structure.
			* This initialization shoud be performed just once per instance.
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			virtual Common::Result initialize(Common::Errata& err);
			
			/**Performs initialization of a frame for Two Level Grid acceleration structure
			* This initialization shoud be performed before each frame. Grid property change will take effect
			* only after execution of this function.
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			virtual Common::Result initializeFrame(Common::Errata& err);
			
			/**Constructs Two-Level Grid acceleration structure, according to associated scene state
			* @param err Error info
			* @return Result of the operation: Success or failure
			*/
			virtual Common::Result construct(Common::Errata& err);
			
			/**Generates hit data for viewing rays, from the constructed Two-Level Grid
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
			* Properties 
			****************************************/
			/**Sets the top level density parameter of the grid, for top level resolution calculation*/
			inline void setTopLevelDensity(CL_FLOAT value) {_topLevelDensity = value;}
			/**Sets the leaf level density parameter of the grid, for leaf level resolution calculation*/
			inline void setLeafDensity(CL_FLOAT value) {_leafDensity = value;}
			/**Gets the top level density parameter of the grid, for top level resolution calculation*/
			inline CL_FLOAT getTopLevelDensity() const {return _topLevelDensity;}
			/**Gets the leaf level density parameter of the grid, for leaf level resolution calculation*/
			inline CL_FLOAT getLeafDensity() const {return _leafDensity;}
			/**Gets the device memory buffer for primary rays - result of generateContacts function*/
			virtual const boost::shared_ptr<OpenCLUtils::CLBuffer> getPrimaryContacts() const {return _primaryContactsArray;}
			
			/***************************************
			* Utility Functions
			****************************************/
			/**
			* Calculates grid resolution according to grid properties
			* @return Grid resolution
			*/
			CL_UINT3 getResolution() const;
			/**
			* Calculates internal grid data for use on GPU
			* @return
			*/
			struct AABB getBounds() const;
			
		private:
			void calculateGridData();
			boost::shared_ptr<Common::BitonicSort> _bitonicSorter;
			boost::shared_ptr<Common::PrefixSum> _prefixSumCalculator;
			CL_FLOAT _topLevelDensity;
			CL_FLOAT _leafDensity;
			CL_UINT _numPrimitives;
			CL_UINT _numPrimitivesPowOfTwo;
			CL_UINT _pairsCount;
			CL_UINT _pairsCountPowOfTwo;
			CL_UINT _cellsCount;
			CL_UINT _cellsCountPowOfTwo;
			CL_UINT _leafCellsCount;
			CL_UINT _leafPairsCount;
			CL_UINT _leafPairsCountPowOfTwo;
			boost::shared_ptr<OpenCLUtils::CLBuffer> _counters;
			boost::shared_ptr<OpenCLUtils::CLBuffer> _prefixSumOutput;
			boost::shared_ptr<OpenCLUtils::CLBuffer> _pairsArray;
			boost::shared_ptr<OpenCLUtils::CLBuffer> _cellRangesArray;
			boost::shared_ptr<OpenCLUtils::CLBuffer> _topLevelCellsArray;
			boost::shared_ptr<OpenCLUtils::CLBuffer> _leafPairsArray;
			boost::shared_ptr<OpenCLUtils::CLBuffer> _deviceTopLevelGrid;
			boost::shared_ptr<OpenCLUtils::CLBuffer> _deviceCamera;
			boost::shared_ptr<OpenCLUtils::CLBuffer> _leafCellRangesArray;
			boost::shared_ptr<OpenCLUtils::CLBuffer> _primaryContactsArray;
			struct GridData _hostGrid;
			boost::shared_ptr<OpenCLUtils::CLProgram> _tlgProgram;
			boost::shared_ptr<OpenCLUtils::CLKernel> _prepareDataKernel;
			boost::shared_ptr<OpenCLUtils::CLKernel> _writePairsKernel;
			boost::shared_ptr<OpenCLUtils::CLKernel> _writeCellRangesKernel;
			boost::shared_ptr<OpenCLUtils::CLKernel> _countLeafCellsKernel;
			boost::shared_ptr<OpenCLUtils::CLKernel> _updateTopLevelCellsWithLeafRangeKernel;
			boost::shared_ptr<OpenCLUtils::CLKernel> _prepareLeafDataKernel;
			boost::shared_ptr<OpenCLUtils::CLKernel> _writeLeafPairsKernel;
			boost::shared_ptr<OpenCLUtils::CLKernel> _extractLeafCellsKernel;
			boost::shared_ptr<OpenCLUtils::CLKernel> _generateContactsKernel;
			boost::shared_ptr<OpenCLUtils::CLKernel> _generateContacts2Kernel;
		
			size_t _maxWorkgroupSize;
			size_t _processors;
			size_t _wavefront;
			CL_ULONG _deviceLocalMemory;
		};
	}
}

#endif //CL_RT_TWO_LEVEL_GRID_BUILDER_H