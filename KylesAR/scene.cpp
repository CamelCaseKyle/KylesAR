/*

	visible objects get updated at max fps
	invisible objects get 1/4 max fps
	moar callbacks to reduce computation on binding

*/

#include "scene.h"

//initialize scene
Scene::Scene() {
	//one for each binding
	objs.resize(NUM_BINDINGS);
}
Scene::~Scene() {

}

//builtins
void Scene::setRenderer(Context *_renderer) {
	//render context to use
	renderer = _renderer;
	
	//set up 2D screen space (render projection)
	hud = glm::ortho(0.f, float(renderer->viewp[2]), 0.f, float(renderer->viewp[3]));
	//set up 3D personal space (render projection)

	//dont set up parent/world space (camera projection)

	_render = true;
}
void Scene::setApps(std::vector<Module<i_SceneObject*>*> *apps) {
	appModules = apps;
	// load up the modules
	for (int i = 0; i < apps->size(); i++) {
		Module<i_SceneObject*> *app = (*apps)[i];
		// if we should load it
		if (!app->shouldLoad()) continue;
		i_SceneObject *inst = app->getInst();
		// give it the camera and register it
		if (inst != nullptr) {
			addContent(inst);
		}
	}
	// build hash map for name -> module
}
i_Camera *Scene::setCamera(std::vector<Module<i_Camera*>*> *cams) {
	camModules = cams;
	cam = nullptr;
	_cam = false;
	// load up the modules
	for (int i = 0; i < cams->size(); i++) {
		Module<i_Camera*> *c = (*cams)[i];
		// if we should load it
		if (!c->shouldLoad()) continue;
		i_Camera *inst = c->getInst();
		// give it the camera and register it
		if (inst != nullptr && cam == nullptr) {
			cam = inst;
			_cam = true;
		}
	}
	// build hash map for name -> module
	return cam;
}
TrackingService *Scene::setServices(std::vector<Module<i_PositionTracker*>*> *srvs) {
	trackModules = srvs;
	_track = false;
	tracking = TrackingService();
	// load up the modules
	for (int i = 0; i < srvs->size(); i++) {
		Module<i_PositionTracker*> *srv = (*srvs)[i];
		// if we should load it
		if (!srv->shouldLoad()) continue;
		i_PositionTracker *inst = srv->getInst();
		// give it the camera and register it
		if (inst != nullptr) {
			inst->setCam(cam);
			tracking.registerTracker(inst);
			_track = true;
		}
	}
	// build hash map for name -> module
	return &tracking;
}

// gets any loaded module by name
template <typename T> Module<T> *Scene::getModuleByName(std::string nom) {
	// loop through all modules
	for (int i = 0; i < camModules->size(); i++) {
		Module<i_Camera*> *m = (*camModules)[i];
		if (m->getName() == nom) return m;
	}
	for (int i = 0; i < trackModules->size(); i++) {
		Module<i_PositionTracker*> *m = (*trackModules)[i];
		if (m->getName() == nom) return m;
	}
	for (int i = 0; i < appModules->size(); i++) {
		Module<i_SceneObject*> *m = (*appModules)[i];
		if (m->getName() == nom) return m;
	}
	return nullptr;
}

