// Onova.Installer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "Onova.Installer.h"

using namespace std;

int main(int argc, char* argv[])
{

#ifndef NDEBUG
	cout << "Attach debugger" << endl;
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
	unsigned int fileSize = static_cast<unsigned int>(executable.tellg());
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
		cout << "Version: " << iter->first << " URL: " << iter->second << endl;
#endif

	// download the newest entry in the manifest
	string newestZipUrl = (--manifestMap.end())->second;
	string downloadZipUrl;

	if (!BuildZipURL(data->ManifestUrl, newestZipUrl, downloadZipUrl))
	{
		cerr << "Could not build the zip url." << endl;
		return 1;
	}

	wstring tempZipPath;

	if (!DownloadPackage(downloadZipUrl, tempZipPath))
	{
		cerr << "Could not download the zip." << endl;
		return 1;
	}

#ifndef NDEBUG
	wcout << L"Written zip to: " << tempZipPath << endl;
#endif

	// unpack the zip
	wstring startupFilePath;
	if (!InstallPackage(data->AppName, tempZipPath, startupFilePath))
	{
		cerr << "Could not install the zip." << endl;
		return 1;
	}

#ifndef NDEBUG
	cout << "Zip unpacked." << endl;
#endif

	// send link to Main Menu
	if (!LinkStartMenu(data->AppName, startupFilePath))
	{
		cerr << "Could not link to start menu." << endl;
		return 1;
	}

#ifndef NDEBUG
	cout << "Linked to start menu." << endl;
#endif

	return 0;
}

bool ParseManifest(string manifestUrl, ManifestMap& manifestMap)
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
		size_t firstPos = line.find(' ');
		if (firstPos == string::npos)
		{
			cerr << "Manifest contains format errors." << endl;
			return false;
		}

		string version = line.substr(0, firstPos);

		// allow other parameters in the manifest
		size_t secondPos = line.find(' ', firstPos + 1);
		if (secondPos == string::npos)
			secondPos = line.length();

		string url = line.substr(firstPos + 1, secondPos - 1);

		manifestMap.insert(pair<string, string>(version, url));
	}

	return true;
}

bool DownloadPackage(string packageUrl, wstring& tempZipPath)
{
	auto tempFolderPath = filesystem::temp_directory_path().wstring();

	size_t lastSegmentPos = packageUrl.find_last_of('/');
	if (lastSegmentPos == string::npos)
	{
		cerr << "URL is not valid." << endl;
		return false;
	}

	auto fileName = packageUrl.substr(lastSegmentPos + 1);
	tempFolderPath.append(ConvertAnsiToWide(fileName));

	auto tempStream = ofstream(tempFolderPath, std::ios::binary | std::ios::out);
	auto session = cpr::Session();
	session.SetUrl(cpr::Url{ packageUrl });
	auto r = session.Download(tempStream);

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

	tempZipPath = tempFolderPath;

	return true;
}

bool BuildZipURL(string manifestUrl, string packageUrl, string& absoluteUrl)
{
	// check if the package url is absolute
	if (packageUrl.rfind("http", 0) == 0)
	{
		absoluteUrl = packageUrl;
		return true;
	}

	stringstream urlBuilder;

	// if the package url starts with '/' it is relative to the root
	if (packageUrl.rfind("/", 0) == 0)
	{

		size_t pos = find_nth(manifestUrl, "/", 3);
		if (pos == string::npos)
		{
			cerr << "URL is not valid." << endl;
			return false;
		}

		string wholePart = manifestUrl.substr(0, pos);
		urlBuilder << wholePart << packageUrl;

		absoluteUrl = urlBuilder.str();
		return true;
	}

	// or its relative to manifest
	int lastSegmentPos = manifestUrl.find_last_of('/');
	if (lastSegmentPos == string::npos)
	{
		cerr << "URL is not valid." << endl;
		return false;
	}

	string absolutePart = manifestUrl.substr(0, lastSegmentPos + 1);

	urlBuilder << absolutePart << packageUrl;

	absoluteUrl = urlBuilder.str();
	return true;
}

