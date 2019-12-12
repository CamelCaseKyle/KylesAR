#pragma once

#include <string>

// Include Tobii EyeX
#include <eyex\EyeX.h>
#include <GL\glew.h>

#include "tracker.h"
#include "shader.h"

// ID of the global interactor that provides our data stream; must be unique within the application.
static const TX_STRING InteractorId = "Realmax";
// global variables
static TX_HANDLE g_hGlobalInteractorSnapshot = TX_EMPTY_HANDLE;

//Tobiif EyeX gaze tracking
class EyeTracker : public PositionTracker {
private:
	float eyeX, eyeY, eyeZ, gazeX, gazeY;
	//talk to the engine
	bool InitializeGlobalInteractorSnapshot(TX_CONTEXTHANDLE hContext);
	//we got a new gaze point!
	void OnGazeDataEvent(TX_HANDLE hGazeDataBehavior);
	//we got a new head position!
	void OnEyePositionDataEvent(TX_HANDLE hEyePositionDataBehavior);
	//callbacks
	void TX_CALLCONVENTION OnSnapshotCommitted(TX_CONSTHANDLE hAsyncData, TX_USERPARAM param);
	void TX_CALLCONVENTION OnEngineConnectionStateChanged(TX_CONNECTIONSTATE connectionState, TX_USERPARAM userParam);
	void TX_CALLCONVENTION HandleEvent(TX_CONSTHANDLE hAsyncData, TX_USERPARAM userParam);

public:
	//used for identification
	std::string name;
	//shader passes
	Shader focusShade, blurShade;
	//context
	TX_CONTEXTHANDLE hContext = TX_EMPTY_HANDLE;
	//if you can use it
	bool isOpen;

	//the usual
	EyeTracker(std::string nom);
	~EyeTracker();

	//must implement for positiontracker
	std::vector<PositionObject*>* getPositions();
};
