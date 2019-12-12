/*
-use initial scan and faster re-detection algorithm
-finally just make a LUT and project the points perspective correct
	-replace ghetto estimation math with opencv homography?
	-fix 2D bary with perspective divide?
-race condition: when marker is not visible and misses the origin call
	-make "marker update" que, hang in que until visible
-update light tracking to use new sphere
	-new light tracking measures known area brightness over time, determines light direction
	-return transformed 3D world space direction and smoothed 3D screenspace normal
-replace first PNP with a prediction and refine from there
-for known markers:
	-chop up image and send to threads, one thread per visible marker
	-custom process image chunk, use goodPointsToTrack
		-match the points based on LMS of prediction
*/

#pragma once

#ifdef FIDUCIAL_EXPORTS
#define FIDUCIAL_API extern "C" __declspec(dllexport)
#else
#define FIDUCIAL_API extern "C"
#endif

#include "stdafx.h"
#include <unordered_map>
#include <vector>

#include <opencv2\core.hpp>
#include <opencv2\calib3d.hpp> //pose estimation and extrinsics
#include <opencv2\highgui.hpp> //shortcut to screen
#include <opencv2\imgproc.hpp> //image processing algorithms

#include <glm.hpp>

//kylesAR
#include <rectExt.h>			//fancy rectangle functions
#include <kibblesmath.h>		//random collection of math
#include <dev\i_Camera.h>		//camera
#include <ext\i_tracker.h>		//tracker interface

#ifdef DBG_TRACKERS
	//fiducial service activity
	//#define DBG_FIDUCIAL
	//only eval first tier of branches
	#define MARKER_TIER_ONE
	//marker quality check
	//#define DBG_MARKER
	//light tracking debug
	//#define DBG_LIGHT
	//print pose
	//#define LOG_POSE
	//enable parameters for the ghetto scene
	//#define GHETTO
#endif

//marker on plane XZ, units in cm (gets multiplied in getMatrix())
static const std::vector<cv::Point3f> quadPoints = {
	cv::Point3f(1.f,  0.f, 1.f),
	cv::Point3f(-1.f, 0.f, 1.f),
	cv::Point3f(-1.f, 0.f, -1.f),
	cv::Point3f(1.f,  0.f, -1.f)
};

//marker physical configuration
struct MarkerAttribs {
	//the edge length of the square in cm
	float sizecm = 0.f, sphereSizecm = 0.f;
	//if true the marker is used for location
	bool stationary = true;
};

//container class for marker
class Marker : public PositionObject {
public:
	//lighting info. use mat3{loc, dir, col}?
	std::vector<LightObject> lights;
	//corners in px
	std::vector<glm::vec2> corners;
	//pixel cords again
	RectExt rect;
	//in camera space
	glm::vec3 loc, lastloc, velocity, whiteBal;
	//in screen space
	glm::vec2 acc, sLoc, sLastloc, sVelocity, center;
	//marker attrib info
	float sphereSizecm, markerSizecm;
	//marker properties
	int smpHi, smpLo, contrast, rootBranch, timeout, area, markerID;
	//boiler plate did it change
	bool changed = true;
	
	//minimal init
	Marker(std::string nom, MarkerAttribs ma);
	~Marker();

	//compute obj and cam pose
	void getMatrix();
};

//branchy makrer tracker
class Fiducial : public i_PositionTracker, public i_LightTracker {
private:
	//need the camera
	i_Camera* cam;
	//all markers
	std::vector<Marker> markers;
	//tracking state
	float defaultMarkerSize;
	int threshold, tolerance, maxtime, originID = -1;
	bool hasCam = false;

	//do the actual tracking
	void trackMarkers();

public:
	//marker attribute lookup hashmap
	std::unordered_map<int, MarkerAttribs> markerattr;
	//should only return visible trackers
	bool onlyVisible;

	//minimal init
	Fiducial();
	~Fiducial();

	//i_PositionTracker
	void setCam(i_Camera* _cam);
	std::vector<PositionObject*> getPositions();
	void originCallback(PositionObject* newOrigin);
	void pauseTracking();
	void resumeTracking();
	//i_LightTracker
	std::vector<LightObject*> getLights();

};

FIDUCIAL_API i_PositionTracker* __stdcall allocate() {
	i_PositionTracker* ret = (i_PositionTracker*)(new Fiducial());
	return ret;
}

FIDUCIAL_API void __stdcall destroy(i_PositionTracker* inst) {
	delete inst;
}
