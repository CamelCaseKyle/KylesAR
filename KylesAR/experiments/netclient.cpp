#include "netclient.h"

void getdMacAddresses(std::vector<std::string> &vMacAddresses, bool forceLease) {
	vMacAddresses.clear();
	IP_ADAPTER_INFO AdapterInfo[32];       // Allocate information for up to 32 NICs
	DWORD dwBufLen = sizeof(AdapterInfo);  // Save memory size of buffer
	// Call GetAdapterInfo [out] buffer to receive data [in] size of receive data buffer
	DWORD dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);

	//No network card? Other error?
	if (dwStatus != ERROR_SUCCESS) return;

	PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
	char szBuffer[512];
	while (pAdapterInfo) {
		time_t lease = pAdapterInfo->LeaseObtained;
		if (!forceLease || lease > 0llu) {
			sprintf_s(szBuffer, sizeof(szBuffer), "%.2x%.2x%.2x%.2x%.2x%.2x",
				pAdapterInfo->Address[0],
				pAdapterInfo->Address[1],
				pAdapterInfo->Address[2],
				pAdapterInfo->Address[3],
				pAdapterInfo->Address[4],
				pAdapterInfo->Address[5]
				);
			vMacAddresses.push_back(szBuffer);
		}
		pAdapterInfo = pAdapterInfo->Next;
	}
}

DWORD WINAPI ThreadFunc(void *data) {
	//get thread data
	if (data == nullptr) return (DWORD)0;
	ThreadData *t = (ThreadData *)data;

	printf("Connecting to hello world server... \n");
	void *context = zmq_ctx_new();
	void *requester = zmq_socket(context, ZMQ_REQ);
	zmq_connect(requester, t->socket.c_str());

	char buffer[11] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	printf("Sending 'Hello'...\n");
	zmq_send(requester, "Hello", 5, 0);

	zmq_recv(requester, buffer, 10, 0);
	printf("Success\nReceived message:\n");
	printf("\t%s\n", &buffer);

	zmq_close(requester);
	zmq_ctx_destroy(context);

	return (DWORD)1;
}

int connect() {
	std::vector<std::string> macs;
	getdMacAddresses(macs);

	ThreadData args;
	args.socket = "tcp://localhost:5555";
	HANDLE thread = CreateThread(NULL, 0, ThreadFunc, (void*)&args, 0, NULL);

	if (thread) {
		printf("Waiting for connection\n");
		DWORD xcode = STILL_ACTIVE;
		int wait = 0, maxwait = 30;
		//check every second
		while (xcode == STILL_ACTIVE) {
			//get return code
			GetExitCodeThread(thread, &xcode);
			if (++wait >= maxwait) break;
			Sleep(1000);
		}
		//if failed
		if (xcode != (DWORD)1) {
			//terminate thread
			TerminateThread(thread, (DWORD)0);
			//tell user
			printf("Connection timed out\n");
		} else {
			printf("Disconnected\n");
		}
	}

	//wait 30 seconds
	Sleep(30000);

	return 0;
}

