// Fiducial.cpp : Defines the exported functions for the DLL application.

#include "stdafx.h"
#include "fiducial.h"

////////////////////////////////////////////////////////////////////// Marker //////////////////////////////////////////////////////////////////////

//usable init
Marker::Marker(std::string nom, MarkerAttribs ma) : PositionObject(nom) {
	markerSizecm = ma.sizecm * .5f;

	rvec = cv::Mat::zeros(3, 1, CV_64FC1);
	tvec = cv::Mat::zeros(3, 1, CV_64FC1);

	size = markerSizecm;
	reliable = ma.stationary;

	origin = glm::mat4(1.f);
}

//destructor
Marker::~Marker() = default;

//adjust openCV output for GL
void Marker::getMatrix() {
	//account for marker size
	for (int i = 0; i < 3; i++) tvec.at<double>(i) *= markerSizecm;
	//get distance
	float x = float(tvec.at<double>(0)), y = float(tvec.at<double>(1)), z = float(tvec.at<double>(2));
	distance = glm::sqrt(x*x + y*y + z*z);
	//at 35cm qual is 100%, at 3.8m qual is 25%
	float d = distance / 200.f - .175f;
	quality = 1.f / (1.f + d*d);
	//get wall time
	time = hrClock::now();
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
	printf("Marker Pose:\n%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n",
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


///////////////////////////////////////////////////////////////////// Fiducial /////////////////////////////////////////////////////////////////////

//light tracking
void trackLight(Marker* newMarker, i_Camera* cam) {
	//view the sphere
	cv::Mat sphereView;
	//bit of magic here
	int sphereSizepx = int(1.f / (newMarker->distance + 35.f)*3000.f - 13.f), halfSizepx = sphereSizepx >> 1,
		biggestLight = 0, biggestLightInd = 0;
	RectExt frameView = RectExt(int(newMarker->center.x - halfSizepx), int(newMarker->center.y - halfSizepx), sphereSizepx, sphereSizepx).intersect(RectExt(0, 0, cam->rgb.frame.cols, cam->rgb.frame.rows));

	//if not looking at anything
	if (frameView.width*frameView.height < 16)
		return;

	cv::cvtColor(cam->rgb.frame(frameView), sphereView, CV_BGR2GRAY);
	
	//filter out everything that isnt light specular
	cv::adaptiveThreshold(sphereView, sphereView, 255., cv::THRESH_BINARY, cv::ADAPTIVE_THRESH_MEAN_C, 3, -18.0);

	std::vector<std::vector<cv::Point2i>> blobs;
	std::vector<cv::Vec4i> unused;

	//get shapes
	cv::findContours(sphereView, blobs, unused, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

	//if contours
	if (blobs.size() == 0) return;

#ifdef DBG_LIGHT
	cv::circle(sphereView, cv::Point(frameView.width >> 1, frameView.height >> 1), int(halfSizepx) - 1, cv::Scalar(64), 1);
#endif

	cv::Point2f lightCenter;
	std::vector<cv::Point2i> blob;
	glm::vec2 blobLoc;
	float lightRadiuxpx, blobDist;

	newMarker->lights.clear();

	//for each shape found
	for (int k = 0; k < blobs.size(); k++) {
		cv::approxPolyDP(blobs[k], blob, 0, true);
		cv::minEnclosingCircle(blob, lightCenter, lightRadiuxpx);

		//get magnitude
		blobLoc = glm::vec2(lightCenter.x, lightCenter.y) - glm::vec2(halfSizepx, halfSizepx);
		blobDist = glm::sqrt(glm::dot(blobLoc, blobLoc));

		//if center outside sphere
		if ((int(blobDist + lightRadiuxpx) >> 1) > halfSizepx) continue;
		//use additional information based on RVEC to filter out branches

		//get biggest blob
		if (lightRadiuxpx > biggestLight) {
			biggestLight = int(lightRadiuxpx);
			biggestLightInd = int(newMarker->lights.size());
		}

		//take a few samples to get light color
		cv::Point sample = lightCenter;
		uchar* rgbdata = cam->rgb.frame.data;
		cv::MatStep stp = cam->rgb.frame.step;
		int chn = cam->rgb.frame.channels();
		//multiply with other samples to saturate color
		glm::vec3 smp = glm::vec3(float(rgbdata[int(sample.y)*stp + int(sample.x + lightRadiuxpx)*chn]) / 128.f + .5f,
								  float(rgbdata[int(sample.y)*stp + int(sample.x)*chn + 1]) / 128.f + .5f,
								  float(rgbdata[int(sample.y)*stp + int(sample.x)*chn + 2]) / 128.f + .5f);
		smp *= glm::vec3(float(rgbdata[int(sample.y + lightRadiuxpx)*stp + int(sample.x)*chn]) / 128.f + .5f,
						 float(rgbdata[int(sample.y)*stp + int(sample.x)*chn + 1]) / 128.f + .5f,
						 float(rgbdata[int(sample.y)*stp + int(sample.x)*chn + 2]) / 128.f + .5f);

		newMarker->lights.push_back(LightObject(newMarker->name));
		//fill in properties
		newMarker->lights.back().direction = glm::vec3(blobLoc * 1.05f, lightRadiuxpx);
		newMarker->lights.back().color = glm::normalize(smp);
		newMarker->lights.back().power = lightRadiuxpx;
	}

	//for each specular found
	for (int j = 0; j < newMarker->lights.size(); j++) {
		LightObject* alight = &newMarker->lights[j];
		float lx = alight->direction.x, ly = alight->direction.y;
		float dx = ((newMarker->center.x + lx) / float(cam->rgb.width >> 1) - 1.f) * cam->rgb.fovH * .5f * DEG2RAD,
			dy = ((newMarker->center.y + ly) / float(cam->rgb.height >> 1) - 1.f) * cam->rgb.fovV * .5f * DEG2RAD;
		//compute reflection			 was sin(dx), sin(dy)
		alight->direction = glm::reflect(glm::vec3(dx, dy, glm::sqrt(1.f - dx*dx - dy*dy)), glm::normalize(glm::vec3(lx, ly, glm::sqrt(halfSizepx*halfSizepx - lx*lx - ly*ly))));
#ifdef DBG_LIGHT
		cv::line(sphereView, cv::Point(int(lx), int(ly)), cv::Point(int(lx + alight->direction.x), int(ly + alight->direction.y)), cv::Scalar(192), 5);
#endif
	}

#ifdef DBG_LIGHT
	cv::imshow(newMarker->name + " lights", sphereView);
#endif

}

//usable init
Fiducial::Fiducial() {
	name = "Fiducial";
	//global tolerance and thresh
	tolerance = 5;
	threshold = 98;

#ifdef GHETTO
	defaultMarkerSize = 5.f;
	markerattr[25].sizecm = 5.f;
	markerattr[25].stationary = true;
	markerattr[30].sizecm = 5.f;
	markerattr[30].stationary = true;
#else
	//load from config file?
	defaultMarkerSize = 15.f;
	markerattr[25].sizecm = 15.f;
	markerattr[25].stationary = true;
	markerattr[30].sizecm = 29.2f;
	markerattr[30].stationary = true;
	markerattr[31].sizecm = 10.f;
	markerattr[31].stationary = false;
#endif

}
//destructor
Fiducial::~Fiducial() = default;

//marker tracking
void Fiducial::trackMarkers() {
	cv::Mat frameBNW, frameVAL;

	//replace checks with confidence levels
	std::vector<std::vector<cv::Point2i>> tmp;
	std::vector<cv::Point2i> contours;
	std::vector<cv::Vec4i> unused;
	std::vector<int> foundInds;

	//temp props
	RectExt aabb;
	int bright, dark, contr, branch, finalID, res[4];
	double size;
	glm::vec2 verts[4], origin, acc;
	glm::vec3 whiteBal = glm::vec3(0.0);
	std::vector<glm::vec2> shape;

	//get grayscale
	cv::cvtColor(cam->rgb.frame, frameVAL, CV_BGR2GRAY);
	//convert image to black and white
	cv::inRange(frameVAL, threshold, 255, frameBNW);

	cv::findContours(frameBNW, tmp, unused, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

	//if no contours
	if (tmp.size() == 0) {
		return;
	}

	contours.resize(1);
	//unused.clear();

	//for each shape found
	for (int h = 0; h < tmp.size(); h++) {
		cv::approxPolyDP(tmp[h], contours, tolerance, true);
		//if its not a quad
		if (contours.size() != 4) continue;
		//check if its not too big or small
		size = cv::contourArea(contours);
		//goldilocks area (min size 24x24 max size 480x480)
		if (size < 1024. || size > 160000.) continue;

		//get the points and other attribs
		verts[0] = glm::vec2(contours[0].x, contours[0].y);
		verts[1] = glm::vec2(contours[1].x, contours[1].y);
		verts[2] = glm::vec2(contours[2].x, contours[2].y);
		verts[3] = glm::vec2(contours[3].x, contours[3].y);

		//get axis align bounding box
		getAABB(verts, 4, aabb);
		if (aabb.width < 12 || aabb.height < 12) continue;

#ifdef DBG_MARKER
		cv::Mat dbg;
		cv::cvtColor(cam->rgb.frame(aabb), dbg, CV_BGR2GRAY);
		//cam->frameVAL(aabb).copyTo(dbg);
		cv::Point offset = cv::Point(aabb.x, aabb.y);
		cv::line(dbg, contours[0] - offset, contours[1] - offset, cv::Scalar(128), 2);
		cv::line(dbg, contours[1] - offset, contours[2] - offset, cv::Scalar(128), 2);
		cv::line(dbg, contours[2] - offset, contours[3] - offset, cv::Scalar(128), 2);
		cv::line(dbg, contours[3] - offset, contours[0] - offset, cv::Scalar(128), 2);
#endif

		//compute center point by intersecting opposing corners
		intersect(verts[0], verts[2], verts[1], verts[3], &origin);

		glm::vec2 sample;
		//this distribution controls the minimum angle
		glm::vec3 wuv(0.92, 0.0, 0.08);
		int smp;
		bool smp_fail = false;

		bright = threshold;
		dark = threshold;

		//sample for black blanace
		for (int i = 0; i < 4; i++) {
			//get the pixel coords of the UV sample
			inverseBarycentric(wuv, verts[i], verts[(i + 1) & 3], verts[(i + 2) & 3], sample);

#ifdef DBG_MARKER
			cv::line(dbg, cv::Point(int(sample.x - aabb.x), int(sample.y - aabb.y)), cv::Point(int(sample.x - aabb.x), int(sample.y - aabb.y)), cv::Scalar(int(smp < threshold) * 255), 5);
#endif

			//return the sample
			smp = frameVAL.data[int(sample.y)*frameVAL.step + int(sample.x)*frameVAL.channels()];

			//if white border failed
			if (smp > threshold) {
				smp_fail = true;
				break;
			}

			//if lighter than smpLo, record it
			if (smp < dark) dark = smp;
		}

		if (smp_fail || (dark + 8 > threshold)) continue;

		//project points here if matched to a known prediction	

		glm::vec2 edge, sample2;
		float len, len2;
		//reset
		whiteBal = glm::vec3(0.0);

		//5 combos per branch (non existant, tier 1, tier 1+CCW, tier 1+CW, tier 1+CCW+CW)
		for (int i = 0; i < 4; ++i) {
			res[i] = 0;
			//get vector from center to each vertex
			edge = verts[i] - origin;
			//get a perspective safe length towards tier 1
			len = 0.8f + glm::length(edge) / 3.2f;
			//vector between samples
			edge = glm::normalize(edge) * len;
			//get the sample coords
			sample = origin + edge;
			//get color of sample
			smp = frameVAL.data[int(sample.y)*frameVAL.step + int(sample.x)*frameVAL.channels()];

#ifdef DBG_MARKER
			cv::line(dbg, cv::Point(int(sample.x - aabb.x), int(sample.y - aabb.y)), cv::Point(int(sample.x - aabb.x), int(sample.y - aabb.y)), cv::Scalar(int(smp < threshold) * 255), 5);
#endif

			//sample darker than thresh
			if (smp < threshold) {
				//branch failed
				if (smp < dark) dark = smp;
				continue;
			}
			else if (smp > bright) {
				int loc = int(sample.y)*int(cam->rgb.frame.step) + int(sample.x)*cam->rgb.frame.channels();
				uchar* dta = cam->rgb.frame.data;
				whiteBal.b += dta[loc];
				whiteBal.g += dta[loc + 1];
				whiteBal.r += dta[loc + 2];
				whiteBal *= .5f;
				//new lightest
				bright = smp;
			}
			//tier 1 is worth 1 point
			res[i] = 1;

#ifdef MARKER_TIER_ONE
			//disable tier 2
			continue;
#endif
			//get slope of nearest edge
			edge = glm::normalize(verts[i] - verts[(i + 1) & 3]);
			//newest length
			len2 = len * (0.35f + abs(edge.x) * 0.2f);
			//newest sample
			sample2 = glm::vec2(sample + edge * len2);
			//get color of sample
			smp = frameVAL.data[int(sample2.y)*frameVAL.step + int(sample2.x)*frameVAL.channels()];

#ifdef DBG_MARKER
			cv::line(dbg, cv::Point(int(sample2.x - aabb.x), int(sample2.y - aabb.y)), cv::Point(int(sample2.x - aabb.x), int(sample2.y - aabb.y)), cv::Scalar(int(smp < threshold) * 255), 5);
#endif

			//if lighter than smpLo, record it
			if (smp < dark) dark = smp;
			else if (smp > bright) {
				int loc = int(sample2.y)*int(cam->rgb.frame.step) + int(sample2.x)*cam->rgb.frame.channels();
				uchar* dta = cam->rgb.frame.data; 
				whiteBal.b += dta[loc];
				whiteBal.g += dta[loc + 1];
				whiteBal.r += dta[loc + 2];
				whiteBal *= .5f;
				//new lightest
				bright = smp;
			}
			//tier 2 CCW is worth 1 point
			res[i] += int(smp > threshold);
			//get slope of nearest edge
			edge = glm::normalize(verts[i] - verts[(i + 3) & 3]);
			//newest length
			len2 = len * (0.35f + abs(edge.x) * 0.2f);
			//newest sample
			sample2 = glm::vec2(sample + edge * len2);
			//get color of sample
			smp = frameVAL.data[int(sample2.y)*frameVAL.step + int(sample2.x)*frameVAL.channels()];

#ifdef DBG_MARKER
			cv::line(dbg, cv::Point(int(sample2.x - aabb.x), int(sample2.y - aabb.y)), cv::Point(int(sample2.x - aabb.x), int(sample2.y - aabb.y)), cv::Scalar(int(smp < threshold) * 255), 5);
#endif

			//if lighter than smpLo, record it
			if (smp < dark) dark = smp;
			else if (smp > bright) {
				int loc = int(sample2.y)*int(cam->rgb.frame.step) + int(sample2.x)*cam->rgb.frame.channels();
				uchar* dta = cam->rgb.frame.data;
				whiteBal.b += dta[loc];
				whiteBal.g += dta[loc + 1];
				whiteBal.r += dta[loc + 2];
				whiteBal *= 0.5f;
				//new lightest
				bright = smp;
			}
			//tier 2 CW is worth 2 points
			res[i] += int(smp > threshold) * 2;
		}

		//compare white pixels to number of detected branches

		contr = bright - dark;
		if (contr < 64) continue;

		//check if rotationally symmetrical or all equal
		if (res[0] == res[2] && res[1] == res[3]) continue;
		else {
			//equal branches can still be adjacent
			int lowestVal = 5;
			std::vector<int> lowestBranches;
			//search for lowest branch (branches may be same)
			for (int b = 0; b < 4; b++) {
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
					if (res[lowestBranches[b]] != res[(lowestBranches[b] + 1) & 3]) branch = lowestBranches[b];
				}
			}
		}

		//generate a few points to get perspective
		std::vector<cv::Point2f> imgPoints;
		Marker *newMarker = nullptr;
		int curBranch = branch;
		bool found = false;

		finalID = 0;
		//wind the correct way
		for (int w = 0; w < 4; w++) {
			curBranch = (branch + w) & 3;
			finalID += int(pow(5, (3 - w))) * res[curBranch];
			imgPoints.push_back(contours[curBranch]);
			shape.push_back(verts[curBranch]);
		}

		//try to find tracker ID in trackers
		for (int i = 0; i < markers.size(); i++) {
			newMarker = &(markers[i]);
			if (newMarker->markerID == finalID) {
				found = true;
				foundInds.push_back(i);
				break;
			}
		}

		//if we found the marker
		if (found && newMarker != nullptr) {
			//check if pose estimation glitched out
			if (abs(newMarker->rvec.at<double>(0) > M_PI)) {
				//this causes pose estimation to start fresh
				newMarker->visible = false;
				newMarker->changed = true;
			}
		} else {
			//get size cm, if 0 then write back default size
			markerattr[finalID].sizecm = (markerattr[finalID].sizecm > 0.0001f) ? markerattr[finalID].sizecm : defaultMarkerSize;

#ifdef DBG_FIDUCIAL
			std::printf("New ID:%i\tBranches: %i %i %i %i\tSize (cm): %f\n", finalID, res[branch], res[(branch + 1) & 3], res[(branch + 2) & 3], res[(branch + 3) & 3], markerattr[finalID].sizecm);
#endif

			//keep track of newly found
			foundInds.push_back(int(markers.size()));
			markers.push_back(Marker(name + std::to_string(finalID), markerattr[finalID]));

			//newborne marker lawl
			newMarker = &markers.back();
			//ensure pose ID is global unique
			newMarker->ID = poseBaseID + finalID;
			newMarker->visible = false;
			newMarker->changed = true;
		}

#ifdef DBG_MARKER
		cv::imshow(newMarker->name, dbg);
#endif

		//save state variables
		newMarker->area = int(size);
		newMarker->contrast = contr;
		newMarker->corners = shape;
		newMarker->markerID = finalID;
		newMarker->rect = aabb;
		newMarker->rootBranch = branch;
		newMarker->smpHi = bright;
		newMarker->smpLo = dark;
		newMarker->whiteBal = whiteBal;
		newMarker->center = origin;
		newMarker->acc = acc;
		newMarker->sLoc = origin;
		newMarker->sLastloc = newMarker->sLoc;
		newMarker->sVelocity = glm::normalize(newMarker->sLoc - newMarker->sLastloc);

		//inch thresh 1/10th the way to the average
		threshold = int(float(threshold * 9 + ((bright + dark) >> 1)) * 0.1f);
		//adjust threshold based on makrer appearance
		if ((threshold - (contr >> 2)) < dark && threshold + 2 <= bright)
			threshold += 2;
		else if ((threshold + (contr >> 2)) > bright && threshold - 2 >= dark)
			threshold -= 2;

		//if its known and pose estimation hasnt glitched out
		bool useGuess = found && newMarker->visible;

		//this is usually a bad idea in low contrast situations. somehow get this alg to follow gradient other way? (towards brightness)
		if (contr > 32) cv::cornerSubPix(frameVAL, imgPoints, cv::Size(3, 3), cv::Size(-1, -1), cv::TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 20, 0.01));
		//this runs quickly but has low accuracy
		cv::solvePnP(quadPoints, imgPoints, cam->rgb.camMat, cam->rgb.camDist, newMarker->rvec, newMarker->tvec, useGuess, cv::SOLVEPNP_P3P);
		//refine until default criteria met
		cv::solvePnP(quadPoints, imgPoints, cam->rgb.camMat, cam->rgb.camDist, newMarker->rvec, newMarker->tvec, true, cv::SOLVEPNP_ITERATIVE);

		//calculate opengl stuff
		newMarker->getMatrix();

		//only track when close enough
		if (newMarker->distance < 240.f)
			trackLight(newMarker, cam);

	} //for each shape

	//set all trackers to invisible here to exploit edge case in solvePNP
	for (int i = 0; i < markers.size(); i++) markers[i].visible = false;
	//if no markers found
	if (foundInds.size() == 0) {
		threshold = (rand() & 127) + 64;
	} else {
		//set found markers to visible
		for (int i = 0; i < foundInds.size(); i++)
			markers[foundInds[i]].visible = true;
	}
}

//set input stream
void Fiducial::setCam(i_Camera* _cam) {
	cam = _cam;
	hasCam = true;
}

//gets list of visible markers
std::vector<PositionObject*> Fiducial::getPositions() {
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
std::vector<LightObject*> Fiducial::getLights() {
	std::vector<LightObject*> out;
	for (int i = 0; i < markers.size(); i++) {
		for (int j = 0; j < markers[i].lights.size(); j++) {
			out.push_back(&markers[i].lights[j]);
		}
	}
	return out;
}

//sets new origin for trackers
void Fiducial::originCallback(PositionObject* newOrigin) {
	//loop through all markers
	for (int i = 0; i < markers.size(); i++) {
		Marker& curMark = markers[i];
		if (curMark.visible && !curMark.isOrigin) curMark.originCallback(newOrigin);
	}
}
//pause tracking
void Fiducial::pauseTracking() {
	paused = true;
}
//resume tracking
void Fiducial::resumeTracking() {
	paused = false;
}