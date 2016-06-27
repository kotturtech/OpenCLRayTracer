/**
 * @file TwoLevelGridManager.cpp
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
 * Implementation of class TwoLevelGridManager - The host interface to GPU implementation of Two Level Grid
 *
 * @implNote The algorithm implemented according to the article: "Two-Level Grids for Ray Tracing on GPUs"
 *           by Javor Kalojanov, Markus Billeter and Philipp Slusallek
 *           http://www.intel-vci.uni-saarland.de/fileadmin/grafik_uploads/publications/59.pdf
 */

#include <math.h>
#include <OpenCLUtils\CLBuffer.h>
#include <Algorithms\Sorting.h>
#include <Algorithms\PrefixSum.h>
#include <Algorithms\TwoLevelGridManager.h>
#include <CLData\AccelerationStructs\TwoLevelGrid.h>
#include <CLData\SceneBufferParser.h>
#include <CLData\CLStructs.h>
#include <Scene\Scene.h>
#include <Common\Deployment.h>
#include <Testing\PrefixSumTest.h>
#include <Testing\TwoLevelGridTest.h>
#include <Testing\SortingTest.h>

//Implementation based on paper: 
//Two-Level Grids for Ray Tracing on GPUs Javor Kalojanov,arkus Billeter and Philipp Slusallek

using namespace std;
using namespace CLRayTracer;
using namespace CLRayTracer::OpenCLUtils;
using namespace CLRayTracer::Common;
using namespace CLRayTracer::AccelerationStructures;

/**C string containing the kernel source*/
extern const char* TwoLevelGridKernelSource;

/**Constructor*/
TwoLevelGridManager::TwoLevelGridManager(const CLExecutionContext& context,const Scene& scene):AccelerationStructureManager(context,scene)
{
	_bitonicSorter.reset(new BitonicSort(context,true));
	_prefixSumCalculator.reset(new PrefixSum(context));
	//Default value for densities, based on paper: 
	//Two-Level Grids for Ray Tracing on GPUs Javor Kalojanov,Markus Billeter and Philipp Slusallek
	_topLevelDensity = 2.0f;
	_leafDensity = 2.0f;
	_numPrimitives = 0;
	_numPrimitivesPowOfTwo = 0;
	_cellsCountPowOfTwo = 0;
	_leafCellsCount = 0;
	_leafPairsCount = 0;
	_leafPairsCountPowOfTwo = 0;
}



/*********************************************************
* Two Level Grid Interface Functions
**********************************************************/