//helper functions
bool Scene::content_attach_obj(i_SceneObject *opt) {
	//if binding is incorrect
	if (opt->info.binding < 0 || opt->info.binding >= objs.size()) {
#ifdef DBG_SCENE
		printf("attach_obj | object '%s' has bad binding (%i)\n", opt->name.c_str(), opt->info.binding);
#endif
		return false;
	}
	//tell content its ID
	opt->info.sID = int(objs[opt->info.binding].size());
	//tell content about self
	opt->info.parent = this;
	//add to list
	objs[opt->info.binding].push_back(opt);
#ifdef LOG_OBJECTS
	printf("Added '%s' to scene\n", opt->name.c_str());
#endif
	return true;
}
bool Scene::content_detach_obj(i_SceneObject *opt) {
	if (opt == nullptr) return true;
	//if binding is bad
	if (opt->info.binding < 0 || opt->info.binding >= objs.size() ||
		//if not member of scene
		opt->info.sID < 0 || opt->info.sID >= objs[opt->info.binding].size()) {
#ifdef DBG_SCENE
		printf("detach_obj | object '%s' has bad binding (%i) or sceneID (%i)\n", opt->name.c_str(), opt->info.binding, opt->info.sID);
#endif
		return false;
	}
	//call remove callback
	opt->removeCallback();
	//if its not the last item in the list
	if (objs[opt->info.binding].size() > 1) {
		int swapTargetID = opt->info.sID;
		//remove from binding list by swapping target with back of stack
		std::swap(objs[opt->info.binding][swapTargetID], objs[opt->info.binding].back());
		//tell the swapped object its new ID
		objs[opt->info.binding][swapTargetID]->info.sID = swapTargetID;
	}
	//pop it off the stack
	objs[opt->info.binding].pop_back();
	//tell them it has been completed
	opt->info.sID = -1;
	//tell them they are a bastard
	opt->info.parent = nullptr;
#ifdef LOG_OBJECTS
	printf("Removed '%s' from scene\n", opt->name.c_str());
#endif
	return true;
}
bool Scene::content_attach_track(i_SceneObject *opt, PositionObject *pose) {
	//if binding is bad
	if (opt->info.binding < 0 || opt->info.binding >= objs.size() ||
		//if not member of scene
		opt->info.sID < 0 || opt->info.sID >= objs[opt->info.binding].size() ||
		//if has pose already
		opt->info.pID != -1) {
#ifdef DBG_SCENE
		printf("attach_track | object '%s' failed binding (%i) sceneID (%i) parentID (%i)\n", opt->name.c_str(), opt->info.binding, opt->info.sID, opt->info.pID);
#endif
		return false;
	}
	//call the callback
	opt->bindCallback(pose);
	//tell the thing it's index
	opt->info.pID = int(track2obj[pose->ID].size());
	//attach to the pose
	track2obj[pose->ID].push_back(opt);
	//tell teh object about its parent
	opt->info.pose = pose;
#ifdef LOG_BINDING
	printf("attach '%s' to '%s'\n", opt->name.c_str(), pose->name.c_str());
#endif
	return true;
}
bool Scene::content_detach_track(i_SceneObject *opt) {
	//if no pID or pose is null
	if (opt->info.pID == -1 || opt->info.pose == nullptr) {
#ifdef DBG_SCENE
		printf("detach_track | object '%s' has parent (%s) parentID (%i)\n", opt->name.c_str(), (opt->info.pose != nullptr) ? "true" : "false", opt->info.pID);
#endif
		return false;
	}
	//less typing!
	int poseID = opt->info.pose->ID;
	//if not last item left
	if (track2obj[poseID].size() > 1) {
		int targetID = opt->info.pID;
		//swap it with back
		std::swap(track2obj[poseID][targetID], track2obj[poseID].back());
		//tell the swapped object its new ID
		track2obj[poseID][targetID]->info.pID = targetID;
	}
	//pop it off the stack
	track2obj[poseID].pop_back();
	//tell them it has been completed
	opt->info.pID = -1;
	//tell teh button its now a bastard
	opt->info.pose = nullptr;
#ifdef LOG_BINDING
	printf("detach '%s' from parent\n", opt->name.c_str());
#endif
	return true;
}

//use for BIND_PERSONAL and BIND_LOCAL
bool Scene::addContent(i_SceneObject *opt) {
	//bind to scene
	if (content_attach_obj(opt)) {
		//call the scene callback
		opt->sceneCallback(this, renderer);
		//tell the scene it has orphans
		unboundObjs++;
		numObjs++;
		return true;
	}
	return false;
}
//replace with Content base class
bool Scene::removeContent(i_SceneObject *opt) {
	//if it has a tracker, remove its bindings too
	if (opt->info.binding == BIND_PARENT) content_detach_track(opt);
	//remove from scene object list
	if (content_detach_obj(opt)) {
		//tell scene one less mouth to feed
		numObjs--;
		//update unbound objects?

		return true;
	}
	return false;
}

