#include "CoSLAM.h"

// handles the callback on behalf of a class
static void generic_cb(struct evhttp_request *request, void *params) {
	CoSLAM* state = (CoSLAM*)params;
	bool pose_success = false, img_success = false;

	evhttp_send_error(request, HTTP_OK, "kthxbye");

	// get the client IP
	char* client_ip;
	u_short client_port;
	evhttp_connection_get_peer(evhttp_request_get_connection(request), &client_ip, &client_port);

	// now only the last octet
	int ipLen = strlen(client_ip);
	char* ipSubstr = &client_ip[ipLen - 3];
	int lastOctet = stoi(ipSubstr);

	printf("message from %s:%i\n", client_ip, client_port);

	// get a copy of the input buffer
	evbuffer* inbuf = evhttp_request_get_input_buffer(request);
	size_t len = evbuffer_get_length(inbuf);
	char* data = new char[len];
	evbuffer_copyout(inbuf, data, len);

	char* pose;
	int poseInd = -1;
	// search for '/pose' headder
	for (int i = 0; i < len; i++) {
		if (data[i] == 'p' && data[i + 1] == 'o' && data[i + 2] == 's' && data[i + 3] == 'e') {
			poseInd = i + 9;
			break;
		}
	}
	// found it
	if (poseInd != -1) {
		pose = &data[poseInd];
		printf("\n\t-pose: '%s'", pose);
		pose_success = state->process_pose(pose, strlen(pose), lastOctet);

		// search for the beginning of the next resource's hash
		for (int i = poseInd; i < len; i++) {
			if (data[i] == '-' && data[i + 1] == '-') {
				data[i - 2] = 0;
				poseInd = i - 1;
				break;
			}
		}
	}

	char* img;
	int imgInd = -1;
	// search for 'frame.png' headder
	for (int i = poseInd; i < len; i++) {
		if (data[i] == 'f' && data[i + 1] == 'r' && data[i + 2] == 'a' && data[i + 3] == 'm' && data[i + 4] == 'e') {
			imgInd = i + 14;
			break;
		}
	}
	// found it
	if (imgInd != -1) {
		img = &data[imgInd];

		int imgEnd = -1;
		// search for the IEND png block
		for (int i = imgInd; i < len; i++) {
			if (data[i] == 'I' && data[i + 1] == 'E' && data[i + 2] == 'N' && data[i + 3] == 'D') {
				imgEnd = i + 8;
				break;
			}
		}
		// found it
		if (imgEnd != -1) {
			printf("\n\t-img: '%s' ... '%s'", &data[imgInd], &data[imgEnd - 8]);
			img_success = state->process_image(img, imgEnd - imgInd, lastOctet);
		}
	}

	if (pose_success && img_success) {
		// switch storage location
		state->pongDb[lastOctet] = 1 - state->pongDb[lastOctet];
	}

	printf("\n");

	delete[] data;
}


void CoSLAM::startServer() {
	// https://libevent-users.monkey.narkive.com/4hHpUsDZ/libevent-2-0-8-crashes-on-windows-without-wsastartup
	WSADATA WSAData;
	WSAStartup(0x101, &WSAData);

	auto ebase = crl::make_resource(event_base_new, event_base_free);
	auto server = crl::make_resource(evhttp_new, evhttp_free, ebase.get());

	// support POST messages
	evhttp_set_allowed_methods(server.get(), EVHTTP_REQ_POST); // EVHTTP_REQ_GET | EVHTTP_REQ_PUT

	// Set the general callback
	evhttp_set_gencb(server.get(), generic_cb, this);

	// Listen locally on port 7777
	if (evhttp_bind_socket(server.get(), "127.0.0.1", 7777) != 0) {
		printf("Couldn't bind to 127.0.0.1:7777\n");
	}

	printf("Bound to 127.0.0.1:7777\n");

	event_base_dispatch(ebase.get());
}

bool CoSLAM::process_image(const char* rawBytes, size_t len, int lastOctet) {
	// decode the image
	Mat m = cv::imdecode(cv::Mat(1, len, CV_8UC1, (void*)rawBytes), -1);
	cv::waitKey(1);
	// did not succeed
	if (m.rows == 0)
		return false;
	// ping pong the image
	imgDb[lastOctet][pongDb[lastOctet]] = m;
	return true;
}

bool CoSLAM::process_pose(const char* rawBytes, size_t len, int lastOctet) {
	std::vector<std::string> fields = split(std::string(rawBytes), " ");
	float x = stof(fields[0]), y = stof(fields[1]), z = stof(fields[2]),
		rx = stof(fields[3]), ry = stof(fields[4]), rz = stof(fields[5]), rw = stof(fields[6]);

	// check for all NaNs
	if (x != x || y != y || z != z || rx != rx || ry != ry || rz != rz || rw != rw)
		return false;

	// quaternion to matrix (thanks GLM)
	glm::mat3x3 m = glm::toMat3(glm::quat(rx, ry, rz, rw));
	// put rotation and translation together
	cv::Mat proj = cv::Mat(3, 4, CV_64F);
	// GLM is column major unlike OpenCV which is row major
	proj.at<double>(0, 0) = m[0][0]; proj.at<double>(0, 1) = m[1][0]; proj.at<double>(0, 2) = m[2][0]; proj.at<double>(0, 3) = x;
	proj.at<double>(1, 0) = m[0][1]; proj.at<double>(1, 1) = m[1][1]; proj.at<double>(1, 2) = m[2][1]; proj.at<double>(1, 3) = y;
	proj.at<double>(2, 0) = m[0][2]; proj.at<double>(2, 1) = m[1][2]; proj.at<double>(2, 2) = m[2][2]; proj.at<double>(2, 3) = z;

	// pingpong poses to LUT by last octet of IP
	projDb[lastOctet][pongDb[lastOctet]] = proj;

	return true;
}
