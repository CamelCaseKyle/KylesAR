//realistic image rendering for AI training

#pragma once

#define GLM_SWIZZLE

#include "..\render.h"

#include "models\Anim8orExport.h"
#include "models\cocacola.c"

namespace IBL {

	static Content iblCont;

	// psudo random
	glm::vec3 hash33(glm::vec3 p);
	// generate direction
	glm::vec3 rndDirection(glm::vec3 chaos);
	// generate orientation
	glm::mat3 rndOrientation(glm::vec3 chaos);
	// setup and render
	int renderScene(Context *ctx, i_Camera *icam);
}