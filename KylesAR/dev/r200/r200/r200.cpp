#include "stdafx.h"
#include "r200.h"

pxcStatus PXCAPI R200::OnConnect(PXCCapture::Device* device, pxcBool connected) {
	return PXC_STATUS_DEVICE_FAILED;
}
pxcStatus PXCAPI R200::OnModuleProcessFrame(pxcUID mid, PXCBase* object) {
	return PXC_STATUS_NO_ERROR;
}
pxcStatus PXCAPI R200::OnNewSample(pxcUID mid, PXCCapture::Sample* sample) {
	PXCCapture::StreamType cStream;
	int cHeight, cWidth, cStride;
	uchar *cData;

	if (sample->depth) {
		cStream = PXCCapture::StreamType::STREAM_TYPE_DEPTH;
		cStride = depth.stride;
		cHeight = depth.height;
		cWidth = depth.width;
		cData = depth.frame.data;
		depth.time = hrClock::now();
	} else if (sample->color) {
		cStream = PXCCapture::StreamType::STREAM_TYPE_COLOR;
		cStride = rgb.stride;
		cHeight = rgb.height;
		cWidth = rgb.width;
		cData = rgb.frame.data;
		rgb.time = hrClock::now();
	} else {
		printf("unhandled event: %i\n", mid);
		return PXC_STATUS_FEATURE_UNSUPPORTED;
	}

	PXCImage *image = (*sample)[cStream];
	pxcStatus sts = image->AcquireAccess(PXCImage::ACCESS_READ, &datafmt);

	if (sts < PXC_STATUS_NO_ERROR) return sts;
	if (datafmt.planes[1] != NULL) return PXC_STATUS_FEATURE_UNSUPPORTED;
	if (datafmt.pitches[0] % cStride != 0) return PXC_STATUS_FEATURE_UNSUPPORTED;

	memcpy(cData, datafmt.planes[0], cHeight*cWidth*cStride*sizeof(pxcBYTE));
	image->ReleaseAccess(&datafmt);

	return PXC_STATUS_NO_ERROR;
}

R200::R200(bool D16stream, bool IRstream, float frameRate) {
	isOpen = false;

	//stream config
	fRate = frameRate;

	//omfg intel why
	manager = PXCSenseManager::CreateInstance();

	//success?
	if (manager == nullptr) {
		printf("Unable to create the SenseManager\n");
		return;
	}

	rgb.width = 640;
	rgb.height = 480;
	rgb.type = CV_8UC4;
	rgb.stride = 4;
	rgb.exists = true;
	manager->EnableStream(PXCCapture::STREAM_TYPE_COLOR, rgb.width, rgb.height, fRate);
	rgb.frame.create(rgb.height, rgb.width, rgb.type);
	
	//optional depth stream
	if (D16stream) {
		depth.width = 480;
		depth.height = 360;
		depth.type = CV_16UC1; //in mm
		depth.stride = 2;
		depth.exists = true;
		manager->EnableStream(PXCCapture::STREAM_TYPE_DEPTH, depth.width, depth.height, fRate);
		depth.frame.create(depth.height, depth.width, depth.type);
	}
}
R200::~R200() {
	if (isOpen) {
		manager->Release();
		rgb.frame.release();
		if (depth.exists)
			depth.frame.release();
	}
}

bool R200::stream() {
	pxcStatus sts = manager->Init((PXCSenseManager::Handler*)this);
	//moar errors
	if (sts != PXC_STATUS_NO_ERROR) {
		printf("Failed to locate specified streams. Error #%i\n", sts);
		manager->Release();
		return false;
	}

	sts = manager->StreamFrames(false);
	//ALL the errors
	if (sts != PXC_STATUS_NO_ERROR) {
		printf("Failed to stream frames\n");
		manager->Release();
		return false;
	}

	//these are for the R200
	camMat = cv::Matx33d(6.1959211982317981e+002, 0., 3.1936803881953682e+002, 0., 6.1956601022127211e+002, 2.3863698322291032e+002, 0., 0., 1.);
	camDist = { 2.9248098353506807e-001, -1.3922288490391708e+000, 1.3629934471475001e-003, -1.2816975768909633e-005, 1.9292929353192863e+000 };
	//theta_x = tan_inv(3.1936803881953682e+002 / 6.1959211982317981e+002) * 2.0;
	rgb.fovV = float(glm::atan(2.3863698322291032e+002 / 6.1956601022127211e+002)) * 2.f * RAD2DEG;
	rgb.fovH = rgb.fovV * (float(rgb.width) / float(rgb.height));
	//assume they both have the same FOV just different sensor resolutions
	depth.fovV = rgb.fovV * (float(rgb.height) / float(depth.height));
	depth.fovH = rgb.fovH * (float(rgb.width) / float(depth.width));

	isOpen = true;
	return true;
}

bool R200::start() { return true; }
bool R200::pause() { return true; }
