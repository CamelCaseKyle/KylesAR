
#pragma once

#ifdef SLAM_EXPORTS
#define SLAM_API extern "C" __declspec(dllexport)
#else
#define SLAM_API extern "C"
#endif

#include <vector>

#include <opencv2\core.hpp>
#include <opencv2\calib3d.hpp> //pose estimation and extrinsics
#include <opencv2\features2d.hpp>
#include <opencv2\imgproc.hpp>
#include <opencv2\video.hpp>
#include <opencv2\highgui.hpp>

#include <dev\i_Camera.h>		//camera
#include <ext\i_tracker.h>		//tracker interface

#define LOG_POSE

using namespace std;
using namespace cv;

typedef vector<Point2f> Points;
typedef vector<float> Floats;
typedef vector<uchar> UChars;
typedef vector<KeyPoint> Feats;
typedef vector<DMatch> Matches;

class SLAM : public i_PositionTracker {
private:
	//try fast
	cv::Ptr<FastFeatureDetector> fast;
	cv::Ptr<DescriptorExtractor> sift;
	cv::Ptr<cv::FlannBasedMatcher> flann;
	//need the camera
	i_Camera* cam;
	//additional calibration
	double apWidth = 2.0, apHeight = 1.2, fovx, fovy, flen, aspect, trajScale = 2.5;
	Point2d pp;
	//camera frame
	Mat cf, lf, traj;
	//feature points (correlated by index)
	Points cpts, lpts;
	Feats cfts, lfts;
	Mat cdesc, ldesc;
	//feature quality (correlated by index)
	Floats qpts;
	//feature status (correlated by index)
	UChars ptstat, femstat;
	bool hasCam = false;
	//output
	PositionObject pose;

	//a few different algorithms are implemented to toy around with them
	void detect(Mat& inp, Points& outp);
	void detect(Mat& inp, Feats& finp, Mat& outp);
	void track(Mat& prevf, Mat& curf, Points& prevp, Points& curp, Floats& qual, UChars& status);
	void match(Mat& in1, Mat& in2, Feats& f1, Feats& f2, Matches& outp);

public:
	//should only return visible trackers
	bool onlyVisible;

	//minimal init
	SLAM();
	~SLAM();

	//i_PositionTracker
	void setCam(i_Camera* _cam);
	vector<PositionObject*> getPositions();
	void originCallback(PositionObject* newOrigin);
	void pauseTracking();
	void resumeTracking();
};

SLAM_API i_PositionTracker* __stdcall allocate() {
	i_PositionTracker* ret = (i_PositionTracker*)(new SLAM());
	return ret;
}

SLAM_API void __stdcall destroy(i_PositionTracker* inst) {
	delete inst;
}
