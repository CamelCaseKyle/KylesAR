#include "recon.h"

//get the bouding polygon of a group of coplanar points
Points QuickHull(Points& cloud) {
	// find min max of a coord
	// bin points into two groups
	// if a group is 1 point skip next step
	// find max SD to line 
	// add point to conv hull outp
	// 
	// 
}


//print a matrix of uchars (CV_8U)
void printi(cv::Mat& m) {
	int nRows = m.rows;
	int nCols = m.cols;
	int j;
	uchar *p;
	if (m.isContinuous()) {
		nRows *= nCols;
		int jj;
		p = m.ptr<uchar>();
		for (j = 0; j < nRows; ++j) {
			jj = j % nCols;
			printf("%i ", p[j]);
			if (jj == nCols - 1) {
				printf("\n");
			}
		}
	} else {
		int i;
		for (i = 0; i < nRows; ++i) {
			p = m.ptr<uchar>(i);
			for (j = 0; j < nCols; ++j) {
				printf("%i ", p[j]);
			}
			printf("\n");
		}
	}
}


//does nothing but remember the camera
Recon::Recon(i_Camera *icam) : cam(icam) {

	//load point cloud shader
	//ptCloudShade = Shader("experiments\\glsl\\ptcloud.vert", "experiments\\glsl\\ptcloud.frag");
	//ptCloudShade.addUniforms({ "stuff", "things" });

	//create cloud on GPU
	//vao = createVertexArray(false);
	//vbo = createBuffer();
	//int ptCloud_size = int(ptCloud.size()) * sizeof(ptCloud[0]);

	//upload cloud
	//uploadBuffer(GL_ARRAY_BUFFER, &ptCloud[0], ptCloud_size, vbo, false);
	//glEnableVertexAttribArray(0);
	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//glBindVertexArray(0);
}

//pre-process the input stream
void Recon::preProcess(Mat &inp) {
	// use full size full color but maybe chop it up into smaller images
	inp.copyTo(tmpFrame);
	//resize(inp, tmpFrame, cv::Size(), 0.5, 0.5);
}

//detect and describe new features (usually faster)
bool Recon::detectAndCompute(Mat &inp, Feats &keypoints, Mat &descriptors) {

	akaze->detectAndCompute(inp, mask, keypoints, descriptors);
	//printf("compute_detect %i\n", descriptors.rows);

	thresh += (keypoints.size() > 200) ? 0.001f : -0.001f;
	thresh = min(max(thresh, 0.001f), 0.1f);
	akaze->setThreshold(thresh);

	return keypoints.size() > 4;
}

//match features between given frames
bool Recon::match(Mat &in1, Mat &in2, Matches &outp) {

	vector<Matches> tmp;
	bfm->knnMatch(in1, in2, tmp, 2);

	outp.clear();
	//Do ratio test to remove outliers from KnnMatch
	for (int i = 0; i < tmp.size(); i++) {
		//Ratio Test for outlier removal, removes ambiguous matches.
		if (tmp[i][0].distance < nnMatchRatio * tmp[i][1].distance) {
			outp.push_back(tmp[i][0]);
		}
	}

	quality = 100.f * float(outp.size()) / float(in1.rows);
	//printf("match inputs: %i %i output: %i (%f%%)\n", in1.rows, in2.rows, outp.size(), quality);

	nnMatchRatio += (quality < matchPercentage) ? 0.01f : -0.01f;
	nnMatchRatio = min(max(nnMatchRatio, 0.5f), 0.95f);

	return (m.size() > minimumMatches) && (quality > matchPercentage);
}

//calculate the reprojection error in both views
float Recon::reprojError(PositionObject *curPose, vector<cv::Point3f> out3d) {
	Points CimgPts, LimgPts;
	// project points into 'normalized' coordinates because of which undistortPoints we chose
	cv::projectPoints(out3d, curPose->rvec, curPose->tvec, cv::Matx33f::eye(), cv::noArray(), CimgPts);
	cv::projectPoints(out3d, lastRvec, lastTvec, cv::Matx33f::eye(), cv::noArray(), LimgPts);

	// out3d, cpts, and lpts are correlated by index
	float error = 0.f;
	for (int i = 0; i < out3d.size(); i++) {
		Point2f d = LimgPts[i] - lpts[i];
		error += d.dot(d);
		d = CimgPts[i] - cpts[i];
		error += d.dot(d);
	}
	error /= float(out3d.size() * 2);
	error = glm::sqrt(error);

	printf("reproj error %f\n", error);

	return error;
}

