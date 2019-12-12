#include "dans.h"

HomographyInfo::HomographyInfo(Mat hom, vector<Point2f> inl1, vector<Point2f> inl2, vector<DMatch> matches) {
	homography = hom;
	inliers1 = inl1;
	inliers2 = inl2;
	inlier_matches = matches;
}

void drawBoundingBox(Mat image, vector<Point2f> bb) {
	for (unsigned i = 0; i < bb.size() - 1; i++) {
		line(image, bb[i], bb[i + 1], Scalar(0, 0, 255), 4);
	}
	line(image, bb[bb.size() - 1], bb[0], Scalar(0, 0, 255), 4);
}

vector<Point2f> toPoints(vector<KeyPoint> keypoints) {
	vector<Point2f> res;
	for (unsigned i = 0; i < keypoints.size(); i++) {
		res.push_back(keypoints[i].pt);
	}
	return res;
}

//Method for calculating and validating a homography matrix from a set of corresponding points.
HomographyInfo GetHomographyInliers(vector<Point2f> pts1, vector<Point2f> pts2) {
	Mat inlier_mask, homography;
	vector<Point2f> inliers1, inliers2;
	vector<DMatch> inlier_matches;
	if (pts1.size() >= 4) {
		homography = findHomography(pts1, pts2,
			RANSAC, ransac_thresh, inlier_mask);
	}

	//Failed to find a homography
	if (pts1.size() < 4 || homography.empty()) {
		return HomographyInfo();
	}

	//Homography determinant test
	const double det = homography.at<double>(0, 0) * homography.at<double>(1, 1) - homography.at<double>(1, 0) * homography.at<double>(0, 1);
	if (det < 0.1) {
		//Homography is not good.
		return HomographyInfo();
	}

	for (unsigned i = 0; i < pts1.size(); i++) {
		if (inlier_mask.at<uchar>(i)) {
			int new_i = static_cast<int>(inliers1.size());
			inliers1.push_back(pts1[i]);
			inliers2.push_back(pts2[i]);
			inlier_matches.push_back(DMatch(new_i, new_i, 0));
			if (inlier_matches.size() > maxMatches) break;
		}
	}
	//Return homography and corresponding inlier point sets
	return HomographyInfo(homography, inliers1, inliers2, inlier_matches);
}

