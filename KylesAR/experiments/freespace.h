#pragma once

#include <freespace/freespace.h>
#include <freespace/freespace_util.h>
#include <freespace/freespace_printers.h>
#include <pthread.h>
#include <chrono>

#include <glm.hpp>
#include <gtx/quaternion.hpp>

typedef std::chrono::high_resolution_clock::time_point timePoint;

// State information for a thread
struct InputLoopState {
	pthread_t thread_;     // A handle to the thread
	pthread_mutex_t lock_; // A mutex to allow access to shared data
	int quit_;             // An input to the thread

	int enable_accel = 0,
		enable_accelNoGrav = 0,
		enable_angPos = 0,
		enable_angVel = 0,
		enable_compass = 0,
		enable_incline = 0,
		enable_mag = 0,
		enable_temp = 0;

	// The shared data updated by the thread
	struct freespace_MotionEngineOutput meOut_; // Motion data
	int updated_; // A flag to indicate that the motion data has been updated
};

class FreespaceIMU {
public:
	int enable_accel = 0,
		enable_accelNoGrav = 0,
		enable_angPos = 0,
		enable_angVel = 0,
		enable_compass = 0,
		enable_incline = 0,
		enable_mag = 0,
		enable_temp = 0;

	struct freespace_message message;
	FreespaceDeviceId device;
	timePoint sampleTime;
	glm::vec3 accel, accelNoGrav, angVel, incline, mag;
	glm::quat angPos;
	float compass, temp;
	struct InputLoopState inputLoop;
	struct freespace_MotionEngineOutput meOut;
	glm::vec3 eulerAngles;
	int rc;

	FreespaceIMU(int _enable_accel = 0, int _enable_accelNoGrav = 1, int _enable_angPos = 1, int _enable_angVel = 1, int _enable_compass = 0, int _enable_incline = 0, int _enable_mag = 0, int _enable_temp = 0);

	void sample();

	// getMotionFromInputThread
	int getMotionFromInputThread(struct InputLoopState * state,
		struct freespace_MotionEngineOutput * meOut);

	// getEulerAnglesFromMotion
	void updateParametersFromMotion(const struct freespace_MotionEngineOutput* meOut);

	// inputThreadFunction (must be specified here)
	static void* inputThreadFunction(void* arg) {
		struct InputLoopState* state = (struct InputLoopState*) arg;
		struct freespace_message message;
		FreespaceDeviceId device;
		int numIds;
		int rc;

		/** --- START EXAMPLE INITIALIZATION OF DEVICE -- **/
		// This example requires that the freespace device already be connected
		// to the system before launching the example.
		rc = freespace_getDeviceList(&device, 1, &numIds);
		if (numIds == 0) {
			printf("freespaceInputThread: Didn't find any devices.\n");
			exit(1);
		}

		rc = freespace_openDevice(device);
		if (rc != FREESPACE_SUCCESS) {
			printf("freespaceInputThread: Error opening device: %d\n", rc);
			exit(1);
		}

		rc = freespace_flush(device);
		if (rc != FREESPACE_SUCCESS) {
			printf("freespaceInputThread: Error flushing device: %d\n", rc);
			exit(1);
		}
		// Configure the device for motion outputs
		memset(&message, 0, sizeof(message)); // Make sure all the message fields are initialized to 0.
		message.messageType = FREESPACE_MESSAGE_DATAMODECONTROLV2REQUEST;
		message.dataModeControlV2Request.packetSelect = 8;  // MotionEngine Outout
		message.dataModeControlV2Request.mode = 0;          // Set full motion
		message.dataModeControlV2Request.formatSelect = 0;  // MEOut format 0
		message.dataModeControlV2Request.ff0 = 1; // Enable outputs
		message.dataModeControlV2Request.ff1 = state->enable_accel;
		message.dataModeControlV2Request.ff2 = state->enable_accelNoGrav;
		message.dataModeControlV2Request.ff3 = state->enable_angVel;
		message.dataModeControlV2Request.ff4 = state->enable_mag; // something
		message.dataModeControlV2Request.ff5 = state->enable_temp; // something
		message.dataModeControlV2Request.ff6 = state->enable_angPos; // something
		message.dataModeControlV2Request.ff7 = 0; // nothing??

		rc = freespace_sendMessage(device, &message);
		if (rc != FREESPACE_SUCCESS) {
			printf("freespaceInputThread: Could not send message: %d.\n", rc);
		}
		/** --- END EXAMPLE INITIALIZATION OF DEVICE -- **/

		// The input loop
		while (!state->quit_) {
			rc = freespace_readMessage(device, &message, 1000 /* 1 second timeout */);
			if (rc == FREESPACE_ERROR_TIMEOUT ||
				rc == FREESPACE_ERROR_INTERRUPTED) {
				continue;
			}
			if (rc != FREESPACE_SUCCESS) {
				printf("freespaceInputThread: Error reading: %d. Trying again after a second...\n", rc);
				_sleep(1000);
				continue;
			}

			// Check if this is a MEOut message.
			if (message.messageType == FREESPACE_MESSAGE_MOTIONENGINEOUTPUT) {
				pthread_mutex_lock(&state->lock_);

				// Update state fields.
				state->meOut_ = message.motionEngineOutput;
				state->updated_ = 1;

				pthread_mutex_unlock(&state->lock_);
			}
		}

		freespace_closeDevice(device);

		// Exit the thread.
		return 0;
	}

	~FreespaceIMU();
};