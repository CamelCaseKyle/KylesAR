#pragma once

#include <iostream>
#include <evhttp.h>
#include <memory>
#include <system_error>
#include <fstream>

#include <opencv2/opencv.hpp>
#include <glm.hpp>
#include <gtx/quaternion.hpp>

#include "../winderps.h"
// SLAM implementation
#include "recon.h"

using namespace cv;

// HTTP make_resource
namespace crl {
	template <typename Creator, typename Destructor, typename... Arguments> auto make_resource(Creator c, Destructor d, Arguments &&... args) {
		auto r = c(std::forward<Arguments>(args)...);
		if (!r) {
			throw std::system_error(errno, std::generic_category());
		}
		return std::unique_ptr<std::decay_t<decltype(*r)>, decltype(d)>(r, d);

		using server_ptr_t = std::unique_ptr<evhttp, decltype(&evhttp_free)>;
	}
} // namespace crl

// handles the callback on behalf of a class
static void generic_cb(struct evhttp_request *request, void *params);

class CoSLAM {
public:
	// go for SoA
	Mat imgDb[255][2];
	Mat projDb[255][2];
	float timeDb[255][2];
	int pongDb[255];
	Recon recon;

	// begin listening for POST packets
	void startServer();
	// extract the image from the POST message
	bool process_image(const char* rawBytes, size_t len, int lastOctet);
	// extract the pose from the POST message
	bool process_pose(const char* rawBytes, size_t len, int lastOctet);
};
