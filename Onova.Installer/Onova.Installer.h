#pragma once

#include <fstream>
#include <iostream>
#include <Windows.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wldap32.lib")
#pragma comment(lib, "crypt32.lib")

#include <cpr/cpr.h>

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

bool ParseManifest(char* manifestUrl, ManifestMap& manifestMap);