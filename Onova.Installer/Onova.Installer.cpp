// Onova.Installer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "Onova.Installer.h"

using namespace std;
using namespace winreg;

int main(int argc, char* argv[])
{
#ifdef NDEBUG
	// hide the console window
	ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif

#ifndef NDEBUG
	cout << "Attach debugger" << endl;
	system("pause");
#endif

	filesystem::path selfFilePath(argv[0]);

	// check if the file is uninstaller
	bool uninstall = false;
	if (selfFilePath.filename() == "uninstall.exe")
		uninstall = true;

	PublisherData_t data;
	if (!LoadPublisherData(selfFilePath, &data))
	{
		PrintError("Installer does not contain any data.");
		return 1;
	}

#ifndef NDEBUG
	cout << data.AppName << endl;
	cout << data.ManifestUrl << endl;
#endif

	// check if the app is already installed
	bool installed;
	UninstallerData_t uninstallerData;
	if (!CheckInstalled(data.AppName, installed, &uninstallerData))
	{
		PrintError("Could not check if the app is already installed.");
		return 1;
	}

	if (installed)
	{
		// let us uninstall the app instead
		if (uninstall)
		{
			if (!UninstallApp(selfFilePath, &uninstallerData))
			{
				PrintError("Could not uninstall the app.");
				return 1;
			}

			return 0;
		}

		PrintError("The app is already installed.");
		return 1;
	}

	// show the window
	ShowWindow(GetConsoleWindow(), SW_SHOW);

	cout << "Welcome to " << data.AppName << " web install" << endl;
	cout << "Downloading the latest version..." << endl;

	// read the manifest
	ManifestMap manifestMap;

	if (!ParseManifest(data.ManifestUrl, manifestMap))
	{
		PrintError("Could not parse the manifest.");
		return 1;
	}

#ifndef NDEBUG
	for (auto iter = manifestMap.begin(); iter != manifestMap.end(); iter++)
		cout << "Version: " << iter->first << " URL: " << iter->second << endl;
#endif

	// download the newest entry in the manifest
	auto newestEntry = (--manifestMap.end());
	string newestZipUrl = newestEntry->second;
	string downloadZipUrl;

	if (!BuildZipURL(data.ManifestUrl, newestZipUrl, downloadZipUrl))
	{
		PrintError("Could not build the zip url.");
		return 1;
	}

	cout << "Installing version " << newestEntry->first << "..." << endl;

	wstring tempZipPath;

	if (!DownloadPackage(downloadZipUrl, tempZipPath))
	{
		PrintError("Could not download the zip.");
		return 1;
	}

#ifndef NDEBUG
	wcout << L"Written zip to: " << tempZipPath << endl;
#endif

	// unpack the zip
	DWORD unpackedSize;
	wstring startupFilePath;
	wstring baseFolderPath;
	if (!InstallPackage(data.AppName, tempZipPath, startupFilePath, baseFolderPath, unpackedSize))
	{
		PrintError("Could not install the zip.");
		return 1;
	}

#ifndef NDEBUG
	cout << "Zip unpacked." << endl;
#endif

	// send link to main menu
	if (!LinkStartMenu(data.AppName, startupFilePath))
	{
		PrintError("Could not link to start menu.");
		return 1;
	}

#ifndef NDEBUG
	cout << "Linked to start menu." << endl;
#endif

	// register the app in registry
	if (!RegisterApp(data.AppName, newestEntry->first, "", unpackedSize, baseFolderPath, startupFilePath))
	{
		PrintError("Could not register the app.");
		return 1;
	}

#ifndef NDEBUG
	cout << "Created registry entry." << endl;
#endif

	// copy the installer and save it as uninstaller
	if (!SetupUninstaller(selfFilePath, baseFolderPath))
	{
		PrintError("Could not setup uninstaller.");
		return 1;
	}

#ifndef NDEBUG
	cout << "Copied uninstaller." << endl;
#endif

	cout << "The app was sucessfully installed." << endl;
	system("pause");

	return 0;
}

