/*

Natrual feature tracking used to find features with depth
	-use world pose to project points into world
	-bind object near best point
	-use points for optical IMU
	-nft mode looks for stable identifiable 3D geometry (RGBD) and procedurally generates a marker pretty much
	-compare frames
	-uses pose estimation like multimarker
	-use linear time pnp to get 4 poses wound ccw (1234) (2341) (3412) (4123)
	-choose closest pose, filter pose

*/

#pragma once

#ifdef FEATURE_EXPORTS
#define FEATURE_API extern "C" __declspec(dllexport)
#else
#define FEATURE_API extern "C"
#endif

#include "tracker.h"
#include "../device/r200.h"

#define DBG_FEATURE

//less typing yay
typedef std::vector<cv::Point2f> Points;
typedef std::vector<cv::Point3f> Points3D;

//This is the 3D marker feature cloud (CAD object tracking)
static const float model_points[] {
	-6.5f, 0.f, -22.25f,
	-6.5f, 0.f, 17.75f,
	-6.5f, 8.5f, -22.25f,
	-6.5f, 8.5f, 17.75f,
	6.5f, 0.f, -22.25f,
	6.5f, 0.f, 17.75f,
	-1.5f, 8.5f, -22.25f,
	-1.5f, 8.5f, 17.75f,
};
static const float model_texcoords[] {
	0., 0., 0., 0.,
	0., 1., 0., 1.,
	1., 0., 1., 0.,
	1., 1., 1., 1.,
};
static const unsigned int model_faces[] {
	0, 4, 6, 2,
	1, 3, 7, 5,
	0, 2, 3, 1,
	4, 5, 7, 6,
	2, 6, 7, 3,
	0, 1, 5, 4,
};

//research better strategy to get the most out of prediction
//class NaturalFeatureKalman : ExtendedKalman {
//public:
//	NaturalFeatureKalman();
//
//	//state transition function (usually identity or sensor calibration)
//	cv::Mat f(cv::Mat _x);
//	//jacobian of state transition (usually identity matrix, unless calibration)
//	cv::Mat getF(cv::Mat _x);
//	//observation function (array of m elements predicting observations)
//	cv::Mat h(cv::Mat _x);
//	//jacobian of observation (m x n matrix)
//	cv::Mat getH(cv::Mat _x);
//};

//featuer point extractor
class FeatureTrack : public PositionTracker, public LightTracker {
	//cv::composeRT computes derivitives from initial pose and change in pose
private:
	//need the camera
	R200* cam;
	//state prediction
	//NaturalFeatureKalman nfk;
	//PositionTracker return object to the tracking service
	PositionObject pose;
	//all feature points this frame
	Points feats;
	//segmented feature points for consideration, ordered matched points for pose estiomation
	Points segment, segMatches;
	//the object's feature points to track, ordered matched points for pose estimation
	Points3D objPoints, objMatches;
	//used by opencv for pose estimation
	cv::Mat rvec, tvec, debugDisp;

	//the filter for considering a point segmented (should let user implement via function pointer?)
	inline bool segmentPoint(const cv::Point& p);
	//matches segment against predicted objPoints and places them (in order) into segMatches / objMatches
	void matchPoints();
	//just uses segMatches and objMatches, then calls getMatrix
	bool poseEstimation();

public:
	//common name used to generate marker names
	std::string name;

	//feature point criteria
	int featPtMax = 100;
	double featPtQual = 0.1, featPtMinDist = 10.0;
	bool needInitialPose = true;

	//most useful construct, featPoints shouldnt change (object should be unchanging)
	FeatureTrack(std::string nom, R200* _cam, const float* obj, const int nPts);

	//set the cad model to be tracked
	void setModel(const float* obj, const int nPts);
	//set initial guess for view
	void setPose(cv::Mat& _rvec, cv::Mat& _tvec);

	//calculate opengl mat4 and the inverse transform to the output PositionObject
	void getMatrix();
	//on demand process image
	void getAllPointsOnce(cv::Mat& frame);
	//just returns ptr to segment
	Points* getSegment();

	//returns a list of inliers used for pose estimation
	std::vector<LightObject*> getLights();
	//just forward the pose out
	std::vector<PositionObject*> getPositions();
};
