/**
 * @file OpenCLRayTracerDemo.cpp
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
 * Demo application for CLRayTracerEngine - An example of library usage
 *
 */

#include <iostream>
#include <string>
#include <cmath>
#include <GL\glew.h>
#include <GL\glut.h>
#include <OpenCLUtils\CLInterface.h>
#include <OpenCLUtils\CLGLInteropContext.h>
#include <OpenCLUtils\CLBuffer.h>
#include <Scene\Scene.h>
#include <CLData\MeshUtils.h>
#include <CLData\CLStructs.h>
#include <CLData\Shading.h>
#include <Scene\SceneDebug.h>
#include <Common\Deployment.h>
#include <Algorithms\BVHManager.h>
#include <Algorithms\TwoLevelGridManager.h>

using namespace std;
using namespace CLRayTracer;
using namespace CLRayTracer::OpenCLUtils;
using namespace CLRayTracer::Common;
using namespace CLRayTracer::CLGLInterop;
using namespace AccelerationStructures;

/**************************************************************************
* Configuration variables
***************************************************************************/
enum AccelerationStruct {INVALID = -1, BVH = 0, GRID = 1};
string Deployment::CLHeadersPath;
AccelerationStruct accelerationStructInUse;
int window_width;
int window_height;
size_t pixelCount;
string scenePath;

//Configuration switch constants
const string WINHEIGHT = "-winH";
const string WINWIDTH = "-winW";
const string ACCSTRUCT = "-accStruct";
const string HDRPATH = "-headersPath";
const string SCENEPATH = "-scene";

const string BVH_VAL = "BVH";
const string GRID_VAL = "GRID";


/**************************************************************************
* Global variables for usage in GLUT callbacks
***************************************************************************/
//GL related variables
int glutWindowHandle = 0;
float translate_z = -1.0f;

// mouse controls
int mouse_old_x = 0, mouse_old_y = 0;
int mouse_buttons = 0;

//CL/GL Memory Objects
boost::shared_ptr<CLGLMemoryBuffer> colorBuffer;
boost::shared_ptr<CLGLMemoryBuffer> posBuffer;
std::vector<cl_mem> CLGLMemBuffers;
//host equivalents of CL/GL buffers
boost::shared_array<cl_float4> colors;
boost::shared_array<cl_float4> vertices;
 
//OpenCL objects
const CLGLExecutionContext* glExecContext;
CLEvent evt;

//Ray Tracing Related Objects
boost::shared_ptr<Scene> scene;
boost::shared_ptr<AccelerationStructureManager> accelerationStruct;

//Camera position and orientation
Camera camera;
CL_FLOAT3 cameraPosition;
struct Quaternion cameraOrientation;

/**************************************************************************
* Utility functions and callback - Declarations
***************************************************************************/
void appRender();
void appDestroy();
void timerCB(int ms);
void appKeyboard(unsigned char key, int x, int y);
void appMouse(int button, int state, int x, int y);
void appMotion(int x, int y);
void initGL(int argc, char** argv);
bool configure(int argc, char* argv[]);
void printUsage();
void RTFrame();

/**************************************************************************
* Main Function
***************************************************************************/

//Fast error check macro
#define CHECKED_CALL(result) if (result != Success){std::cout << *err; int x; cin >> x; return -1;}

