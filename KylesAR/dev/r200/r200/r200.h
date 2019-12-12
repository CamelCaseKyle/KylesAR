/*	
	
*/

#pragma once

#ifdef R200_EXPORTS
#define R200_API extern "C" __declspec(dllexport)
#else
#define R200_API extern "C"
#endif

#include <iostream>
#include <vector>
#include <mutex>

#include <pxcimage.h>
#include <pxcsensemanager.h>
#include <PXCScenePerception.h>
#include <pxcsession.h>

#include <opencv2\core.hpp>

#include <glm.hpp>
#include <gtc\type_ptr.hpp>

#include <kibblesmath.h>
#include <dev\i_Camera.h>

class R200 : public i_Camera, public PXCSenseManager::Handler {
public:
	PXCCapture::Device* device;
	PXCSenseManager* manager;
	PXCCaptureManager* capture;
	PXCProjection* projection;
	PXCScenePerception* perception;
	PXCImage::ImageData datafmt;

	//raw output from tracking
	glm::mat4x3 pose32f = glm::mat4x3(1.);

	//init and setup streams
	R200(bool D16stream = false, bool IRstream = false, float frameRate = 60.f);
	~R200();
	//start async frame streaming
	bool stream();

	virtual bool start();
	virtual bool pause();

	virtual pxcStatus PXCAPI OnConnect(PXCCapture::Device* device, pxcBool connected);
	virtual pxcStatus PXCAPI OnModuleProcessFrame(pxcUID mid, PXCBase* object);
	virtual pxcStatus PXCAPI OnNewSample(pxcUID mid, PXCCapture::Sample* sample);
};

R200_API i_Camera* __stdcall allocate() {
	R200* cam = new R200(true);
	if (cam->stream()) return (i_Camera*)cam;
	delete cam;
	return nullptr;
}

R200_API void __stdcall destroy(i_Camera* cam) {
	delete cam;
}
