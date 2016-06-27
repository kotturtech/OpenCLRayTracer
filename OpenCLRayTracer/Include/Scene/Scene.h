/**
 * @file Scene.h
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
 * Class Scene - Contains operation for loading a 3D scene from file, formatting the data
 * and uploading to GPU memory
 * 
 * The scene file that can be loaded by this class is a text file that contains description of an object
 * at every line.
 * Supported types of objects are: SCENE,LIGHT, and MESH.
 * Light example: 
 *
 * LIGHT 0.0 10.0 -10.0 1000.0
 *
 * Light above is located at x-y-z position (first 3 floats) and the light intensity is the last number
 *
 * Sphere example:
 *
 * SPHERE 0.0 0.0 50.0 5.0
 *
 * Sphere above is located at x-y-z position, and has radius set by the fourth number
 *
 * Mesh example:
 *
 * MESH models\swfighter\star-wars-vader-tie-fighter.obj
 *
 * Mesh above is loaded from file set by the path, that follows the word MESH, relative to working directory.
 * The mesh must be in Wavefront OBJ format, and the material file also should be located at the indicated path.
 */

#ifndef CL_RT_SCENE
#define CL_RT_SCENE

#include <OpenCLUtils\CLExecutionContext.h>
#include <OpenCLUtils\CLBuffer.h>
#include <Common\Errata.h>

namespace CLRayTracer
{

		/**
		* Class Scene - Contains operation for loading a 3D scene from file, formatting the data
		* and uploading to GPU memory
		*/
		class Scene
		{
		public:
			/**constructor*/
			Scene();
			/**destructor*/
			virtual ~Scene();
			/**Loads scene from file. The expected scene file format is explained above
			* @param filename Scene file to load
			* @param [out]err Error info, if error occurred
			* @return Result that indicates whether the operation succeeded or failed
			*/
			Common::Result load(const char* filename,Common::Errata& err);
			
			/**Loads scene from host memory to device memory
			* @param OpenCL context
			* @param [out]err Error info, if error occurred
			* @return Result that indicates whether the operation succeeded or failed
			*/
			Common::Result loadToGPU(const OpenCLUtils::CLExecutionContext& context, Common::Errata& err);
			
			/** Returns pointer to Device memory, that contains the scene
			* @return Returns pointer to Device memory, that contains the scene
			*/
			inline const cl_mem& getDeviceSceneData() const {return _deviceSceneData->getCLMem();}
			
			/** Returns pointer to Host memory, that contains the scene
			* @return Returns pointer to Host memory, that contains the scene
			*/
			inline const char* const getHostSceneData() const {return _hostSceneData;}

			/**Returns size of scene data in bytes
			*  @return Size of scene data in bytes
			*/
			inline cl_ulong getSceneDataSize() const {return _sceneDataSize;}
			
		private:
			char* _hostSceneData;
			cl_ulong _sceneDataSize;
			boost::shared_ptr<OpenCLUtils::CLBuffer> _deviceSceneData;
		};
}

#endif //CL_RT_SCENE