//Main function
int main(int argc, char* argv[])
{
	//1.Fill configuration according to command line parameters
	if (!configure(argc,argv))
	{
		printUsage();
		return 0;
	}
	pixelCount = window_width * window_height;
	cout << "Configuration parameters:" << endl
		 <<	"    Window width: " << window_width << endl
		 << "    Window heigth: " << window_height << endl
		 << "    Acceleration struct: " << (accelerationStructInUse == BVH ? "BVH" : "GRID") << endl
		 << "    CL Headers Path: " << Deployment::CLHeadersPath << endl
		 << "    Scene file: " << scenePath << endl;

	colors.reset(new cl_float4[pixelCount]);
	memset(colors.get(),0,pixelCount * sizeof(cl_float4));
	vertices.reset(new cl_float4[pixelCount]);
 
	//2.Initialize GL library - Must be called before attempting to initiate GL/CL Interop
	cout << "Initializing GL Library....." <<endl;
	initGL(argc,argv);
	
	//3.Initialize OpenCL
	cout << "Initializing OpenCL....." <<endl;
	CLInterface c;
	boost::shared_ptr<Errata> err(new Errata());
	CHECKED_CALL(c.initCL(*err));
	
	//4.Initialize OpenCL/OpenGL interop context
	cout << "Initializing OpenCL/OpenCL Interop....." <<endl;
	CLGLInteropContext glInteropContext;
	CHECKED_CALL(glInteropContext.initialize(c,*err));
	cout << "*******Interop Platform to be used:******* " << endl << *glInteropContext.getInteropPlatform() << endl;
	cout << "********Interop Device to be used:******** " << endl << *glInteropContext.getInteropDevice() << endl;	
	glExecContext = glInteropContext.getExecutionContext();

	//5.Loading the scene
	cout << "Loading scene: " << scenePath << " - This may take a while if the scene is large......"; 
	scene.reset(new Scene);
	CHECKED_CALL(scene->load(scenePath.c_str(),*err));
	SceneHeader* sceneHeader = SCENE_HEADER(scene->getHostSceneData());
	cout << "Scene loaded!" << endl;
	{
		string units = "bytes";
		unsigned long sceneDataSize = sceneHeader->totalDataSize;
		if (sceneDataSize > 10240)
		{
			units = "KB";
			sceneDataSize/=1024;
		}
		if (sceneDataSize > 10240)
		{
			units = "MB";
			sceneDataSize/=1024;
		}

		cout << "Scene info:" << endl
			<< "   Bounds: Min: X=" << sceneHeader->modelsBoundingBox.bounds[0].x << " Y=" << sceneHeader->modelsBoundingBox.bounds[0].y << " Z=" << sceneHeader->modelsBoundingBox.bounds[0].z << endl 
		    << "           Max: X=" << sceneHeader->modelsBoundingBox.bounds[1].x << " Y=" << sceneHeader->modelsBoundingBox.bounds[1].y << " Z=" << sceneHeader->modelsBoundingBox.bounds[1].z << endl
			<< "   Num of Materials: " << sceneHeader->numberOfMaterials << endl
			<< "   Num of Lights: " << sceneHeader->numberOfLights << endl
			<< "   Num of Tris: " << sceneHeader->totalNumberOfTriangles << endl
			<< "   Total scene data size: " << sceneDataSize << " " << units << endl;
		
		cout << "Loading scene to GPU memory....";
		CHECKED_CALL(scene->loadToGPU(*glExecContext,*err));
		cout << "Done!"  << endl;
	}

	//6.Initializing Camera Position
	camera.resX=window_width;
	camera.resY=window_height;
	camera.FOVDistance=FOVDistFromAngle(90,window_width,window_height);
	camera.supersampilngFactor = 1;

	//Set the camera to look at the model
	CL_FLOAT3 sceneHalfSizes;
	sceneHalfSizes.x = (sceneHeader->modelsBoundingBox.bounds[1].x - sceneHeader->modelsBoundingBox.bounds[0].x) * 0.5f;
	sceneHalfSizes.y = (sceneHeader->modelsBoundingBox.bounds[1].y - sceneHeader->modelsBoundingBox.bounds[0].y) * 0.5f;
	sceneHalfSizes.z = (sceneHeader->modelsBoundingBox.bounds[1].z - sceneHeader->modelsBoundingBox.bounds[0].z) * 0.5f;
	
	cameraPosition.x = sceneHeader->modelsBoundingBox.bounds[0].x + sceneHalfSizes.x;
	cameraPosition.y = sceneHeader->modelsBoundingBox.bounds[0].y + sceneHalfSizes.y;
	cameraPosition.z = sceneHeader->modelsBoundingBox.bounds[0].z - sceneHalfSizes.z;
	camera.viewTransform = identityTransform();
	cameraOrientation = zeroRotation();
	
	//7.Constructing Acceleration Structure Managing Object
	cout << "Initializing acceleration structure manager: " << (accelerationStructInUse == BVH ? "BVH" : "GRID") << ".......";
	if ( accelerationStructInUse == BVH)
		accelerationStruct.reset(new BVHManager(*glExecContext,*scene));
	else
		accelerationStruct.reset(new TwoLevelGridManager(*glExecContext,*scene));
	CHECKED_CALL(accelerationStruct->initialize(*err));
	cout << "Done!" << endl;

	//8.Initializing memory and constructing the acceleration structure
	
	/*Technically, it would be more accurate to put those operations into the frame rendering logic,
	 *to be performed every frame. 
	 *In this example, however, we do not change the scene at all - Therefore there is no need to 
	 *reconstruct the acceleration structure, once constructed - It will always be valid.
	 */
	cout << "Initializing memory for frame.....";
	CHECKED_CALL(accelerationStruct->initializeFrame(*err));
	cout << "Completed!" << endl;
	cout << "Constructing AC....";
	CHECKED_CALL(accelerationStruct->construct(*err));
	cout << "Completed!" << endl;
	cout << "USE ARROW KEYS AND MOUSE (While holding LMB) TO CONTROL THE CAMERA" << endl;

	//9.Initializing and Allocating CL/GL buffers for shading
	
	//initialize pos buffer
	for(int idx = 0; idx < pixelCount; idx++)
	{
		//Just paint the whole screen
		cl_float4 pos;
		pos.x = translateScale(0.0f,window_width,(idx%window_width),-1.0f,1.0f); 
		pos.y = translateScale(0.0f,window_height,(idx/window_height),-1.0f,1.0f);
		pos.z = 0.0f;
		pos.w = 1.0f;
		vertices[idx] = pos;
	}
	//Allocate buffers for CL/GL interop
	CHECKED_CALL(glExecContext->createCLGLBuffer(vertices.get(),pixelCount*sizeof(cl_float4),posBuffer,*err));
	CHECKED_CALL(glExecContext->createCLGLBuffer(colors.get(),pixelCount*sizeof(cl_float4),colorBuffer,*err));
	
	CLGLMemBuffers.push_back(colorBuffer->getCLBuffer());
	CLGLMemBuffers.push_back(posBuffer->getCLBuffer());

	glutMainLoop();

	return 0;
}

