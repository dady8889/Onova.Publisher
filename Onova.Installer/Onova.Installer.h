#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <fstream>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <codecvt>
#include <locale>

#include <fcntl.h>
#include <io.h>
#include <Windows.h>
#include <ShlObj.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wldap32.lib")
#pragma comment(lib, "crypt32.lib")

#include <cpr/cpr.h>

#include "StringUtils.h"
#include "WinReg.hpp"
#include "zip_file.hpp"

constexpr size_t PUBLISHER_DATA_APPNAME_LEN = (32 * sizeof(wchar_t));
constexpr size_t PUBLISHER_DATA_MANIFESTURL_LEN = (1024 * sizeof(char));
constexpr size_t PUBLISHER_DATA_LEN = (PUBLISHER_DATA_APPNAME_LEN + PUBLISHER_DATA_MANIFESTURL_LEN);

using namespace std;

typedef struct {
    std::wstring AppName;
    std::string ManifestUrl;
} PublisherData_t;

typedef struct {
    wstring UninstallerPath;
    wstring BaseFolderPath;
    wstring UninstallerBaseKey;
    wstring AppName;
} UninstallerData_t;

typedef map<string, string> ManifestMap;

bool LoadPublisherData(wstring selfFilePath, PublisherData_t* publisherData);
bool ParseManifest(string manifestUrl, ManifestMap& manifestMap);
bool DownloadPackage(string packageUrl, wstring& tempZipPath);
bool BuildZipURL(string manifestUrl, string packageUrl, string& absoluteUrl);
bool InstallPackage(wstring appName, wstring zipPath, wstring& startupFilePath, wstring& baseFolderPath, DWORD& unpackedSize);
bool GetLocalAppDataFolder(wstring& folderPath);
bool Unzip(wstring targetZip, wstring targetPath, DWORD& unpackedSize);
bool LinkStartMenu(wstring appName, wstring startupFilePath);
bool CheckInstalled(wstring appName, bool& installed, UninstallerData_t* uninstallerData);
bool RegisterApp(wstring appName, string version, string company, DWORD unpackedSize, wstring installLocation, wstring exeLocation);
bool SetupUninstaller(wstring selfFilePath, wstring targetFolder);
bool UninstallApp(wstring selfFilePath, UninstallerData_t* uninstallerData);
void PrintError(wstring message);