//get all position objects
std::vector<PositionObject*> Scene::getPoses() {
	std::vector<PositionObject*> out;
	for (int i = 0; i < poseObjs.size(); i++) out.push_back(poseObjs[i]);
	return out;
}
//get all bound content
std::vector<i_SceneObject*> Scene::getObjects() {
	std::vector<i_SceneObject*> out;
	for (int i = 0; i < NUM_BINDINGS; i++) {
		for (int j = 0; j < objs[i].size(); j++)
			out.push_back(objs[i][j]);
	}
	return out;
}
//find position object by name
PositionObject *Scene::getPoseByName(std::string nom) {
	PositionObject *out = nullptr;
	for (int i = 0; i < poseObjs.size(); i++) {
		if (poseObjs[i]->name != nom) continue;
		out = poseObjs[i];
		break;
	}
	return out;
}
//find object by name
i_SceneObject *Scene::getObjectByName(std::string nom) {
	i_SceneObject *out = nullptr;
	for (int i = 0; i < NUM_BINDINGS; i++) {
		for (int j = 0; j < objs[i].size(); j++) {
			if (objs[i][j]->name != nom) continue;
			out = objs[i][j];
			break;
		}
	}
	return out;
}
//get all objects bound to a position object
std::vector<i_SceneObject*> Scene::getObjectsByParent(int parentID) {
	return track2obj[parentID];
}
//get all objects in a binding
std::vector<i_SceneObject*> Scene::getObjectsByBinding(BINDINGS target) {
	return objs[target];
}

//handles binding to the HUD
void Scene::bindHUD() {

}
//handles binding to the bubble
void Scene::bindPersonal() {

}
//handles binding to an object
void Scene::bindParent() {
	//try to bind unbound content
	for (int i = 0; i < objs[BIND_PARENT].size(); i++) {
		//gonna need these aliases lolol
		i_SceneObject *target = objs[BIND_PARENT][i];
		SceneInfo *opt = &(target->info);

		//if has a parent and doesnt jump around
		if (opt->pID != -1 && (opt->behavior == PARENT_FIRST || opt->behavior == PARENT_EMPTY || opt->behavior == PARENT_ID)) continue;

		if (opt->behavior == PARENT_CLOSE) {
			int bestInd = -1;
			float bestDist = 999999.9f;
			//find close tracker
			for (int j = 0; j < poseObjs.size(); j++) {
				if (poseObjs[j]->visible && poseObjs[j]->distance > 0.0001f && poseObjs[j]->distance < bestDist) {
					bestDist = poseObjs[j]->distance;
					bestInd = j;
				}
			}
			//if didnt find anything
			if (bestInd == -1) continue;
			//if object has pose
			if (opt->pID != -1 && opt->pose != nullptr) {
				//if found current parent
				if (poseObjs[bestInd]->ID == opt->pose->ID) continue;
				//has pose and needs new one
				content_detach_track(target);
			}
			//attach to found
			content_attach_track(target, poseObjs[bestInd]);
		}
		else if (opt->behavior == PARENT_QUALITY) {
			float bestQual = 0.f;
			int bestInd = -1;
			for (int j = 0; j < poseObjs.size(); j++) {
				if (poseObjs[j]->visible && poseObjs[j]->reliable && poseObjs[j]->quality > bestQual) {
					bestQual = poseObjs[j]->quality;
					bestInd = j;
				}
			}
			if (bestInd == -1) continue;
			if (opt->pID != -1 && opt->pose != nullptr) {
				if (poseObjs[bestInd]->ID == opt->pose->ID) continue;
				content_detach_track(target);
			}
			content_attach_track(target, poseObjs[bestInd]);
		}
		else if (opt->behavior == PARENT_ID) {
			//find defined tracker
			for (int j = 0; j < poseObjs.size(); j++) {
				//if not parent, continue
				if (poseObjs[j]->ID != opt->desiredPoseID) continue;
				//if has parent detach
				if (opt->pID != -1) content_detach_track(target);
				//attach
				content_attach_track(target, poseObjs[j]);
				//gtfo
				break;
			}
		}
		else if (opt->behavior == PARENT_FIRST) {
			//if nothing there
			if (poseObjs.size() == 0) continue;
			//attach to first thing there
			content_detach_track(target);
			content_attach_track(target, poseObjs[0]); 
		}
		else if (opt->behavior == PARENT_LAST) {
			//if nothing there or already bound
			if (poseObjs.size() == 0 || opt->pID == poseObjs.size() - 1) continue;
			//attach to last thing there
			content_detach_track(target);
			content_attach_track(target, poseObjs.back());
		}
		else if (opt->behavior == PARENT_EMPTY) {
			//find first empty tracker
			for (int j = 0; j < poseObjs.size(); j++) {
				//check if empty
				if (track2obj[poseObjs[j]->ID].size()) continue;
				//attach to first one found
				content_detach_track(target);
				content_attach_track(target, poseObjs[j]);
				//gtfo
				break;
			}			
		}
	}
}
//handles binding to the World
void Scene::bindWorld() {

}

