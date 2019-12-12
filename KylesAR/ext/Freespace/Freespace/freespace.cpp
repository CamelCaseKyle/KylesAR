#include "stdafx.h"
#include "freespace.h"

std::vector<PositionObject*> FreespaceIMU::getPositions() {
	std::vector<PositionObject*> ret;
	if (paused) return ret;

	// If new motion was available, use it
	if (getMotionFromInputThread(&inputLoop, &meOut)) {
		// update public variables
		updateParametersFromMotion(&meOut);
	}

	po.view_cam = glm::toMat4(rot);
	po.view_cam[0].w = pos.x;
	po.view_cam[1].w = pos.y;
	po.view_cam[2].w = pos.z;
	po.view_cam[3].w = 1.f;

	po.view = glm::transpose(glm::inverse(po.view_cam));
	
	po.time = hrClock::now();

	ret.push_back(&po);
	return ret;
}
void FreespaceIMU::pauseTracking() {
	paused = true;
}
void FreespaceIMU::resumeTracking() {
	paused = false;
}

FreespaceIMU::FreespaceIMU(int _enable_accel, int _enable_accelNoGrav, int _enable_angPos, int _enable_angVel, int _enable_compass, int _enable_incline, int _enable_mag, int _enable_temp) {

	enable_accel = _enable_accel;
	enable_accelNoGrav = _enable_accelNoGrav;
	enable_angPos = _enable_angPos;
	enable_angVel = _enable_angVel;
	enable_compass = _enable_compass;
	enable_incline = _enable_incline;
	enable_mag = _enable_mag;
	enable_temp = _enable_temp;

	// Initialize the freespace library
	rc = freespace_init();
	if (rc != FREESPACE_SUCCESS) {
		printf("Initialization error. rc=%d\n", rc);
		return;
	}

	// Setup the input loop thread
	memset(&inputLoop, 0, sizeof(InputLoopState));                      // Clear the state info for the thread
	inputLoop.enable_accel = _enable_accel;
	inputLoop.enable_accelNoGrav = _enable_accelNoGrav;
	inputLoop.enable_angPos = _enable_angPos;
	inputLoop.enable_angVel = _enable_angVel;
	inputLoop.enable_compass = _enable_compass;
	inputLoop.enable_incline = _enable_incline;
	inputLoop.enable_mag = _enable_mag;
	inputLoop.enable_temp = _enable_temp;
	pthread_mutex_init(&inputLoop.lock_, NULL);                                // Initialize the mutex
	pthread_create(&inputLoop.thread_, NULL, inputThreadFunction, &inputLoop); // Start the input thread

	po.quality = 1.f;
	po.distance = 0.f;
	po.ID = 9999;
	po.hasOrigin = false;
	po.isOrigin = false;
	po.reliable = false;
	po.visible = true;
}

FreespaceIMU::~FreespaceIMU() {

	// Cleanup the input loop thread
	inputLoop.quit_ = 1;                     // Signal the thread to stop
	pthread_join(inputLoop.thread_, NULL);   // Wait until it does
	pthread_mutex_destroy(&inputLoop.lock_); // Get rid of the mutex

	// Finish using the library gracefully
	freespace_exit();
}

// getMotionFromInputThread
int FreespaceIMU::getMotionFromInputThread(InputLoopState * state, freespace_MotionEngineOutput * meOut) {
	int updated;

	pthread_mutex_lock(&state->lock_);   // Obtain ownership of the input loop thread's shared state information
	*meOut = state->meOut_;              // Copy the motion packet to the main thread
	updated = state->updated_;           // Remember the updated_ flag
	state->updated_ = 0;                 // Mark the data as read
	pthread_mutex_unlock(&state->lock_); // Release ownership of the input loop thread's shared state information

	return updated;
}

// getEulerAnglesFromMotion
void FreespaceIMU::updateParametersFromMotion(const freespace_MotionEngineOutput* meOut) {
	// will be written to
	MultiAxisSensor sensor;
	// Get the data from the MotionEngineOut packet
	if (enable_accel) {
		freespace_util_getAcceleration(meOut, &sensor);
		accel = glm::vec3(sensor.x, sensor.y, sensor.z);
	}
	if (enable_accelNoGrav) {
		freespace_util_getAccNoGravity(meOut, &sensor);
		accelNoGrav = glm::vec3(sensor.x, sensor.y, sensor.z);
	}
	if (enable_angPos) {
		freespace_util_getAngPos(meOut, &sensor);
		angPos = glm::conjugate(glm::quat(sensor.x, sensor.y, sensor.z, sensor.w));
	}
	if (enable_angVel) {
		freespace_util_getAngularVelocity(meOut, &sensor);
		angVel = glm::vec3(sensor.x, sensor.y, sensor.z);
	}
	if (enable_compass) {
		freespace_util_getCompassHeading(meOut, &sensor);
		compass = sensor.x;
	}
	if (enable_incline) {
		freespace_util_getInclination(meOut, &sensor);
		incline = glm::vec3(sensor.x, sensor.y, sensor.z);
	}
	if (enable_mag) {
		freespace_util_getMagnetometer(meOut, &sensor);
		mag = glm::vec3(sensor.x, sensor.y, sensor.z);
	}
	if (enable_temp) {
		freespace_util_getTemperature(meOut, &sensor);
		temp = sensor.w;
	}
	pos = pos + accelNoGrav;
	rot = rot * glm::quat(angVel);
}
