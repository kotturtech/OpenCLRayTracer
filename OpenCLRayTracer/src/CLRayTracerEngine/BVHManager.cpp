/**
 * @file BVHManager.cpp
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
 * Implementation for class BVHManager, the host interface to GPU implementation of BVH
 *
 * @implNote 
 * The implemented BVH variation is based on radix trees, 
 * adopted from this source:  http://devblogs.nvidia.com/parallelforall/wp-content/uploads/2012/11/karras2012hpg_paper.pdf
 * and related blog entry:    https://devblogs.nvidia.com/parallelforall/thinking-parallel-part-iii-tree-construction-gpu/
 * Handling duplicate Morton codes implemented based on this source: https://github.com/Ooken/NekoEngine
 * 
 */

#include <bitset>
#include <Algorithms\Sorting.h>
#include <Algorithms\BVHManager.h>
#include <CLData\AccelerationStructs\BVH.h>
#include <CLData\AccelerationStructs\BVHData.h>
#include <CLData\SceneBufferParser.h>
#include <CLData\CLStructs.h>
#include <Scene\Scene.h>
#include <Common\Deployment.h>
#include <OpenCLUtils\CLBuffer.h>
#include <Testing\SortingTest.h>
#include <Testing\BVHTest.h>

using namespace std;
using namespace CLRayTracer;
using namespace CLRayTracer::OpenCLUtils;
using namespace CLRayTracer::Common;
using namespace CLRayTracer::AccelerationStructures;

/**String that contains the source of the kernels*/
extern const char* BVHKernelSource;

/**Constructor*/
BVHManager::BVHManager(const CLExecutionContext& context,const Scene& scene):AccelerationStructureManager(context,scene)
{
	_bitonicSorter.reset(new BitonicSort(context,true));
	_mortonBufferItems = 0;
	_deviceLocalMemory = 0;
	_bvhLeavesCount = 0;
	//Default memory allocation = For 20000 triangles, for single-ray per pixel 512x512 resolution
	_bvhNodes.reset(new CLBuffer(context,39999 * sizeof(struct BVHNode),CLBufferFlags::CLBufferAccess::ReadWrite));
	_sortedMortonCodes.reset(new CLBuffer(context,largestPowerOfTwo(20000*sizeof(CL_UINT2)),CLBufferFlags::CLBufferAccess::ReadWrite));
	_nodeVisitCounters.reset(new CLBuffer(context,39999 * sizeof(CL_UINT),CLBufferFlags::CLBufferAccess::ReadWrite));
	_primaryContactsArray.reset(new CLBuffer(context,512*512*sizeof(struct Contact),CLBufferFlags::CLBufferAccess::ReadWrite));
}

