#pragma once

#include <glm.hpp>
#include <gtx/transform.hpp>
#include <gtx/euler_angles.hpp>

#include "../render.h"
#include "freespace.h"
#include "ibl.h"

class Reprojection {
public:
	Context* renderer;
	i_Camera* camera;
	FreespaceIMU* imu;
	Content reprojCont;
	Content reproj_pass2Cont;
	Shader reprojShade;
	Shader reprojToScreen;
	int fbo;

	Reprojection(Context* ctx, i_Camera* icam, FreespaceIMU* fimu);

	void render();
};