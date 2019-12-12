#pragma once

#ifdef D435_EXPORTS
#define D435_API extern "C" __declspec(dllexport)
#else
#define D435_API extern "C"
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <thread>

#include <librealsense2/rs.hpp>     // Include RealSense Cross Platform API

#include <string>
#include <map>
#include <algorithm>
#include <mutex>                    // std::mutex, std::lock_guard

#include <dev/i_Camera.h>

class D435 : public i_Camera {
public:
	//Contruct a pipeline which abstracts the device
	rs2::pipeline pipe;
	//Create a configuration for configuring the pipeline with a non default profile
	rs2::config cfg;
	//stream thread
	HANDLE thrd;
	//Are we good to start()?
	bool isOpen = false;

	//init and setup streams
	D435(bool D16stream = false, bool IRstream = false, float frameRate = 60.f);
	~D435();

	virtual bool start();
	virtual bool pause();
};

D435_API i_Camera* __stdcall allocate() {
	D435* cam = new D435(false);
	if (cam->start()) return (i_Camera*)cam;
	delete cam;
	return nullptr;
}

D435_API void __stdcall destroy(i_Camera* cam) {
	delete cam;
}
