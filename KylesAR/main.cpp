/*
IN PROGRESS:
	-sparse reconstruction 6dof+rgb
	-modules loading into scene, factory create buttons in handheld
	-handheld laser
	-scene mesh / plane detection
	-SLAM
	-parallax
	-perception tracker pose?
		-perception tracker not going to origin
	-move shader init inside classes
	-update light tracking (markerSize, sphereSize)
	-scene crashes when sees handheld last..?

-we need an event system damnit
	[async] device independent input events
	[async 0] new_frame - post process streams like transform depth
	[async 1] tracking  - services (sorted by update priority)
	[async 2] meshing   - services (sorted by update priority)
	[sync 0] update        - apps (sorted by update priority)
	[sync 1] before_render - AR background and occlusion stuff (sorted by render priority)
	[sync 2] render_scene  - apps (sorted by render priority)
	[sync 3] render_gui    - apps (sorted by render priority)
	[sync 4] after_render  - post process dewarp/timewarp

-Calibration step: put on glasses, reach forward max, put hand on planar surface
	-measure hand attributes
	-measure reach for personal,
	-calibrate depth camera and depth plus
-if has IMU
	-tracking services return poses predicted in the future
	-timewarp is difference between prediction and reality at render time
-Add Glasses object that stores view transform, distortion, etc
-think about augmented objects casting shadows on real world
-paint program where pallete is handheld and your finger is the brush on any surface
-spend a million years on memory management / cache usage
-use opencv LUT(), it's very fast
-investigate PBO for many things
-expand kibblesmath / exrect to cubes for UI manager
-add 3D model format support (obj, gltf?)
-epic rock band like game with 3D content (buttons in active area)
	-rays of volumetric eye candy tell you when to hit button
-optical/depth flow gradient space tracks changes in 3D motion (box of smoke)
-2D gui or 3D world element that is like an ink pad, charges your finger/pencil/tool with a use-n-times function
	-subtract environment to get picture of thing to track

services: ( incomplete:_ broken/dated:- )
	 pose
	_mesh
	-light
	 input

devices:
	 generic camera
	_stereo rig
	 r200
	 d435
	
reality c137 (scene)
	augmented (not an actual object)
	|	apps (scene.appModules)
	|	regions ()
	|	surfaces ()
	|	trackers (scene.poseObjs)
	|	|	landmark poses (static)
	|	|	input poses (dynamic)
	|	|	user pose
	real (not an actual object)
	|	mesh (scene.mesh)
	|	environment (weather, time, etc)
	|	landmark (GPS coords, nearby stuff)
	|	computer ()
	|	|	info (memory and compute capabilities)
	|	|	hardware (not an actual object)
	|	|	|	network ()
	|	|	|	display ()
	|	|	|	IO devices (camera, microphone, lidar)

*/

#include "scene.h"
#include "render.h"

//#define SCAN
#ifdef SCAN
#include "experiments\recon.h"
#endif

#define TRACK
#ifdef TRACK
#include "experiments\knft.h"
#define FeatureCloudFile1 "models\\room.ftcloud"
#endif

//#define REPROJ
#ifdef REPROJ
#include "experiments\reprojection.h"
#endif

//#define COSLAM
#ifdef COSLAM
#include "experiments\CoSLAM.h"
#endif

//#define KITTI_SLAM
#ifdef KITTI_SLAM
#include "experiments\SLAM.h"
#endif


