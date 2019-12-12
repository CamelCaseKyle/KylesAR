// hexmarker.cpp : Defines the exported functions for the DLL application.

#include "stdafx.h"
#include "hexmarker.h"

////////////////////////////////////////////////////////////////////// Hexmarker //////////////////////////////////////////////////////////////////////

//usable init
Hexmarker::Hexmarker(int id, MarkerAttribs ma, i_Camera* cam) : PositionObject("Hexmarker" + std::to_string(id)) {
	frame = &cam->rgb.frame;
	fovH = cam->rgb.fovH;
	fovV = cam->rgb.fovV;
	rvec = cv::Mat::zeros(3, 1, CV_64FC1);
	tvec = cv::Mat::zeros(3, 1, CV_64FC1);
	size = ma.sizecm;
	reliable = ma.stationary;
	origin = glm::mat4(1.f);
}

//destructor
Hexmarker::~Hexmarker() = default;

//adjust openCV output for GL
void Hexmarker::getMatrix() {
	//account for marker size
	for (int i = 0; i < 3; i++) tvec.at<double>(i) *= size;
	//get distance
	float x = float(tvec.at<double>(0)), y = float(tvec.at<double>(1)), z = float(tvec.at<double>(2));
	distance = glm::sqrt(x*x + y*y + z*z);
	//at 35cm qual is 100%, at 3.8m qual is 25%
	float d = distance / 200.f - .175f;
	quality = 1.f / (1.f + d*d);

	//get the matricies for OpenGL
	cv::Mat rotation(3, 3, CV_64FC1), viewMatrix(4, 4, CV_64FC1);
	//convert rvec to rotation matrix
	cv::Rodrigues(rvec, rotation);
	//fix and build the matrix
	for (int i = 0; i < 3; i++) view[i] = glm::vec4(rotation.at<double>(i, 0), rotation.at<double>(i, 1), rotation.at<double>(i, 2), tvec.at<double>(i));
	view[3] = glm::vec4(0.f, 0.f, 0.f, 1.f);

	//invert the y and z axis
	glm::mat4 cv2gl = glm::mat4(1.0f); cv2gl[1].y = -1.0f; cv2gl[2].z = -1.0f;
	view = cv2gl * glm::transpose(view);

#ifdef LOG_POSE
	printf("Hexmarker Pose:\n%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n",
		view[0][0], view[0][1], view[0][2], view[0][3],
		view[1][0], view[1][1], view[1][2], view[1][3],
		view[2][0], view[2][1], view[2][2], view[2][3],
		view[3][0], view[3][1], view[3][2], view[3][3]
	);
#endif

	//inverse rotation
	cv::Mat R = rotation.t();
	//compuate inverse translation
	cv::Mat t = -R * tvec;
	//build matrix
	for (int i = 0; i < 3; i++) view_cam[i] = glm::vec4(R.at<double>(i, 0), R.at<double>(i, 1), R.at<double>(i, 2), 0.0f);
	view_cam[3] = glm::vec4(t.at<double>(0), t.at<double>(1), t.at<double>(2), 1.0f);
}

//light tracking
void Hexmarker::trackLight() { }

//find self with prediction
void Hexmarker::trackSelf() { }

///////////////////////////////////////////////////////////////////// Hextracker /////////////////////////////////////////////////////////////////////

//usable init
Hextracker::Hextracker() {
	name = "Hextracker";
	//global tolerance and thresh
	tolerance = 5;
	threshold = 98;

	//load from config file?
	defaultMarkerSize = EdgeLength;
}
//destructor
Hextracker::~Hextracker() = default;

