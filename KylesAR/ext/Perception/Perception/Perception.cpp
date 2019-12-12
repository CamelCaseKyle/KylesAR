#include "stdafx.h"
#include "perception.h"

void PerceptionTracker::getMatrix(float* p, float qual) {
	//update time
	thisTime = std::chrono::high_resolution_clock::now();
	float deltaTime = float(std::chrono::duration_cast<std::chrono::duration<double>>(thisTime - lastTime).count());
	//copy quality too
	finalQual = (qual > 0.f) ? qual : 0.f;
	//transform to align coord systems?
	glm::mat4 tmp = glm::mat4(1.f), lastPose = finalPose, lastD = dPose, fix = glm::mat4(-1.f, 0.f, 0.f, 0.f, 0.f, -1.f, 0.f, 0.f, 0.f, 0.f, -1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
	//copy the pxcF32 into a GLM mat
	std::copy(p, p + 12, glm::value_ptr(finalPose));
	finalPose[3][0] = 0.f;
	finalPose[3][1] = 0.f;
	finalPose[3][2] = 0.f;
	finalPose[3][3] = 1.f;
	//fix pose?
	finalPose = glm::transpose(fix * glm::inverse(finalPose));
	//convert from m to cm
	finalPose[3][0] *= pose.size;
	finalPose[3][1] *= pose.size;
	finalPose[3][2] *= pose.size;

	finalPose[0][3] = 0.f;
	finalPose[1][3] = 0.f;
	finalPose[2][3] = 0.f;
	finalPose[3][3] = 1.f;

	//get delta transform, divide by time
	dPose = glm::inverse(lastPose) * finalPose;
	ddPose = glm::inverse(lastD) * dPose;
}

PerceptionTracker::PerceptionTracker() : pose("Perception Tracker") {
	pose.reliable = true;
	pose.size = 100.f;
	pose.visible = false;
	pose.ID = 1000;
	poseBaseID = 1000;
	name = "Perception Tracker";
	open = false;
	lastTime = std::chrono::high_resolution_clock::now();

	//get realsense manager
	manager = PXCSenseManager::CreateInstance();
	//success?
	if (manager == nullptr) {
		printf("Unable to create the SenseManager\n");
		return;
	}

	#define chk(a) if (sts != pxcStatus::PXC_STATUS_NO_ERROR) printf("Cannot configure perception %s (%i)\n", a, sts)

	//enable scene perception
	pxcStatus sts = manager->EnableScenePerception();
	chk(":");

	//configure scene perception
	perception = manager->QueryScenePerception();
	if (perception == nullptr) {
		printf("Unable to create Perception\n");
		return;
	}

	sts = perception->SetVoxelResolution(PXCScenePerception::VoxelResolution::LOW_RESOLUTION);
	chk("voxel");
	sts = perception->EnableInertialSensorSupport(true);
	chk("inertial");
	sts = perception->EnableGravitySensorSupport(true);
	chk("gravity");

	//initial pose in front of camera
	float p[12] = {1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1., -.25};
	sts = perception->SetInitialPose(p);
	chk("pose");

	// Initialize
	sts = manager->Init((PXCSenseManager::Handler*)this);
	chk("init");
	sts = manager->StreamFrames(false);
	chk("stream");

	open = true;
}

PerceptionTracker::~PerceptionTracker() {
	manager->Release();
}

pxcStatus PXCAPI PerceptionTracker::OnModuleProcessedFrame(pxcUID mid, PXCBase *module, PXCCapture::Sample *sample) {
	// check if the callback is from the scene perception module.
	if (mid == PXCScenePerception::CUID) {
		PXCScenePerception *sp = module->QueryInstance<PXCScenePerception>();
		float p[12];
		sp->GetCameraPose(&p[0]);
		getMatrix(&p[0], sp->CheckSceneQuality(sample));
	}
	// return NO_ERROR to continue, or any error to abort.
	return PXC_STATUS_NO_ERROR;
}

std::vector<PositionObject*> PerceptionTracker::getPositions() {
	//output
	std::vector<PositionObject*> o;
	//if we have bad output
	if (finalQual < .0001) return o;
	
#ifdef LOG_POSE
	printf("Percep Pose:\n%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n",
			finalPose[0][0], finalPose[0][1], finalPose[0][2], finalPose[0][3],
			finalPose[1][0], finalPose[1][1], finalPose[1][2], finalPose[1][3],
			finalPose[2][0], finalPose[2][1], finalPose[2][2], finalPose[2][3],
			finalPose[3][0], finalPose[3][1], finalPose[3][2], finalPose[3][3]
		);
#endif

	//get last polling time
	//compare to last pose time
	//make sure real pose is returned first time its accessed
	//mul pose by slerp(dpose, mat4(1.), timeRatio)
	//mul dpose by slerp(ddpose, mat4(1.), timeRatio)

	//fill in pose object
	pose.quality = finalQual;
	pose.view = finalPose;
	pose.world = finalPose;

	o.push_back(&pose);

	return o;
}

void PerceptionTracker::originCallback(PositionObject* newOrigin) {
	pose.originCallback(newOrigin);
}

void PerceptionTracker::pauseTracking() {
	paused = true;
	manager->PauseScenePerception(paused);
}

void PerceptionTracker::resumeTracking() {
	paused = false;
	manager->PauseScenePerception(paused);
}