bool LoadPublisherData(wstring selfFilePath, PublisherData_t* publisherData)
{
	ifstream executable;
	IMAGE_DOS_HEADER dosHeader;
	IMAGE_NT_HEADERS peHeader;
	IMAGE_SECTION_HEADER sectionHeader;

	// open current running exe
	executable.open(selfFilePath, ifstream::binary);

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
		return false;

	// seek to the start of appended data
	executable.seekg(imageSize, executable.beg);

	// read the appended data to buffer
	char* buffer = new char[PUBLISHER_DATA_LEN];
	executable.read(buffer, PUBLISHER_DATA_LEN);

	// parse the structure
	char* pointer = buffer;

	memcpy(publisherData->AppName, pointer, PUBLISHER_DATA_APPNAME_LEN);
	pointer += PUBLISHER_DATA_APPNAME_LEN;

	memcpy(publisherData->ManifestUrl, pointer, PUBLISHER_DATA_MANIFESTURL_LEN);
	pointer += PUBLISHER_DATA_MANIFESTURL_LEN;

	return true;
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

bool InstallPackage(string appName, wstring zipPath, wstring& startupFilePath, wstring& baseFolderPath, DWORD& unpackedSize)
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

	baseFolderPath = appBasePath;

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

	if (!Unzip(zipPath, appUnpackPath, unpackedSize))
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

bool Unzip(wstring targetZip, wstring targetPath, DWORD& unpackedSize)
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
	unpackedSize = 0;

	for (int i = 0; i < zipItemCount; i++)
	{
		ZIPENTRY zipItem;
		returnCode = GetZipItem(zipHandle, i, &zipItem);

		if (returnCode != ZR_OK)
			return false;

		returnCode = UnzipItem(zipHandle, i, zipItem.name);

		if (returnCode != ZR_OK)
			return false;

		unpackedSize += zipItem.unc_size;
	}

	unpackedSize /= 1024; // size in kilobytes

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

bool CheckInstalled(string appName, bool& installed, UninstallerData_t* uninstallerData)
{
	try
	{
		wstringstream keyBuilder;
		keyBuilder << L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" << ConvertAnsiToWide(appName);

		RegKey key;
		RegResult result = key.TryOpen(HKEY_CURRENT_USER, keyBuilder.str());

		if (!result)
		{
			installed = false;
			return true;
		}

		installed = true;
		uninstallerData->UninstallerBaseKey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\";
		uninstallerData->AppName = ConvertAnsiToWide(appName);
		uninstallerData->UninstallerPath = key.GetStringValue(L"UninstallString");
		uninstallerData->BaseFolderPath = key.GetStringValue(L"InstallLocation");
	}
	catch (const exception& ex)
	{
		cerr << "Error accessing registry." << endl;
		wcerr << ex.what() << endl;
		return false;
	}

	return true;
}

bool RegisterApp(string appName, string version, string company, DWORD unpackedSize, wstring installLocation, wstring exeLocation)
{
	wstringstream keyBuilder;
	keyBuilder << L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" << ConvertAnsiToWide(appName);

	RegKey key;
	RegResult result = key.TryCreate(HKEY_CURRENT_USER, keyBuilder.str());
	if (!result)
	{
		cerr << "Error accessing registry." << endl;
		wcerr << result.ErrorMessage() << endl;
		return false;
	}

	key.SetStringValue(L"DisplayName", ConvertAnsiToWide(appName));
	key.SetStringValue(L"DisplayVersion", ConvertAnsiToWide(version));
	key.SetStringValue(L"DisplayIcon", exeLocation);
	if (!company.empty())
		key.SetStringValue(L"Publisher", ConvertAnsiToWide(company));
	key.SetDwordValue(L"EstimatedSize", unpackedSize);
	key.SetDwordValue(L"NoModify", 1);
	key.SetDwordValue(L"NoRepair", 1);
	key.SetStringValue(L"InstallLocation", installLocation);

	time_t now = chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	char buf[100] = { 0 };
	std::strftime(buf, sizeof(buf), "%Y%m%d", localtime(&now));
	wstring installDate = wstring(ConvertAnsiToWide(buf));
	key.SetStringValue(L"InstallDate", installDate);

	wstringstream uninstallPathBuilder;
	uninstallPathBuilder << installLocation << L"\\uninstall.exe";
	key.SetStringValue(L"UninstallString", uninstallPathBuilder.str());

	return true;
}

bool SetupUninstaller(wstring selfFilePath, wstring targetFolder)
{
	try
	{
		filesystem::path uninstallerPath(targetFolder);
		uninstallerPath /= "uninstall.exe";
		filesystem::copy(selfFilePath, uninstallerPath, filesystem::copy_options::overwrite_existing);
	}
	catch (const exception&)
	{
		return false;
	}

	return true;
}

bool UninstallApp(wstring selfFilePath, UninstallerData_t* uninstallerData)
{
	try
	{
		// the installer is in invalid path
		if (uninstallerData->UninstallerPath.compare(selfFilePath) != 0)
			return false;

		filesystem::path baseFolderPath(uninstallerData->BaseFolderPath);

		// delete the app package
		filesystem::remove_all(baseFolderPath / "oapp");

		// delete start menu entry
		PWSTR startMenuPath = nullptr;

		HRESULT result = SHGetKnownFolderPath(FOLDERID_Programs, 0, NULL, &startMenuPath);
		if (FAILED(result))
			return false;

		wstringstream linkPathBuilder;
		linkPathBuilder << startMenuPath << L"\\" << uninstallerData->AppName << ".lnk";

		wstring linkPath = linkPathBuilder.str();
		if (filesystem::exists(linkPath))
			filesystem::remove(linkPath);

		// unregister the app
		RegKey key{ HKEY_CURRENT_USER, uninstallerData->UninstallerBaseKey };
		key.DeleteKey(uninstallerData->AppName, KEY_WRITE);

		// delete the uninstaller itself
		wstringstream commandBuilder;
		commandBuilder << "start /min cmd /c del " << uninstallerData->UninstallerPath;

		_wsystem(commandBuilder.str().c_str());
	}
	catch (const exception&)
	{
		return false;
	}

	return true;
}

void PrintError(string message)
{
	ShowWindow(GetConsoleWindow(), SW_SHOW);
	cerr << message << endl;
	system("pause");
}