//marker tracking
void Hextracker::trackMarkers() {

	//get grayscale and B&W
	cv::Mat frameBNW, frameVAL;
	cv::cvtColor(cam->rgb.frame, frameVAL, CV_BGR2GRAY);
	cv::inRange(frameVAL, threshold, 255, frameBNW);

	//get contours
	std::vector<std::vector<cv::Point2i>> tmp;
	std::vector<cv::Vec4i> unused;
	cv::findContours(frameBNW, tmp, unused, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

	//if no contours
	if (tmp.size() == 0) {
		return;
	}

	int bright, dark, contr, branch, finalID, res[6];
	RectExt aabb;
	cv::Point i1, i2, origin;
	cv::Scalar whiteBal = cv::Scalar(0, 0, 0);
	std::vector<int> foundInds;
	std::vector<cv::Point> contours(1), shape;

	//for each shape found
	for (int h = 0; h < tmp.size(); h++) {
		cv::approxPolyDP(tmp[h], contours, tolerance, true);
		//if its not a hex
		if (contours.size() != 6) continue;
		//check if its not too big or small
		double size = cv::contourArea(contours);
		//goldilocks area (min size 24x24 max size 480x480)
		if (size < 1024. || size > 160000.) continue;

		//get axis align bounding box with some padding if possible
		getAABB(contours, aabb);

		if (aabb.width < 12 || aabb.height < 12) continue;

#ifdef DBG_MARKER
		cv::Mat dbg;
		cv::cvtColor(cam->rgb.frame(aabb), dbg, CV_BGR2GRAY);
		//cam->frameVAL(aabb).copyTo(dbg);
#endif

		//compute center point by intersecting opposing corners
		intersect(contours[0], contours[3], contours[1], contours[4], &i1);
		intersect(contours[2], contours[5], contours[0], contours[3], &i2);
		// just average them i guess
		origin = (i1 + i2) / 2;

		cv::Point sample;
		int smp;
		bool smp_fail = false;

		bright = threshold;
		dark = threshold;

		//sample for black blanace
		for (int i = 0; i < 6; i++) {
			//get the pixel coords of the UV sample
			sample = contours[i] * 0.9f + origin * 0.1f;

			//return the sample
			smp = frameVAL.data[sample.y*frameVAL.step + sample.x*frameVAL.channels()];

#ifdef DBG_MARKER
			cv::line(dbg, cv::Point(int(sample.x - aabb.x), int(sample.y - aabb.y)), cv::Point(int(sample.x - aabb.x), int(sample.y - aabb.y)), cv::Scalar(int(smp < threshold) * 255), 5);
#endif

			//if black border failed
			if (smp > threshold) {
				smp_fail = true;
				break;
			}

			//if lighter than smpLo, record it
			if (smp < dark) dark = smp;
		}

		if (smp_fail || (dark + 8 > threshold)) {

#ifdef DBG_MARKER
			cv::imshow("dbg", dbg);
#endif

			continue;
		}

		//project points here if matched to a known prediction	

		glm::vec2 edge, sample2;
		float len, len2;
		//reset
		whiteBal = cv::Scalar(0.,0.,0.);

		//just sample for the small white square
		for (int i = 0; i < 6; ++i) {
			sample = contours[i] * 0.25f + origin * 0.75f;
			//get color of sample
			smp = frameVAL.data[int(sample.y)*frameVAL.step + int(sample.x)*frameVAL.channels()];

#ifdef DBG_MARKER
			cv::line(dbg, cv::Point(int(sample.x - aabb.x), int(sample.y - aabb.y)), cv::Point(int(sample.x - aabb.x), int(sample.y - aabb.y)), cv::Scalar(int(smp < threshold) * 255), 5);
#endif
			res[i] = 0;
			//sample darker than thresh
			if (smp < threshold) {
				//branch failed
				if (smp < dark) dark = smp;
				continue;
			} else if (smp > bright) {
				int loc = int(sample.y)*int(cam->rgb.frame.step) + int(sample.x)*cam->rgb.frame.channels();
				uchar* dta = cam->rgb.frame.data;
				whiteBal[2] = (whiteBal[2] + dta[loc]) * 0.5f;
				whiteBal[1] = (whiteBal[1] + dta[loc+1]) * 0.5f;
				whiteBal[0] = (whiteBal[0] + dta[loc+2]) * 0.5f;
				//new lightest
				bright = smp;
			}
			//tier 1 is worth 1 point
			res[i] = 1;
		}

		//compare white pixels to number of detected branches

		contr = bright - dark;
		if (contr < 64) continue;

		//check if rotationally symmetrical or all equal
		if (res[0] == res[3] && res[1] == res[4] && res[2] == res[5]) continue;
		else {
			//equal branches can still be adjacent
			int lowestVal = 5;
			std::vector<int> lowestBranches;
			//search for lowest branch (branches may be same)
			for (int b = 0; b < 6; b++) {
				if (res[b] <= lowestVal) {
					lowestVal = res[b];
					if (res[b] < lowestVal) lowestBranches.clear();
					lowestBranches.push_back(b);
				}
			}
			//this shouldnt happen
			if (lowestBranches.size() == 0) continue;
			else if (lowestBranches.size() == 1) branch = lowestBranches.front();
			else {
				//because the branches can still be adjacent, look at neighbor CW to it
				for (int b = 0; b < lowestBranches.size(); b++) {
					//if found edge case
					if (res[lowestBranches[b]] != res[(lowestBranches[b] + 1) % 6]) branch = lowestBranches[b];
				}
			}
		}

		//generate a few points to get perspective
		std::vector<cv::Point2f> imgPoints;
		Hexmarker *newHexmarker = nullptr;
		int curBranch = branch;
		bool found = false;

		finalID = 0;
		//wind the correct way
		for (int w = 0; w < 6; w++) {
			curBranch = (branch + w) % 6;
			finalID += int(pow(2, 5-w)) * res[curBranch];
			imgPoints.push_back(contours[curBranch]);
			shape.push_back(contours[curBranch]);
		}

		//try to find tracker ID in trackers
		for (int i = 0; i < markers.size(); i++) {
			newHexmarker = &(markers[i]);
			if (newHexmarker->markerID == finalID) {
				found = true;
				foundInds.push_back(i);
				break;
			}
		}

		//if we found the marker
		if (found && newHexmarker != nullptr) {
			//check if pose estimation glitched out
			if (abs(newHexmarker->rvec.at<double>(0) > M_PI)) {
				//this causes pose estimation to start fresh
				newHexmarker->visible = false;
				newHexmarker->changed = true;
			}
		} else {
			//get size cm, if 0 then write back default size
			markerattr[finalID].sizecm = (markerattr[finalID].sizecm > 0.0001f) ? markerattr[finalID].sizecm : defaultMarkerSize;

			//keep track of newly found
			foundInds.push_back(int(markers.size()));
			markers.push_back(Hexmarker(finalID, markerattr[finalID], cam));

			//newborne marker lawl
			newHexmarker = &markers.back();
			//ensure pose ID is global unique
			newHexmarker->ID = poseBaseID + finalID;
			newHexmarker->visible = false;
			newHexmarker->changed = true;
		}

#ifdef DBG_MARKER
		cv::imshow(newHexmarker->name, dbg);
#endif

		//save state variables
		newHexmarker->area = int(size);
		newHexmarker->contrast = contr;
		newHexmarker->corners = shape;
		newHexmarker->markerID = finalID;
		newHexmarker->rect = aabb;
		newHexmarker->rootBranch = branch;
		newHexmarker->smpHi = bright;
		newHexmarker->smpLo = dark;
		newHexmarker->whiteBal = whiteBal;
		newHexmarker->center = origin;
		
		//inch thresh 1/10th the way to the average
		threshold = int(float(threshold * 9 + ((bright + dark) >> 1)) * 0.1f);
		//adjust threshold based on makrer appearance
		if ((threshold - (contr >> 2)) < dark && threshold + 2 <= bright)
			threshold += 2;
		else if ((threshold + (contr >> 2)) > bright && threshold - 2 >= dark)
			threshold -= 2;

		//if its known and pose estimation hasnt glitched out
		bool useGuess = found && newHexmarker->visible;

		//this is usually a bad idea in low contrast situations. somehow get this alg to follow gradient other way? (towards brightness)
		if (contr > 32) cv::cornerSubPix(frameVAL, imgPoints, cv::Size(3, 3), cv::Size(-1, -1), cv::TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 20, 0.01));
		
#ifdef DBG_MARKER
		cv::Point2f offset = cv::Point2f(aabb.x, aabb.y);
		cv::line(dbg, imgPoints[0] - offset, imgPoints[1] - offset, cv::Scalar(128), 2);
		cv::line(dbg, imgPoints[1] - offset, imgPoints[2] - offset, cv::Scalar(128), 2);
		cv::line(dbg, imgPoints[2] - offset, imgPoints[3] - offset, cv::Scalar(128), 2);
		cv::line(dbg, imgPoints[3] - offset, imgPoints[4] - offset, cv::Scalar(128), 2);
		cv::line(dbg, imgPoints[4] - offset, imgPoints[5] - offset, cv::Scalar(128), 2);
		cv::line(dbg, imgPoints[5] - offset, imgPoints[0] - offset, cv::Scalar(128), 2);
#endif

		//refine until default criteria met
		cv::solvePnP(hexPoints, imgPoints, cam->rgb.camMat, cam->rgb.camDist, newHexmarker->rvec, newHexmarker->tvec, useGuess, cv::SOLVEPNP_ITERATIVE);

		//calculate opengl stuff
		newHexmarker->getMatrix();

	} //for each shape

	//set all trackers to invisible here to exploit edge case in solvePNP
	for (int i = 0; i < markers.size(); i++) markers[i].visible = false;
	//if no markers found
	if (foundInds.size() == 0) {
		threshold = (rand() & 127) + 16;
	} else {
		//set found markers to visible
		for (int i = 0; i < foundInds.size(); i++)
			markers[foundInds[i]].visible = true;
	}
}

