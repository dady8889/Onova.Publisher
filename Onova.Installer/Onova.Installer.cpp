// Onova.Installer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

using namespace std;

#include <fstream>
#include <iostream>
#include <Windows.h>

#define PUBLISHER_DATA_APPNAME_LEN 64
#define PUBLISHER_DATA_MANIFESTURL_LEN 1024
#define PUBLISHER_DATA_RESERVED_LEN 1024
#define PUBLISHER_DATA_LEN (PUBLISHER_DATA_APPNAME_LEN + PUBLISHER_DATA_MANIFESTURL_LEN + PUBLISHER_DATA_RESERVED_LEN)

typedef struct {
    char AppName[PUBLISHER_DATA_APPNAME_LEN];
    char ManifestUrl[PUBLISHER_DATA_MANIFESTURL_LEN];
} PublisherData_t;

int main(int argc, char* argv[])
{

#ifndef NDEBUG
    cout << "Attach debugger..." << endl;
    system("pause");
#endif

    ifstream executable;
    IMAGE_DOS_HEADER dosHeader;
    IMAGE_NT_HEADERS peHeader;
    IMAGE_SECTION_HEADER sectionHeader;

    // open current running exe
    executable.open(argv[0], ifstream::binary);

    // get file size on disk
    executable.seekg(0, executable.end);
    unsigned int fileSize = executable.tellg();
    executable.seekg(0, executable.beg);

    // read dos header
    executable.read((char*)&dosHeader, sizeof(IMAGE_DOS_HEADER));

    // read pe header
    executable.seekg(dosHeader.e_lfanew, executable.beg);
    executable.read((char*)&peHeader, sizeof(IMAGE_NT_HEADERS));

    // get exe image size
    unsigned int imageSize = 0;
    DWORD maxPointer = 0;
    int curHeader = 0;

    do
    {
        executable.read((char*)&sectionHeader, sizeof(IMAGE_SECTION_HEADER));
        if (sectionHeader.PointerToRawData > maxPointer)
        {
            maxPointer = sectionHeader.PointerToRawData;
            imageSize = sectionHeader.PointerToRawData + sectionHeader.SizeOfRawData;
        }

        curHeader++;
    } while (curHeader < peHeader.FileHeader.NumberOfSections);

#ifndef NDEBUG
    // print sizes
    cout << "File size: " << fileSize << endl;
    cout << "Image size: " << imageSize << endl;
#endif

    // file does not have the publisher data
    if (imageSize == fileSize)
    {
        cout << "Installer does not contain any data." << endl;
        return 1;
    }

    // seek to the start of appended data
    executable.seekg(imageSize, executable.beg);

    // read the appended data to buffer
    char* buffer = new char[PUBLISHER_DATA_LEN];
    executable.read(buffer, PUBLISHER_DATA_LEN);

    // parse the structure
    PublisherData_t* data = new PublisherData_t;
    char* pointer = buffer;

    memcpy(data->AppName, pointer, PUBLISHER_DATA_APPNAME_LEN);
    pointer += PUBLISHER_DATA_APPNAME_LEN;

    memcpy(data->ManifestUrl, pointer, PUBLISHER_DATA_MANIFESTURL_LEN);
    pointer += PUBLISHER_DATA_MANIFESTURL_LEN;

#ifndef NDEBUG
    // print publisher data
    cout << data->AppName << endl;
    cout << data->ManifestUrl << endl;
#endif

    return 0;
}
