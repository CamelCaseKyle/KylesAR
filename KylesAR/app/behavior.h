/*
	These are the bindings and behaviors for apps; more of a document than a functional header
*/

#pragma once

//scene behaviors
const int NUM_BINDINGS = 5;
enum BINDINGS {
	BIND_HUD,		// ! the content is put on a flat, fullscreen overlay using no tracking of any kind
	BIND_PERSONAL,	// ! the content floats within reach using a fusion of Compass, IMU, and GPS
	BIND_PARENT,	//the content is attached to a parent object (obj.model * parent.view * projection)
	BIND_WORLD,		//the content exists in the world independant of other objects (obj.model * world.view * projection)
	BIND_PURGATORY	//the content is not drawn or updated (usually used with WAIT)
};

enum BIND_BEHAVIOR {
	//2D ortho, heads up display, no tracking
	HUD_GUI,			// ! generic UI element
	HUD_WARNING,		// ! alert message that gains precedence

	//2D and 3D, perspective, IMU/Compass/GPS exists between depthNear and Reach
	PERSONAL_GUI,		// ! generic UI element
	PERSONAL_WARNING,	// ! generic message that gains precedence

	//2D and 3D content, perspective, bind to parent
	PARENT_FIRST,		//first tracker seen chronologically
	PARENT_LAST,		//most recent tracker seen
	PARENT_EMPTY,		//chronologically first tracker that is empty
	PARENT_CLOSE,		//closest tracker
	PARENT_QUALITY,		//highest quality reliable tracker
	PARENT_ID,			//custom configuration

	//2D and 3D content, world coords only
	WORLD_ORIGIN,		//world origin at first seen pose

	//do nothing
	WAIT
};
