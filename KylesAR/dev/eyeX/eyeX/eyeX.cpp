#include "gaze.h"

/*
* Initializes g_hGlobalInteractorSnapshot with an interactor that has the Gaze Point behavior.
*/
bool EyeTracker::InitializeGlobalInteractorSnapshot(TX_CONTEXTHANDLE hContext) {
	bool success;
	TX_HANDLE hInteractor = TX_EMPTY_HANDLE;
	//gaze
	TX_GAZEPOINTDATAPARAMS params = { TX_GAZEPOINTDATAMODE_LIGHTLYFILTERED };
	//location
	TX_HANDLE hBehaviorWithoutParameters = TX_EMPTY_HANDLE;

	//gaze
	success = txCreateGlobalInteractorSnapshot(
		hContext,
		InteractorId,
		&g_hGlobalInteractorSnapshot,
		&hInteractor) == TX_RESULT_OK;
	success &= txCreateGazePointDataBehavior(hInteractor, &params) == TX_RESULT_OK;
	//location
	//success = txCreateGlobalInteractorSnapshot(
	//	hContext,
	//	InteractorId,
	//	&g_hGlobalInteractorSnapshot,
	//	&hInteractor) == TX_RESULT_OK;
	success &= txCreateInteractorBehavior(hInteractor, &hBehaviorWithoutParameters, TX_BEHAVIORTYPE_EYEPOSITIONDATA) == TX_RESULT_OK;

	txReleaseObject(&hInteractor);

	return success;
}

/*
* Callback function invoked when a snapshot has been committed.
*/
void TX_CALLCONVENTION EyeTracker::OnSnapshotCommitted(TX_CONSTHANDLE hAsyncData, TX_USERPARAM param)
{
	// check the result code using an assertion.
	// this will catch validation errors and runtime errors in debug builds. in release builds it won't do anything.

	TX_RESULT result = TX_RESULT_UNKNOWN;
	txGetAsyncDataResultCode(hAsyncData, &result);
	_ASSERT(result == TX_RESULT_OK || result == TX_RESULT_CANCELLED);
}

/*
* Callback function invoked when the status of the connection to the EyeX Engine has changed.
*/
void TX_CALLCONVENTION EyeTracker::OnEngineConnectionStateChanged(TX_CONNECTIONSTATE connectionState, TX_USERPARAM userParam)
{
	switch (connectionState) {
	case TX_CONNECTIONSTATE_CONNECTED: {
		bool success;
		printf("The connection state is now CONNECTED (We are connected to the EyeX Engine)\n");
		// commit the snapshot with the global interactor as soon as the connection to the engine is established.
		// (it cannot be done earlier because committing means "send to the engine".)
		success = txCommitSnapshotAsync(g_hGlobalInteractorSnapshot, this->OnSnapshotCommitted, NULL) == TX_RESULT_OK;
		if (!success) {
			printf("Failed to initialize the data stream.\n");
		}
		else {
			printf("Waiting for gaze data to start streaming...\n");
		}
	}
									   break;

	case TX_CONNECTIONSTATE_DISCONNECTED:
		printf("The connection state is now DISCONNECTED (We are disconnected from the EyeX Engine)\n");
		break;

	case TX_CONNECTIONSTATE_TRYINGTOCONNECT:
		printf("The connection state is now TRYINGTOCONNECT (We are trying to connect to the EyeX Engine)\n");
		break;

	case TX_CONNECTIONSTATE_SERVERVERSIONTOOLOW:
		printf("The connection state is now SERVER_VERSION_TOO_LOW: this application requires a more recent version of the EyeX Engine to run.\n");
		break;

	case TX_CONNECTIONSTATE_SERVERVERSIONTOOHIGH:
		printf("The connection state is now SERVER_VERSION_TOO_HIGH: this application requires an older version of the EyeX Engine to run.\n");
		break;
	}
}

/*
* Handles an event from the Gaze Point data stream.
*/
void EyeTracker::OnGazeDataEvent(TX_HANDLE hGazeDataBehavior) {
	//Gaze data provides the X and Y coords in screen pixels.
	TX_GAZEPOINTDATAEVENTPARAMS eventParams;
	if (txGetGazePointDataEventParams(hGazeDataBehavior, &eventParams) == TX_RESULT_OK) {
		gazeX += float(eventParams.X);
		gazeY += float(eventParams.Y);
		gazeX *= 0.5f;
		gazeY *= 0.5f;
		//printf("got GAZE at %f, %f \n", gazeX, gazeY);
	}
	else {
		printf("Failed to interpret gaze data event packet.\n");
	}
}

