#include "tracking.h"

/*called after origin has been found //moved to class decleration
void PositionObject::calculateWorldMatrix() {
	if (hasOrigin) { 
		//calculate world matrix
		world = view * origin;
		//world_cam = glm::inverse(world);
	} else if (isOrigin) {
		//view is world matrix
		world = view;
		world_cam = view_cam;
	}
}

//called when recieved a new origin pose
void PositionObject::originCallback(PositionObject *newOrigin) {
	//calculate quality
	float newAcc = newOrigin->quality + quality;
	//check quality first
	if (newAcc > originAccuracy && newAcc <= 2.f) {
		originAccuracy = newAcc;
		origin = glm::inverse(view) * newOrigin->view;
		hasOrigin = true;
#ifdef DBG_TRACKING
		printf("TrackingService: %s origin accuracy %f\n", name.c_str(), newAcc);
#endif
	}
}

//called when promoted to origin
void PositionObject::setAsOrigin() {
	origin = glm::mat4(1.f);
	originAccuracy = 0.;
	hasOrigin = false;
	isOrigin = true;
} */


//add new position tracker
int TrackingService::registerTracker(i_PositionTracker *newTracker) {
	//newTracker->poseBaseID = posTracks.size();
	posTracks.push_back(newTracker);
	return newTracker->poseBaseID;
}

//add new light tracker
int TrackingService::registerTracker(i_LightTracker *newTracker) {
	newTracker->lightBaseID = int(lightTracks.size());
	lightTracks.push_back(newTracker);
	return newTracker->lightBaseID;
}

//updates trackers
std::vector<PositionObject*> TrackingService::getAllPoses() {
	//clear data
	poses.clear();
	origin = nullptr;
	
	//for each tracker get all poses
	for (i_PositionTracker *trak : posTracks) {
		if (trak == nullptr) continue;
		std::vector<PositionObject *> tmp = trak->getPositions();
		poses.insert(poses.end(), tmp.begin(), tmp.end());
	}
	
	//if nothing to look at just gtfo
	if (poses.size() == 0) return poses;

	//if never had origin to begin with
	if (originID == -1) {
		float bestQual = 0.;
		int bestInd = -1;
		//find best pose for origin
		for (int j = 0; j < poses.size(); j++) {
			if (poses[j] == nullptr) continue;
			if (poses[j]->quality > bestQual && poses[j]->reliable) {
				bestQual = poses[j]->quality;
				bestInd = j;
			}
		}
		//if found
		if (bestInd >= 0) {
			origin = poses[bestInd];
			originID = origin->ID;
			origin->setAsOrigin();
#ifdef DBG_TRACKING
			printf("TrackingService: %s is new origin\n", origin->name.c_str());
#endif
		}
	} else {
		//search for the origin
		for (int i = 0; i < poses.size(); i++) {
			if (poses[i]->ID == originID) {
				origin = poses[i];
				//update all trackers' origin
				for (int i = 0; i < posTracks.size(); i++) posTracks[i]->originCallback(origin);
				//gtfo
				break;
			}
		}
	}
	return poses;
}

//updates lights
std::vector< LightObject *> TrackingService::getAllLights() {
	lights.clear();
	
	//for each tracker
	for (int i = 0; i < lightTracks.size(); i++) {
		//get the light objects
		std::vector< LightObject *> tmp = lightTracks[i]->getLights();
		//insert all found trackers into return buffer
		lights.insert(lights.end(), tmp.begin(), tmp.end());
	}

	return lights;
}

//fuses world pose
glm::mat4 TrackingService::getWorldPose() {
	//if not enough poses
	if (poses.size() <= 0) return glm::mat4(1.);
	else if (poses.size() == 1) {
		if (!poses[0]->hasOrigin && !poses[0]->isOrigin) return glm::mat4(1.);
		//just the one
		poses[0]->calculateWorldMatrix();
		return poses[0]->world;
	}
	//copy pose pointer list
	PositionObject *bestPose = nullptr, *curPose = nullptr;
	float bestQual = 0.;
	
	//*now calc all world coord poses and get the best one
	for (int j = 0; j < poses.size(); j++) {
		curPose = poses[j];
		if (curPose->hasOrigin || curPose->isOrigin) {
			curPose->calculateWorldMatrix();
			if (curPose->quality > bestQual) {
				bestPose = curPose;
				bestQual = curPose->quality;
			}
		}
	}
	if (bestPose != nullptr) return bestPose->world;
	return glm::mat4(1.f); //*/
	
	/*sort poses
	std::vector<PositionObject*> sortPose = poses;
	std::sort(sortPose.begin(), sortPose.end(), gt_poseObj_qual());
	//new output
	float totalQual = 0.f;
	glm::vec3 finalLoc = glm::vec3(sortPose[0]->world[3]);
	glm::mat3 finalRot = glm::mat3(sortPose[0]->world);
	glm::mat4 finalPose = glm::mat4(1.f);
	//weignted average
	for (int n = 1; n < sortPose.size(); n++) {
		//lerp each location inside pose
		finalLoc += glm::vec3(sortPose[n]->world[3]) * sortPose[n]->quality;
		totalQual += sortPose[n]->quality;
		//get the quality and rotation
		float qual = (sortPose[0]->quality - sortPose[n]->quality)*0.5 + 0.5;
		glm::mat3 tmpRot = glm::mat3(sortPose[n]->world);
		//slerp each ortho normal basis inside rotations based on quality
		for (int r = 0; r < 3; r++) finalRot[r] = slerp(tmpRot[r], finalRot[r], qual);
	}
	//divide accumulator
	finalLoc /= totalQual;
	//put it back together
	for (int i = 0; i < 3; i++) finalPose[i] = glm::vec4(finalRot[i], 0.f);
	finalPose[3] = glm::vec4(finalLoc, 1.f);
	return finalPose; //*/
}

//find the pose related to the given ID
glm::mat4 TrackingService::getPoseFromID(int id) {
	for (int i = 0; i < poses.size(); ++i)
		if (poses[i]->ID == id)
			return poses[i]->view;
	return glm::mat4(1.f);
}
