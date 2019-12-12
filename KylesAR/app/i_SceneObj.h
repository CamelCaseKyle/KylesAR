/*	MUST INCLUDE SCENE.H

*/

#pragma once

#include <unordered_map>
#include <vector>
#include <thread>

#include <opencv2\core.hpp>
#include <opencv2\calib3d.hpp>

#include <glm.hpp>
#include <gtc\matrix_inverse.hpp>

#include "..\ext\i_Tracker.h"	//PositionObject
#include "behavior.h"			//bindings, behavior
#include "..\render.h"			//content

//forward declare here, must include scene.h from KylesAR in files that include this
class Scene;

//the base container for scene behavior
struct SceneInfo {
	//pointer to scene parent?
	Scene *parent = nullptr;
	//the scene index for when an object gets added to scene
	int sID = -1,
		//the pose index for when an object binds to a pose
		pID = -1,
		//the desired pose ID to bind
		desiredPoseID = -1;
	//desired binding target
	BINDINGS binding = BIND_PURGATORY;
	//desired binding behavior
	BIND_BEHAVIOR behavior = WAIT;
	//pointer to actual parent, if exists
	PositionObject *pose = nullptr;
};

//inherit this class for scene functionality
class i_SceneObject {
public:
	//great for debugging
	std::string name = "";
	//render support
	Content cont;
	//scene support
	SceneInfo info;
	//visibility determination
	glm::vec4 boundingSphere = glm::vec4(0.0);
	//is initialized
	bool _scene = false;

	virtual ~i_SceneObject() = default;

	//default callback behavior
	virtual void sceneCallback(Scene *scene, Context *renderer) = 0;
	virtual void bindCallback(PositionObject *parent) = 0;
	//virtual void systemCallback(some args) = 0;
	virtual void removeCallback() = 0;

	//objects should implement something useful here
	virtual void update() = 0;
	virtual void render() = 0;
};