//do the 3D triangulation
void Recon::recon(PositionObject* curPose, cv::Mat& out4d) {
	cv::Mat rotMat(3, 3, CV_64F);

	// current pose
	cv::Mat augMatC(3, 4, CV_64F);
	// convert rotation vector into rotation matrix
	cv::Rodrigues(curPose->rvec, rotMat);
	// append translation vector to rotation matrix
	cv::hconcat(rotMat, curPose->tvec, augMatC);

	// last pose
	cv::Mat augMatL(3, 4, CV_64F);
	cv::Rodrigues(lastRvec, rotMat);
	cv::hconcat(rotMat, lastTvec, augMatL);

	out4d = cv::Mat(4, cpts.size(), CV_32F);
	// triangulate into 4D homogenous coords
	cv::triangulatePoints(augMatL, augMatC, lpts, cpts, out4d);
}

//draw debug information
void Recon::debug(PositionObject *curPose) {
	//always draw matches
	cv::Mat tmp;
	cv::drawMatches(lastFrame, lfts, tmpFrame, cfts, m, tmp);

	if (ptCloud.size() > 0) {
		cv::Point pd = cv::Point(2, 2), pd2 = cv::Point(2, -2), ofst = cv::Point(lastFrame.cols, 0);
		Points CimgPts, LimgPts;

		cv::projectPoints(ptCloud, curPose->rvec, curPose->tvec, cam->rgb.camMat, cv::noArray(), CimgPts);
		cv::projectPoints(ptCloud, lastRvec, lastTvec, cam->rgb.camMat, cv::noArray(), LimgPts);

		for (int i = 0; i < CimgPts.size(); ++i) {
			cv::Point cvp = cv::Point(CimgPts[i].x, CimgPts[i].y);
			cv::line(tmp, ofst + cvp - pd, ofst + cvp + pd, cv::Scalar(255, 0, 0));
			cv::line(tmp, ofst + cvp - pd2, ofst + cvp + pd2, cv::Scalar(255, 0, 0));
		}

		for (int i = 0; i < LimgPts.size(); ++i) {
			cv::Point cvp = cv::Point(LimgPts[i].x, LimgPts[i].y);
			cv::line(tmp, cvp - pd, cvp + pd, cv::Scalar(0, 0, 255));
			cv::line(tmp, cvp - pd2, cvp + pd2, cv::Scalar(0, 0, 255));
		}

		printf("____________________\n");

		printf("ptcloud %i MLDB 0 3 %f 2 2 1\n", (int)ptCloud.size(), thresh);
		// dump 3D points, one x y z per line
		for (int i = 0; i < ptCloud.size(); ++i) {
			printf("%f %f %f\n", ptCloud[i].x, ptCloud[i].y, ptCloud[i].z);
		}
	}

	// ptcloud and mdesc are correlated by index, dump them right in
	printi(mdesc);

	printf("____________________\n");

	imshow("results", tmp);
}

/*
void debugGPU(GLuint fbo) {

//use the transform shader
Shader &shade = *shaders[s_depth];
glUseProgram(shade.getID());

//use the projection vertex array
glBindVertexArray(vao[a_projection]);

glm::mat4 pfd = depth_projection * fixDepth;

//transform matrix
glUniform1f(shade.uniform("aspect"), .75f); //rcp aspect of depth image
glUniform1f(shade.uniform("scale"), 6500.f); //depth is mm converted from ushort [0-65535] to float [0-1]
glUniformMatrix4fv(shade.uniform("PFD"), 1, GL_FALSE, &pfd[0][0]);

//sensor data
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, textures[t_depth]);
glUniform1i(shade.uniform("depthFrame"), 0);
//sensor correction
glActiveTexture(GL_TEXTURE1);
glBindTexture(GL_TEXTURE_2D, textures[t_dcalib]);
glUniform1i(shade.uniform("depthCalib"), 1);

// Draw the points!
glDrawArrays(GL_POINTS, 0, pro_vert_size);

glBindVertexArray(0);
glDisable(textures[t_depth]);
glDisable(textures[t_dcalib]);
}
*/

