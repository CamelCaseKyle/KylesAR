/*

slide functions need implementation
cant find shader vert/frag becasue shader.h is in different location? 
setup occlusion, should make OC shade have a default

*/

#pragma once

#ifdef BUTTON_EXPORTS
#define BUTTON_API extern "C" __declspec(dllexport)
#else
#define BUTTON_API extern "C"
#endif

#include "stdafx.h"

#include <opencv2\core.hpp>

#include <GL\glew.h>

#include <app\behavior.h>
#include <app\i_ocfb.hpp>
#include <app\i_SceneObj.h>
#include <render.h>
#include <scene.h>

const float button_verts[12] = {
	-2.25f, 0.f, -1.5f,
	-2.25f, 0.f,  1.5f,
	 2.25f, 0.f,  1.5f,
	 2.25f, 0.f, -1.5f
};
const float button_norms[12] = {
	0.f, 1.f, 0.f,
	0.f, 1.f, 0.f,
	0.f, 1.f, 0.f,
	0.f, 1.f, 0.f
};
const float button_uvs[8] = {
	1.f, 1.f,
	1.f, 0.f,
	0.f, 0.f,
	0.f, 1.f
};
const unsigned int button_inds[6] = {
	0, 1, 2,
	0, 2, 3
};

class Button : public i_SceneObject, public i_OcclusionFeedback {
	//pretty much augment occlusion shader with gamma control
	Shader buttonShade;
	//oc mask back from GPU, gl Tex2D
	cv::Mat tex_inactive, tex_active;
	//gamma brightness for highlighting
	float brightness = 1.f;
	//hold is accumulated presses, cooldown is accumulated holds
	int hold, cooldown, delay = 6, timeout = 60,
		hover_hold, hover_cooldown, hover_delay = 3,
		slideRL_hold, slideLR_hold, slide_delay = 3;

	//the state of the button
	bool pressing = false, hovering = false, _textures = false, _geometry = false, visible = false;

public:
	//content directory
	const enum texIDs { t_active, t_inactive };
	const enum vaoIDs { a_quad };
	const enum vboIDs { v_quad_verts, v_quad_norms, v_quad_uvs, v_quad_ind };
	
	//current state
	bool enabled = true;

	//partial init
	Button(std::string nom);
	//scene init
	Button(std::string nom, BINDINGS binding, BIND_BEHAVIOR behavior, int pID = -1);
	
	//setup textures on gpu
	void setup_textures(const cv::Mat &active, const cv::Mat &inactive);
	//set up occlusion feedback
	void setup_occlusion(int oc_width, int oc_height, Shader* _ocShade);
	//sets up geometry and GPU presence
	void setup_geometry();
	//just the shader
	void setup_shader();

	//add to scene callback
	void sceneCallback(Scene* scene, Context* renderer);
	//bind to parent callback
	void bindCallback(PositionObject* parent);
	//remove from scene callback
	void removeCallback();

	//get a simple button press event
	bool press();
	//get a hover event
	bool hover();
	//get a 'slide to unlock' type event
	bool slideR2L();
	bool slideL2R();

	//getters
	int getDelay() { return delay; }
	int getHoverDelay() { return hover_delay; }
	int getSlideDelay() { return slide_delay; }
	int getTimeout() { return timeout; }
	bool hasTextures() { return _textures; }
	bool hasGeometry() { return _geometry; }
	bool isVisible() { return visible; }

	//setters
	void setDelay(int newDelay) { delay = newDelay; }
	void setHoverDelay(int newHover) { hover_delay = newHover; }
	void setSlideDelay(int newSlide) { slide_delay = newSlide; }
	void setTimeout(int newTimeout) { timeout = newTimeout; }

	//swaps texture IDs
	void swapTextures();

	//must implement for scene functionality
	void update();
	void render();
};

//Entry point of DLL
BUTTON_API i_SceneObject* __stdcall allocateStr(std::string args) {
	//try to split around '|'
	int ofs = int(args.find_first_of('|', 0));
	if (ofs > 0 && ofs < args.length()) {
		
		//get texture directories
		std::string activeDir = getCWD(), inactiveDir = activeDir;
		activeDir += trim(args.substr(0, ofs));
		inactiveDir += trim(args.substr(ofs + 1, args.length() - ofs - 1));
		
		//load textures from path
		cv::Mat active, inactive;
		active = cv::imread(activeDir);
		inactive = cv::imread(inactiveDir);
		
		//setup!
		Button* but = new Button("Button", BIND_WORLD, WORLD_ORIGIN);
		//Button* but = new Button("Button", BIND_PARENT, PARENT_ID, 31);
		but->setup_geometry();
		but->setup_shader();
		but->setup_textures(active, inactive);

		return (i_SceneObject*)but;
	}

	return nullptr;
}

//Exit point of DLL
BUTTON_API void __stdcall destroy(i_SceneObject* inst) {
	delete inst;
}
