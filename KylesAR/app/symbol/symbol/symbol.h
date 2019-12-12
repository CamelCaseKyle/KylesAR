/*

Symbol input should be finished
	-finish distance field testing
	-fix line seg splitting

*/

#pragma once

#ifdef SYMBOL_EXPORTS
#define SYMBOL_API extern "C" __declspec(dllexport)
#else
#define SYMBOL_API extern "C"
#endif

#include "stdafx.h"

#include <opencv2/core.hpp>
#include "../scene.h"
#include "touch.h"

static const float input_verts[] = {
	-5.f, 0.f, -5.f,
	-5.f, 0.f, 5.f,
	5.f, 0.f, -5.f,
	5.f, 0.f, 5.f,
};

class SymbolInput : public SceneObject, public TouchInput {
	//input buffer (drawing input) active and inactive textures
	cv::Mat ib, tex_inactive, tex_active;
	//usdf templates
	std::vector<std::vector<cv::Point>> templates;
	//input raw line segment
	std::vector<cv::Point> seg;
	//time held down
	int hold = 0, delay = 10, timeout = 20, inpClrTime = 0;
	//if things have been loaded
	bool _textures = false, _geometry = false, _templates = false, visible = false;

	//set up templates
	void loadTemplates();
	//compare seg against templates
	int matchTemplates();

public:
	//content directory
	const enum texIDs { t_active };
	const enum vaoIDs { a_quad };
	const enum vboIDs { v_quad_verts, v_quad_norms, v_quad_uvs, v_quad_ind };
	
	//if enabled
	bool enabled = true;
	
	//give scene name
	SymbolInput(std::string nom);
	//scene graphical init
	SymbolInput(std::string nom, BINDINGS _binding, BIND_BEHAVIOR _behavior, int pID = -1);
	//occlusion feedback
	void setup_occlusion(int oc_width, int oc_height, float cam_fov, Shader* _ocShade);
	//partial setup
	void setup_textures(cv::Mat &active, cv::Mat &inactive, Shader* _shade);
	//setup gpu geometry
	void setup_geometry(const float *v, const float *n, const float *u, const unsigned int *i, int nPts, int nInds);

	//add to scene callback
	void sceneCallback(Scene* scene, Context* renderer);
	//bind to parent callback
	void bindCallback(PositionObject* parent);
	//remove from scene callback
	void removeCallback() {}

	//getters
	int getDelay() { return delay; }
	int getTimeout() { return timeout; }
	bool hasTextures() { return _textures; }
	bool hasGeometry() { return _geometry; }
	bool hasTemplates() { return _templates; }
	bool isVisible() { return visible; }
	
	//setters
	void setDelay(int newDelay) { delay = newDelay; }
	void setTimeout(int newTimeout) { timeout = newTimeout; }

	//must implement for scene functionality
	void update();
	void render();
};
