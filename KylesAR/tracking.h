/*

origin pose is first seen
weighted average of all world poses 

Tracking fusion service
	-enfoce global unique ID
	-do multimarker here instead of fiducial
	-do pose filtering here
	-decides which tracking model(s) to use
	-if marker pose is bad then supplement with optical IMU
	-if no markers then full SLAM
	-save last good pose as a seed for when services restart

Motion model
	-3D linear regression
	-derive at current time
	-kalman filter

*/

#pragma once

#define DBG_TRACKING

#include <unordered_map>
#include <algorithm>
#include <vector>

#include <opencv2\core.hpp>
#include <opencv2\calib3d.hpp>
#include <opencv2\highgui.hpp>
#include <opencv2\imgproc.hpp>

#include <glm.hpp>
#include "dsp.h"
#include "kibblesmath.h"
#include "ext\i_tracker.h"

//all this does is put everything together and manage origin
class TrackingService {
private:
	
	//all trackers encountered
	std::vector<i_PositionTracker*> posTracks;
	std::vector<i_LightTracker*> lightTracks;

	//origin pose for fusion
	PositionObject *origin = nullptr;
	int originID = -1;

public:
	//all objects encountered in chronological order
	std::vector<PositionObject*> poses;
	std::vector<LightObject*> lights;

	//i guess do nothing and wait for an implemented class to get registered
	TrackingService() {}

	//add new tracker
	int registerTracker(i_PositionTracker *newTracker);
	//add new tracker
	int registerTracker(i_LightTracker *newTracker);

	//updates position objects from all trackers
	std::vector<PositionObject*> getAllPoses();
	//updates light objects from all trackers
	std::vector<LightObject*> getAllLights();
	//get the best fused pose
	glm::mat4 getWorldPose();
	//get a certain ID
	glm::mat4 getPoseFromID(int id);

	~TrackingService() {}
};
