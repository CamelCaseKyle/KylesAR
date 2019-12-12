/*

*/

#pragma once

#ifdef PERCEPTION_EXPORTS
#define PERCEPTION_API extern "C" __declspec(dllexport)
#else
#define PERCEPTION_API extern "C"
#endif

#include "stdafx.h"
//time for smoothing
#include <chrono>
//intel realsense
#include <pxcimage.h>
#include <pxcsensemanager.h>
#include <PXCScenePerception.h>
#include <pxcsession.h>
//opencv
#include <opencv2\core.hpp>
//glm
#include <glm.hpp>
#include <gtc\type_ptr.hpp>
//kylesAR
#include <dev\i_Camera.h>
#include <tracking.h>

//print pose
#define LOG_POSE

//Intel R200 perception tracking wrapper
class PerceptionTracker : public i_PositionTracker, public PXCSenseManager::Handler {
public:

	std::chrono::high_resolution_clock::time_point lastTime, thisTime;
	PositionObject pose;
	PXCCapture::Device* device;
	PXCSenseManager* manager;
	PXCCaptureManager* capture;
	PXCProjection* projection;
	PXCScenePerception* perception;
	glm::mat4 finalPose, dPose, ddPose;
	float finalQual;

	//get tracking matrix
	void getMatrix(float* p, float qual);

	bool open = false;
	
	PerceptionTracker();
	~PerceptionTracker();

	//handler
	virtual pxcStatus PXCAPI OnModuleProcessedFrame(pxcUID mid, PXCBase *module, PXCCapture::Sample *sample);

	//i_PositionTracker
	void setCam(i_Camera* _cam) {}
	std::vector<PositionObject*> getPositions();
	void originCallback(PositionObject* newOrigin);
	void pauseTracking();
	void resumeTracking();
};

PERCEPTION_API i_PositionTracker* __stdcall allocate() {
	PerceptionTracker* trk = new PerceptionTracker();
	if (trk->open) return (i_PositionTracker*)trk;
	delete trk;
	return nullptr;
}

PERCEPTION_API void __stdcall destroy(i_PositionTracker* inst) {
	delete inst;
}