/**Performs main initialization of the Two Level Grid structure.
* This initialization shoud be performed just once per instance.
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result TwoLevelGridManager::initialize(Common::Errata& err)
{
	//Initialization of utility classes
	if (Success != _bitonicSorter->initialize(err))
		return Error;
	if (Success != _prefixSumCalculator->initialize(err))
		return Error;
	
	//Compilation of kernels 
	_tlgProgram.reset(new CLProgram(_context));
	if (Success != _tlgProgram->compile(TwoLevelGridKernelSource,"-I " + Deployment::CLHeadersPath,err))
		return Error;

	//Retrieving kernel objects for further use
	CLKernel *k = NULL;
	if (Success != _tlgProgram->getKernel("prepareDataKernel",k,err))
		return Error;
	_prepareDataKernel.reset(k);

	if (Success != _tlgProgram->getKernel("writePairsKernel",k,err))
		return Error;
	_writePairsKernel.reset(k);

	if (Success != _tlgProgram->getKernel("extractCellRangesKernel",k,err))
		return Error;
	_writeCellRangesKernel.reset(k);

	if (Success != _tlgProgram->getKernel("countLeavesAndFillCellKernel",k,err))
		return Error;
	_countLeafCellsKernel.reset(k);

	if (Success != _tlgProgram->getKernel("updateTopLevelCellsWithLeafRange",k,err))
		return Error;
	_updateTopLevelCellsWithLeafRangeKernel.reset(k);

	if (Success != _tlgProgram->getKernel("prepareGridDataForLeaves",k,err))
		return Error;
	_prepareLeafDataKernel.reset(k);
	
	if (Success != _tlgProgram->getKernel("writeLeafPairsKernel",k,err))
		return Error;
	_writeLeafPairsKernel.reset(k);

	if (Success != _tlgProgram->getKernel("extractLeafCellsKernel",k,err))
		return Error;
	_extractLeafCellsKernel.reset(k);

	if (Success != _tlgProgram->getKernel("generateContactsKernel",k,err))
		return Error;
	_generateContactsKernel.reset(k);

	if (Success != _tlgProgram->getKernel("generateContacts2Kernel",k,err))
		return Error;
	_generateContacts2Kernel.reset(k);

	if (Success != _context.getDevice().getWorkGroupDimensions().getMaxWorkGroupSize(_maxWorkgroupSize,err))
		return Error;
	
	if(Success != _context.getDevice().getMemoryInfo().getLocalMemSize(_deviceLocalMemory,err))
		return Error;

	if (Success != _context.getMaximalLaunchExecParams(*_writePairsKernel,_processors,_wavefront,err))
		return Error;

	return Success;
}

/**Performs initialization of a frame for Two Level Grid acceleration structure
* This initialization shoud be performed before each frame. Grid property change will take effect
* only after execution of this function.
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result TwoLevelGridManager::initializeFrame(Common::Errata& err)
{
	
	calculateGridData();
	_cellsCount = _hostGrid.resX * _hostGrid.resY * _hostGrid.resZ; 
	_cellsCountPowOfTwo = largestPowerOfTwo(_cellsCount) << 1;
		
	_numPrimitives = SCENE_HEADER(_scene.getHostSceneData())->totalNumberOfTriangles;
	_numPrimitivesPowOfTwo = (largestPowerOfTwo(_numPrimitives) << 1);

	size_t countersArraySize = sizeof(CL_UINT) * max(_numPrimitivesPowOfTwo,_cellsCountPowOfTwo);
	_counters.reset(new CLBuffer(_context,countersArraySize,CLBufferFlags::CLBufferAccess::ReadWrite));
			
	CL_UINT prefixSumOutputArraySize = countersArraySize;
	_prefixSumOutput.reset(new CLBuffer(_context,prefixSumOutputArraySize,CLBufferFlags::CLBufferAccess::ReadWrite));

	CL_UINT cellRangesArraySize = _cellsCount * sizeof(CL_UINT2);
	_cellRangesArray.reset(new CLBuffer(_context,cellRangesArraySize,CLBufferFlags::CLBufferAccess::ReadWrite));
		
	CL_UINT topLevelCellsArraySize = _cellsCount * sizeof(struct TopLevelCell);
	_topLevelCellsArray.reset(new CLBuffer(_context,topLevelCellsArraySize,CLBufferFlags::CLBufferAccess::ReadWrite));
		
	//Buffer Initialization
	CL_UINT pattern = 0;
	if (Success != _context.enqueueFillBuffer(_counters->getCLMem(),&pattern,_counters->getActualSize(),sizeof(CL_UINT),err))
		return Error;

	if (Success != _context.enqueueFillBuffer(_prefixSumOutput->getCLMem(),&pattern,_prefixSumOutput->getActualSize(),sizeof(CL_UINT),err))
		return Error;

	CL_UINT2 pattern2;
	pattern2.x = pattern2.y = 0;
	if (Success != _context.enqueueFillBuffer(_cellRangesArray->getCLMem(),&pattern2,_cellRangesArray->getActualSize(),sizeof(CL_UINT2),err))
		return Error;

	struct TopLevelCell cellPattern;
	memset(&cellPattern,0,sizeof(struct TopLevelCell));
	if (Success != _context.enqueueFillBuffer(_topLevelCellsArray->getCLMem(),&cellPattern,_topLevelCellsArray->getActualSize(),sizeof(struct TopLevelCell),err))
		return Error;

	return Success;
}

/**Constructs Two-Level Grid acceleration structure, according to associated scene state
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result TwoLevelGridManager::construct(Common::Errata& err)
{
	//Load grid to GPU
	_deviceTopLevelGrid.reset(new CLBuffer(_context,sizeof(struct GridData),&_hostGrid,CLBufferFlags::ReadOnly));
		
	//Prepare data prepare kernel execution
	CL_UINT workSize = closestMultipleTo(SCENE_HEADER(_scene.getHostSceneData())->totalNumberOfTriangles,_wavefront);
	CLEvent evt;
	{
		SET_KERNEL_ARGS((*_prepareDataKernel),_scene.getDeviceSceneData(),_deviceTopLevelGrid->getCLMem(),_counters->getCLMem());
		CLKernelWorkDimension globalDim(1,workSize);
		CLKernelWorkDimension localDim(1,_wavefront);
	
		CLKernelExecuteParams prepareDataKernelExecParams(&globalDim,&localDim,&evt);

		//Call the data prepare kernel
		if(Success != _context.enqueueKernel(*_prepareDataKernel,prepareDataKernelExecParams,err))
			return Error;
		if (Success != _context.flushQueue(err))
			return Error;
		if (Success != evt.wait(err))
			return Error;
	}

	//Calculate the prefix sum
	if (Success != _prefixSumCalculator->computePrefixSum(_counters->getCLMem(),_prefixSumOutput->getCLMem(),_numPrimitivesPowOfTwo,err))
		return Error;

	_pairsCount = 0;

	if (Success != _context.enqueueReadBuffer(_prefixSumOutput->getCLMem(),&_pairsCount,(_numPrimitives-1) * sizeof(CL_UINT),sizeof(CL_UINT),err))
		return Error;
	_pairsCountPowOfTwo = largestPowerOfTwo(_pairsCount) << 1;

	
	//Reallocating pairs array
	CL_UINT pairsArraySize = _pairsCountPowOfTwo * sizeof(CL_UINT2);
	if (_pairsArray)
		_pairsArray->resize(pairsArraySize);
	else
		_pairsArray.reset(new CLBuffer(_context,pairsArraySize,CLBufferFlags::ReadWrite));
	

	CL_UINT2 pattern;
	pattern.x = pattern.y = UINT_MAX;
	if (Success != _context.enqueueFillBuffer(_pairsArray->getCLMem(),&pattern,_pairsArray->getActualSize(),sizeof(CL_UINT2),err))
		return Error;

	//Prepare write pairs kernel execution
	{
		SET_KERNEL_ARGS((*_writePairsKernel),_scene.getDeviceSceneData(),_deviceTopLevelGrid->getCLMem(),
			_prefixSumOutput->getCLMem(),
			_counters->getCLMem(),
			_pairsArray->getCLMem());

		CLKernelWorkDimension globalDim(1,workSize);
		CLKernelWorkDimension localDim(1,_wavefront);
		CLKernelExecuteParams writePairsKernelExecParams(&globalDim,&localDim,&evt);

		//Call the write pairs kernel
		if(Success != _context.enqueueKernel(*_writePairsKernel,writePairsKernelExecParams,err))
			return Error;
		if (Success != _context.flushQueue(err))
			return Error;
		if (Success != evt.wait(err))
			return Error;
	}

	//Sorting the pairs
	if (Success != _bitonicSorter->sort(_pairsArray->getCLMem(),_pairsCountPowOfTwo,err))
		return Error;

	//Preparing counters array for reuse
	{
		CL_UINT pattern = 0;
		if (Success != _context.enqueueFillBuffer(_counters->getCLMem(),&pattern,_counters->getActualSize(),sizeof(CL_UINT),err))
			return Error;

		if (Success != _context.enqueueFillBuffer(_prefixSumOutput->getCLMem(),&pattern,_prefixSumOutput->getActualSize(),sizeof(CL_UINT),err))
			return Error;
	}

	//Writing cell Ranges
	{
		SET_KERNEL_ARGS((*_writeCellRangesKernel),_pairsArray->getCLMem(),_pairsCount,_cellRangesArray->getCLMem());
		CL_UINT availableLocalArraySize = _deviceLocalMemory / sizeof(CL_UINT2);
		CL_UINT requiredLocalArraySize = (_wavefront + 1) * sizeof(CL_UINT2);
		if (requiredLocalArraySize > availableLocalArraySize)
		{
			FILL_ERRATA(err,"Not enough local memory on device!");
			return Error;
		}
		CLKernelArgument localBufferArg(((CL_UINT)requiredLocalArraySize));
		
		CLKernelArgument workingBlockArg = _wavefront;
		if (Success != _writeCellRangesKernel->setKernelArgument(localBufferArg,3,err))
			return Error;
		if (Success != _writeCellRangesKernel->setKernelArgument(_wavefront,4,err))
			return Error;

		CLKernelWorkDimension globalDim(1,closestMultipleTo(_pairsCount,_wavefront));
		CLKernelWorkDimension localDim(1,_wavefront);
		CLKernelExecuteParams writeCellRangesKernelExecParams(&globalDim,&localDim,&evt);

		//Call the write pairs kernel
		if(Success != _context.enqueueKernel(*_writeCellRangesKernel,writeCellRangesKernelExecParams,err))
			return Error;
		if (Success != _context.flushQueue(err))
			return Error;
		if (Success != evt.wait(err))
			return Error;
	}

	//Fill the top level cells data - Resolution and range, count leaf cells
	{
		SET_KERNEL_ARGS((*_countLeafCellsKernel),_cellRangesArray->getCLMem(),_counters->getCLMem(),
			_topLevelCellsArray->getCLMem(),_cellsCount,_deviceTopLevelGrid->getCLMem());

		CLKernelWorkDimension globalDim(1,closestMultipleTo(_cellsCount,_wavefront));
		CLKernelWorkDimension localDim(1,_wavefront);
		CLKernelExecuteParams countLeafCellsKernelExecParams(&globalDim,&localDim,&evt);

		//Call the write pairs kernel
		if(Success != _context.enqueueKernel(*_countLeafCellsKernel,countLeafCellsKernelExecParams,err))
			return Error;
		if (Success != _context.flushQueue(err))
			return Error;
		if (Success != evt.wait(err))
			return Error;

	}

	//Prefix sum to find out how many total leaf cells do we have
	//Calculate the prefix sum
	if (Success != _prefixSumCalculator->computePrefixSum(_counters->getCLMem(),_prefixSumOutput->getCLMem(),_cellsCountPowOfTwo,err))
		return Error;

	//Get the leaf cells count
	if (Success != _context.enqueueReadBuffer(_prefixSumOutput->getCLMem(),&_leafCellsCount,(_cellsCount-1) * sizeof(CL_UINT),sizeof(CL_UINT),err))
		return Error;

	
	//Fill the top level cells data - The beginning of leaf range
	{
		SET_KERNEL_ARGS((*_updateTopLevelCellsWithLeafRangeKernel),_topLevelCellsArray->getCLMem(),_prefixSumOutput->getCLMem(),_cellsCount);

		CLKernelWorkDimension globalDim(1,closestMultipleTo(_cellsCount,_wavefront));
		CLKernelWorkDimension localDim(1,_wavefront);
		CLKernelExecuteParams updateTopLevelCellsWithLeafRangeKernelExecParams(&globalDim,&localDim,&evt);

		//Call the updating kernel
		if(Success != _context.enqueueKernel(*_updateTopLevelCellsWithLeafRangeKernel,updateTopLevelCellsWithLeafRangeKernelExecParams,err))
			return Error;
		if (Success != _context.flushQueue(err))
			return Error;
		if (Success != evt.wait(err))
			return Error;
	}

	//Count leaf pairs
	{
		//Reallocate array of counters if necessary
		CL_ULONG requiredCounterArraySize = _pairsCountPowOfTwo * sizeof(CL_UINT);
		
		_counters->resize(requiredCounterArraySize);
		_prefixSumOutput->resize(requiredCounterArraySize);
		
		SET_KERNEL_ARGS((*_prepareLeafDataKernel),_scene.getDeviceSceneData(),_pairsArray->getCLMem(),_pairsCount,_deviceTopLevelGrid->getCLMem(),_topLevelCellsArray->getCLMem(),_counters->getCLMem());
		CL_UINT workSize = _pairsCountPowOfTwo;
		CLKernelWorkDimension globalDim(1,workSize);
		CLKernelWorkDimension localDim(1,_wavefront);
		CLKernelExecuteParams prepareLeafDataKernelExecParams(&globalDim,&localDim,&evt);

		//Call the counting kernel
		if(Success != _context.enqueueKernel(*_prepareLeafDataKernel,prepareLeafDataKernelExecParams,err))
			return Error;
		if (Success != _context.flushQueue(err))
			return Error;
		if (Success != evt.wait(err))
			return Error;
		
		//And, do the prefix sum to calculate the amount of leaf pairs
		if (Success != _prefixSumCalculator->computePrefixSum(_counters->getCLMem(),_prefixSumOutput->getCLMem(),workSize,err))
			return Error;

		//Fill the leaf pairs count
		if (Success != _context.enqueueReadBuffer(_prefixSumOutput->getCLMem(),&_leafPairsCount,_prefixSumOutput->getSize() - sizeof(CL_UINT),sizeof(CL_UINT),err))
			return Error;
	}

	//Allocate Leaf Pairs
	_leafPairsCountPowOfTwo = largestPowerOfTwo(_leafPairsCount) << 1;

	//Allocating pairs array accordingly
	size_t leafPairsArraySize = _leafPairsCountPowOfTwo * sizeof(CL_UINT2); 
	if (_leafPairsArray)
		_leafPairsArray->resize(leafPairsArraySize);
	else
		_leafPairsArray.reset(new CLBuffer(_context,leafPairsArraySize,CLBufferFlags::CLBufferAccess::ReadWrite));
	
	{
		CL_UINT2 pattern;
		pattern.x = pattern.y = UINT_MAX;
		if (Success != _context.enqueueFillBuffer(_leafPairsArray->getCLMem(),&pattern,_leafPairsArray->getActualSize(),sizeof(CL_UINT2),err))
			return Error;
	}
	
	//write leaf pairs
	{
		SET_KERNEL_ARGS((*_writeLeafPairsKernel),_scene.getDeviceSceneData(),_pairsArray->getCLMem(),_topLevelCellsArray->getCLMem(),_deviceTopLevelGrid->getCLMem(),_prefixSumOutput->getCLMem(),_counters->getCLMem(),_leafPairsArray->getCLMem(),_pairsCount);
		CLKernelWorkDimension globalDim(1,closestMultipleTo(_pairsCount,_wavefront));
		CLKernelWorkDimension localDim(1,_wavefront);
		CLKernelExecuteParams writeLeafPairsKernelExecParams(&globalDim,&localDim,&evt);
		
		//Call the write leaf pairs kernel
		if(Success != _context.enqueueKernel(*_writeLeafPairsKernel,writeLeafPairsKernelExecParams,err))
			return Error;
		if (Success != _context.flushQueue(err))
			return Error;
		if (Success != evt.wait(err))
			return Error;
	}

	//Sorting the leaf pairs
	if (Success != _bitonicSorter->sort(_leafPairsArray->getCLMem(),_leafPairsCountPowOfTwo,err))
		return Error;

	//Allocating leaf cell ranges array
	if (_leafCellRangesArray)
		_leafCellRangesArray->resize(sizeof(CL_UINT2) * _leafCellsCount);
	else
		_leafCellRangesArray.reset(new CLBuffer(_context,sizeof(CL_UINT2) * _leafCellsCount,CLBufferFlags::CLBufferAccess::ReadWrite));

	//Extract Leaf Ranges
	{
		SET_KERNEL_ARGS((*_extractLeafCellsKernel),_leafPairsArray->getCLMem(),_leafPairsCount,_leafCellRangesArray->getCLMem());
		CL_UINT availableLocalArraySize = _deviceLocalMemory / sizeof(CL_UINT2);
		CL_UINT requiredLocalArraySize = (_wavefront + 1) * sizeof(CL_UINT2);
		if (requiredLocalArraySize > availableLocalArraySize)
		{
			FILL_ERRATA(err,"Not enough local memory on device!");
			return Error;
		}
		CLKernelArgument localBufferArg(((CL_UINT)requiredLocalArraySize));
		//CLKernelArgument workingBlockArg = _wavefront;
		if (Success != _extractLeafCellsKernel->setKernelArgument(localBufferArg,3,err))
			return Error;
		
		CLKernelWorkDimension globalDim(1,closestMultipleTo(_leafPairsCount,_wavefront));
		CLKernelWorkDimension localDim(1,_wavefront);
		CLKernelExecuteParams extractLeafCellsKernelExecParams(&globalDim,&localDim,&evt);

		//Call the write pairs kernel
		if(Success != _context.enqueueKernel(*_extractLeafCellsKernel,extractLeafCellsKernelExecParams,err))
			return Error;
		if (Success != _context.flushQueue(err))
			return Error;
		if (Success != evt.wait(err))
			return Error;
	}

	return Success;
}

/**Generates hit data for viewing rays, from the constructed Two-Level Grid
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result TwoLevelGridManager::generateContacts(Camera& cam,Common::Errata& err)
{
	_deviceCamera.reset(new CLBuffer(_context,sizeof(struct Camera),&cam,CLBufferFlags::ReadOnly));
	size_t resolution = cam.resX * cam.resY;
	if (_primaryContactsArray)
		_primaryContactsArray->resize(resolution * sizeof(struct Contact));
	else
		_primaryContactsArray.reset(new CLBuffer(_context,resolution * sizeof(struct Contact),CLBufferFlags::ReadWrite));

	SET_KERNEL_ARGS((*_generateContactsKernel),_deviceCamera->getCLMem(),
											   _scene.getDeviceSceneData(),
											   _deviceTopLevelGrid->getCLMem(),
											   _topLevelCellsArray->getCLMem(),
											   _leafCellRangesArray->getCLMem(),
											   _leafPairsArray->getCLMem(),
											   _primaryContactsArray->getCLMem());

	CLEvent evt;

	cl_uint totalWorkItems = closestMultipleTo(resolution,_wavefront);
	cl_uint workGroup = _wavefront; 
	CLKernelWorkDimension globalDim(1,totalWorkItems);
	CLKernelWorkDimension localDim(1,workGroup);
	CLKernelExecuteParams contactsKernelExecParams(&globalDim,&localDim,&evt);

	//Execute kernel
	if (Success != _context.enqueueKernel((*_generateContactsKernel),contactsKernelExecParams,err))
		return Error;

	//Flush and make sure that calculation is over before returning control
	if (Success != _context.flushQueue(err))
		return Error;

	if (Success != evt.wait(err))
		return Error;

	return Success;

}

/**Generates contacts for rays and fills the contacts array
* @param rays The rays to be traced - A device memory buffer object that contains rays as struct Ray
* @param contact The target device memory that will contain the result - For each ray rays[i] the buffer will contain
*                data about its closest intersection with object in the scene as contacts[i]. The contact data will be stored
*                as struct Contact
* @param rayCount The number of rays to trace
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result TwoLevelGridManager::generateContacts(CLBuffer& rays,CLBuffer& contacts, const unsigned int rayCount, Errata& err)
{

SET_KERNEL_ARGS((*_generateContacts2Kernel),rays.getCLMem(),
											rayCount,
											   _scene.getDeviceSceneData(),
											   _deviceTopLevelGrid->getCLMem(),
											   _topLevelCellsArray->getCLMem(),
											   _leafCellRangesArray->getCLMem(),
											   _leafPairsArray->getCLMem(),
											  contacts.getCLMem());

	CLEvent evt;
	cl_uint totalWorkItems = closestMultipleTo(rayCount,_wavefront);
	cl_uint workGroup = _wavefront; 
	CLKernelWorkDimension globalDim(1,totalWorkItems);
	CLKernelWorkDimension localDim(1,workGroup);
	CLKernelExecuteParams contactsKernelExecParams(&globalDim,&localDim,&evt);

	//Execute kernel
	if (Success != _context.enqueueKernel((*_generateContacts2Kernel),contactsKernelExecParams,err))
		return Error;

	//Flush and make sure that calculation is over before returning control
	if (Success != _context.flushQueue(err))
		return Error;

	if (Success != evt.wait(err))
		return Error;

	return Success;

}

/*********************************************************
* Utility functions
**********************************************************/

