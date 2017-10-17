//------------------------------------------------------------------------------
#include "chai3d.h"
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#ifndef MACOSX
#include "GL/glut.h"
#else
#include "GLUT/glut.h"
#endif
//------------------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#include <iostream>

using namespace chai3d;
using namespace std;
#pragma comment(lib,"ws2_32.lib")

SOCKET sock;          // this is the socket that we will use it
SOCKET sock2[200];    // this is the sockets that will be recived from the Clients and sended to them
SOCKADDR_IN i_sock2;  // this will containt informations about the clients connected to the server
SOCKADDR_IN i_sock;   // this will containt some informations about our socket
WSADATA Data;         // this is to save our socket version
int clients = 0;      // we will use it in the accepting clients

struct MyPacket
{
	int mylong;
	char mystring[256];
};


struct XYpacket
{
	double x;
	double y;
};

struct ForceToApply
{
	double x;
	double y;
};

//------------------------------------------------------------------------------
// GENERAL SETTINGS
//------------------------------------------------------------------------------

// stereo Mode
/*
C_STEREO_DISABLED:            Stereo is disabled
C_STEREO_ACTIVE:              Active stereo for OpenGL NVDIA QUADRO cards
C_STEREO_PASSIVE_LEFT_RIGHT:  Passive stereo where L/R images are rendered next to each other
C_STEREO_PASSIVE_TOP_BOTTOM:  Passive stereo where L/R images are rendered above each other
*/
cStereoMode stereoMode = C_STEREO_DISABLED;

// fullscreen mode
bool fullscreen = false;



//------------------------------------------------------------------------------
// DECLARED VARIABLES
//------------------------------------------------------------------------------

// a world that contains all objects of the virtual environment
cWorld* world;

// a camera to render the world in the window display
cCamera* camera;

// a light source to illuminate the objects in the world
cDirectionalLight *light;

// a haptic device handler
cHapticDeviceHandler* handler;

// a pointer to the current haptic device
// cGenericHapticDevicePtr hapticDevice;

// number of haptic devices
const int numHapticDevices = 2;

// a pointer to the current haptic device
cGenericHapticDevicePtr hapticDevices[numHapticDevices];

// a label to display the haptic device model
cLabel* labelHapticDeviceModel[numHapticDevices];

// a label to display the position [m] of the haptic device
cLabel* labelHapticDevicePosition[numHapticDevices];

// a global variable to store the position [m] of the haptic device
cVector3d hapticDevicePosition[numHapticDevices];

// a label to display the rate [Hz] at which the simulation is running
cLabel* labelHapticRate[numHapticDevices];

// a small sphere (cursor) representing the haptic device 
cToolCursor* cursor[numHapticDevices];

// audio device to play sound
cAudioDevice* audioDevice;

// audio buffers to store sound files
cAudioBuffer* audioBuffer1;

// audio buffer to store sound of a drill
cAudioBuffer* audioBufferDrill;

// audio source of a drill
cAudioSource* audioSourceDrill;

// rackets
cMesh* controller1;
cMesh* controller2;


// objects
cShapeSphere* object1;
cShapeSphere* object0;
cShapeSphere* object02;
cShapeSphere* object4;
cShapeSphere* object42;

//walls
cMesh* object2;
cMesh* object3;

// sphere objects
cShapeSphere* ball;

// linear velocity of each sphere
cVector3d ballVel;


// flag for using damping (ON/OFF)
bool useDamping = false;

// flag for using force field (ON/OFF)
bool useForceField = false;
bool useVibration = false;

// controller setup

cVector3d desiredPosition;
double desiredY = 0.0;
double desiredZ = 0.01;
double kp = 25;


// flag to indicate if the haptic simulation currently running
bool simulationRunning = false;

// flag to indicate if the haptic simulation has terminated
bool simulationFinished = true;

// frequency counter to measure the simulation haptic rate
cFrequencyCounter frequencyCounter;

// information about computer screen and GLUT display window
int screenW;
int screenH;
int windowW;
int windowH;
int windowPosX;
int windowPosY;

int width;
int height;
float ball_dir_x = -0.1f;
float ball_dir_y = 0.0f;

//score variables
int score_player1 = 0;
int score_player2 = 0;
char buffer1[15];
char buffer2[5];

// root resource path
string resourceRoot;
// convert to resource path
#define RESOURCE_PATH(p)    (char*)((resourceRoot+string(p)).c_str())

//------------------------------------------------------------------------------
// DECLARED FUNCTIONS
//------------------------------------------------------------------------------

