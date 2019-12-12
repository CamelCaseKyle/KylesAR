#pragma once

#include <opencv2\core.hpp>
#include <glm.hpp>
#include <chrono>

typedef std::chrono::high_resolution_clock hrClock;
typedef hrClock::time_point timePoint;

//all info about a single camera stream including the most recent image
class CameraStream {
public:
	bool exists = false;
	int width = 0, height = 0, type = 0, stride = 0;
	float fovH = 0.f, fovV = 0.f, fRate = 0.f;
	timePoint time;
	std::vector<double> camDist;
	std::string name = "";
	cv::Matx33d camMat;
	cv::Mat frame;

	CameraStream() = default;
	CameraStream(std::string _name) : name(_name), exists(true) {}
	
	~CameraStream() = default;
};

//all they have to do is inherit the interface
class i_Camera {
protected:
	//has initialized properly, has optional streams
	bool isOpen;
public:
	std::string name = "";

	//camera extrinsic transformation
	glm::mat4 depth2rgb;
	//common streams
	CameraStream rgb, depth;
	//special streams
	std::vector<CameraStream> stream;

	//important
	virtual ~i_Camera() = default;

	//start/resume streaming
	virtual bool start() = 0;
	//pause streaming
	virtual bool pause() = 0;

	//check if opened
	bool open() { return isOpen; }
};