/**************************************************************************
* Ray Tracing Frame Rendering Logic
***************************************************************************/
void RTFrame()
{
	Errata err;
	//Setup camera
	normalizeQuaternion(&cameraOrientation);
	setOrientationAndPos(&camera.viewTransform,cameraOrientation,cameraPosition);
	//Using acceleration structures to generate contacts for primary rays
	Result res = accelerationStruct->generateContacts(camera,err);
	if (res != Success)
	{
		cout << "Rendering failed! Reason:" << endl;
		cout << err;
		return;
	}

	//Copying resulting contacts to host for shading
	int contactsCount = accelerationStruct->getPrimaryContacts()->getActualSize() / sizeof(struct Contact);
	boost::shared_array<struct Contact> contacts(new struct Contact[contactsCount]);
	res = accelerationStruct->getPrimaryContacts()->copyToHost(contacts.get(),err);
	if (res != Success)
	{
		cout << "Rendering failed! Reason:" << endl;
		cout << err;
		return;
	}

	//Clearing colors buffer, to avoid mess when moving camera
	memset(colors.get(),0,pixelCount * sizeof(cl_float4));

	//Doing the shading - on host
	for (int i = 0; i <contactsCount; i++)
	{
		initVector4(color,0.0f,0.0f,0.0f,1.0f);
		struct Contact contact = contacts[i];
		if (contact.contactDist > 0)
		{
			cpuShadeBlinnPhong(&camera,&contact,(char*)scene->getHostSceneData(),color);
			colors[contact.pixelIndex].x = color.x;
			colors[contact.pixelIndex].y = color.y;
			colors[contact.pixelIndex].z = color.z;
			colors[contact.pixelIndex].w = color.w;
			color.x = 0.0;
			color.y = 0.0;
			color.z = 0.0;
		}
	}

	//Upload generated data to GPU
	res = glExecContext->enqueueWriteBuffer(colors.get(),CLGLMemBuffers[0],pixelCount*sizeof(cl_float4),err);
	if (res != Success)
	{
		cout << "Rendering failed! Reason:" << endl;
		cout << err;
		return;
	}	
}

