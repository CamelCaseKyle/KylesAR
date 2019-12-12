/*
	deal with glsl loading shaders ect
*/

#pragma once

#ifdef WINAPP_EXPORTS
#define WINAPP_API extern "C" __declspec(dllexport)
#else
#define WINAPP_API extern "C"
#endif

#include "stdafx.h"

#include <opencv2\core.hpp>
#include <opencv2\highgui.hpp>

/*
#include <GL\glew.h>
#include <GLFW\glfw3.h>
*/

#include <glm.hpp>
#include <gtc\matrix_transform.hpp>

#include <dsp.h>
#include <scene.h>
#include <render.h>
#include <winderps.h>

#include <app\behavior.h>
#include <app\i_ocfb.hpp>
#include <app\i_SceneObj.h>

//front side where the window is
const float win_verts[12] = {
	-1.f, 0.f, 0.f,
	 1.f, 0.f, 0.f,
	 1.f, 2.f, 0.f,
	-1.f, 2.f, 0.f
};
const float win_norms[12] = {
	0.f, 0.f, 1.f,
	0.f, 0.f, 1.f,
	0.f, 0.f, 1.f,
	0.f, 0.f, 1.f
};
const float win_uvs[8] = {
	0.f, 0.f,
	1.f, 0.f,
	1.f, 1.f,
	0.f, 1.f
};
const unsigned int win_inds[6] = {
	0, 1, 2,
	0, 2, 3
};

const float win_back_verts[12] = {
	-1.f, 0.f, 0.f,
	-1.f, 2.f, 0.f,
	1.f, 2.f, 0.f,
	1.f, 0.f, 0.f
};
const float win_back_norms[12] = {
	0.f, 0.f, -1.f,
	0.f, 0.f, -1.f,
	0.f, 0.f, -1.f,
	0.f, 0.f, -1.f
};
const float win_back_uvs[8] = {
	0.f, 0.f,
	0.f, 1.f,
	1.f, 1.f,
	1.f, 0.f
};

class WinApp : public i_SceneObject, public i_OcclusionFeedback {
	//windows integration
	WindowsIntegration win;

	//pretty much augment occlusion shader
	Shader winShade;

	//time held down
	int hold = 0, delay = 10, timeout = 20;
	//if things have been loaded
	bool hwnd = false, _geometry = false, _textures = false, visible = false;

public:
	//content directory
	const enum texIDs { t_win, t_back };
	const enum vaoIDs { a_quad, a_back };
	const enum vboIDs { v_quad_verts, v_quad_norms, v_quad_uvs, v_quad_ind,
						v_back_quad_verts, v_back_quad_norms, v_back_quad_uvs, v_back_quad_ind };

	//if click-able
	bool enabled = true;

	//give scene name
	WinApp(std::string _exe);
	//scene graphical init
	WinApp(std::wstring _path, std::wstring _exe, BINDINGS _binding, BIND_BEHAVIOR _behavior, int pID = -1);

	//occlusion feedback
	void setup_occlusion(int oc_width, int oc_height);
	//setup gpu geometry
	void setup_geometry();
	//just the shader
	void setup_shader(bool hasDepth);

	//i_SceneObject (dont override default behavior)
	void sceneCallback(Scene* scene, Context* renderer);
	void bindCallback(PositionObject* parent);
	void removeCallback();

	//getters
	WindowsIntegration* getWin() { return &win; }
	int getDelay() { return delay; }
	int getTimeout() { return timeout; }
	bool hasHWND() { return hwnd; }
	bool hasGeometry() { return _geometry; }
	bool hasTextures() { return _textures; }
	bool isVisible() { return visible; }

	//setters
	void setDelay(int newDelay) { delay = newDelay; }
	void setTimeout(int newDelay) { delay = newDelay; }

	bool launch();

	//must implement for scene functionality
	void update();
	void render();
};

WINAPP_API i_SceneObject* __stdcall allocateStr(std::string args) {
	int ofs = int(args.find_first_of('|', 0));
	
	if (ofs > 0 && ofs < args.length()) {
		std::wstring path = toWstr(trim(args.substr(0, ofs))),
					 name = toWstr(trim(args.substr(ofs + 1, args.length() - ofs - 1)));
		WinApp* win = new WinApp(path, name, BIND_WORLD, WORLD_ORIGIN);
		bool success = win->launch();
		if (!success) {
			delete win;
			return nullptr;
		}
		win->setup_geometry();
		return (i_SceneObject*)win;
	}
	return nullptr;
}

//Exit point of DLL
WINAPP_API void __stdcall destroy(i_SceneObject* inst) {
	delete inst;
}
