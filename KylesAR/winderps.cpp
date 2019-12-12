#include "winderps.h"

//Convert a wide Unicode string to an UTF8 string
std::string toStr(const std::wstring &wstr) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.to_bytes(wstr);
}
//Convert a UTF8 string to a wide Unicode String
std::wstring toWstr(const std::string &str) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(str);
}

//opens a DLL file for reading
HINSTANCE openDLL(std::wstring &path) {
	//load dynamic library
	HINSTANCE dllinst = LoadLibrary(path.c_str());
	//check if found
	if (!dllinst) {
#ifdef LOG_WIN_ERR
		printf("Could not load the DLL. Error %i\n", GetLastError());
#endif
		return NULL;
	}
	return dllinst;
}
//loads a function from an instance returned by openDLL(string path)
FARPROC loadFunction(HINSTANCE &dllinst, std::string &name) {
	//resolve function address
	FARPROC func = GetProcAddress(dllinst, name.c_str());
	//check if exist
	if (!func) {
#ifdef LOG_WIN_ERR
		printf("Could not locate the function %s. Error %i\n", name.c_str(), GetLastError());
#endif
		return NULL;
	}
	return func;
}
//closes an instance returned by openDLL(string path)
void closeDLL(HINSTANCE &dllinst) {
	FreeLibrary(dllinst);
}

//opens a text file, puts each line in a vector<string>
bool openTxt(string &path, vector<string> &out) {
	ifstream ifs(path.c_str(), ios::in);
	if (ifs.is_open()) {
		string line = "";
		while (getline(ifs, line)) {
			out.push_back(line);
		}
		return true;
	} else {
#ifdef LOG_WIN_ERR
		printf("Could not locate the file: %s\n", path.c_str());
#endif
		return false;
	}
}
//opens a text file, dumps all in string
bool openTxt(string &path, string &out) {
	ifstream ifs(path.c_str(), ios::in | ios::binary | ios::ate);
	if (ifs.is_open()) {
		ifstream::pos_type fileSize = ifs.tellg();
		ifs.seekg(0, ios::beg);
		vector<char> bytes(fileSize);
		ifs.read(bytes.data(), fileSize);
		out = string(bytes.data(), fileSize);
		return true;
	} else {
#ifdef LOG_WIN_ERR
		printf("Could not locate the file: %s\n", path.c_str());
#endif
		return false;
	}
}

//opens a text file, dumps string in file
bool saveTxt(string &path, string &buf) {
	ofstream ofs(path.c_str(), ios::out | ios::binary | ios::ate);
	if (ofs.is_open()) {
		ofs.write(buf.c_str(), buf.size());
		return true;
	} else {
#ifdef LOG_WIN_ERR
		printf("Could not locate the file: %s\n", path.c_str());
#endif
		return false;
	}
}

// trim from end of string (right)
void rtrim(std::string& s, const char* t) {
	s.erase(s.find_last_not_of(t) + 1);
}
// trim from beginning of string (left)
void ltrim(std::string& s, const char* t) {
	s.erase(0, s.find_first_not_of(t));
}
// trim from both ends of string (left & right)
string& trim(std::string& s, const char* t) {
	rtrim(s, t); ltrim(s, t);
	return s;
}
// split a string around a character as a delimiter
vector<string> split(std::string s, const char* t) {
	vector<string> ret;

	uint start = 0;
	uint len = s.length();
	uint lt = strlen(t);
	uint end = s.find(t);
	while (end < len) {
		ret.push_back(s.substr(start, end - start));
		start = end + lt;
		end = s.find(t, start);
	}
	ret.push_back(s.substr(start, end));
	return ret;
}


//get absolute path we are running from
string getCWD() {
	char cpath[MAX_PATH];
	HMODULE hm = NULL;
	if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&getCWD, &hm)) {
		int ret = GetLastError();
		fprintf(stderr, "GetModuleHandle returned %d\n", ret);
	}
	GetModuleFileNameA(hm, cpath, sizeof(cpath));
	//chop off the exe/dll name
	std::string path(cpath);
	size_t slicer = path.find_last_of('\\');
	return path.substr(0, slicer);
}

//gets a window by ID
BOOL CALLBACK EnumByPidProc(HWND hWnd, LPARAM lParam) {
	// Retrieve storage location for communication data
	EnumData &ed = *(EnumData*)lParam;
	DWORD dwProcessId = 0;
	// Query process ID for hWnd
	GetWindowThreadProcessId(hWnd, &dwProcessId);
	// Apply filter
	if (ed.dwProcessId == dwProcessId) {
		// Found a window matching the process ID
		ed.hWnd = hWnd;
		// Report success
		ed.success = true;
		// Stop enumeration
		return FALSE;
	}
	// Continue enumeration
	return TRUE;
}
//gets HWND from process ID
bool getHWNDfromID(DWORD processID, HWND &out) {
	EnumData res;
	res.dwProcessId = processID;
	EnumWindows(EnumByPidProc, (LPARAM)&res);
	if (res.success) {
		out = res.hWnd;
		return true;
	}
	return false;
}

