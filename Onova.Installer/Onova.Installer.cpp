// Onova.Installer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "Onova.Installer.h"

using namespace std;

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
        cerr << "Installer does not contain any data." << endl;
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

    // read the manifest
    ManifestMap manifestMap;
    
    if (!ParseManifest(data->ManifestUrl, manifestMap))
    {
        cerr << "Could not parse the manifest." << endl;
        return 1;
    }

#ifndef NDEBUG
    // print manifest data
    for (auto iter = manifestMap.begin(); iter != manifestMap.end(); iter++)
    {
        cout << "Version: " << iter->first << " URL: " << iter->second << endl;
    }
#endif

    return 0;
}

bool ParseManifest(char* manifestUrl, ManifestMap &manifestMap)
{
    cpr::Response r = cpr::Get(
        cpr::Url{ manifestUrl }
    );

    if (r.status_code == 0)
    {
        cerr << r.error.message << endl;
        return false;
    }
    else if (r.status_code >= 400)
    {
        cerr << "Error " << r.status_code << " making request." << endl;
        return false;
    }

    if (r.text.empty())
    {
        cerr << "Manifest is empty." << endl;
        return false;
    }

    string line;
    stringstream ss(r.text);
    
    while (getline(ss, line))
    {
        int firstPos = line.find(' ');
        if (firstPos == string::npos)
        {
            cerr << "Manifest contains format errors." << endl;
            return false;
        }

        string version = line.substr(0, firstPos);

        // allow other parameters in manifest
        int secondPos = line.find(' ');
        if (secondPos == string::npos)
            secondPos = line.length() - 1;

        string url = line.substr(firstPos + 1, secondPos);

        manifestMap.insert(pair<string, string>(version, url));
    }

    return true;
}