int Dans() {
	cout << CV_VERSION << endl;

	///// 1 - INITIALIZE OPENCV STUFF/////
	//Create some Mats to hold common images on each main loop pass.
	unsigned fnum = 0;
	Mat gray, prevGray, image, markerImage, markerGray, frame, result, warpTmp;
	vector<Mat> pyramid, prevPyramid;
	vector<KeyPoint> first_kp;
	cv::Mat first_desc;
	//cv::Mat first_desc32;
	HomographyInfo homoInfo = HomographyInfo();
	RNG rng(0xFFFFFFFF);

	//GET THE VIDEO INPUT READY
	VideoCapture cap;
	cap.open(0);

	cout << "Setup" << endl;
	if (!cap.isOpened()) {
		cout << "Could not initialize capturing...\n";
		return 0;
	}

	cap >> frame;

	if (frame.empty()) {
		cout << "Input image empty" << endl;
		return 0;
	}

	//Get the grayscale image for faster processing.
	cvtColor(frame, gray, COLOR_BGR2GRAY);

	//Create empty warp image for template generation.
	if (warpTmp.empty())
		warpTmp = cv::Mat::zeros(frame.size(), frame.type());


	///// 2 - PREPARE MARKER /////
	//Load a marker image from somewhere.
	cv::Mat marker = cv::imread("images\\Alterra.jpg");
	cvtColor(marker, markerGray, COLOR_BGR2GRAY);
	if (marker.empty()) {
		cout << "Marker image empty" << endl;
		return 0;
	}

	//CREATE BOUNDING BOX FOR OUTPUT USING HOMOGRAPHY
	vector<Point2f> bb;
	bb.push_back(cv::Point2f(0, 0));
	bb.push_back(cv::Point2f(marker.cols, 0));
	bb.push_back(cv::Point2f(marker.cols, marker.rows));
	bb.push_back(cv::Point2f(0, marker.rows));

	//CREATE RECOGNIZER USING AKAZE FEATURES
	Ptr<AKAZE> akaze = AKAZE::create();
	akaze->setThreshold(akaze_thresh);

	//GET DESCRIPTOR FOR BEST FEATURES TO TRACK IN THE MARKER IMAGE
	akaze->detectAndCompute(marker, noArray(), first_kp, first_desc);
	//desc.convertTo(first_desc32, CV_32S);


	//Use FLANN Matcher for better performance. AKAZE uses 8 bit for feature type, FLANN expects floating point - Conversion needed.
	Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("BruteForce-Hamming");
	//Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("FlannBased");
	//TODO: Use BOW TRAINER TO IMPROVE IMAGE RECOGNITION AND INCREASE VOLUME OF IMAGES

	//LOAD MARKER INTO RECOGNIZER

	//GET BEST FEATURES TO TRACK FROM THE MARKER IMAGE
	vector<Point2f> trackablePoints;
	vector<Point2f> trackablePointsTmp;
	TermCriteria termcrit(TermCriteria::COUNT | TermCriteria::EPS, 20, 0.03);
	goodFeaturesToTrack(markerGray, trackablePointsTmp, MAX_COUNT, 0.1, 10, Mat(), 3, false, 0.04);
	cornerSubPix(markerGray, trackablePointsTmp, subPixWinSize, Size(-1, -1), termcrit);

	//Calculate distribution of points across the image and only 
	//use one single point from each bin in a numberOfBins x numberOfBins grid across the image
	int numberOfBins = 20;
	std::map<int, vector<cv::Point2f> > trackingPointBin;
	int totalXBins = markerGray.cols / numberOfBins;
	int totalYBins = markerGray.rows / numberOfBins;
	for (int i = 0; i<(numberOfBins * numberOfBins); i++) {
		trackingPointBin.insert(pair<int, vector<cv::Point2f> >(i, vector<cv::Point2f>()));
	}

	//Iterate the points and add points to each bin
	for (int i = 0; i<trackablePointsTmp.size(); i++) {
		int bx = (int)trackablePointsTmp[i].x / totalXBins;
		int by = (int)trackablePointsTmp[i].y / totalYBins;
		int index = bx + (by * totalXBins);
		trackingPointBin[index].push_back(trackablePointsTmp[i]);
	}

	//Start the main loop
	cout << "Loop start" << endl;

	//Set initial values for main loop
	bool isDetected = false;
	bool isTracking = false;
	for (;;) {
		//Get new frame from the camera
		cap >> frame;
		if (frame.empty())
			break;

		++fnum;

		//Build image pyramid from frame with maxLevel as 3
		int optRes = cv::buildOpticalFlowPyramid(gray, pyramid, winSize, maxLevel);

		//CREATE TEMP IMAGES FOR DRAWING
		frame.copyTo(image);
		marker.copyTo(markerImage);

		//CREATE GRAYSCALE IMAGE FOR PROCESSING
		cvtColor(image, gray, COLOR_BGR2GRAY);

		//ARE WE ALREADY TRACKING SOMETHING?
		if (!isDetected) {
			//DETECT ALL AKAZE KEY POINTS AND FEATURES FROM THE NEW IMAGE FRAME
			vector<KeyPoint> kp;
			Mat desc;
			akaze->detectAndCompute(gray, noArray(), kp, desc);

			//MATCH THE DETECTIONS WITH OUR KNOWN DESCRIPTORS FROM THE MARKER
			//TODO: This should be BowTrainer or FLANN matcher.
			//cv::Mat desc32;
			//desc.convertTo(desc32, CV_32S);
			vector< vector<DMatch> > matches;
			matcher->knnMatch(first_desc, desc, matches, 2);
			//matcher->knnMatch(first_desc32, desc32, matches, 2);

			//Do ratio test to remove outliers from KnnMatch
			vector<KeyPoint> matched1, matched2;
			for (unsigned i = 0; i < matches.size(); i++) {
				//Ratio Test for outlier removal, removes ambiguous matches.
				if (matches[i][0].distance < nn_match_ratio * matches[i][1].distance) {
					matched1.push_back(first_kp[matches[i][0].queryIdx]);
					matched2.push_back(kp[matches[i][0].trainIdx]);
				}
			}

			//GET HOMOGRAPHY AND PERFORM HOMOGRAPHY TEST FOR CURRENT INLIERS
			homoInfo = GetHomographyInliers(toPoints(matched1), toPoints(matched2));

			if (homoInfo.inliers2.size() > 25) {

				//SAVE POINTS FOR NEXT ITERATION AND FINISH DRAWING RESULTS
				swap(homoInfo.inliers2, trackablePoints);

				//Object detected so we can now start tracking it!
				isDetected = true;

				//Purely for debug purposes it is possible to draw the bouning box of the detected features.
				vector<Point2f> new_bb;
				perspectiveTransform(bb, new_bb, homoInfo.homography);
				drawBoundingBox(image, new_bb);
			}
		} else if (isDetected) {
			if (!isTracking) {
				//trackablePoints.clear();

				for (auto const &track : trackingPointBin) {
					if (track.second.size() > 0) {
						//Get a random idex for this track and choose that point from the points bin
						int tIndex = rng.uniform(0, track.second.size());
						trackablePoints.push_back(track.second[tIndex]);
					}
				}
				isTracking = true;
			}

			//-- Get the corners from the image_1 ( the object to be "detected" )
			vector<Point2f> tmpTrackablePoints(trackablePoints.size());
			perspectiveTransform(trackablePoints, tmpTrackablePoints, homoInfo.homography);
			//swap(trackablePoints, tmpTrackablePoints);

			//GET THE OPTICAL FLOW FOR KNOWN POINTS IN PREVIOUS FRAME
			vector<Point2f> flowResultPoints, filteredResultPoints;
			vector<Point2f> filteredTmpTrackablePoints;
			vector<uchar> status;
			vector<float> err;

			calcOpticalFlowPyrLK(prevPyramid, pyramid, tmpTrackablePoints, flowResultPoints, status, err, winSize, 3, termcrit, 0, 0.001);
			cornerSubPix(gray, flowResultPoints, winSize, Size(-1, -1), termcrit);

			filteredResultPoints.resize(trackablePoints.size());
			//REMOVE ANY ITEMS WITH A BAD STATUS FROM OPTICAL FLOW
			size_t i, k;
			for (i = k = 0; i < status.size(); i++) {
				if (!status[i])
					continue;
				filteredResultPoints[k] = trackablePoints[i];
				tmpTrackablePoints[k] = tmpTrackablePoints[i];
				flowResultPoints[k] = flowResultPoints[i];
				k++;
			}
			filteredResultPoints.resize(k);
			tmpTrackablePoints.resize(k);
			flowResultPoints.resize(k);

			//GET HOMOGRAPHY AND PERFORM HOMOGRAPHY TEST FOR INLIERS
			homoInfo = GetHomographyInliers(filteredResultPoints, flowResultPoints);

			//How did the tracking go? How many points are left?
			//Enough to continue, or should we reset??
			if (homoInfo.inliers2.size() > 15 && (fnum % recalc_freq) != 0) {
				vector<Point2f> new_bb;
				perspectiveTransform(bb, new_bb, homoInfo.homography);
				drawBoundingBox(image, new_bb);
			} else {
				isTracking = false;
				isDetected = false;
			}

			if (isTracking) {
				//We are now tracking
				int markerTemplateWidth = 20;

				//Get a handle on the corresponding points from current image and the marker
				vector<cv::Point2f> currentTrackedPoints = homoInfo.inliers2;
				vector<cv::Point2f> markerTrackedPoints = homoInfo.inliers1;

				//Warp marker to warpTmp using the tracked homography of the image
				warpPerspective(markerGray, warpTmp, homoInfo.homography, warpTmp.size());

				//tmpTrackablePoints
				for (int i = 0; i<currentTrackedPoints.size(); i++) {
					auto pt = currentTrackedPoints[i];
					auto ptOrig = markerTrackedPoints[i];

					//Check that we can fit a whole template size around the feature point i.e. Not too close to the edge of the marker.
					cv::Rect templateRoi(pt.x - (markerTemplateWidth >> 1), pt.y - (markerTemplateWidth >> 1), markerTemplateWidth, markerTemplateWidth);
					bool is_inside = (templateRoi & cv::Rect(0, 0, warpTmp.cols, warpTmp.rows)) == templateRoi;

					if (is_inside) {
						cv::Mat templ = warpTmp(templateRoi);

						//Create ROIs for searching for the templates
						cv::Rect frameROI(0, 0, image.cols, image.rows);
						cv::Rect searchROI(pt.x - templateRadius, pt.y - templateRadius, templateRadius * 2, templateRadius * 2);

						//Get search area, so long as it is inside of the frame boundary
						searchROI = searchROI & frameROI;
						cv::Mat searchImage = gray(searchROI);

						//Create an empty result image - May be able to pre-initialize this container
						int result_cols = searchImage.cols - markerTemplateWidth + 1;
						int result_rows = searchImage.rows - markerTemplateWidth + 1;
						result.create(result_rows, result_cols, CV_32FC1);
						matchTemplate(searchImage, templ, result, match_method);

						//Normalize the result and find best location by match method
						normalize(result, result, 0, 1, NORM_MINMAX, -1, Mat());
						double minVal; double maxVal; Point minLoc; Point maxLoc;

						//Get the best match location for the template within the result image.
						Point matchLoc;
						minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());
						if (match_method == TM_SQDIFF || match_method == TM_SQDIFF_NORMED) {
							matchLoc = minLoc;
						} else {
							matchLoc = maxLoc;
						}

						//GEt the location of the rectangle with respect to the input frame image from the camera.
						cv::Rect templateResultRect = cv::Rect(matchLoc.x + (pt.x - templateRadius), matchLoc.y + (pt.y - templateRadius), markerTemplateWidth, markerTemplateWidth);

						//Draw some stuff. Search ROI and template result on the input frame copy.
						rectangle(image, searchROI, Scalar(125, 255, 255), 1, 8, 0);
						rectangle(image, templateResultRect, Scalar(255, 0, 0), 2, 8, 0);

						cv::Mat markerOut = marker.clone();
						//Draw all of the points that exist on the copy of the marker image
						for (int j = 0; j < trackablePointsTmp.size(); j++) {
							circle(markerOut, trackablePointsTmp[j], 4, Scalar(0, 0, 255), -1, 8);
						}
						//Draw all of the points selected for tracking on the copy of the marker image.
						for (int j = 0; j < trackablePoints.size(); j++) {
							circle(markerOut, trackablePoints[j], 4, Scalar(255, 0, 0), -1, 8);
						}
						//Draw all of the tracked inlier points on the marker
						for (int j = 0; j < markerTrackedPoints.size(); j++) {
							circle(markerOut, markerTrackedPoints[j], 4, Scalar(0, 255, 0), -1, 8);
						}


					}
				}
			}
		}

		cv::Mat Rvec;
		cv::Mat_<float> Tvec;
		cv::Mat raux, taux;

		//cv::solvePnP(patternPoints3d, points2d, calibration.getIntrinsic(), calibration.getDistorsion(),raux,taux);
		//raux.convertTo(Rvec,CV_32F);
		//taux.convertTo(Tvec ,CV_32F);

		//cv::Mat_<float> rotMat(3,3); 
		//cv::Rodrigues(Rvec, rotMat);

		// Copy to transformation matrix
		//for (int col=0; col<3; col++)
		//{
		//    for (int row=0; row<3; row++)
		//    {        
		//        pose3d.r().mat[row][col] = rotMat(row,col); // Copy rotation component
		//    }
		//    pose3d.t().data[col] = Tvec(col); // Copy translation component
		//}

		// Since solvePnP finds camera location, w.r.t to marker pose, to get marker pose w.r.t to the camera we invert it.
		//pose3d = pose3d.getInverted();


		cv::Mat outImg;
		cv::Mat res;

		//Make a buffer image as the marker must have the same height as the input image for hconcat.
		//vconcat(marker, cv::Mat::zeros(cv::Size(marker.cols, image.rows - marker.rows), CV_8UC3), res);
		vconcat(image, cv::Mat::zeros(cv::Size(image.cols, marker.rows - image.rows), CV_8UC3), res);

		hconcat(marker, res, outImg);

		if (isTracking) {
			//Draw all of the tracking features.
			if (homoInfo.inliers2.size() > 15) {
				for (int i = 0; i < homoInfo.inliers2.size(); i++) {
					cv::Point2f pt1 = cv::Point2f(homoInfo.inliers1[i].x, homoInfo.inliers1[i].y);
					cv::Point2f pt2 = cv::Point2f(homoInfo.inliers2[i].x + marker.cols, homoInfo.inliers2[i].y);

					circle(outImg, pt1, 2, Scalar(0, 125, 0), -1, 8);
					line(outImg, pt1, pt2, Scalar(0, 125, 0), 2, 8);
					circle(outImg, pt1, 3, Scalar(0, 125, 0), -1, 8);
				}
			}
		}

		imshow("Window", outImg);
		int c = waitKey(2);
		if (c == 27)
			break;

		//Update previous frame here.
		if (prevGray.empty())
			gray.copyTo(prevGray);
		cv::swap(prevGray, gray);
		cv::swap(pyramid, prevPyramid);

	}

	return 0;
}
