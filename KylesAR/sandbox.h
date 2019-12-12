/*

Raytrace sandbox needs catching up
	-renders over other things somehow
	-world matrix doesnt work

*/

#pragma once

//#include "scene.h"
#include "app\i_SceneObj.h"

//full screen raytraced content
class Sandbox : public i_SceneObject {
	//what to render with
	Shader rayShade;
	//this is derived from the pose
	glm::mat3 eyePose;
	glm::vec3 eyeLoc;
	//state info
	int rayMode;
	bool _geometry = false;

public:
	//content directory
	const enum vaoIDs { a_quad };
	const enum vboIDs { v_quad_verts };
	const enum fboIDs { f_ray };
	const enum rentexIDs { r_ray };

	//scene setup
	Sandbox(std::string nom, BINDINGS _binding, BIND_BEHAVIOR _behavior, int pID = -1);
	//graphic setup
	void setup(const Context &txt, bool hasDepth);
	
	//getters
	bool hasGeometry() { return _geometry; }
	
	//change shader mode
	void changeMode(int mode);

	//implement these for scene functionality
	void update();
	//batch process somehow by adding to render queue?
	void render();
};