// callback when the window display is resized
void resizeWindow(int w, int h);

// callback when a key is pressed
void keySelect(unsigned char key, int x, int y);

// callback to render graphic scene
void updateGraphics(void);

// callback of GLUT timer
void graphicsTimer(int data);

// function that closes the application
void close(void);

// main haptics simulation loop
void updateHaptics(void);

const char *score_string(int player1, int player2);


const char *score_string(int player1, int player2){ //convert integers scores into chars
	_itoa(score_player1, buffer1, 10);
	_itoa(score_player2, buffer2, 10);
	strcat(buffer1, " : ");
	strcat(buffer1, buffer2);

	return buffer1;
}

// Server functions
int StartServer(int Port);
int Send(char *Buf, int len, int Client);
int Recive(char *Buf, int len, int Client);
void PressEnterToContinue();
int EndSocket();


//==============================================================================
/*
DEMO:  main
*/
//==============================================================================

int main(int argc, char* argv[])
{

	// Start server
	StartServer(8000);

	// Sending 
	MyPacket packet1;
	packet1.mylong = 123;
	const char *hi = "Ping";
	int x = strcpy_s(packet1.mystring, sizeof packet1.mystring, hi);
	cout << "Sending Ping to Clients" << endl;
	Send((char *)&packet1, sizeof(packet1)+1, 0);
	Send((char *)&packet1, sizeof(packet1)+1, 1);

	// Receiving
	MyPacket packet21;
	Recive((char *)&packet21, sizeof(packet21), 0);
	cout << "Received from client 1:" << packet21.mystring << endl;

	MyPacket packet22;
	Recive((char *)&packet22, sizeof(packet22), 1);
	cout << "Received from client 2:" << packet22.mystring << endl;

	PressEnterToContinue();
	desiredPosition.set(0.0, desiredY, desiredZ);


	//--------------------------------------------------------------------------
	// INITIALIZATION
	//--------------------------------------------------------------------------

	cout << endl;
	cout << "-----------------------------------" << endl;
	// cout << "CHAI3D" << endl;
	// cout << "Demo: 01-mydevice" << endl;
	// cout << "Copyright 2003-2016" << endl;
	// cout << "-----------------------------------" << endl << endl << endl;
	cout << "Keyboard Options:" << endl << endl;
	cout << "[1] - Enable/Disable magnets" << endl;
	cout << "[2] - Enable/Disable fluid" << endl;
	//cout << "[3] - Enable/Disable vibration" << endl;
	cout << "[f] - Enable/Disable full screen mode" << endl;
	cout << "[x] - Exit application" << endl;
	cout << endl << endl;

	// parse first arg to try and locate resources
	resourceRoot = string(argv[0]).substr(0, string(argv[0]).find_last_of("/\\") + 1);

	//--------------------------------------------------------------------------
	// OPENGL - WINDOW DISPLAY
	//--------------------------------------------------------------------------

	// initialize GLUT
	glutInit(&argc, argv);

	// retrieve  resolution of computer display and position window accordingly
	screenW = glutGet(GLUT_SCREEN_WIDTH);
	screenH = glutGet(GLUT_SCREEN_HEIGHT);
	windowW = (int)(0.8 * screenH);
	windowH = (int)(0.5 * screenH);
	windowPosY = (screenH - windowH) / 2;
	windowPosX = windowPosY;

	// initialize the OpenGL GLUT window
	glutInitWindowPosition(windowPosX, windowPosY);
	glutInitWindowSize(windowW, windowH);

	if (stereoMode == C_STEREO_ACTIVE)
		glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE | GLUT_STEREO);
	else
		glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);

	// create display context and initialize GLEW library
	glutCreateWindow(argv[0]);

#ifdef GLEW_VERSION
	// initialize GLEW
	glewInit();