/**
* Calculates grid resolution according to grid properties
* @return Grid resolution
*/
CL_UINT3 TwoLevelGridManager::getResolution() const
{
	struct AABB bounds = getBounds();
	float deltaX = bounds.bounds[1].x - bounds.bounds[0].x;
	float deltaY = bounds.bounds[1].y - bounds.bounds[0].y;
	float deltaZ = bounds.bounds[1].z - bounds.bounds[0].z;
	float volume = boxVolume(bounds);
	float prims = SCENE_HEADER(_scene.getHostSceneData())->totalNumberOfTriangles;
	float a = pow(_topLevelDensity * prims / volume,oneThird);
	CL_UINT3 result;
	fillVector3(result,deltaX * a,deltaY * a, deltaZ * a);
	return result;
}

/**
* Calculates internal grid data for use on GPU
* @return
*/
void TwoLevelGridManager::calculateGridData()
{
	struct GridData grid;
	//Calculating grid basic data
	grid.box = SCENE_HEADER(_scene.getHostSceneData())->modelsBoundingBox;
	cl_uint3 resolution = getResolution();
	grid.resX = resolution.x;
	grid.resY = resolution.y;
	grid.resZ = resolution.z;

	grid.stepX = (grid.box.bounds[1].x - grid.box.bounds[0].x)/grid.resX;
	grid.stepY = (grid.box.bounds[1].y - grid.box.bounds[0].y)/grid.resY;
	grid.stepZ = (grid.box.bounds[1].z - grid.box.bounds[0].z)/grid.resZ;
	grid.leafDensity = getLeafDensity();

	_hostGrid = grid;
}

/**
* Retrieves bounding box of associated scene
* @return Bounding box of associated scene
*/
struct AABB TwoLevelGridManager::getBounds() const
{
	return SCENE_HEADER(_scene.getHostSceneData())->modelsBoundingBox;
}