/**************************************************************************
* GLUT callbacks
***************************************************************************/
void appRender()
{
	Errata err;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //Make sure OpenGL is done using our VBOs
	glFinish();
	
	glExecContext->enqueueAcquireGLObject(CLGLMemBuffers,evt,err);
	evt.wait(err);
	
	RTFrame();

	glExecContext->enqueueReleaseGLObject(CLGLMemBuffers,evt,err);
	evt.wait(err);
	glExecContext->finishQueue(err);

    //render the particles from VBOs
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POINT_SMOOTH);
    glPointSize(5.);
    
    //printf("color buffer\n");
	glBindBuffer(GL_ARRAY_BUFFER, colorBuffer->getVBOId());
    glColorPointer(4, GL_FLOAT, 0, 0);

    //printf("vertex buffer\n");
	glBindBuffer(GL_ARRAY_BUFFER, posBuffer->getVBOId());
    glVertexPointer(4, GL_FLOAT, 0, 0);

    //printf("enable client state\n");
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    //Need to disable these for blender
    glDisableClientState(GL_NORMAL_ARRAY);
  
	glDrawArrays(GL_POINTS, 0, pixelCount);

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    
    glutSwapBuffers();
	glFinish();
}

void appKeyboard(int key, int x, int y)
{
	CL_FLOAT3 forwardVec = forward(camera.viewTransform);
	const float delta = 0.3f;
	switch(key)
	{
	case GLUT_KEY_LEFT:
		{
			CL_FLOAT3 sideVec = side(camera.viewTransform);
			cameraPosition.x+=(sideVec.x * delta);
			cameraPosition.y+=(sideVec.y * delta);
			cameraPosition.z+=(sideVec.z * delta);
			break;
		}
	case GLUT_KEY_UP:
		{
			cameraPosition.x+=(forwardVec.x * delta);
			cameraPosition.y+=(forwardVec.y * delta);
			cameraPosition.z+=(forwardVec.z * delta);
			break;
		}
	case GLUT_KEY_RIGHT:
		{
			CL_FLOAT3 sideVec = side(camera.viewTransform);
			cameraPosition.x+=(sideVec.x * -delta);
			cameraPosition.y+=(sideVec.y * -delta);
			cameraPosition.z+=(sideVec.z * -delta);
			break;
			break;
		}
	case GLUT_KEY_DOWN:
		{
			cameraPosition.x+=(forwardVec.x * -delta);
			cameraPosition.y+=(forwardVec.y * -delta);
			cameraPosition.z+=(forwardVec.z * -delta);
			break;
		}
	}
	glutPostRedisplay();
}

void appMouse(int button, int state, int x, int y)
{

}

