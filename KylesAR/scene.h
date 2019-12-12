/*	MUST INCLUDE APP\I_SCENEOBJECT.H

Scene is an absolute mess
	-crashes when handheld is last to be added to multimarker scene
	-more scene callbacks, cameras connected, apps launched
	-normal/geometry extraction
		-when has pose, captures transformed depth buffer
		-calculates quantisized normals
		-masked by rendered scene mesh
		-gets downloaded by the CPU
		-OpenCV thresholds colors calculated based on pose to find walls, tables, etc
		-extremedies are found using findContours
		-geometry is generated and added to scene mesh
		-objects can specify a 3D volume in the scene to utilize.
		-objects recieve information about real world geometry inside the volume
		-voids, planes, etc
	-keep track of unbound objects better
	-make configuration flow downstream (eg camera fov -> scene fov -> renderer fov)
	-personal bubble VR
		-world rendered like fog
		-real world within reach is visible, fades out after reach
	-check against natural features to tell if marker is moving?
	-implement world binding visibility test
	-if visible dot (origin - cam, object - cam) < (fov^2 - boundingSpereR^2)
	-get camera coords in world for smoothing
	-get input coords in world for smoothing/selecting

*/

#pragma once

#include <unordered_map>
 
#include <vector>
#include <thread>
#include <mutex>

#include <opencv2\core.hpp>
#include <opencv2\calib3d.hpp>

#include <glm.hpp>
#include <gtc\matrix_inverse.hpp>

#include "dev\i_Camera.h"
#include "app\behavior.h"
#include "app\i_SceneObj.h"
#include "dsp.h"
#include "tracking.h"
#include "module.hpp"
#include "render.h"

//failures
#define DBG_SCENE
//log object binding
#define LOG_BINDING
//log scene objects
#define LOG_OBJECTS

//forward declare here, must include i_SceneObj.h in files that include this
class i_SceneObject;
struct SceneInfo;

class Scene {
private:
	//tracking and active trackers
	TrackingService tracking;
	//different contexts for each scene
	Context *renderer;
	//current real camera
	i_Camera *cam;
	//modules
	std::vector<Module<i_PositionTracker*>*> *trackModules;
	std::vector<Module<i_SceneObject*>*> *appModules;
	std::vector<Module<i_Camera*>*> *camModules;

	//scene mesh
	std::vector<cv::Mat> texs;
	std::vector<glm::vec3> verts, norms;
	std::vector<glm::vec2> uvs;
	std::vector<unsigned int> inds;
	Content mesh;

	//all pose objects ever encountered sorted by tracker then chronologically
	std::vector<PositionObject*> poseObjs;
	//all objects [ BINDING ] [ ID ]
	std::vector<std::vector<i_SceneObject*>> objs;
	//relates pose object's ID to content in BIND_PARENT binding
	std::unordered_map<int, std::vector<i_SceneObject*>> track2obj;

	//HUD is ortho screen, here because it never changes
	glm::mat4 hud = glm::mat4(1.f);
	//just so i dont have to loop through all the bindings
	int numObjs = 0, unboundObjs = 0;
	//state info
	bool _track = false, _render = false, _cam = false;

	//These help handle things reliably
	bool content_attach_obj(i_SceneObject *opt);
	bool content_detach_obj(i_SceneObject *opt);
	bool content_attach_track(i_SceneObject *opt, PositionObject *pose);
	bool content_detach_track(i_SceneObject *opt);

	//handles bind behavior
	void bindHUD();
	void bindPersonal();
	void bindParent();
	void bindWorld();

public:

	Scene();
	~Scene();

	//special objects
	void setRenderer(Context *_renderer);
	void setApps(std::vector<Module<i_SceneObject*>*> *apps);
	i_Camera *setCamera(std::vector<Module<i_Camera*>*> *cams);
	TrackingService *setServices(std::vector<Module<i_PositionTracker*>*> *srvs);
	
	//persistant object
	bool addContent(i_SceneObject *opt);
	bool addSceneMesh(float *v, float *n, int *i);
	bool removeContent(i_SceneObject *opt);

	//getters
	bool hasTracking() { return _track; }
	bool hasRendering() { return _render; }
	bool hasCamera() { return _cam; }

	//gets the tracking service
	TrackingService *getTracking() { return &tracking; }
	//gets the renderer
	Context *getRenderer() { return renderer; }
	//gets the active webcam
	i_Camera *getCamera() { return cam; }
	//gets the extracted 3D real world geometry
	Content *getSceneMesh() { return &mesh; }

	template <typename T> Module<T> *getModuleByName(std::string nom);

	//get all position objects
	std::vector<PositionObject*> getPoses();
	//get position object by name
	PositionObject *getPoseByName(std::string nom);

	//get all bound content
	std::vector<i_SceneObject*> getObjects();
	//get scene object by name
	i_SceneObject *getObjectByName(std::string nom);
	//get all objects bound to a position object
	std::vector<i_SceneObject*> getObjectsByParent(int parentID);
	//get all objects in a binding
	std::vector<i_SceneObject*> getObjectsByBinding(BINDINGS target);

	//update tracking
	void doTracking();
	//update objects
	void update();
	//render content
	void render();
};
