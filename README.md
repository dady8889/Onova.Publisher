# Onova.Publisher - Like Squirrel but Simpler™
[![Version](https://img.shields.io/nuget/v/Onova.Publisher.svg)](https://nuget.org/packages/Onova.Publisher)  

It has been years since Squirrel.Windows was deprecated, with no real modern replacement.
This project aims to replace the basic functionality of Squirrel.Windows.
The basis of this project is the great [Onova update library](https://github.com/Tyrrrz/Onova), which provides seamless updating of your application.
However, Onova is unaware of how the application got installed on the computer.
This fact can be an upside for some, but having different installer/updater can be cumbersome.

# Introduction

**Onova.Publisher is a tool that allows you to create new packages directly from Visual Studio Package Manager Console.**  
The package will be targeted for the [Onova WebPackageResolver](https://github.com/Tyrrrz/Onova#webpackageresolver).  
Publisher also takes care of creating the manifest file, which is consumed by the updater.  

The most important part is the installer - in comparison to Squirrel, which packed the latest version of your application into the installer itself, Onova.Publisher packs only the information necessary to download the latest version from your deployment server.   This type of installer is also known as a web installer. The upsides and downsides are obvious, the installer is small and you don't need to update it every time you update your app (also, it made the programming a lot simpler).  

**Like Squirrel, Onova.Publisher builds an installer which will install your application to the local appdata folder, without needing admin privileges.**


Enough talking, let us show you what we got.

# Documentation
The only **requirement** for Onova.Publisher to work is that your project **targets .NET 5** *(actually, it may work for other .NET Core versions but I have not tested it)*.

Let us assume you have built and dotnet published your app into the *\publish* folder of your solution.
Select your startup project as the default project in the Package Manager Console.
Now, let's check out the Onova.Publisher command:

```
PM> Onova.Publisher -h
Onova.Publisher
  Publishes your application for Onova.

Usage:
  Onova.Publisher [options]

Options:
  --name <name>        Your application's name (name of executable without extension). Maximum 64 characters.
  --version <version>  Version in format major.minor[.build[.revision]].
  --url <url>          URL to web where the manifest resides. Maximum 1024 characters.
  --target <target>    Folder which will get packed into a zip.
  --output <output>    Output folder which will contain the publish folder. Publish folder will contain the updated manifest file, zip and installer. [default: .]
```

After reading the documentation, let's try publishing our DummyApp version 1.2.3 to our web server at https://dummy.com/files/.

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
    websetup.exe
```
The MANIFEST file will contain one line, *1.2.3 DummyApp-1.2.3.zip*. This file is precisely formatted for the [Onova WebPackageResolver](https://github.com/Tyrrrz/Onova#webpackageresolver).

You can now upload the files to the desired location. We will assume you have implemented the updating in your application like this:
```csharp
using (var mgr = new UpdateManager(new WebPackageResolver("https://dummy.com/files/MANIFEST"), new ZipPackageExtractor()))
{
  var result = await mgr.CheckForUpdatesAsync();
  ...
}
```
For the sake of simplicity of the installer, I am standardising the name of the manifest file as MANIFEST. *(however, if it is a big problem, it could be made as a parameter)*
Onova itself does not require any specific manifest name.

You can now try creating another version, by running similar command:
```
PM> Onova.Publisher --name DummyApp --version 1.3.0 --url https://dummy.com/files/ --target publish\
```
Now, you should just need to upload the new zip file and update the latest MANIFEST entry. *(unless a new version of the publisher had come out in the meantime, in which case an update for the installer may be issued)*

After everything is in place, let's try downloading and running the *webinstaller.exe* of our application.
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

# Missing (planned) features
- [ ] Changelog support
- [ ] Code signing and timestamping
- [ ] Start menu link in publisher name folder
- [ ] Run app after installation
- [ ] Powershell bindings

# Missing (not planned) features
* Bigger customization eg. icons, custom UI
* Dedicated non-web installer
* Other OS support or backporting to older .NET versions
* Bootstrapping

# Known issues
- [ ] No wiki
- [ ] No tests
- [ ] App name and URL allows only ANSI encoding
- [ ] The installer code is a bit inconsistent

# Contributions
I am open to suggestions, PRs, bug reports.
Any contribution is welcome.

# Licensing
The project uses following licensed works:

* unzip.h unzip.cpp - Lucian Wischik, Jean-Loup Gailly, Mark Adler, zlib
* WinReg.hpp - Giovanni Dicanio
* [cpr](https://whoshuu.github.io/cpr/) (installed using vcpkg)

My provided code is licensed under the MIT license.
