#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Iphlpapi.h>
#include <vector>
#include <thread>
#include <zmq.h>

class ThreadData {
public:
	std::string socket = "";
};

void getdMacAddresses(std::vector<std::string> &vMacAddresses, bool forceLease = false);

DWORD WINAPI ThreadFunc(void *data);

int connect();
