/*

Occlusion feedback needs some attention and research
	1. occlusion feedback 2 passes
		- render occlusion buffer
		- render rgb in f_vertex
	2. occlusion feedback layered rendering with texture array
		- geometry shader transforms and spits out a matching fullscreen vert for each vert
		- pass through vertex shader
		- fragment shader renders to layers, depending on layer use uv to write to occlusion buffer, otherwise render rgb to f_vertex
	3. occlusion queries, this would require that transform depth be bound to depth buffer instead of uniform
		- add support for stenciling to touch input

*/

#pragma once

#include <vector>

#include <opencv2\core.hpp>
#include <opencv2\imgproc.hpp>

#include "..\scene.h"
#include "..\dsp.h"
#include "..\render.h"

#define DEBUG_OCFB

//location and size
typedef std::pair<cv::Point, float> TouchEvent;

//inherit this for functionality
class i_OcclusionFeedback {
private:
	//the default occlusion feedback shader
	Shader OcclusionFeedbackShader = Shader("glsl\\occlusion.feedback.vert", "glsl\\occlusion.feedback.frag", false);
	//dsp for input smoothing
	OneEuroFilter smoothx, smoothy;
	//time not interacted with until input cache clear
	int clearTime = 0, clearDelay = 15, rangeLo = 1, rangeHi = 24;
	//is setup complete
	bool _occlusion = false, _geometry = false, intersected = false, occluders = false, freshData = false;
	
	//input processing
	void processMultiTouch();

protected:
	//only let extended classes init
	i_OcclusionFeedback() : smoothx(0.7), smoothy(0.7) {
		OcclusionFeedbackShader.addUniforms({"depthFrame", "MVP"});
	}
	~i_OcclusionFeedback() = default;

public:
	//content map
	const enum fboIDs { f_ocbuff };
	const enum rentexIDs { r_ocout };
	const enum vaoIDs { a_ocmesh };
	const enum vboIDs { v_ocmesh_uv, v_ocmesh_verts, v_ocmesh_ind };

	Content occont;
	//touch events (location and size)
	std::vector<TouchEvent> curTouches, lastTouches;
	//last point or lack thereof used for single point matching
	cv::Point curSingle, lastSingle;
	//cpu copy of occlusion feedback rendertex
	cv::Mat ocuv;
	//for throttling
	unsigned long time = 0;

	//getters
	int getRangeLow() { return rangeLo; }
	int getRangeHigh() { return rangeHi; }
	int getDelay() { return clearDelay; }
	bool hasOcclusion() { return _occlusion; }
	bool hasGeometry() { return _geometry; }
	bool isOccluded() { return occluders; }
	bool isIntersected() { return intersected; }
	bool isRecent() { return freshData; }

	//setters
	void setDelay(int newDelay) { clearDelay = newDelay; }
	void setRange(int low, int high) { rangeLo = low; rangeHi = high; }

	//let people overload this so I can get their info
	void setup_geometry(const float *v, const float *n, const float *u, const unsigned int *i, int nPts, int nInds);
	//create occlusion buffer, feedback and render passes
	void setup_occlusion(int oc_width, int oc_height, Shader *_ocShade);

	//get informed about everything
	void sceneCallback(Scene *scene, Context *renderer);

	//order output points by time, match over time
	std::vector<cv::Point> multiTouch();
	//narrow multitouch to single smoothed touch event
	cv::Point2i singleTouch();

	//update oc buffer
	void update();
	//just render occlusion buffer
	void render();

};