#endif

	// setup GLUT options
	glutDisplayFunc(updateGraphics);
	glutKeyboardFunc(keySelect);
	glutReshapeFunc(resizeWindow);
	glutSetWindowTitle("CHAI3D");

	// set fullscreen mode
	if (fullscreen)
	{
		glutFullScreen();
	}


	//--------------------------------------------------------------------------
	// WORLD - CAMERA - LIGHTING
	//--------------------------------------------------------------------------

	// create a new world.
	world = new cWorld();

	// set the background color of the environment
	world->m_backgroundColor.setBlack();

	// create a camera and insert it into the virtual world
	camera = new cCamera(world);
	world->addChild(camera);

	// position and orient the camera
	camera->set(cVector3d(1.0, 0.0, 0.0),    // camera position (eye)
		cVector3d(0.0, 0.0, 0.0),    // look at position (target)
		cVector3d(0.0, 0.0, 1.0));   // direction of the (up) vector

	// set the near and far clipping planes of the camera
	camera->setClippingPlanes(0.01, 10.0);

	// set stereo mode
	camera->setStereoMode(stereoMode);

	// set stereo eye separation and focal length (applies only if stereo is enabled)
	camera->setStereoEyeSeparation(0.01);
	camera->setStereoFocalLength(0.5);


	// create a directional light source
	light = new cDirectionalLight(world);

	// insert light source inside world
	world->addChild(light);

	// enable light source
	light->setEnabled(true);

	// define direction of light beam
	light->setDir(-0.5, 0, 0.5);


	// create a font
	cFont *font = NEW_CFONTCALIBRI20();

	//--------------------------------------------------------------------------
	// HAPTIC DEVICE
	//--------------------------------------------------------------------------
	//// create a haptic device handler
	handler = new cHapticDeviceHandler();

	int i = 0;
	while (i < numHapticDevices)
	{
		cGenericHapticDevicePtr newHapticDevice;

		// get a handle to the first haptic device
		handler->getDevice(newHapticDevice, i);

		hapticDevices[i] = newHapticDevice;


		// retrieve information about the current haptic device
		cHapticDeviceInfo info = hapticDevices[i]->getSpecifications();

		// create a 3D tool and add it to the world
		cursor[i] = new cToolCursor(world);
		camera->addChild(cursor[i]);

		// create a label to display the haptic device model
		labelHapticDeviceModel[i] = new cLabel(font);
		camera->m_frontLayer->addChild(labelHapticDeviceModel[i]);

		cursor[i]->setLocalPos(-1.0, i * 1 - 0.5, 0.0);

		// connect the haptic device to the tool
		cursor[i]->setHapticDevice(hapticDevices[i]);

		// define a radius for the tool
		cursor[i]->setRadius(0.0);

		// map the physical workspace of the haptic device to a larger virtual workspace.
		//cursor[i]->setWorkspaceRadius(5);
		cursor[i]->setWorkspaceScaleFactor(5);

		//	// haptic forces are enabled only if small forces are first sent to the device;
		//	// this mode avoids the force spike that occurs when the application starts when 
		//	// the tool is located inside an object for instance. 
		cursor[i]->setWaitForSmallForce(true);

		// start the haptic tool
		cursor[i]->start();


		i++;
	}



	//--------------------------------------------------------------------------
	// SETUP AUDIO MATERIAL
	//--------------------------------------------------------------------------

	// create an audio device to play sounds
	audioDevice = new cAudioDevice();

	// attach audio device to camera
	camera->attachAudioDevice(audioDevice);

	// create an audio buffer and load audio wave file
	audioBuffer1 = audioDevice->newAudioBuffer();
	bool fileload1 = audioBuffer1->loadFromFile(RESOURCE_PATH("../resources/sounds/metal-scraping.wav"));
	if (!fileload1)
	{
#if defined(_MSVC)
		fileload1 = audioBuffer1->loadFromFile("../../../bin/resources/sounds/metal-scraping.wav");
#endif
	}

	// check for errors
	if (!(fileload1))
	{
		cout << "Error - Sound file failed to load or initialize correctly." << endl;
		close();
		return (-1);
	}



	//--------------------------------------------------------------------------
	// SETUP AUDIO FOR A DRILL
	//--------------------------------------------------------------------------

	audioBufferDrill = audioDevice->newAudioBuffer();
	bool fileload5 = audioBufferDrill->loadFromFile(RESOURCE_PATH("../resources/sounds/drill.wav"));
	if (!fileload5)
	{
#if defined(_MSVC)
		fileload5 = audioBufferDrill->loadFromFile("../../../bin/resources/sounds/drill.wav");
#endif
	}
	if (!fileload5)
	{
		cout << "Error - Sound file failed to load or initialize correctly." << endl;
		close();
		return (-1);
	}

	// create audio source
	audioSourceDrill = audioDevice->newAudioSource();

	// assign auio buffer to audio source
	audioSourceDrill->setAudioBuffer(audioBufferDrill);

	// loop playing of sound
	audioSourceDrill->setLoop(true);

	// turn off sound for now
	audioSourceDrill->setGain(0.0);

	// set pitch
	audioSourceDrill->setPitch(0.2);

	// play sound
	audioSourceDrill->play();


	//--------------------------------------------------------------------------
	// Object BALL
	//--------------------------------------------------------------------------

	// create a sphere and define its radius
	ball = new cShapeSphere(0.015);


	// add sphere primitive to world
	world->addChild(ball);

	ball->m_material->setWhite();

	//////////////////////////////////////////////////////////////////////////////
	//// SHAPE - Controller
	//////////////////////////////////////////////////////////////////////////////

	controller1 = new cMesh();
	cCreateBox(controller1, 0.1, 0.02, 0.1);
	world->addChild(controller1);
	controller1->m_material->setGrayDark();


	controller2 = new cMesh();
	cCreateBox(controller2, 0.1, 0.02, 0.1);
	world->addChild(controller2);
	controller2->m_material->setGrayDark();


	// retrieve information about the current haptic device
	cHapticDeviceInfo info = hapticDevices[0]->getSpecifications();

	// read the scale factor between the physical workspace of the haptic
	// device and the virtual workspace defined for the tool
	double workspaceScaleFactor = cursor[0]->getWorkspaceScaleFactor();

	// get properties of haptic device
	double maxLinearForce = cMin(info.m_maxLinearForce, 7.0);
	double maxStiffness = info.m_maxLinearStiffness / workspaceScaleFactor;
	double maxDamping = info.m_maxLinearDamping / workspaceScaleFactor;

	////////////////////////////////////////////////////////////////////////
	// OBJECT 0: "Magnet"
	////////////////////////////////////////////////////////////////////////

	//----------------------------------------------------------------------
	// Object Magnet for left racket

	// create a mesh
	object0 = new cShapeSphere(0.025);

	// add object to world
	world->addChild(object0);

	// set the position of the object
	object0->setLocalPos(0.0, -0.6, 0.0);


	// load texture map
	bool fileload;
	object0->m_texture = cTexture2d::create();
	fileload = object0->m_texture->loadFromFile(RESOURCE_PATH("../resources/images/spheremap-3.jpg"));
	if (!fileload)
	{
#if defined(_MSVC)
		fileload = object0->m_texture->loadFromFile("../../../bin/resources/images/spheremap-3.jpg");
#endif
	}
	if (!fileload)
	{
		cout << "Error - Texture image failed to load correctly." << endl;
		close();
		return (-1);
	}

	// set graphic properties
	object0->m_texture->setSphericalMappingEnabled(true);
	object0->setUseTexture(true);
	object0->m_material->setWhite();


	// set haptic properties
	object0->m_material->setStiffness(0.4 * maxStiffness);          // % of maximum linear stiffness
	object0->m_material->setMagnetMaxForce(0.6 * maxLinearForce);   // % of maximum linear force 
	object0->m_material->setMagnetMaxDistance(0.5);
	object0->m_material->setViscosity(0.1 * maxDamping);            // % of maximum linear damping

	// create a haptic surface effect
	object0->createEffectSurface();

	// create a haptic magnetic effect
	object0->createEffectMagnetic();

	// create a haptic viscous effect
	object0->createEffectViscosity();
	object0->setEnabled(false);

	//----------------------------------------------------------------------
	// Object Magnet for right racket

	// create a mesh
	object02 = new cShapeSphere(0.025);

	// add object to world
	world->addChild(object02);

	// set the position of the object
	object02->setLocalPos(0.0, 0.6, 0.0);


	// load texture map
	object02->m_texture = cTexture2d::create();
	fileload = object02->m_texture->loadFromFile(RESOURCE_PATH("../resources/images/spheremap-3.jpg"));
	if (!fileload)
	{
#if defined(_MSVC)
		fileload = object02->m_texture->loadFromFile("../../../bin/resources/images/spheremap-3.jpg");
#endif
	}
	if (!fileload)
	{
		cout << "Error - Texture image failed to load correctly." << endl;
		close();
		return (-1);
	}

	// set graphic properties
	object02->m_texture->setSphericalMappingEnabled(true);
	object02->setUseTexture(true);
	object02->m_material->setWhite();


	// set haptic properties
	object02->m_material->setStiffness(0.4 * maxStiffness);          // % of maximum linear stiffness
	object02->m_material->setMagnetMaxForce(0.6 * maxLinearForce);   // % of maximum linear force 
	object02->m_material->setMagnetMaxDistance(0.5);
	object02->m_material->setViscosity(0.1 * maxDamping);            // % of maximum linear damping

	// create a haptic surface effect
	object02->createEffectSurface();

	// create a haptic magnetic effect
	object02->createEffectMagnetic();

	// create a haptic viscous effect
	object02->createEffectViscosity();
	object02->setEnabled(false);

	////////////////////////////////////////////////////////////////////////
	// OBJECT 1: "FLUID"
	////////////////////////////////////////////////////////////////////////

	// create a sphere and define its radius
	object1 = new cShapeSphere(0.8);

	// add object to world
	world->addChild(object1);

	// set the position of the object at the center of the world
	object1->setLocalPos(0.0, 0, 0.0);

	// load texture map
	object1->m_texture = cTexture2d::create();
	fileload = object1->m_texture->loadFromFile(RESOURCE_PATH("../resources/images/spheremap-2.jpg"));
	if (!fileload)
	{
#if defined(_MSVC)
		fileload = object1->m_texture->loadFromFile("../../../bin/resources/images/spheremap-2.jpg");
#endif
	}
	if (!fileload)
	{
		cout << "Error - Texture image failed to load correctly." << endl;
		close();
		return (-1);
	}

	// set graphic properties
	object1->m_material->m_ambient.set(0.1, 0.1, 0.6, 0.5);
	object1->m_material->m_diffuse.set(0.3, 0.3, 0.9, 0.5);
	object1->m_material->m_specular.set(1.0, 1.0, 1.0, 0.5);
	object1->m_material->setWhite();
	object1->setTransparencyLevel(0.6);
	object1->setUseTexture(true);
	object1->m_texture->setSphericalMappingEnabled(true);
	// set haptic properties
	object1->m_material->setViscosity(0.9 * maxDamping);    // % of maximum linear damping

	// create a haptic viscous effect
	object1->createEffectViscosity();
	object1->setEnabled(false);

	/////////////////////////////////////////////////////////////////////////
	// OBJECT 2: WALL BOTTOM
	/////////////////////////////////////////////////////////////////////////

	// create a mesh
	object2 = new cMesh();

	// create plane
	cCreatePlane(object2, 0.4, 1.2);


	// add object to world
	world->addChild(object2);

	// set the position of the object
	object2->setLocalPos(0.0, 0.0, -0.365);

	object2->m_material->setGreenForest();
	object2->setEnabled(true);

	/////////////////////////////////////////////////////////////////////////
	// OBJECT 3: WALL TOP
	/////////////////////////////////////////////////////////////////////////

	// create a mesh
	object3 = new cMesh();

	// create plane
	cCreatePlane(object3, 0.4, 1.2);


	// add object to world
	world->addChild(object3);

	// set the position of the object
	object3->setLocalPos(0.0, 0.0, 0.365);

	object3->m_material->setBlueLightSky();

	////////////////////////////////////////////////////////////////////////
	// OBJECT 4: "VIBRATIONS"
	////////////////////////////////////////////////////////////////////////

	//----------------------------------------------------------------------
	// Object Vibration for left racket

	// create a sphere and define its radius
	object4 = new cShapeSphere(0.3);

	// add object to world
	world->addChild(object4);

	// set the position of the object at the center of the world
	object4->setLocalPos(0.0, -0.5, 0);

	// load texture map
	object4->m_texture = cTexture2d::create();
	fileload = object4->m_texture->loadFromFile(RESOURCE_PATH("../resources/images/spheremap-4.jpg"));
	if (!fileload)
	{
#if defined(_MSVC)
		fileload = object4->m_texture->loadFromFile("../../../bin/resources/images/spheremap-4.jpg");
#endif
	}
	if (!fileload)
	{
		cout << "Error - Texture image failed to load correctly." << endl;
		close();
		return (-1);
	}

	// set graphic properties
	object4->m_texture->setSphericalMappingEnabled(true);
	object4->setUseTexture(true);
	object4->m_material->setWhite();
	object4->setTransparencyLevel(0.5);

	// set haptic properties
	object4->m_material->setVibrationFrequency(50);
	object4->m_material->setVibrationAmplitude(0.2 * maxLinearForce);   // % of maximum linear force
	//object4->m_material->setStiffness(0.1 * maxStiffness);              // % of maximum linear stiffness

	// create a haptic vibration effect
	object4->createEffectVibration();

	object4->setEnabled(false);


	//----------------------------------------------------------------------
	// Object Vibration for right racket

	// create a sphere and define its radius
	object42 = new cShapeSphere(0.3);

	// add object to world
	world->addChild(object42);

	// set the position of the object at the center of the world
	object42->setLocalPos(0.0, 0.5, 0);

	// load texture map
	object42->m_texture = cTexture2d::create();
	fileload = object42->m_texture->loadFromFile(RESOURCE_PATH("../resources/images/spheremap-4.jpg"));
	if (!fileload)
	{
#if defined(_MSVC)
		fileload = object42->m_texture->loadFromFile("../../../bin/resources/images/spheremap-4.jpg");
#endif
	}
	if (!fileload)
	{
		cout << "Error - Texture image failed to load correctly." << endl;
		close();
		return (-1);
	}

	// set graphic properties
	object42->m_texture->setSphericalMappingEnabled(true);
	object42->setUseTexture(true);
	object42->m_material->setWhite();
	object42->setTransparencyLevel(0.5);

	// set haptic properties
	object42->m_material->setVibrationFrequency(50);
	object42->m_material->setVibrationAmplitude(0.2 * maxLinearForce);   // % of maximum linear force

	// create a haptic vibration effect
	object42->createEffectVibration();

	object42->setEnabled(false);

	//--------------------------------------------------------------------------
	// START SIMULATION
	//--------------------------------------------------------------------------

	// create a thread which starts the main haptics rendering loop
	cThread* hapticsThread = new cThread();
	hapticsThread->start(updateHaptics, CTHREAD_PRIORITY_HAPTICS);

	// setup callback when application exits
	atexit(close);

	// start the main graphics rendering loop
	glutTimerFunc(50, graphicsTimer, 0);
	glutMainLoop();

	// exit
	return (0);
}

