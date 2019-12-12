/*

*/

#pragma once

#ifdef GENERICCAM_EXPORTS
#define GENERICCAM_API extern "C" __declspec(dllexport)
#else
#define GENERICCAM_API extern "C"
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <thread>

#include <opencv2\core.hpp>
#include <opencv2\highgui.hpp>

#include <glm.hpp>
#include <gtc\type_ptr.hpp>

#include <dev\i_Camera.h>

class GenericCamera : public i_Camera {
private:
	cv::VideoCapture cap;
	HANDLE thrd;
public:
	GenericCamera();
	//important
	~GenericCamera();

	//start/resume streaming
	bool start();
	//pause streaming
	bool pause();
};

GENERICCAM_API i_Camera* __stdcall allocate() {
	GenericCamera* cam = new GenericCamera();
	i_Camera* ret = (i_Camera*)cam;
	if (ret->open()) return ret;
	delete ret;
	return nullptr;
}

GENERICCAM_API void __stdcall destroy(i_Camera* inst) {
	delete inst;
}
