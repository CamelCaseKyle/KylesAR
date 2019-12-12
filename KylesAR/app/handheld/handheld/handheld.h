/*

render objects that get near handheld
collide content with scene 'real world mesh'
disable button X axis rotate so its almost like a billboard

simple state system based on graph approach
	-states
	-transitions

class state
	int ID
	switchTo()		//when state is activated
	switchedFrom()	//just before state is deactivated
	update()		//while state is active

class state system
	vector<state> states
	unordered_map<sID, vector<sID>> transitions		//map of possible transitions based on int ID
	vector<state> history							//state history for undo buffer

struct app entry
	cv::mat icon
	string path

screen states:
idle
	-display blank
apps
	-display list of apps
placing
	-display possible binding picture
	-after placed display list of bindings

buttons:
	green - back
	blue - scroll / select

*/

#pragma once

#ifdef HANDHELD_EXPORTS
#define HANDHELD_API extern "C" __declspec(dllexport)
#else
#define HANDHELD_API extern "C"
#endif

#include "stdafx.h"

#include <opencv2\core.hpp>
#include <opencv2\highgui.hpp>

/*
#include <GL\glew.h>
#include <GLFW\glfw3.h>
*/

#include <glm.hpp>
#include <gtc\matrix_transform.hpp>

#include <dsp.h>
#include <scene.h>
#include <render.h>
#include <winderps.h>
#include <cpudraw.h>
#include <module.hpp>

#include <app\behavior.h>
#include <app\i_ocfb.hpp>
#include <app\i_SceneObj.h>

//display
const float handheld_verts[12] = {
	-1.f, 0.f, -1.f,
	-1.f, 0.f,  1.f,
	 1.f, 0.f,  1.f,
	 1.f, 0.f, -1.f
};
const float handheld_norms[12] = {
	0.f, 1.f, 0.f,
	0.f, 1.f, 0.f,
	0.f, 1.f, 0.f,
	0.f, 1.f, 0.f
};
const float handheld_uvs[8] = {
	1.f, 1.f,
	1.f, 0.f,
	0.f, 0.f,
	0.f, 1.f
};
const unsigned int handheld_inds[6] = {
	0, 1, 2,
	0, 2, 3
};

//cursor (laser)
const float cursor_verts[12] = {
	-.5f, 0.f, 7.5f,
	-.5f, 0.f, 107.5f,
	 .5f, 0.f, 107.5f,
	 .5f, 0.f, 7.5f
};
const float cursor_norms[12] = {
	0.f, 1.f, 0.f,
	0.f, 1.f, 0.f,
	0.f, 1.f, 0.f,
	0.f, 1.f, 0.f
};
const float cursor_uvs[8] = {
	0.f, 1.f,
	1.f, 1.f,
	1.f, 0.f,
	0.f, 0.f
};

class HandheldInput : public i_SceneObject, public i_OcclusionFeedback {
	//states
	const static int nStates = 3;
	const enum States { s_idle, s_apps, s_place };
	//backgrounds
	const static int nBackgrounds = 3;
	const enum Backgrounds { b_idle, b_title, b_cursor };
	//apps
	const static int nProgramIcons = 2;
	const enum ProgramIcons { p_firefox, p_symbol };
	//bindings
	const static int nBindIcons = 4;

	//input devices
	i_SceneObject *blue, *green;
	Shader screenShade, laserShade;
	//all images
	std::vector<cv::Mat> bgs, pgmICOs, bindICOs;
	//output
	cv::Mat screen, laser;
	//the current screen state
	States state = s_idle;
	BINDINGS curBind = BIND_PURGATORY, lastBind = BIND_PURGATORY;
	//state info
	float distance = 0.f, distDiff = 0.f, lastDist = 0.f, lastDiff = 0.f, znear = 20.f, reach = 80.f, bubble = 140.f;
	//time alive
	unsigned long time = 0;
	//things
	int hold = 0, delay = 10, timeout = 0, dw = 0, dh = 0;
	//this makes new features strait forward
	bool isfacing = false, isresting = false,
		 isclose = false, isfar = false,
		 isapproaching = false, isretreating = false,
		 manualBinding = false, visible = false,
		 _textures = false, _geometry = false, _shade = false;

	//march depth image
	void traceDepth();

	//process state functions
	void process_idle();
	void process_place();
	void process_apps();

	//render the screen only when it changes, not usually every frame
	void renderState();

public:
	//content directory
	const enum texIDs { t_active, t_laser };
	const enum vaoIDs { a_screen, a_laser };
	const enum vboIDs {
		v_screen_verts, v_screen_norms, v_screen_uvs, v_screen_ind,
		v_laser_verts, v_laser_norms, v_laser_uvs, v_laser_ind
	};
	
	bool enabled = true;

	//scene init
	HandheldInput(int pID = -1);
	//partial setup
	bool setup_shaders();
	//sets up geometry and GPU presence
	void setup_geometry();
	//set up occlusion feedback
	void setup_occlusion(int oc_width, int oc_height);
	
	//callback functions
	void sceneCallback(Scene* scene, Context* renderer);
	void bindCallback(PositionObject* parent);
	void removeCallback() {}

	//getters
	float getZnear() { return znear; }
	float getReach() { return reach; }
	float getBubble() { return bubble; }
	int getState() { return int(isfacing) & (int(isresting) << 1) & (int(isclose) << 2) & (int(isfar) << 3) & (int(isapproaching) << 4) & (int(isretreating) << 5); }
	int getDelay() { return delay; }
	int getTimeout() { return timeout; }
	bool hasTextures() { return _textures; }
	bool hasGeometry() { return _geometry; }
	bool isVisible() { return visible; }

	//setters
	void setZnear(float newZnear) { znear = newZnear; }
	void setReach(float newReach) { reach = newReach; }
	void setBubble(float newBubble) { bubble = newBubble; }
	void setDelay(int newDelay) { delay = newDelay; }
	void setTimeout(int newTimeout) { timeout = newTimeout; }
	
	//must implement for scene functionality
	void update();
	void render();
};

HANDHELD_API i_SceneObject* __stdcall allocate() {
	HandheldInput* hhi = new HandheldInput(31);
	hhi->setup_geometry();
	hhi->setup_shaders();
	return (i_SceneObject*)hhi;
}

//Exit point of DLL
HANDHELD_API void __stdcall destroy(i_SceneObject* inst) {
	delete inst;
}
