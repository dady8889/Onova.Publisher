#pragma once

#define WIN32_LEAN_AND_MEAN

#include <fstream>
#include <filesystem>
#include <iostream>
#include <sstream>

#include <Windows.h>
#include <ShlObj.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wldap32.lib")
#pragma comment(lib, "crypt32.lib")

#include <cpr/cpr.h>

#include "StringUtils.h"
#include "unzip.h"

#define PUBLISHER_DATA_APPNAME_LEN 64
#define PUBLISHER_DATA_MANIFESTURL_LEN 1024
#define PUBLISHER_DATA_RESERVED_LEN 1024
#define PUBLISHER_DATA_LEN (PUBLISHER_DATA_APPNAME_LEN + PUBLISHER_DATA_MANIFESTURL_LEN + PUBLISHER_DATA_RESERVED_LEN)

using namespace std;

typedef struct {
    char AppName[PUBLISHER_DATA_APPNAME_LEN];
    char ManifestUrl[PUBLISHER_DATA_MANIFESTURL_LEN];
} PublisherData_t;

typedef map<string, string> ManifestMap;

bool ParseManifest(string manifestUrl, ManifestMap& manifestMap);
bool DownloadPackage(string packageUrl, wstring& tempZipPath);
bool BuildZipURL(string manifestUrl, string packageUrl, string& absoluteUrl);
bool InstallPackage(string appName, wstring zipPath, wstring& startupFilePath);
bool GetLocalAppDataFolder(wstring& folderPath);
bool Unzip(wstring targetZip, wstring targetPath);
bool LinkStartMenu(string appName, wstring startupFilePath);