//set input stream
void Hextracker::setCam(i_Camera* _cam) {
	cam = _cam;
	hasCam = true;
}

//gets list of visible markers
std::vector<PositionObject*> Hextracker::getPositions() {
	//out vector markers
	std::vector<PositionObject*> out;
	//cant track without cam
	if (!hasCam) return out;
	//updates markers
	trackMarkers();
	for (int i = 0; i < markers.size(); i++) {
		//configurable care about ones we cant see
		if (onlyVisible && !markers[i].visible) continue;
		out.push_back((PositionObject *)&markers[i]);
	}
	return out;
}
//gets list of visible lights
std::vector<LightObject*> Hextracker::getLights() {
	std::vector<LightObject*> out;
	for (int i = 0; i < markers.size(); i++) {
		for (int j = 0; j < markers[i].lights.size(); j++) {
			out.push_back(&markers[i].lights[j]);
		}
	}
	return out;
}

//sets new origin for trackers
void Hextracker::originCallback(PositionObject* newOrigin) {
	//loop through all markers
	for (int i = 0; i < markers.size(); i++) {
		Hexmarker& curMark = markers[i];
		if (curMark.visible && !curMark.isOrigin) curMark.originCallback(newOrigin);
	}
}
//pause tracking
void Hextracker::pauseTracking() {
	paused = true;
}
//resume tracking
void Hextracker::resumeTracking() {
	paused = false;
}