//last pressed keys
char keys[255];
int main(int argc, char *argv[]) {

	printf("Kyles AR v3.18.01\n");

	//Simple CNN trained on the iris dataset
	//DataSet iris;
	//vector<string> fileNames = { "dataset\\iris_training.dat", "dataset\\iris_validation.dat", "dataset\\iris_test.dat" };
	//iris.start(fileNames);

#ifdef COSLAM
	CoSLAM coslam = CoSLAM();
	coslam.startServer();
	cv::waitKey(0);
	return 0;
#endif

	vector<ModuleConfigs> mods;
	loadStartupCFG(mods);

	Scene scene = Scene();

	printf("Loading:\n... Camera\n");
	vector<Module<i_Camera*>*> cams;
	loadModules<i_Camera*>(mods[D_CAM], cams);
	i_Camera *icam = scene.setCamera(&cams);
	// can't continue without camera?
	if (icam == nullptr) {
		Sleep(10000);
		return 4;
	}

#ifdef SCAN
		// my sparse 3D reconstruction
		Recon recon(icam);
#endif
#ifdef TRACK
		// my sparse feature NFT
		KylesNFT knft(icam);
		printf("load ft cloud\n");
	#ifdef FeatureCloudFile1
		knft.loadFtCloud(FeatureCloudFile1);
	#endif
	#ifdef FeatureCloudFile2
		knft.loadFtCloud(FeatureCloudFile2);
	#endif
	#ifdef FeatureCloudFile3
		knft.loadFtCloud(FeatureCloudFile3);
	#endif
		printf("done\n");
#endif
#ifdef KITTI_SLAM
		SLAM kitti = SLAM(icam);
#endif

	printf("... Renderer\n");
	Context renderer = Context(icam, "Kyle's AR Demo");
	//probably the last to become a DLL
	if (renderer.name == "") {
		Sleep(10000);
		return 3;
	}
	renderer.mode = 3;
	renderer.ClearColor(glm::vec4(0., 0., 0., 0.));
	scene.setRenderer(&renderer);

	//my model renderer
	//renderScene(&renderer, icam);
	//cv::waitKey(0);
	//return 0;

#ifdef REPROJ
	FreespaceIMU fimu = FreespaceIMU();
	Reprojection reproj = Reprojection(&renderer, icam, &fimu);
#endif

	printf("... Services\n");
	//set up services
	vector<Module<i_PositionTracker*>*> srvs;
	loadModules<i_PositionTracker*>(mods[D_TRACK], srvs);
	scene.setServices(&srvs);

	printf("... Apps\n");
	//add apps to scene
	vector<Module<i_SceneObject*>*> apps;
	loadModules<i_SceneObject*>(mods[D_APP], apps);
	scene.setApps(&apps);

	printf("\nDone.\n\n");
	//ShowWindow(GetConsoleWindow(), SW_HIDE);
	
	//event loop
	bool running = true;
	int numPix = 0;
	timePoint lastRGB = icam->rgb.time,
			  lastDepth = icam->depth.time;
	while (running) {
		if (icam->rgb.time != lastRGB) {
			//send camera frame to OpenGL
			uploadTexture(icam->rgb.frame, renderer.textures[renderer.t_frame]);
			lastRGB = icam->rgb.time;
			//do RGB tracking
			scene.doTracking();

#ifdef SCAN
			//recon update
			if (scene.getTracking()->poses.size())
				recon.update(icam->rgb.frame, scene.getTracking()->poses[0]);
#endif
#ifdef TRACK
			//knft update
			knft.update(icam->rgb.frame);
#endif
		}
		//send depth if we have it
		if (icam->depth.exists) {
			if (icam->depth.time != lastDepth) {
				//fix the depth image and move to GPU
				uploadTexture(icam->depth.frame, renderer.textures[renderer.t_depth]);
				lastDepth = icam->depth.time;
				renderer.transformDepth();
			}
		}

#ifdef REPROJ
		timePoint lastTime = fimu.sampleTime;
		fimu.sample();
#endif
#ifdef KITTI_SLAM
		kitti.update();
#endif

		// update and do logic
		scene.update();
		// draw content
		scene.render();
		// put restults on screen
		renderer.display();

#ifdef REPROJ
		reproj.render();
#endif

		//swap buffers
		glfwSwapBuffers(renderer.window);

		//user input
		glfwPollEvents();
		if (glfwGetKey(renderer.window, GLFW_KEY_ESCAPE) == GLFW_PRESS || glfwWindowShouldClose(renderer.window) != 0) running = false;

		bool m_pressed = glfwGetKey(renderer.window, GLFW_KEY_M) == GLFW_PRESS;
		if (m_pressed && !keys['m']) {
			keys['m'] = true;
			renderer.mode += 1;
			if (renderer.mode > NUM_MODES)
				renderer.mode = 0;
		} else if (!m_pressed) keys['m'] = false;

		bool sp_pressed = glfwGetKey(renderer.window, GLFW_KEY_SPACE) == GLFW_PRESS;
		if (sp_pressed && !keys[' ']) {
			keys[' '] = true;
			cv::imwrite(to_string(++numPix) + ".jpg", icam->rgb.frame);
			printf("saved %i.jpg\n", numPix);
		} else if (!sp_pressed) keys[' '] = false;
	}
	
	//clean up!
	for (int i = 0; i < cams.size(); i++) delete cams[i];
	for (int i = 0; i < srvs.size(); i++) delete srvs[i];
	for (int i = 0; i < apps.size(); i++) delete apps[i];
	
	return 0;
}