/*
* Handles an event from the Eye Position data stream.
*/
void EyeTracker::OnEyePositionDataEvent(TX_HANDLE hEyePositionDataBehavior) {
	//The 3D position consists of X,Y,Z coordinates expressed in millimeters
	//in relation to the center of the screen where the eye tracker is mounted.

	//The normalized coordinates are expressed in relation to the track box
	//i.e. the volume in which the eye tracker is theoretically able to track eyes.
	// - (0,0,0) represents the upper, right corner closest to the eye tracker.
	// - (1,1,1) represents the lower, left corner furthest away from the eye tracker.

	TX_EYEPOSITIONDATAEVENTPARAMS eventParams;
	if (txGetEyePositionDataEventParams(hEyePositionDataBehavior, &eventParams) == TX_RESULT_OK) {
		//if we have the left eye
		if (abs(eventParams.LeftEyeX) > 0.0001f, abs(eventParams.LeftEyeY) > 0.0001f, abs(eventParams.LeftEyeZ) > 0.0001f) {
			//and the right
			if (abs(eventParams.RightEyeX) > 0.0001f, abs(eventParams.RightEyeY) > 0.0001f, abs(eventParams.RightEyeZ) > 0.0001f) {
				//average them
				eyeX = (eventParams.LeftEyeX + eventParams.RightEyeX) * 0.5f;
				eyeY = (eventParams.LeftEyeY + eventParams.RightEyeY) * 0.5f;
				eyeZ = (eventParams.LeftEyeZ + eventParams.RightEyeZ) * 0.5f;
				//printf("got POS at %f, %f, %f", eyeX, eyeY, eyeZ);
			}
		}
	}
	else {
		printf("Failed to interpret eye position data event packet.\n");
	}
}

/*
* Callback function invoked when an event has been received from the EyeX Engine.
*/
void TX_CALLCONVENTION EyeTracker::HandleEvent(TX_CONSTHANDLE hAsyncData, TX_USERPARAM userParam)
{
	TX_HANDLE hEvent = TX_EMPTY_HANDLE;
	TX_HANDLE hBehavior = TX_EMPTY_HANDLE;
	TX_HANDLE gBehavior = TX_EMPTY_HANDLE;

	txGetAsyncDataContent(hAsyncData, &hEvent);

	// NOTE. Uncomment the following line of code to view the event object. The same function can be used with any interaction object.
	//OutputDebugStringA(txDebugObject(hEvent));

	if (txGetEventBehavior(hEvent, &gBehavior, TX_BEHAVIORTYPE_EYEPOSITIONDATA) == TX_RESULT_OK) {
		OnEyePositionDataEvent(gBehavior);
		txReleaseObject(&gBehavior);
	}
	if (txGetEventBehavior(hEvent, &hBehavior, TX_BEHAVIORTYPE_GAZEPOINTDATA) == TX_RESULT_OK) {
		OnGazeDataEvent(hBehavior);
		txReleaseObject(&hBehavior);
	}


	// NOTE since this is a very simple application with a single interactor and a single data stream, 
	// our event handling code can be very simple too. A more complex application would typically have to 
	// check for multiple behaviors and route events based on interactor IDs.

	txReleaseObject(&hEvent);
}


