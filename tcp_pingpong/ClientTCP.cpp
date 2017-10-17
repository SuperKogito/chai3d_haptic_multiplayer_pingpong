/*
Simple udp client connected to a haptic device
*/
//------------------------------------------------------------------------------
#include "chai3d.h"
//------------------------------------------------------------------------------
using namespace chai3d;
using namespace std;
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

using namespace std;
#pragma comment(lib,"ws2_32.lib")

SOCKET sock;        // this is the socket that we will use it
SOCKADDR_IN i_sock; // this will containt some informations about our socket
WSADATA Data;       // this is to save our socket version

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

// mirrored display
bool mirroredDisplay = false;


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
cGenericHapticDevicePtr hapticDevice;

// a label to display the haptic device model
cLabel* labelHapticDeviceModel;

// a label to display the position [m] of the haptic device
cLabel* labelHapticDevicePosition;

// a global variable to store the position [m] of the haptic device
cVector3d hapticDevicePosition;

// a label to display the rate [Hz] at which the simulation is running
cLabel* labelHapticRate;

// a small sphere (cursor) representing the haptic device 
cShapeSphere* cursor;

// a line representing the velocity vector of the haptic device
cShapeLine* velocity;

// flag for using damping (ON/OFF)
bool useDamping = false;

// flag for using force field (ON/OFF)
bool useForceField = false;

// controller setup

cVector3d desiredPosition;
double desiredY = 0.0;
double desiredZ = 0.01;
double kp = 250;

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


int Connect(char *IP, int Port);
int Send(char *Buf, int len);
int Recive(char *Buf, int len);
int EndSocket();

//==============================================================================
/*
main Client
*/
//==============================================================================

int main(int argc, char* argv[])
{

	Connect("10.162.40.77", 8000);

	MyPacket packet2; // this is the struct description
	Recive((char *)&packet2, sizeof(packet2)); // our function that we have created to recive data
	printf("Received: %d \r\n", packet2.mylong);
	// cout << packet2.mystring << endl;

	MyPacket packet1; // this is the struct description
	packet1.mylong = 321;
	const char *pi = "Pong";
	strcpy_s(packet1.mystring, sizeof(packet1.mystring), pi);
	Send((char *)&packet1, sizeof(packet1));

	desiredPosition.set(0.0, desiredY, desiredZ);


	//--------------------------------------------------------------------------
	// INITIALIZATION
	//--------------------------------------------------------------------------

	cout << endl;
	cout << "-----------------------------------" << endl;
	cout << "CHAI3D" << endl;
	cout << "Demo: 01-mydevice" << endl;
	cout << "Copyright 2003-2016" << endl;
	cout << "-----------------------------------" << endl << endl << endl;
	cout << "Keyboard Options:" << endl << endl;
	cout << "[1] - Enable/Disable potential field" << endl;
	cout << "[2] - Enable/Disable damping" << endl;
	cout << "[f] - Enable/Disable full screen mode" << endl;
	cout << "[m] - Enable/Disable vertical mirroring" << endl;
	cout << "[x] - Exit application" << endl;
	cout << endl << endl;


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
	camera->set(cVector3d(0.5, 0.0, 0.0),    // camera position (eye)
		cVector3d(0.0, 0.0, 0.0),    // look at position (target)
		cVector3d(0.0, 0.0, 1.0));   // direction of the (up) vector

	// set the near and far clipping planes of the camera
	camera->setClippingPlanes(0.01, 10.0);

	// set stereo mode
	camera->setStereoMode(stereoMode);

	// set stereo eye separation and focal length (applies only if stereo is enabled)
	camera->setStereoEyeSeparation(0.01);
	camera->setStereoFocalLength(0.5);

	// set vertical mirrored display mode
	camera->setMirrorVertical(mirroredDisplay);

	// create a directional light source
	light = new cDirectionalLight(world);

	// insert light source inside world
	world->addChild(light);

	// enable light source
	light->setEnabled(true);

	// define direction of light beam
	light->setDir(-1.0, 0.0, 0.0);

	// create a sphere (cursor) to represent the haptic device
	cursor = new cShapeSphere(0.01);

	// insert cursor inside world
	world->addChild(cursor);

	// create small line to illustrate the velocity of the haptic device
	velocity = new cShapeLine(cVector3d(0, 0, 0),
		cVector3d(0, 0, 0));

	// insert line inside world
	world->addChild(velocity);


	//--------------------------------------------------------------------------
	// HAPTIC DEVICE
	//--------------------------------------------------------------------------

	// create a haptic device handler
	handler = new cHapticDeviceHandler();

	// get a handle to the first haptic device
	handler->getDevice(hapticDevice, 0);

	// open a connection to haptic device
	hapticDevice->open();

	// calibrate device (if necessary)
	hapticDevice->calibrate();

	// retrieve information about the current haptic device
	cHapticDeviceInfo info = hapticDevice->getSpecifications();

	// display a reference frame if haptic device supports orientations
	if (info.m_sensedRotation == true)
	{
		// display reference frame
		cursor->setShowFrame(true);

		// set the size of the reference frame
		cursor->setFrameSize(0.05);
	}

	// if the device has a gripper, enable the gripper to simulate a user switch
	hapticDevice->setEnableGripperUserSwitch(true);

	//--------------------------------------------------------------------------
	// WIDGETS
	//--------------------------------------------------------------------------

	// create a font
	cFont *font = NEW_CFONTCALIBRI20();

	// create a label to display the haptic device model
	labelHapticDeviceModel = new cLabel(font);
	camera->m_frontLayer->addChild(labelHapticDeviceModel);
	labelHapticDeviceModel->setText(info.m_modelName);

	// create a label to display the position of haptic device
	labelHapticDevicePosition = new cLabel(font);
	camera->m_frontLayer->addChild(labelHapticDevicePosition);

	// create a label to display the haptic rate of the simulation
	labelHapticRate = new cLabel(font);
	camera->m_frontLayer->addChild(labelHapticRate);


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
			cout << "> Enable force field     \r";
		else
			cout << "> Disable force field    \r";
	}

	// option 2: enable/disable damping
	if (key == '2')
	{
		useDamping = !useDamping;
		if (useDamping)
			cout << "> Enable damping         \r";
		else
			cout << "> Disable damping        \r";
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

	if (key == 'a')
	{
		desiredY = 0.025;
		desiredPosition.set(0.0, desiredY, desiredZ);
	}
	if (key == 'd')
	{
		desiredY = -0.025;
		desiredPosition.set(0.0, desiredY, desiredZ);
	}



	// option m: toggle vertical mirroring
	if (key == 'm')
	{
		mirroredDisplay = !mirroredDisplay;
		camera->setMirrorVertical(mirroredDisplay);
	}
}

