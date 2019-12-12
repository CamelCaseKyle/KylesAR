// GenericCam.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "GenericCam.h"

using namespace cv;

static DWORD WINAPI streamThread(void* data) {
	//check if data
	if (data == nullptr) return (DWORD)0;
	//recover stream
	CameraStream* rgb = (CameraStream*)data;
	//open capture
	VideoCapture cap;
	for (int i = 0; i < 5; i++) {
		cap = VideoCapture(i);
		if (cap.isOpened()) break;
	}
	//fail check
	if (!cap.isOpened()) {
		printf("Cannot open camera\n");
		return (DWORD)0;
	}

	//for some reason opencv doesnt work without this shit
	cap.read(rgb->frame);
	imshow("Generic Camera", rgb->frame);
	waitKey(1);
	destroyWindow("Generic Camera");

	//fill out stream props
	rgb->type = rgb->frame.type();
	rgb->width = rgb->frame.cols;
	rgb->height = rgb->frame.rows;
	rgb->exists = true;
	//get properties
	rgb->name = "VideoCapture";
	rgb->fovH = 60;
	rgb->fovV = 42;
	rgb->fRate = float(cap.get(CV_CAP_PROP_FPS));
	
	//fail check
	if (rgb->fRate > 1.f || rgb->width*rgb->height == 0) {
		printf("Framerate or size insufficient\n");
		return (DWORD)0;
	}
	//main loop
	bool success = true;
	while (success) {
		success = cap.read(rgb->frame);
		//if (success) cv::imshow("cam", rgb->frame);
		rgb->time = hrClock::now();
		waitKey(1);
	}
	return (DWORD)0;
}

GenericCamera::GenericCamera() {
	name = "Generic Camera";
	//these are for the R200
	rgb.camDist = { 2.9248098353506807e-001, -1.3922288490391708e+000, 1.3629934471475001e-003, -1.2816975768909633e-005, 1.9292929353192863e+000 };
	rgb.camMat = { 6.1959211982317981e+002, 0., 3.1936803881953682e+002, 0., 6.1956601022127211e+002, 2.3863698322291032e+002, 0., 0., 1. };
	isOpen = start();
}
//important
GenericCamera::~GenericCamera() {
	cap.release();
}

//start/resume streaming
bool GenericCamera::start() {
	//if thread exists, resume it
	if (thrd) ResumeThread(thrd);
	//if it doesnt, create it
	else thrd = CreateThread(NULL, 0, streamThread, (void*)&rgb, 0, NULL);
	//if thread exists and hasnt died
	if (thrd) {
		DWORD xcode = STILL_ACTIVE;
		//check thrice ;)
		for (int i = 0; i < 3; i++) {
			//wait a bit
			Sleep(1000);
			//get status
			GetExitCodeThread(thrd, &xcode);
			//if exited
			if (xcode != STILL_ACTIVE) return false;
		}
		return true;
	}
	return false;
}
//pause streaming
bool GenericCamera::pause() {
	return SuspendThread(thrd) != 0;
}
