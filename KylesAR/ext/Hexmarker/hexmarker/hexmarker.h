/*

essentially monte carlo pathtrace cubemap off of hemisphere reflections and pose for IBL


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

#ifdef  HEX_EXPORTS
#define HEX_API extern "C" __declspec(dllexport)
#else
#define HEX_API extern "C"
#endif

#define DBG_MARKER
#define LOG_POSE

#include "stdafx.h"
#include <unordered_map>
#include <vector>

#include <opencv2\core.hpp>
#include <opencv2\calib3d.hpp>
#include <opencv2\highgui.hpp>
#include <opencv2\imgproc.hpp>

#include <glm.hpp>

#include <dsp.h>
#include <rectExt.h>
#include <kibblesmath.h>
#include <dev\i_Camera.h>
#include <ext\i_tracker.h>

const float s60 = .86602540378f;
const float EdgeLength = 8.45; // cm

//marker on plane XZ, units in cm (gets multiplied in getMatrix())
static const std::vector<cv::Point3f> hexPoints = {
	cv::Point3f( 1.f, 0.f,  0.f),
	cv::Point3f( .5f, 0.f,  s60),
	cv::Point3f(-.5f, 0.f,  s60),
	cv::Point3f(-1.f, 0.f,  0.f),
	cv::Point3f(-.5f, 0.f, -s60),
	cv::Point3f( .5f, 0.f, -s60)
};

//marker physical configuration
struct MarkerAttribs {
	//the edge length of the shape in cm
	float sizecm = 0.f, sphereSizecm = 0.f;
	//if true the marker is used for location, else input
	bool stationary = true;
};

//container class for marker
class Hexmarker : public PositionObject {
public:
	//lighting info. use mat3{loc, dir, col}?
	std::vector<LightObject> lights;

	//marker attrib info
	MarkerAttribs ma;

	//in screen space
	cv::Mat *frame;
	std::vector<cv::Point> corners;
	cv::Point center;
	cv::Scalar whiteBal;
	RectExt rect;
	//in camera space for prediction
	glm::mat3 rot, lastRot, angular, angAcc;
	glm::vec3 loc, lastLoc, velocity, lastVelocity;
	float fovH, fovV;
	//marker appearence
	int smpHi, smpLo, contrast, rootBranch, timeout, area, markerID;
	//boiler plate did it change
	bool changed = true;

	//minimal init
	Hexmarker(int id, MarkerAttribs ma, i_Camera* cam);
	~Hexmarker();

	//compute obj and cam pose
	void getMatrix();
	//predict, ROI, lines, corners, etc
	void trackSelf();
	//get the hemisphere reflections
	void trackLight();
};

//multimakrer tracking service
class Hextracker : public i_PositionTracker, public i_LightTracker {
private:
	//need the camera
	i_Camera* cam;
	//marker attribute lookup hashmap
	std::unordered_map<int, MarkerAttribs> markerattr;
	//all markers
	std::vector<Hexmarker> markers;
	//tracking state
	float defaultMarkerSize;
	int threshold, tolerance, maxtime, originID = -1;
	bool hasCam = false;

	//do the actual tracking
	void trackMarkers();

public:
	//should only return visible trackers
	bool onlyVisible;

	//minimal init
	Hextracker();
	~Hextracker();

	//i_PositionTracker
	void setCam(i_Camera* _cam);
	std::vector<PositionObject*> getPositions();
	void originCallback(PositionObject* newOrigin);
	void pauseTracking();
	void resumeTracking();
	//i_LightTracker
	std::vector<LightObject*> getLights();

};

HEX_API i_PositionTracker* __stdcall allocate() {
	i_PositionTracker* ret = new Hextracker();
	return (i_PositionTracker*)ret;
}

HEX_API void __stdcall destroy(i_PositionTracker* inst) {
	delete inst;
}
