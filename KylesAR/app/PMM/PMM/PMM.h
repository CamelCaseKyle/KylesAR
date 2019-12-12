/*

*/

#pragma once

#ifdef PMM_EXPORTS
#define PMM_API extern "C" __declspec(dllexport)
#else
#define PMM_API extern "C"
#endif

#include "stdafx.h"
#include <vector>

#include <opencv2\core.hpp>

#include <scene.h>
#include <app\i_SceneObj.h>

struct plane {
private:
	//calc plane points in screen space, perspective, used for perspective transform
	std::vector<cv::Point2f> _screenspace(std::vector<glm::vec3>& plnPts, cv::Size screensize);
	//to obj file
	void _obj(std::string& output, std::string& mtlfile, std::vector<glm::vec3>& plnp, int plnNum, int faceIndOffset);

public:
	glm::mat3 o; //orientation = mat3[tangent, normal, bitangent]
	glm::vec4 s; //size
	glm::vec3 l; //loc
	float m;	 //minimum size
	bool v;		 //visible

	//construct without orthonormal basis
	plane(glm::vec3 pl, glm::vec3 pn);
	//construct with orthonormal basis
	plane(glm::vec3 pl, glm::mat3 po);

	//returns signed distance
	float sd(glm::vec3 _l);
	//returns distance to plane from ro along rd
	float rs(glm::vec3 ro, glm::vec3 rd);
	//returns UV of point on plane
	glm::vec2 map(glm::vec3 _l);
	//returns 3D point from uv on plane
	glm::vec3 map(glm::vec2 uv);
	//updates most content
	void update(glm::vec3 pl, glm::mat3 po, bool vi);
	//calculate plane end points (camera space)
	std::vector<glm::vec3> plnPoints();

	//interfaces for functions
	std::vector<cv::Point2f> toScreenspace(std::vector<glm::vec3>& plnPts, cv::Size screensize);
	std::vector<cv::Point2f> toScreenspace(std::vector<glm::vec3>& plnPts);
	std::vector<cv::Point2f> toScreenspace();

	//interfaces for functions
	void toObj(std::string& output, std::string& mtlfile, std::vector<glm::vec3>& plnp, int plnNum, int faceIndOffset);
	void toObj(std::string& output, std::string& mtlfile, int plnNum, int faceIndOffset);
	void toObj(std::string& output, std::string& mtlfile);
};

class PMM : public i_SceneObject {
private:
	//camera for picture
	i_Camera* cam = nullptr;
	//scene for info, writing to world geometry
	Scene* scene = nullptr;

	//textures for each pose (progressive refine)
	std::vector<cv::Mat> plnImgs;
	//plane bound render
	cv::Mat debug;
	//list of planes
	std::vector<plane> plns;
	//maps marker ID to plane number
	std::unordered_map<int, int> knownIDs;
	//a generic object with a loc and orient for snapping (set to marker 31)
	plane object = plane(glm::vec3(0.), glm::mat3(0.));
	//colors to draw plane outlines
	std::vector<cv::Scalar> cols;
	//center of screen
	cv::Point2f midpt;
	//move the snapping thing to the scene geometry part
	float SNAP_DIST = 10.f, zFar = 100.f;
	
	//do the poor mans mapping
	void map();
	void texture();
	//how to integrate successive bits of info
	void traceResize2(plane& l, plane& r);
	void traceResize(plane& l, plane& r);

public:
	//do pretty much nothing
	PMM();
	//add to scene callback
	void sceneCallback(Scene* scene, Context* renderer);
	//bind to parent callback
	void bindCallback(PositionObject* parent);
	//remove from scene callback
	void removeCallback();

	//must implement for scene functionality
	void update();
	void render();
};

PMM_API i_SceneObject* __stdcall allocateStr(std::string args) {
	//use args to load scene bind behavior in SceneInfo
	PMM *pmm = new PMM();
	return (i_SceneObject*)pmm;
}

PMM_API void __stdcall destroy(i_SceneObject* inst) {
	delete inst;
}