/**Performs main initialization of the BVH structure.
* This initialization shoud be performed just once per instance.
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result BVHManager::initialize(Errata& err)
{
	//Initialize sorter
	if (Success != _bitonicSorter->initialize(err))
		return Error;

	//Compiling of the kernels
	//Create and compile CL program
	_bvhProgram.reset(new CLProgram(_context));
	if (Success != _bvhProgram->compile(BVHKernelSource,"-I " + Deployment::CLHeadersPath,err))
		return Error;

	CLKernel *k = NULL;
	if (Success != _bvhProgram->getKernel("calculateMortonCodes",k,err))
		return Error;
	_mortonCalcKernel.reset(k);

	if (Success != _bvhProgram->getKernel("buildRadixTree",k,err))
		return Error;
	_radixTreeBuildKernel.reset(k);

	if (Success != _bvhProgram->getKernel("computeBoundingBoxes",k,err))
		return Error;
	_bbCalcKernel.reset(k);

	if (Success != _bvhProgram->getKernel("generateContacts",k,err))
		return Error;
	_contactGenerateKernel.reset(k);

	if (Success != _bvhProgram->getKernel("generateContacts2",k,err))
		return Error;
	_contactGenerateKernel2.reset(k);


	if(Success != _context.getDevice().getMemoryInfo().getLocalMemSize(_deviceLocalMemory,err))
		return Error;

	return Success;
}

/**Performs initialization of a frame for BVH acceleration structure
* This initialization shoud be performed before each frame.
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result  BVHManager::initializeFrame(Errata& err)
{
	//Calculating sizes for memory allocation
	_bvhLeavesCount = SCENE_HEADER(_scene.getHostSceneData())->totalNumberOfTriangles;
	//Desired size - the quantity of leaves and the quantity of the inner nodes of the tree
	CL_ULONG bvhNodesBufSize = _bvhLeavesCount * sizeof(BVHNode) + (_bvhLeavesCount - 1) * sizeof(BVHNode);
	//Since we use a bitonic sorter we need that the size will be a power of 2
	_mortonBufferItems = largestPowerOfTwo(_bvhLeavesCount);
	if (_mortonBufferItems < _bvhLeavesCount)
		_mortonBufferItems*=2;

	//Allocating the buffer (CLBuffer does nothing if it already has the needed memory, so no performance penalty here)
	_bvhNodes->resize(bvhNodesBufSize);
	_sortedMortonCodes->resize(_mortonBufferItems * sizeof(CL_UINT2));
	_nodeVisitCounters->resize(_bvhLeavesCount * sizeof(CL_UINT));

	//Filling the buffer with the defaut values of UINT_MAX
	
	CL_UINT2 mcPattern;
	mcPattern.x = mcPattern.y = UINT_MAX;
	if (Success != _context.enqueueFillBuffer(_sortedMortonCodes->getCLMem(),&mcPattern,_sortedMortonCodes->getActualSize(),sizeof(CL_UINT2),err))
		return Error;
	
	CL_UINT ctr = 0;
	if (Success != _context.enqueueFillBuffer(_nodeVisitCounters->getCLMem(),&ctr,_nodeVisitCounters->getActualSize(),sizeof(CL_UINT),err))
		return Error;

	return Success;
}

/**Constructs BVH acceleration structure, according to associated scene state
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result BVHManager::construct(Common::Errata& err)
{

	try
	{
		SET_KERNEL_ARGS((*_mortonCalcKernel),_bvhNodes->getCLMem(),_sortedMortonCodes->getCLMem(),_scene.getDeviceSceneData());
		SET_KERNEL_ARGS((*_radixTreeBuildKernel),_bvhNodes->getCLMem(),_sortedMortonCodes->getCLMem(),_bvhLeavesCount);
		SET_KERNEL_ARGS((*_bbCalcKernel),_bvhNodes->getCLMem(),_nodeVisitCounters->getCLMem(), _bvhLeavesCount);
	}
	catch (CLInterfaceException e)
	{
		err = Errata(e);
		return Error;
	}

	CLEvent evt;
	evt.reset();
	size_t processors, warp;
	if (Success != _context.getMaximalLaunchExecParams(*_mortonCalcKernel,processors,warp,err))
		return Error;

	CL_UINT worksize = closestMultipleTo(_bvhLeavesCount,warp);
	CLKernelWorkDimension globalDim(1,worksize);
	CLKernelWorkDimension localDim(1,warp);
	CLKernelExecuteParams mortonKernelExecParams(&globalDim,&localDim,&evt);

	//1. Morton Codes
	//Execute kernel
	if (Success != _context.enqueueKernel((*_mortonCalcKernel),mortonKernelExecParams,err))
		return Error;

	//Flush and make sure that calculation is over before returning control
	if (Success != _context.flushQueue(err))
		return Error;

	if (Success != evt.wait(err))
		return Error;

	//2.Sort the leaves by Morton Codes
	if (Success != _bitonicSorter->sort(_sortedMortonCodes->getCLMem(),_mortonBufferItems,err))
		return Error;

	//3. Build Radix Tree
	worksize = closestMultipleTo(_bvhLeavesCount-1,warp);
	CLKernelWorkDimension globalDim_Radix(1,worksize);
	CLKernelWorkDimension localDim_Radix(1,warp);
	CLKernelExecuteParams radixKernelExecParams(&globalDim_Radix,&localDim_Radix,&evt);
	//Execute kernel
	if (Success != _context.enqueueKernel((*_radixTreeBuildKernel),radixKernelExecParams,err))
		return Error;

	//Flush and make sure that calculation is over before returning control
	if (Success != _context.flushQueue(err))
		return Error;

	if (Success != evt.wait(err))
		return Error;

	worksize = closestMultipleTo(_bvhLeavesCount,warp);
	CLKernelExecuteParams boundingBoxKernelExecParams(new CLKernelWorkDimension(1,worksize),new CLKernelWorkDimension(1,warp,&evt));
	//Execute kernel
	if (Success != _context.enqueueKernel((*_bbCalcKernel),boundingBoxKernelExecParams,err))
		return Error;

	//Flush and make sure that calculation is over before returning control
	if (Success != _context.flushQueue(err))
		return Error;

	if (Success != evt.wait(err))
		return Error;

	return Success;
}

/**Generates hit data for viewing rays, from the constructed BVH
* @param err Error info
* @return Result of the operation: Success or failure
*/
Result BVHManager::generateContacts(Camera& cam,Errata& err)
{
	//Uploading camera state to GPU
	_deviceCamera.reset(new CLBuffer(_context,sizeof(struct Camera),&cam,CLBufferFlags::ReadOnly));
	
	//Allocating memory for primary contacts
	size_t contactBufSize = cam.resX * cam.resY * sizeof(struct Contact);
	if (_primaryContactsArray)
		_primaryContactsArray->resize(contactBufSize);
	else
		_primaryContactsArray.reset(new CLBuffer(_context,contactBufSize,CLBufferFlags::ReadWrite));
	
	//Getting the launch parameters
	size_t processors, warp;
	if (Success != _context.getMaximalLaunchExecParams(*_contactGenerateKernel,processors,warp,err))
		return Error;

	//Setting kernel args
	try
	{
		SET_KERNEL_ARGS((*_contactGenerateKernel),_deviceCamera->getCLMem(),_bvhNodes->getCLMem(),_bvhLeavesCount,_scene.getDeviceSceneData(),_primaryContactsArray->getCLMem());
	}
	catch (CLInterfaceException e)
	{
		err = Errata(e);
		return Error;
	}

	CLEvent evt;
	evt.reset();
	cl_uint totalWorkItems = closestMultipleTo(cam.resX * cam.resY,warp);
	CLKernelWorkDimension globalDim(1,totalWorkItems);
	CLKernelWorkDimension localDim(1,warp);
	CLKernelExecuteParams contactsKernelExecParams(&globalDim,&localDim,&evt);

	//Execute kernel
	if (Success != _context.enqueueKernel((*_contactGenerateKernel),contactsKernelExecParams,err))
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
Result BVHManager::generateContacts(OpenCLUtils::CLBuffer& rays,OpenCLUtils::CLBuffer& contacts, const unsigned int rayCount, Common::Errata& err)
{
	//Setting kernel args
	try
	{
		SET_KERNEL_ARGS((*_contactGenerateKernel2),rays.getCLMem(),rayCount,_bvhNodes->getCLMem(),_bvhLeavesCount,_scene.getDeviceSceneData(),contacts.getCLMem());
	}
	catch (CLInterfaceException e)
	{
		err = Errata(e);
		return Error;
	}

	//Getting the launch parameters
	size_t processors, warp;
	if (Success != _context.getMaximalLaunchExecParams(*_contactGenerateKernel2,processors,warp,err))
		return Error;

	CLEvent evt;
	evt.reset();
	cl_uint totalWorkItems = closestMultipleTo(rayCount,warp);
	CLKernelWorkDimension globalDim(1,totalWorkItems);
	CLKernelWorkDimension localDim(1,warp);
	CLKernelExecuteParams contactsKernelExecParams(&globalDim,&localDim,&evt);

	//Execute kernel
	if (Success != _context.enqueueKernel((*_contactGenerateKernel2),contactsKernelExecParams,err))
		return Error;

	//Flush and make sure that calculation is over before returning control
	if (Success != _context.flushQueue(err))
		return Error;

	if (Success != evt.wait(err))
		return Error;

	return Success;
}
			