bool InstallPackage(string appName, wstring zipPath, wstring& startupFilePath)
{
	wstring localAppDataPath;

	if (!GetLocalAppDataFolder(localAppDataPath))
	{
		cerr << "Could not get local appdata folder." << endl;
		return false;
	}

	wstringstream appBasePathBuilder;

	appBasePathBuilder << localAppDataPath << L"\\" << ConvertAnsiToWide(appName);
	wstring appBasePath = appBasePathBuilder.str();

	appBasePathBuilder << L"\\" << "oapp";
	wstring appUnpackPath = appBasePathBuilder.str();

	if (!filesystem::is_directory(appBasePath) || !filesystem::exists(appBasePath))
	{
		if (!filesystem::create_directory(appBasePath))
		{
			cerr << "Could not create app base folder." << endl;
			return false;
		}
	}

	if (!filesystem::is_directory(appUnpackPath) || !filesystem::exists(appUnpackPath))
	{
		if (!filesystem::create_directory(appUnpackPath))
		{
			cerr << "Could not create app base data folder." << endl;
			return false;
		}
	}

	if (!Unzip(zipPath, appUnpackPath))
	{
		cerr << "Could not unzip the file." << endl;
		return false;
	}

	appBasePathBuilder << L"\\" << ConvertAnsiToWide(appName) << ".exe";
	startupFilePath = appBasePathBuilder.str();

	if (!filesystem::exists(startupFilePath))
	{
		cerr << "Startup exe not found." << endl;
		return false;
	}

	return true;
}

bool GetLocalAppDataFolder(wstring& folderPath)
{
	PWSTR path = nullptr;
	HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &path);

	if (FAILED(result))
		return false;

	folderPath = wstring(path);

	CoTaskMemFree(path);
	return true;
}

bool Unzip(wstring targetZip, wstring targetPath)
{
	HZIP zipHandle = OpenZip(targetZip.c_str(), NULL);

	if (zipHandle == NULL)
		return false;

	ZRESULT returnCode = SetUnzipBaseDir(zipHandle, targetPath.c_str());

	if (returnCode != ZR_OK)
		return false;

	ZIPENTRY zipEntry;
	returnCode = GetZipItem(zipHandle, -1, &zipEntry);

	if (returnCode != ZR_OK)
		return false;

	int zipItemCount = zipEntry.index;
	for (int i = 0; i < zipItemCount; i++)
	{
		ZIPENTRY zipItem;
		returnCode = GetZipItem(zipHandle, i, &zipItem);

		if (returnCode != ZR_OK)
			return false;

		returnCode = UnzipItem(zipHandle, i, zipItem.name);

		if (returnCode != ZR_OK)
			return false;
	}

	CloseZip(zipHandle);

	return true;
}

bool LinkStartMenu(string appName, wstring startupFilePath)
{
	PWSTR startMenuPath = nullptr;

	HRESULT result = SHGetKnownFolderPath(FOLDERID_Programs, 0, NULL, &startMenuPath);
	if (FAILED(result))
		return false;

	wstringstream linkPathBuilder;
	linkPathBuilder << startMenuPath << L"\\" << ConvertAnsiToWide(appName) << ".lnk";

	wstring linkPath = linkPathBuilder.str();

	result = CoInitialize(NULL);
	if (FAILED(result))
		return false;

	IShellLinkW* shellLink = nullptr;

	result = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_ALL, IID_IShellLinkW, (void**)&shellLink);
	if (FAILED(result))
		return false;

	shellLink->SetPath(startupFilePath.c_str());
	//shellLink->SetDescription(ConvertAnsiToWide(appName).c_str());
	shellLink->SetIconLocation(startupFilePath.c_str(), 0);

	IPersistFile* persistFile = nullptr;
	result = shellLink->QueryInterface(IID_IPersistFile, (void**)&persistFile);
	if (FAILED(result))
		return false;

	result = persistFile->Save(linkPath.c_str(), TRUE);

	persistFile->Release();
	shellLink->Release();

	CoTaskMemFree(startMenuPath);

	return true;
}

