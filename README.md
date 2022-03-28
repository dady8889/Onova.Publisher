# Onova.Publisher - Like Squirrel but Simpler™
[![Version](https://img.shields.io/nuget/v/Onova.Publisher.svg)](https://nuget.org/packages/Onova.Publisher)  

It has been years since Squirrel.Windows had been deprecated, without a modern replacement.
This project aims to provide the basic functionality of Squirrel.Windows.
We build upon the [Onova update library](https://github.com/Tyrrrz/Onova), which provides seamless update process for your application.
One of Onova's upsides is the unawareness of how the application has been installed.
However, to make the process of installing and updating as intuitive as possible, it is useful to make them depend on each other.

## Introduction

**Onova.Publisher is a tool that allows you to create new packages directly from Visual Studio Package Manager Console.**  
The package will be targeted for usage by [WebPackageResolver](https://github.com/Tyrrrz/Onova#webpackageresolver).  
Publisher also takes care of creating the manifest file, which is consumed by the updater.  

The most important part is the installer - in comparison to Squirrel, which packed the latest version of your application into the installer itself, Onova.Publisher packs only the information necessary to download the latest version from your deployment server.   This type of installer is also known as a web installer. The major upsides are that the installer is small and you don't need to update it with your app.  

**Like Squirrel, Onova.Publisher builds an installer which will install your application to the local appdata folder, without needing admin privileges.**  

## Documentation
The only **requirement** for Onova.Publisher to work is that your project **targets .NET 5** *(actually, it may work for other .NET Core versions but I have not tested it)*.

Let us assume you have built and dotnet published your app into the *\publish* folder of your solution.
Select your startup project as the default project in the Package Manager Console.

```
PM> Onova.Publisher -h
Onova.Publisher
  Publishes your application for Onova.

Usage:
  Onova.Publisher [options]

Options:
  -n, --name <name>             Your application's name (name of executable without extension). Maximum 64 characters.
  -v, --version <version>       Version in format major.minor[.build[.revision]].
  -u, --url <url>               URL to web where the manifest resides. Maximum 1024 characters.
  -i, --in, --target <target>   Folder which will get packed into a zip.
  -o, --out, --output <output>  Output folder which will contain the publish folder. Publish folder will contain the updated manifest file, zip and installer. [default: .]
  --no-releasenotes, --no-rn    Disables generation of an empty release note (.rn) file.
  -?, -h, --help                Show help and usage information
```

After reading the tool's help, let's try publishing our DummyApp version 1.2.3 to our web server at https://dummy.com/files/.

```
PM> Onova.Publisher --name DummyApp --version 1.2.3 --url https://dummy.com/files/ --target publish\
```

This should produce the following output:
```
Successfully published DummyApp version 1.2.3 into DummyApp-1.2.3.zip.
```
If we check our solution folder, we can see a new folder called *OnovaPublish*.
The contents of the folder are following:
```
    MANIFEST
    DummyApp-1.2.3.zip
    DummyApp-1.2.3.rn
    websetup.exe
```
The MANIFEST file will contain one line, *1.2.3 DummyApp-1.2.3.zip*. This file is precisely formatted for [WebPackageResolver](https://github.com/Tyrrrz/Onova#webpackageresolver).

The .rn file is an empty text file, in which you can add release notes for your release. For more information, check out [Onova.ReleaseNotes](https://github.com/dady8889/Onova.ReleaseNotes).

You can now upload the files to the desired location.  

Continuing, we will assume you have implemented the updating in your application as:
```csharp
using (var mgr = new UpdateManager(new WebPackageResolver("https://dummy.com/files/MANIFEST"), new ZipPackageExtractor()))
{
  var result = await mgr.CheckForUpdatesAsync();
  ...
}
```

You can try creating another version, by running a similar command:
```
PM> Onova.Publisher --name DummyApp --version 1.3.0 --url https://dummy.com/files/ --target publish\
```
Now, you should just need to upload the new zip file and update the latest MANIFEST entry. *(unless a new version of the publisher had come out in the meantime, in which case an update for the installer may be required)*

After uploading the files, let's try downloading and running the *webinstaller.exe* of our application.
The installer is written in C++ and statically linked for x86, compiled with MSVC, which means it should run on any reasonable Windows installation.
Running the installer will greet us with a console window with the following contents:
```
Welcome to DummyApp web install
Downloading the latest version...
Installing version 1.3.0...
The app was sucessfully installed.
Press any key to continue . . .
```

The installer did the following:
* Checked the manifest in the URL specified during publishing
* Parsed the manifest and selected the latest entry
* Downloaded the latest zip to temp directory
* Unpacked the zip to %localappdata%\DummyApp\oapp\
* Created link to the start menu
* Registered the app in the Windows Registry
* Copied an uninstaller to %localappdata%\DummyApp\uninstall.exe

Of course, if anything during the process went wrong, the installer would output an error message in the console.
Currently, the installed app **will not** be started after the installation.

If you want to uninstall the application, you can use the standard Windows uninstall procedure using Settings, or you can directly execute the uninstall.exe in the application base directory. *Also, in comparison with Squirrel, your application will have an icon visible in the installed programs menu, and the uninstaller leaves no installed files behind.*

## Missing (planned) features
- [x] Changelog support
- [ ] Code signing and timestamping
- [ ] Start menu link in publisher name folder
- [ ] Run app after installation
- [ ] Silent install

## Missing (not planned) features
* Bigger customization eg. icons, custom UI
* Dedicated non-web installer
* Other OS support or backporting to older .NET versions
* Bootstrapping

## Known issues
- [ ] No wiki
- [ ] No tests
- [ ] App name and URL allows only ANSI encoding
- [ ] The installer code is a bit inconsistent
- [ ] MANIFEST rebuilding sorts alphabetically, not semantically

## Contributions
I am open to suggestions, PRs, bug reports.
Any contribution is welcome.

## Licensing
The project uses following licensed works:

* unzip.h unzip.cpp - Lucian Wischik, Jean-Loup Gailly, Mark Adler, zlib
* WinReg.hpp - Giovanni Dicanio
* [cpr](https://github.com/libcpr/cpr)

My provided code is licensed under the MIT license.