//whenever a new input event happens
void Scene::doTracking() {
	//get current poses
	poseObjs = tracking.getAllPoses();
}
//update scene content, try to find unbound content a home
void Scene::update() {
	//no objects or no tracking
	if (numObjs == 0 || poseObjs.size() == 0) return;

	//update bindings?
	bindParent();

	//get a new pose prediction
	//tracking.predict();

	//update everything with a visible parent
	for (int i = 0; i < poseObjs.size(); i++) {
		//if visible
		if (!poseObjs[i]->visible) continue;
		//get list of objects attached to pose
		std::vector<i_SceneObject*> &oblist = track2obj[poseObjs[i]->ID];
		//loop through tracker objects
		for (int j = 0; j < oblist.size(); j++) {
			//tell the content its matrix
			oblist[j]->cont.view = poseObjs[i]->view;
			oblist[j]->cont.view_cam = poseObjs[i]->view_cam;
			//update object
			oblist[j]->update();
		}
	}
	//get fused world pose
	glm::mat4 worldPose = tracking.getWorldPose(),
			  invWorldPose = glm::inverse(worldPose);

	//use a position object and content_attach_track() instead of manipulating their shit

	//update everything in the world (update everything in view full speed, outside view @ 1/4 speed)
	for (int i = 0; i < objs[BIND_WORLD].size(); i++) {
		//tell the content its matrix
		objs[BIND_WORLD][i]->cont.view = worldPose;
		objs[BIND_WORLD][i]->cont.view_cam = invWorldPose;
		//some kind of distance or visibility test?
		objs[BIND_WORLD][i]->update();
	}

	//update everything in the HUD
	for (int i = 0; i < objs[BIND_HUD].size(); i++) {
		objs[BIND_HUD][i]->update();
	}

} 
//draw 3D scene content
void Scene::render() {

	//we are drawing on top of point cloud
	glBindFramebuffer(GL_FRAMEBUFFER, renderer->fbo[renderer->f_vertex]);

	//	BIND_PARENT		Use parent coords
	for (int i = 0; i < poseObjs.size(); i++) {
		//if visible
		if (!poseObjs[i]->visible) continue;
		//get list of objects attached to pose
		std::vector<i_SceneObject*> &oblist = track2obj[poseObjs[i]->ID];
		//loop through tracker objects
		for (int j = 0; j < oblist.size(); j++) {
			//render it
			oblist[j]->render();
		}
	}

	//	BIND_WORLD		Use world coords
	for (int i = 0; i < objs[BIND_WORLD].size(); i++) {
		//render it
		objs[BIND_WORLD][i]->render();
	}
	
	glDisable(GL_DEPTH_TEST);

	//	BIND_HUD		Use screen coords
	for (int i = 0; i < objs[BIND_HUD].size(); i++) {
		//render it
		objs[BIND_HUD][i]->render();
	}

	glEnable(GL_DEPTH_TEST);
}
