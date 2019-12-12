/*

Winapp needs to be more robust
	-occlusion buffer fraction of RECT?
	-resize geometry on bind
	-look at memory leaks, possible cleanup needs (HBITMAP)

*/

#pragma once

#define LOG_WIN_ERR
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <minwindef.h>
#include <ShellScalingAPI.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <psapi.h>
#include <iostream>
#include <fstream>
#include <locale>
#include <codecvt>

#include <opencv2\core.hpp>
#include <opencv2\calib3d.hpp> //pose estimation and extrinsics
#include <opencv2\highgui.hpp> //shortcut to screen
#include <opencv2\imgproc.hpp> //image processing algorithms

#include <GL/glew.h>

using namespace std;

//Convert a wide Unicode string to an UTF8 string
string toStr(const wstring& wstr);
//Convert a UTF8 string to a wide Unicode String
wstring toWstr(const string& str);

//opens a DLL file for reading
HINSTANCE openDLL(wstring& path);
//loads a function from an instance returned by openDLL(string path)
FARPROC loadFunction(HINSTANCE& dllinst, string& name);
//closes an instance returned by openDLL(string path)
void closeDLL(HINSTANCE& dllinst);

//opens a text file, puts each line in a vector<string>
bool openTxt(string& path, vector<string>& out);
//opens a text file, dumps all in string
bool openTxt(string& path, string& out);

//opens a text file, dumps string in file
bool saveTxt(string& path, string& buf);

// trim from end of string (right)
void rtrim(std::string& s, const char* t = " \t\n\r\f\v");
// trim from beginning of string (left)
void ltrim(std::string& s, const char* t = " \t\n\r\f\v");
// trim from both ends of string (left & right)
string& trim(std::string& s, const char* t = " \t\n\r\f\v");
// split around a delimiter
vector<string> split(std::string s, const char* t = " ");

//get current working directory
string getCWD();

// Structure used to communicate data from and to enumeration procedure
struct EnumData {
	DWORD dwProcessId;
	HWND hWnd;
	bool success = false;
};

// Launches a windows program in this one
class WindowsIntegration {
private:
	//windows info
	PROCESS_INFORMATION pi;
	wstring path, exe; 
	HWND TargetWindow;
	HDC TargetDC;
	HDC MemoryDC;
	RECT TargetRect;
	HBITMAP MemoryBitmap;
	BITMAPINFO Binfo = {};
	void* Buffer;

	bool isInit = false;

public:
	//UI scale (win property i cant find)
	float scale = 1.f;
	//winow properties
	int Width, Height;
	//just the *buffer in a container
	cv::Mat picture;

	//initialize
	WindowsIntegration() {}
	WindowsIntegration(wstring& path, wstring& exe);
	//release
	~WindowsIntegration();

	//get
	bool isInitialized() { return isInit; }
	HWND getHWND() { return TargetWindow; }

	//set
	void sendClick(cv::Point& pt);

	//launch the program
	void start();
	//locate handle to window
	void hook();
	//capture using printwindow
	void capture();
};