//window operations
void maximize(HWND TargetWindow) { ShowWindow(TargetWindow, SW_MAXIMIZE); }
void minimize(HWND TargetWindow) { ShowWindow(TargetWindow, SW_MINIMIZE); }
void hide(HWND TargetWindow) { ShowWindow(TargetWindow, SW_HIDE); }
void restore(HWND TargetWindow) { ShowWindow(TargetWindow, SW_RESTORE); }

//load hook start capture
WindowsIntegration::WindowsIntegration(std::wstring &_path, std::wstring &_exe) {
	path = _path;
	exe = _exe;
}
//release
WindowsIntegration::~WindowsIntegration() {
	DeleteDC(MemoryDC);
	DeleteObject(MemoryBitmap);
	ReleaseDC(NULL, TargetDC);
}

//send left click that lasts 5ms
void WindowsIntegration::sendClick(cv::Point &pt) {
	PostMessage(TargetWindow, WM_LBUTTONDOWN, 0, (pt.x - TargetRect.left) | ((pt.y - TargetRect.top) << 16));
	Sleep(5);
	PostMessage(TargetWindow, WM_LBUTTONUP, 0, (pt.x - TargetRect.left) | ((pt.y - TargetRect.top) << 16));
}

//take screenshot of client area
void WindowsIntegration::capture() {
	//migh have to do this
	//SelectObject(MemoryDC, MemoryBitmap);
	PrintWindow(TargetWindow, MemoryDC, PW_CLIENTONLY);
	picture = cv::Mat(Height, Width, CV_8UC4, Buffer, 0);
}
//start program
void WindowsIntegration::start() {
	// additional information
	STARTUPINFO si;
	auto lpApplicationName = path + L"\\" + exe; // , CP_UTF8);

	// set the size of the structures
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.wShowWindow = SW_SHOWMAXIMIZED;

	ZeroMemory(&pi, sizeof(pi));

	// start the program up
	if (!CreateProcess(lpApplicationName.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
#ifdef LOG_WIN_ERR
		printf("create process failed\n");
#endif
		return;
	}

	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}
//get hwnd
void WindowsIntegration::hook() {
	bool success = false;
	//try up to 3 times
	for (int i = 0; i < 4; i++) {
		//try to locate
		if (getHWNDfromID(pi.dwProcessId, TargetWindow)) {
			success = true;
			break;
		}
		//wait for program
		Sleep(250);
	}
	if (!success) {
#ifdef LOG_WIN_ERR
		printf("could not get HWND\n");
#endif
		return;
	}

	TargetDC = GetDC(TargetWindow);
	MemoryDC = CreateCompatibleDC(NULL);

	//DEVICE_SCALE_FACTOR dsf;
	//GetScaleFactorForMonitor(MonitorFromWindow(TargetWindow, MONITOR_DEFAULTTONEAREST), &dsf);
	
	success = false;
	//try up to 3 times
	for (int i = 0; i < 4; i++) {
		//get window size (why bother)
		GetClientRect(TargetWindow, &TargetRect);
		//wtf windows
		Width = int(float(TargetRect.right - TargetRect.left) * scale);
		Height = int(float(TargetRect.bottom - TargetRect.top) * scale);
		//if success
		if (Height > 100) {
			success = true;
			break;
		}
		//wait for program
		Sleep(250);
	}
	if (!success) {
#ifdef LOG_WIN_ERR
		printf("size failed (%i, %i)", Width, Height);
#endif
		return;
	}

	//bitmap
	ZeroMemory(&Binfo, sizeof(BITMAPINFO));
	Binfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	Binfo.bmiHeader.biWidth = Width;
	Binfo.bmiHeader.biHeight = Height;
	Binfo.bmiHeader.biPlanes = 1;
	Binfo.bmiHeader.biBitCount = 32;
	MemoryBitmap = CreateDIBSection(TargetDC, &Binfo, DIB_RGB_COLORS, &Buffer, NULL, 0);
	//select memory
	SelectObject(MemoryDC, MemoryBitmap);
	//create cv mat
	picture = cv::Mat(Height, Width, CV_8UC4, cv::Scalar(0, 0, 0, 0));
	isInit = true;
}