//------------------------------------------------------------------------------

void close(void)
{
	// stop the simulation
	simulationRunning = false;

	// wait for graphics and haptics loops to terminate
	while (!simulationFinished) { cSleepMs(100); }

	// close haptic device
	hapticDevice->close();
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
	// UPDATE WIDGETS
	/////////////////////////////////////////////////////////////////////

	// update position of label
	labelHapticDeviceModel->setLocalPos(20, windowH - 40, 0);

	// display new position data
	labelHapticDevicePosition->setText(hapticDevicePosition.str(3));

	// update position of label
	labelHapticDevicePosition->setLocalPos(20, windowH - 60, 0);

	// display haptic rate data
	labelHapticRate->setText(cStr(frequencyCounter.getFrequency(), 0) + " Hz");

	// update position of label
	labelHapticRate->setLocalPos((int)(0.5 * (windowW - labelHapticRate->getWidth())), 15);


	/////////////////////////////////////////////////////////////////////
	// RENDER SCENE
	/////////////////////////////////////////////////////////////////////

	// update shadow maps (if any)
	world->updateShadowMaps(false, mirroredDisplay);

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

	// main haptic simulation loop
	while (simulationRunning)
	{
		/////////////////////////////////////////////////////////////////////
		// READ HAPTIC DEVICE
		/////////////////////////////////////////////////////////////////////

		// read position 
		cVector3d position;
		hapticDevice->getPosition(position);


		// read orientation 
		cMatrix3d rotation;
		hapticDevice->getRotation(rotation);

		// read gripper position
		double gripperAngle;
		hapticDevice->getGripperAngleRad(gripperAngle);

		// read linear velocity 
		cVector3d linearVelocity;
		hapticDevice->getLinearVelocity(linearVelocity);

		// read angular velocity
		cVector3d angularVelocity;
		hapticDevice->getAngularVelocity(angularVelocity);

		// read gripper angular velocity
		double gripperAngularVelocity;
		hapticDevice->getGripperAngularVelocity(gripperAngularVelocity);

		// read user-switch status (button 0)
		bool button0, button1, button2, button3;
		button0 = false;
		button1 = false;
		button2 = false;
		button3 = false;

		hapticDevice->getUserSwitch(0, button0);
		hapticDevice->getUserSwitch(1, button1);
		hapticDevice->getUserSwitch(2, button2);
		hapticDevice->getUserSwitch(3, button3);


		/////////////////////////////////////////////////////////////////////
		// UPDATE 3D CURSOR MODEL
		/////////////////////////////////////////////////////////////////////

		// update arrow
		velocity->m_pointA = position;
		velocity->m_pointB = cAdd(position, linearVelocity);

		// update position and orientation of cursor
		// Send device positon
		XYpacket DevicePos;
		DevicePos.x = position.y();
		DevicePos.y = position.z();
		Send((char *)&DevicePos, sizeof(DevicePos));
		//cout << "Device Positions Sent" << endl;

		cursor->setLocalPos(position);
		cursor->setLocalRot(rotation);

		// adjust the  color of the cursor according to the status of
		// the user-switch (ON = TRUE / OFF = FALSE)
		if (button0)
		{
			cursor->m_material->setGreenMediumAquamarine();
		}
		else if (button1)
		{
			cursor->m_material->setYellowGold();
		}
		else if (button2)
		{
			cursor->m_material->setOrangeCoral();
		}
		else if (button3)
		{
			cursor->m_material->setPurpleLavender();
		}
		else
		{
			cursor->m_material->setBlueRoyal();
		}

		// update global variable for graphic display update
		hapticDevicePosition = position;


		/////////////////////////////////////////////////////////////////////
		// COMPUTE AND APPLY FORCES
		/////////////////////////////////////////////////////////////////////
		// desired position



		// desired orientation
		cMatrix3d desiredRotation;
		desiredRotation.identity();

		// variables for forces    
		cVector3d force(0, 0, 0);
		cVector3d torque(0, 0, 0);
		double gripperForce = 0.0;

		// apply force field
		if (useForceField)
		{
			// compute linear force

			cVector3d forceField = kp * (desiredPosition - position);
			force.add(forceField);

			// compute angular torque
			double Kr = 0.05; // [N/m.rad]
			cVector3d axis;
			double angle;
			cMatrix3d deltaRotation = cTranspose(rotation) * desiredRotation;
			deltaRotation.toAxisAngle(axis, angle);
			torque = rotation * ((Kr * angle) * axis);
		}

		// apply damping term
		if (useDamping)
		{
			cHapticDeviceInfo info = hapticDevice->getSpecifications();

			// compute linear damping force
			double Kv = 1.0 * info.m_maxLinearDamping;
			cVector3d forceDamping = -Kv * linearVelocity;
			force.add(forceDamping);

			// compute angular damping force
			double Kvr = 1.0 * info.m_maxAngularDamping;
			cVector3d torqueDamping = -Kvr * angularVelocity;
			torque.add(torqueDamping);

			// compute gripper angular damping force
			double Kvg = 1.0 * info.m_maxGripperAngularDamping;
			gripperForce = gripperForce - Kvg * gripperAngularVelocity;
		}

		// send computed force, torque, and gripper force to haptic device
		hapticDevice->setForceAndTorqueAndGripperForce(force, torque, gripperForce);
		// update frequency counter
		frequencyCounter.signal(1);
	}

	// exit haptics thread
	simulationFinished = true;
}

int Connect(char *IP, int Port)
{
	WSAStartup(MAKEWORD(2, 2), &Data);
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{

		return 1;
	}
	i_sock.sin_family = AF_INET;
	i_sock.sin_addr.s_addr = inet_addr(IP);
	i_sock.sin_port = htons(Port);
	int ss = connect(sock, (struct sockaddr *)&i_sock, sizeof(i_sock));
	if (ss != 0)
	{
		printf("Cannot connect\r\n");
		return 0;
	}

	printf("Succefully connected\r\n");
	return 1;
}

int Send(char *Buf, int len)
{
	int slen;
	slen = send(sock, Buf, len, 0);
	if (slen < 0)
	{
		printf("cannot send data\r\n");
		return 1;
	}
	return slen;
}

int Recive(char *Buf, int len)
{
	int slen;
	slen = recv(sock, Buf, len, 0);
	if (slen < 0)
	{
		printf("cannot recive data\r\n");
		return 1;
	}
	return slen;
}

int EndSocket()
{
	closesocket(sock);
	WSACleanup();
	return 1;
}