void Recon::update(cv::Mat &curFrame, PositionObject *curPose) {

	if (firstFrame) {
		//downsize or adjust in some way
		preProcess(curFrame);
		//populate points
		if (!detectAndCompute(tmpFrame, cfts, cdesc)) {
			printf("not enough features\n");
			return;
		}
		//remember stuff for next time
		lastPose = curPose->view_cam;
		lastRvec = curPose->rvec.clone();
		lastTvec = curPose->tvec.clone();
		tmpFrame.copyTo(lastFrame);
		firstFrame = false;
		return;
	}

	//if (PHASE == 0) {
	//not enough distance
	if (glm::distance(curPose->view_cam[3], lastPose[3]) < minDist)
		return;

	timePoint t1 = hrClock::now();

	//last state
	lfts = cfts;
	ldesc = cdesc;

	//downsize or adjust in some way
	preProcess(curFrame);

	//PHASE++;
	//return;
	//}

	//if (PHASE == 1) {
	//t1 = hrClock::now();

	//populate points
	if (!detectAndCompute(tmpFrame, cfts, cdesc)) {
		printf("not enough features\n");
		return;
	}
	//PHASE++;
	//return;
	//}

	//if (PHASE == 2) {
	//t1 = hrClock::now();

	//find matches
	if (!match(ldesc, cdesc, m)) {
		printf("not enough matches\n");
		return;
	}

	//PHASE++;
	//return;
	//}

	//if (PHASE == 3) {
	//t1 = hrClock::now();

	cpts.clear();
	lpts.clear();
	mdesc = cv::Mat::zeros(m.size(), cdesc.cols, cdesc.type());

	//extract points and descriptors, sort by matches
	for (int i = 0; i < m.size(); ++i) {
		cpts.push_back(cfts[m[i].trainIdx].pt);
		lpts.push_back(lfts[m[i].queryIdx].pt);
		cdesc.row(m[i].trainIdx).copyTo(mdesc.row(i));
	}

	//undistort cam points into 'normalized' coordinates
	undistortPoints(cpts, cpts, cam->rgb.camMat, cam->rgb.camDist);
	undistortPoints(lpts, lpts, cam->rgb.camMat, cam->rgb.camDist);
	//undistort cam points into camMat coordinates
	//undistortPoints(cpts, cpts, cam->rgb.camMat, cam->rgb.camDist, cv::noArray(), cam->rgb.camMat);
	//undistortPoints(lpts, lpts, cam->rgb.camMat, cam->rgb.camDist, cv::noArray(), cam->rgb.camMat);

	//PHASE++;
	//return;
	//}

	//if (PHASE == 4) {
	//t1 = hrClock::now();

	cv::Mat out4d;
	recon(curPose, out4d);

	//PHASE++;
	//return;
	//}


	// somehow broken!
	//cv::convertPointsFromHomogeneous(out4d, out3d);

	// my implementation
	vector<cv::Point3f> out3d;
	for (int i = 0; i < cpts.size(); ++i) {
		cv::Point3f pt = cv::Point3f(out4d.at<float>(0, i), out4d.at<float>(1, i), out4d.at<float>(2, i));
		out3d.push_back(pt / out4d.at<float>(3, i));
	}

	// calculate the reproj error of both views to make sure we have nice 3D points
	float reproj = reprojError(curPose, out3d);
	if (reproj > maxReprojError) {
		printf("reproj error too high\n");
		return;
	}

	maxReprojError = (maxReprojError + reproj) * 0.5f;

	// overwrite point cloud
	ptCloud.clear();
	// append point cloud
	ptCloud.insert(ptCloud.end(), out3d.begin(), out3d.end());

	debug(curPose);

	// merge point clouds?

	// RANSAC or SVD plane eq
	cv::Mat covar, mean,
		ptCloud_m = Mat(ptCloud.size(), 3, CV_32F, ptCloud.data());
	cv::calcCovarMatrix(ptCloud_m, covar, mean, CV_COVAR_NORMAL | CV_COVAR_ROWS, CV_32F);
	cv::Mat w, u, vt;
	cv::SVDecomp(covar, w, u, vt);

	printf("w matrix\n");
	printm(w);
	printf("u matrix\n");
	printm(u);
	printf("vt matrix\n");
	printm(vt);

	// get coplanar points

	// get a convex hull around the points

	//if (PHASE == 5) {
	//t1 = hrClock::now();

	unsigned long t2 = (hrClock::now() - t1).count();
	printf("RECON %.2fms\n", double(t2) / 1000000.);

	lastPose = curPose->view_cam;
	lastRvec = curPose->rvec.clone();
	lastTvec = curPose->tvec.clone();
	tmpFrame.copyTo(lastFrame);

	//PHASE = 0;
	//return;
	//}
}