EyeTracker::EyeTracker(std::string nom) : name(nom) {
	//Initialize EyeX
	TX_TICKET hConnectionStateChangedTicket = TX_INVALID_TICKET;
	TX_TICKET hEventHandlerTicket = TX_INVALID_TICKET;
	isOpen = false;

	// initialize and enable the context that is our link to the EyeX Engine.
	isOpen = txInitializeEyeX(TX_EYEXCOMPONENTOVERRIDEFLAG_NONE, NULL, NULL, NULL, NULL) == TX_RESULT_OK;
	isOpen &= txCreateContext(&hContext, TX_FALSE) == TX_RESULT_OK;
	isOpen &= InitializeGlobalInteractorSnapshot(hContext);
	isOpen &= txRegisterConnectionStateChangedHandler(hContext, &hConnectionStateChangedTicket, OnEngineConnectionStateChanged, NULL) == TX_RESULT_OK;
	isOpen &= txRegisterEventHandler(hContext, &hEventHandlerTicket, HandleEvent, NULL) == TX_RESULT_OK;
	isOpen &= txEnableConnection(hContext) == TX_RESULT_OK;

	//doesnt care if cam has depth, its already in the GPU
	focusShade = Shader("glsl/eyetracking/focus.vert", "glsl/eyetracking/focus.frag");
	blurShade = Shader("glsl/eyetracking/blur.vert", "glsl/eyetracking/blur.frag");

	// Get main shader handles
	focusShade.addUniform("LightPosition_worldspace");
	focusShade.addUniform("EyePosition_worldspace");
	focusShade.addUniform("MVP");
	focusShade.addUniform("V");
	focusShade.addUniform("M");
	focusShade.addUniform("myTextureSampler");
	//mainShade.addUniform("clipPrecomp");

	// Get blur shader handles
	blurShade.addUniform("inputImageTexture");
	blurShade.addUniform("focusImageTexture");
	blurShade.addUniform("texelWidthOffset");
	blurShade.addUniform("texelHeightOffset");

	//the fullscreen quad's vertex array
	GLuint quad_VertexArrayID;
	glGenVertexArrays(1, &quad_VertexArrayID);
	glBindVertexArray(quad_VertexArrayID);

	float quad_verts[] = {
		-1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
	};

	//quad buffer
	GLuint quad_vertexbuffer;
	glGenBuffers(1, &quad_vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_verts) * sizeof(float), quad_verts, GL_STATIC_DRAW);

}

EyeTracker::~EyeTracker() {
	// disable and delete the context.
	txDisableConnection(hContext);
	txReleaseObject(&g_hGlobalInteractorSnapshot);
	//these can return errors but whatever
	txShutdownContext(hContext, TX_CLEANUPTIMEOUT_DEFAULT, TX_FALSE) == TX_RESULT_OK;
	txReleaseContext(&hContext) == TX_RESULT_OK;
	txUninitializeEyeX() == TX_RESULT_OK;
}

std::vector<PositionObject*> EyeTracker::getPositions() {
	std::vector<PositionObject> output;
	PositionObject eyeScreenSpace = PositionObject(name);
	eyeScreenSpace.model[3] = glm::vec4(eyeX, eyeY, 0.0, 0.0);
	return &output;
}