void appMotion(int x, int y)
{
	int deltaX = mouse_old_x - x;
	int deltaY = mouse_old_y - y;
	float delta = 5.0f * DEG2RAD; //Delta converted to radians
	
	if (abs(deltaX) > abs(deltaY))
	{	
		if (deltaX < 0)
			delta*=-1.0f;
		CL_FLOAT3 cameraRotation;
		cameraRotation.y = delta;
		cameraRotation.x = cameraRotation.z = 0.0f;
		rotateByVector(&cameraOrientation,cameraRotation);
	}
	else
	{
		if (deltaY < 0)
			delta*=-1.0f;
		CL_FLOAT3 cameraRotation;
		cameraRotation.x = delta;
		cameraRotation.y = cameraRotation.z = 0.0f;
		rotateByVector(&cameraOrientation,cameraRotation);
	}
	mouse_old_x = x;
	mouse_old_y = y;
	glutPostRedisplay();
}

void timerCB(int ms)
{
    //Example function for calling rendering function every several ms
    //glutTimerFunc(ms, timerCB, ms);
    //glutPostRedisplay();
}

/**************************************************************************
* Utility Functions
***************************************************************************/
void initGL(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(window_width,window_height);
    glutInitWindowPosition (glutGet(GLUT_SCREEN_WIDTH)/2 - window_width/2, 
                            glutGet(GLUT_SCREEN_HEIGHT)/2 - window_height/2);

    
    std::stringstream ss;
    ss << "OpenCL Ray Tracer Demo" << std::ends;
    glutWindowHandle = glutCreateWindow(ss.str().c_str());

    glutDisplayFunc(appRender); //main rendering function
    glutTimerFunc(30, timerCB, 30); //determine a minimum time between frames
    glutSpecialFunc(appKeyboard);
    glutMouseFunc(appMouse);
    glutMotionFunc(appMotion);

    glewInit();

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glDisable(GL_DEPTH_TEST);

    // viewport
    glViewport(0, 0, window_width, window_height);

    // projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(90.0, (GLfloat)window_width / (GLfloat) window_height, 0.1, 1000.0);

    // set view matrix
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0, 0.0, translate_z);
}

void printUsage()
{
	cout << "OpenCLRayTracer demo, copyright(c) 2016, Timur Sizov" << endl;
	cout << "Command line parameters:" << endl 
		<< WINHEIGHT << " <Window Height>" << endl
		<< WINWIDTH << " <Window Height>" << endl
		<< ACCSTRUCT << " <Acceleration Structure> - Acceleration Structure to be used, valid values: " << BVH_VAL << ", " << GRID_VAL << endl
		<< HDRPATH << " <Path to headers> Path to OpenCL headers - Needed by device compiler to compile the Ray Tracing kernels" << endl
		<< SCENEPATH << " <Path to scene file> Path to Scene file to be rendered. See example file that comes with this example" << endl;
}

bool configure(int argc, char* argv[])
{
	//Set defaults
	Deployment::CLHeadersPath = "";
	accelerationStructInUse = INVALID;
	window_width = 0;
	window_height = 0;
	pixelCount = 0;
	scenePath = "";
	
	//Find configuration parameters
	for (int i = 0; i < argc; i++)
	{
		string current = argv[i];
		if (current == WINHEIGHT)
			window_height = i+1 < argc ? atoi(argv[i+1]) : 0;	
		else if (current == WINWIDTH)
			window_width = i+1 < argc ? atoi(argv[i+1]) : 0;	
		else if (current == ACCSTRUCT)
		{
			if (i+1 < argc)
			{
				string param = argv[i+1];
				if (param == BVH_VAL)
					accelerationStructInUse = BVH;
				else if (param == GRID_VAL)
					accelerationStructInUse = GRID;
			}
		}
		else if (current == HDRPATH)
			Deployment::CLHeadersPath =  i+1 < argc ? argv[i+1] : "";	
		else if (current == SCENEPATH)
			scenePath = i+1 < argc ? argv[i+1] : "";
		
	}//for

	return window_height > 0 && window_width > 0 && accelerationStructInUse != AccelerationStruct::INVALID 
		&& !scenePath.empty() && !Deployment::CLHeadersPath.empty();

}