//------------------------------------------------------------------------------

void resizeWindow(int w, int h)
{
	windowW = w;
	windowH = h;
}

//------------------------------------------------------------------------------

void keySelect(unsigned char key, int x, int y)
{
	// option ESC: exit
	if ((key == 27) || (key == 'x'))
	{
		exit(0);
	}

	// option 1: enable/disable force field
	if (key == '1')
	{
		useForceField = !useForceField;
		if (useForceField)
			cout << "> Enable magnets            \r";
		else
			cout << "> Disable magnets           \r";

		if (useForceField){
			object0->setEnabled(true);
			object02->setEnabled(true);
		}
		else{
			object0->setEnabled(false);
			object02->setEnabled(false);
		}
	}

	// option 2: enable/disable damping
	if (key == '2')
	{
		useDamping = !useDamping;
		if (useDamping)
			cout << "> Enable fluid             \r";

		else
			cout << "> Disable fluid            \r";

		if (useDamping)
			object1->setEnabled(true);
		else
			object1->setEnabled(false);
	}

	// option f: toggle fullscreen
	if (key == 'f')
	{
		if (fullscreen)
		{
			windowPosX = glutGet(GLUT_INIT_WINDOW_X);
			windowPosY = glutGet(GLUT_INIT_WINDOW_Y);
			windowW = glutGet(GLUT_INIT_WINDOW_WIDTH);
			windowH = glutGet(GLUT_INIT_WINDOW_HEIGHT);
			glutPositionWindow(windowPosX, windowPosY);
			glutReshapeWindow(windowW, windowH);
			fullscreen = false;
		}
		else
		{
			glutFullScreen();
			fullscreen = true;
		}
	}

}

