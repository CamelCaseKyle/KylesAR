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

//debug all derived trackers
#define DBG_TRACKERS

#include <vector>
#include <opencv2\core.hpp>
#include <glm.hpp>
#include <chrono>

#include "..\dev\i_Camera.h"

typedef std::chrono::high_resolution_clock hrClock;
typedef hrClock::time_point timePoint;

//The container for an object with a pose
struct PositionObject {
public:
	//global tracker ID for scene
	int ID = -1;
	//colloquial name
	std::string name = "";
	//view matrix, inverse view matrix
	glm::mat4 view = glm::mat4(1.f), view_cam = glm::mat4(1.f),
			  //world matrix, inverse world matrix
			  world = glm::mat4(1.f), world_cam = glm::mat4(1.f),
			  //origin world matrix
			  origin = glm::mat4(1.f);
	timePoint time;
	//original tracking output for convienience
	cv::Mat rvec, tvec;
	//quality related variables
	float distance = 0.f, quality = 0.f, size = 1.f, originAccuracy = 0.f;
	//if any data
	bool visible = false, reliable = false, isOrigin = false, hasOrigin = false;

	//minimal init
	PositionObject(std::string nom) : name(nom) {}
	~PositionObject() = default;

	void calculateWorldMatrix() {
		if (hasOrigin) {
			//calculate world matrix
			world = view * origin;
			world_cam = glm::inverse(world);
		} else if (isOrigin) {
			//view is world matrix
			world = view;
			world_cam = view_cam;
		}
	}
	//called when recieved a new origin pose
	void originCallback(PositionObject *newOrigin) {
		//calculate quality
		float newAcc = newOrigin->quality + quality;
		//check quality first
		if (reliable && newOrigin->reliable && newAcc > originAccuracy && newAcc <= 2.f) {
			originAccuracy = newAcc;
			origin = glm::inverse(view) * newOrigin->view;
			hasOrigin = true;
#ifdef DBG_TRACKERS
			printf("New origin accuracy %f between ID %i and %i\n", originAccuracy, ID, newOrigin->ID);
#endif
		}
	}
	//called when promoted to origin
	void setAsOrigin() {
		origin = glm::mat4(1.f);
		originAccuracy = 0.;
		hasOrigin = false;
		isOrigin = true;
	}
};
//The container for a light trace
struct LightObject {
public:
	//scene ID
	int ID = -1;
	//colloquial name
	std::string name = "";
	//output properties
	glm::vec3 direction = glm::vec3(0.f, 0.f, 0.f), color = glm::vec3(0.f, 0.f, 0.f);
	float power = 0.f, quality = 0.f;
	//if any data
	bool visible = false, reliable = false;

	//minimal init
	LightObject(std::string nom) : name(nom) {}
};

//all they have to do is inherit the interface
class i_PositionTracker {
public:
	std::string name = "";

	//global unique tracker ID
	int poseBaseID = 0;
	bool paused = false;

	//important
	virtual ~i_PositionTracker() = default;

	//set input stream
	virtual void setCam(i_Camera *_cam) = 0;
	//get all pose objects
	virtual std::vector<PositionObject*> getPositions() = 0;

	//calculate new origin transform (pass origin to PoseObjects)
	virtual void originCallback(PositionObject *newOrigin) = 0;
	//pause tracking
	virtual void pauseTracking() = 0;
	//resume tracking
	virtual void resumeTracking() = 0;
};
//inherit the interface for functionality
class i_LightTracker {
public:
	int lightBaseID = 0;

	//important
	virtual ~i_LightTracker() = default;
	//get all light objects
	virtual std::vector< LightObject *> getLights() = 0;
	//pause tracking
	virtual void pauseTracking() = 0;
	//resume tracking
	virtual void resumeTracking() = 0;
};

//this lets us sort things
struct gt_poseObj_qual {
	inline bool operator() (const PositionObject *pt1, const PositionObject *pt2) {
		return (pt1->quality > pt2->quality);
	}
};