/*
	////////////////////////////// pass 1: render RGB //////////////////////////////

	// Render to our framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0); // renderer.fbo); //replace FramebufferName with 0 for screen!
	glViewport(0, 0, int(renderer.viewport[2]), int(renderer.viewport[3]));

	// Clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Use our shader
	glUseProgram(mainShade.id);



	//convert eye location to world location
	mat3 gazeWorld = client2world(vec2(gazeX, gazeY), renderer.view * renderer.model, renderer.viewport, renderer.projection);

	//run a search for a cube
	vec4 res = FindPolyTRI(gazeWorld[0], gazeWorld[2], plane.meshes[0].vertices);
	int i = int(res.w);

	if (i >= 0) {
	float w = (1.f - res.y - res.z);
	//attempt to calculate world position from res(T, U, V, I)
	eyePos.x = plane.meshes[0].vertices[i].x * w + plane.meshes[0].vertices[i + 1].x * res.y + plane.meshes[0].vertices[i + 2].x * res.z;
	eyePos.y = plane.meshes[0].vertices[i].y * w + plane.meshes[0].vertices[i + 1].y * res.y + plane.meshes[0].vertices[i + 2].y * res.z;
	eyePos.z = plane.meshes[0].vertices[i].z * w + plane.meshes[0].vertices[i + 1].z * res.y + plane.meshes[0].vertices[i + 2].z * res.z;
	}
	else {
	//run a search for a plane
	res = FindPolyTRI(gazeWorld[0], gazeWorld[2], plane.meshes[0].vertices);
	//extract result
	i = int(res.w);

	//if found a plane
	if (i >= 0) {
	float w = (1.f - res.y - res.z);
	//attempt to calculate world position from res(T, U, V, I)
	eyePos.x = plane.meshes[0].vertices[i].x * w + plane.meshes[0].vertices[i + 1].x * res.y + plane.meshes[0].vertices[i + 2].x * res.z;
	eyePos.y = plane.meshes[0].vertices[i].y * w + plane.meshes[0].vertices[i + 1].y * res.y + plane.meshes[0].vertices[i + 2].y * res.z;
	eyePos.z = plane.meshes[0].vertices[i].z * w + plane.meshes[0].vertices[i + 1].z * res.y + plane.meshes[0].vertices[i + 2].z * res.z;
	}
	}

	//update camera
	//cam.focus = vec3(cam.loc.x * 0.8f, 0.f, 30.f + cam.loc.z);
	cam.focus = vec3(0.0, 0.0, 0.0);

	//update variables
	processKeys();

	cam.vel += cam.accel;
	cam.vel *= 0.95f; //friction :D
	//cam.loc += cam.vel;

	//post changes to matrix
	renderer.update(cam);

	//initialize shader matricies
	glUniformMatrix4fv(mainShade.uniforms["MVP"], 1, GL_FALSE, &renderer.MVP[0][0]);
	glUniformMatrix4fv(mainShade.uniforms["M"], 1, GL_FALSE, &renderer.model[0][0]);
	glUniformMatrix4fv(mainShade.uniforms["V"], 1, GL_FALSE, &renderer.view[0][0]);
	//custom clip coords
	//glUniformMatrix4fv(mainShade.uniforms["clipPrecomp"], 1, GL_FALSE, &clipPrecomp[0][0]);
	//light position
	glUniform3f(mainShade.uniforms["LightPosition_worldspace"], 0.f, 0.1f, -6.f);
	//eye pos
	glUniform3f(mainShade.uniforms["EyePosition_worldspace"], eyePos.x, eyePos.y, eyePos.z);


	/////////////// draw cubes ///////////////
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cubes.mats[0].texHandle);
	// Set our sampler to user Texture Unit 0
	glUniform1i(mainShade.uniforms["myTextureSampler"], 0);

	// 1st attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, cubes.meshes[0].vertexbuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// 2nd attribute buffer : uvs
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, cubes.meshes[0].uvbuffer);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// 3rd attribute buffer : normals
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, cubes.meshes[0].normbuffer);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// Draw the model!
	glDrawArrays(GL_TRIANGLES, 0, cubes.meshes[0].vertices.size()); // 12*3 indices starting at 0 -> 12 triangles



	/////////////// draw plane ///////////////
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, plane.mats[0].texHandle);
	// Set our sampler to user Texture Unit 0
	glUniform1i(mainShade.uniforms["myTextureSampler"], 0);

	// 1st attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, plane.meshes[0].vertexbuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// 2nd attribute buffer : uvs
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, plane.meshes[0].uvbuffer);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// 3rd attribute buffer : normals
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, plane.meshes[0].normbuffer);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);


	// Draw the model!
	glDrawArrays(GL_TRIANGLES, 0, plane.meshes[0].vertices.size()); // 12*3 indices starting at 0 -> 12 triangles


	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glBindTexture(GL_TEXTURE_2D, 0);


	////////////////////////////// pass two/three: blur //////////////////////////////

	// Render to the main buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, int(renderer.viewport[2]), int(renderer.viewport[3]));

	// Clear it
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//use the focus pass shader
	glUseProgram(blurShade.id);



	//bind texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, renderer.resID[0]);
	//bind  texture in Texture Unit 1
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, renderer.resID[1]);

	//set sampler to user Texture Unit 0
	glUniform1i(blurShade.uniforms["inputImageTexture"], 0);
	//set sampler to user Texture Unit 1
	glUniform1i(blurShade.uniforms["focusImageTexture"], 1);



	/////////////// blur horiz ///////////////
	glUniform1f(blurShade.uniforms["texelWidthOffset"], 2.f / 1920.f);
	glUniform1f(blurShade.uniforms["texelHeightOffset"], 0.f); //was 0

	// 1st attrib: verts
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	//draw on screen
	glDrawArrays(GL_TRIANGLES, 0, sizeof(quad_vertexbuffer) * sizeof(float));



	/////////////// blur vert ///////////////
	glUniform1f(blurShade.uniforms["texelWidthOffset"], 0.f);
	glUniform1f(blurShade.uniforms["texelHeightOffset"], 2.f / 1080.f);

	glDrawArrays(GL_TRIANGLES, 0, sizeof(quad_vertexbuffer) * sizeof(float));

	glDisableVertexAttribArray(0);
*/