//------------------------------------------------------------------------------

void close(void)
{
	// stop the simulation
	simulationRunning = false;

	// wait for graphics and haptics loops to terminate
	while (!simulationFinished) { cSleepMs(100); }

	// close/stop streaming objects
	delete audioDevice;

	// close haptic device
	cursor[0]->stop();
	cursor[1]->stop();
}

//------------------------------------------------------------------------------

void graphicsTimer(int data)
{
	if (simulationRunning)
	{
		glutPostRedisplay();
	}

	glutTimerFunc(50, graphicsTimer, 0);
}

//------------------------------------------------------------------------------

void updateGraphics(void)
{

	/////////////////////////////////////////////////////////////////////
	// RENDER SCENE
	/////////////////////////////////////////////////////////////////////

	// render world
	camera->renderView(windowW, windowH);

	// swap buffers
	glutSwapBuffers();

	// wait until all GL commands are completed
	glFinish();

	// check for any OpenGL errors
	GLenum err;
	err = glGetError();
	if (err != GL_NO_ERROR) cout << "Error:  %s\n" << gluErrorString(err);
}

//------------------------------------------------------------------------------

void updateHaptics(void)
{

	// initialize frequency counter
	frequencyCounter.reset();

	// simulation in now running
	simulationRunning = true;
	simulationFinished = false;

	int zeit = 0.0;

	// main haptic simulation loop
	while (simulationRunning)
	{

		/////////////////////////////////////////////////////////////////////
		// DISPLAY SCORE
		/////////////////////////////////////////////////////////////////////

		string text = score_string(score_player1, score_player2);
		labelHapticDeviceModel[0]->setText(text);
		labelHapticDeviceModel[0]->setLocalPos(405, 15, 0);

		if (score_player1 == 10){
			labelHapticDeviceModel[1]->setLocalPos(40, 0, 0);
			labelHapticDeviceModel[1]->setText("PLAYER 1 WINS");
			score_player1 = 0;
			score_player2 = 0;
			object42->setEnabled(false);
			object4->setEnabled(false);
		}
		else if (score_player2 == 10){
			labelHapticDeviceModel[1]->setLocalPos(700, 0, 0);
			labelHapticDeviceModel[1]->setText("PLAYER 2 WINS");
			score_player1 = 0;
			score_player2 = 0;
			object42->setEnabled(false);
			object4->setEnabled(false);
		}

		if (score_player1 >= 8){
			object42->setEnabled(true);
		}
		if (score_player2 >= 8){
			object4->setEnabled(true);
		}

		if (score_player1 == 1 && score_player2 != 10 || score_player1 != 10 && score_player2 == 1){
			labelHapticDeviceModel[1]->setText("");
		}

		/////////////////////////////////////////////////////////////////////
		// READ HAPTIC DEVICE
		/////////////////////////////////////////////////////////////////////


		int j = 0;
		while (j < numHapticDevices)
		{
			// compute global reference frames for each object
			world->computeGlobalPositions(true);

			// update position and orientation of tool
			cursor[j]->updateFromDevice();
			// compute interaction forces
			cursor[j]->computeInteractionForces();
			// send forces to haptic device
			cursor[j]->applyToDevice();

			j++;
		}

		//************************************************************************************************************************************
		// Receiving
		XYpacket XYpacket21;
		Recive((char *)&XYpacket21, sizeof(XYpacket21), 0);
		//cout << "player 1: X= " << XYpacket21.x << " Y= " << XYpacket21.y << endl;

		//cVector3d pos0;
		// hapticDevices[0]->getPosition(pos0);
		cursor[0]->setLocalPos((XYpacket21.y, XYpacket21.x, 0.0));

		// Receiving
		XYpacket XYpacket22;
		Recive((char *)&XYpacket22, sizeof(XYpacket22), 1);
		//cout << "player 2: X= " << XYpacket22.x << " Y= " << XYpacket22.y << endl;

		//cVector3d pos1;
		// hapticDevices[1]->getPosition(pos1);
		cursor[1]->setLocalPos((XYpacket22.y, XYpacket22.x, 0.0));


		//cVector3d position_c1 = (cVector3d(XYpacket21.y, XYpacket21.x, 0.0)*cursor[0]->getWorkspaceScaleFactor() + cursor[0]->getGlobalPos());
		//cVector3d position_c2 = (cVector3d(XYpacket22.y, XYpacket22.x, 0.0)*cursor[1]->getWorkspaceScaleFactor() + cursor[1]->getGlobalPos());


		cVector3d position_c1 = (cVector3d(0.0, XYpacket21.x, XYpacket21.y)*cursor[0]->getWorkspaceScaleFactor() + cursor[0]->getGlobalPos());
		cVector3d position_c2 = (cVector3d(0.0, XYpacket22.x, XYpacket22.y)*cursor[1]->getWorkspaceScaleFactor() + cursor[1]->getGlobalPos());

		//cout << "client1: y=  " << position_c1.y() << " z = " << position_c1.z() << endl;
		//cout << "client2: y=  " << position_c2.y() << " z = " << position_c2.z() << endl;

		//************************************************************************************************************************************

		double temp_y;
		if (position_c1.y() > -0.45)
			temp_y = -0.45;
		else if (position_c1.y() < -0.55)
			temp_y = -0.55;
		else
			temp_y = position_c1.y();


		double temp2_y;
		if (position_c2.y() < 0.45)
			temp2_y = 0.45;
		else if (position_c2.y() > 0.55)
			temp2_y = 0.55;
		else
			temp2_y = position_c2.y();

		controller1->setLocalPos(0, temp_y, position_c1.z());
		controller2->setLocalPos(0, temp2_y, position_c2.z());

		cVector3d ball_pos = ball->getLocalPos();
		cVector3d controller1_pos = controller1->getLocalPos();
		cVector3d controller2_pos = controller2->getLocalPos();

		// hit by left racket
		if (ball_pos.y() <= controller1_pos.y() + 0.025 && ball_pos.y() >= controller1_pos.y()
			&& ball_pos.z() <= controller1_pos.z() + 0.065
			&& ball_pos.z() >= controller1_pos.z() - 0.065) {
			// set fly direction depending on where it hit the racket
			// (t is 0.5 if hit at top, 0 at center, -0.5 at bottom)
			float t = ((ball_pos.z() - controller1_pos.z()) / 0.1) / 2;
			ball_dir_x = fabs(ball_dir_x); // force it to be positive
			ball_dir_y = t;

			// set audio gain
			audioSourceDrill->setGain(1.0);
			zeit = 0;
		}



		// hit by right racket
		if (ball_pos.y() >= (controller2_pos.y() - 0.025) && ball_pos.y() <= controller2_pos.y()
			&& ball_pos.z() <= (controller2_pos.z() + 0.065) // 0.065 (Racketh\F6he + Ballradius) und 0.025 (Racketbreite + Ballradius)
			&& ball_pos.z() >= controller2_pos.z() - 0.065) {
			// set fly direction depending on where it hit the racket
			// (t is 0.5 if hit at top, 0 at center, -0.5 at bottom)
			float t = ((ball_pos.z() - controller2_pos.z()) / 0.1) / 2;
			ball_dir_x = -fabs(ball_dir_x); // force it to be negative
			ball_dir_y = t;
			// set audio gain
			audioSourceDrill->setGain(1.0);
			zeit = 0;
		}

		if (zeit > 50){
			audioSourceDrill->setGain(0.0);
		}


		// hit left wall
		if (ball_pos.y() < -0.75) {
			++score_player2;
			ball->setLocalPos(0, 0, 0);
			ball_dir_x = fabs(ball_dir_x); // force it to be positive
			ball_dir_y = 0;
		}

		// hit right wall
		if (ball_pos.y() > 0.75) {
			++score_player1;
			ball->setLocalPos(0, 0, 0);
			ball_dir_x = -fabs(ball_dir_x); // force it to be negative
			ball_dir_y = 0;
		}

		// hit top wall
		if (ball_pos.z() > 0.3) {
			ball_dir_y = -fabs(ball_dir_y); // force it to be negative
		}

		// hit bottom wall
		if (ball_pos.z() < -0.3) {
			ball_dir_y = fabs(ball_dir_y); // force it to be positive
		}

		ballVel = cVector3d(0, ball_dir_x, ball_dir_y);
		ballVel.normalizer(ballVel);

		// compute position
		cVector3d spherePos = ball->getLocalPos() + 0.0005 * ballVel;

		// update value to sphere object
		ball->setLocalPos(spherePos);

		// update frequency counter
		frequencyCounter.signal(1);
		zeit++;
	}

	// exit haptics thread
	simulationFinished = true;
}

//------------------------- Server functions -----------------------------------------------------
int StartServer(int Port)
{
	// Creating the socket
	int err;
	WSAStartup(MAKEWORD(2, 2), &Data);
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		Sleep(4000);
		exit(0);
		return 0;
	}
	i_sock.sin_family = AF_INET;
	i_sock.sin_addr.s_addr = htonl(INADDR_ANY);
	i_sock.sin_port = htons(Port);

	// Binding the socket
	err = ::bind(sock, (LPSOCKADDR)&i_sock, sizeof(i_sock));
	if (err != 0) { return 0; }

	// Listen for connections
	err = listen(sock, 2);
	if (err == SOCKET_ERROR) { return 0; }

	// Accept connections
	while (clients < 2)
	{
		int so2len = sizeof(i_sock2);
		sock2[clients] = accept(sock, (sockaddr *)&i_sock2, &so2len);
		if (sock2[clients] == INVALID_SOCKET) { return 0; }
		printf("client %d has joined the server(IP: %d)\n", clients, i_sock2.sin_addr.s_addr);
		clients++;
	}
	return 1;
}


int Send(char *Buf, int len, int Client)
{
	int slen;
	slen = send(sock2[Client], Buf, len, 0);
	if (slen < 0)
	{
		printf("Cannot send data !");
		return 1;
	}
	return slen;
}

int Recive(char *Buf, int len, int Client)
{
	int slen = recv(sock2[Client], Buf, len, 0);
	if (slen < 0)
	{
		printf("Cannot receive data !");
		return 1;
	}
	return slen;
}

void PressEnterToContinue()
{
	int c;
	printf("Press ENTER to continue... ");
	fflush(stdout);
	do c = getchar(); while ((c != '\n') && (c != EOF));
}

int EndSocket()
{
	closesocket(sock);
	WSACleanup();
	return 1;
}
//-----------------------------------------------------------------------------------------------------------
