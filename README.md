# ProjFS Managed API

|Branch|Functional Tests|
|:--:|:--:|
|**main**|[![Build status](https://dev.azure.com/projfs/ci/_apis/build/status/PR%20-%20Build%20and%20Functional%20Test%20-%202019?branchName=main)](https://dev.azure.com/projfs/ci/_build/latest?definitionId=5)|
|**release**|[![Build status](https://dev.azure.com/microsoft/OS/_apis/build/status/ProjFS%20CI%20-%20Build,%20Sign,%20Package)](https://dev.azure.com/microsoft/OS/_build/latest?definitionId=37476)|

## About ProjFS

ProjFS is short for Windows Projected File System.  ProjFS allows a user-mode application called a
"provider" to project hierarchical data into the file system, making it appear as files and directories
in the file system. For example, a simple provider could project the Windows registry into the file
system, making registry keys and values appear as files and directories, respectively. An example of
a more complex provider is [VFS for Git](https://github.com/Microsoft/VFSForGit), used to virtualize
very large git repos.

Conceptual documentation for ProjFS along with documentation of its Win32 API is at
[docs.microsoft.com](https://docs.microsoft.com/en-us/windows/desktop/projfs/projected-file-system).

## Enabling ProjFS

ProjFS enablement is **required** for this library to work correctly. ProjFS ships as an [optional component](https://docs.microsoft.com/en-us/windows/desktop/projfs/enabling-windows-projected-file-system) starting in Windows 10 version 1809. 

## About the ProjFS Managed API

The Windows SDK contains a native C API for ProjFS.  The ProjFS Managed API provides a wrapper around
the native API so that developers can write ProjFS providers using managed code.

Note that to use this library on a computer that does not have Visual Studio installed, you must install the [Visual C++ redistributable](https://visualstudio.microsoft.com/downloads/#microsoft-visual-c-redistributable-for-visual-studio-2019). This applies whether you build this source code yourself or use the pre-compiled library [available from nuget.org](https://www.nuget.org/packages/Microsoft.Windows.ProjFS/).

## Solution Layout

### ProjectedFSLib.Managed project

This project contains the code that builds the API wrapper, ProjectedFSLib.Managed.dll.  It is in the
ProjectedFSLib.Managed.API directory.

### SimpleProviderManaged project

This project builds a simple ProjFS provider, SimpleProviderManaged.exe, that uses the managed API.
It projects the contents of one directory (the "source") into another one (the "virtualization root").

### ProjectedFSLib.Managed.Test project

This project builds an NUnit test, ProjectedFSLib.Managed.Test.exe, that uses the SimpleProviderManaged
provider to exercise the API wrapper.  The managed API is a fairly thin wrapper around the native API,
and the native API has its own much more comprehensive tests that are routinely executed at Microsoft
in the normal course of OS development.  So this managed test is just a basic functional test to get
coverage of the managed wrapper API surface.

## Building the ProjFS Managed API

* Install [Visual Studio 2019 Community Edition](https://www.visualstudio.com/downloads/) version 16.4 or higher.
  * Include the following workloads:
    * **.NET desktop development**
    * **.NET Core cross-platform development**
    * **Desktop development with C++**
  * Include the following individual components:
    * **.NET Framework 4.8 SDK**
    * **C++/CLI support**
    * **Windows 10 SDK (10.0.19041.0)**
* Create a folder to clone into, e.g. `C:\Repos\ProjFS-Managed`
* Clone this repo into the `src` subfolder, e.g. `C:\Repos\ProjFS-Managed\src`
* Run `src\scripts\BuildProjFS-Managed.bat`
  * You can also build in Visual Studio by opening `src\ProjectedFSLib.Managed.sln` and building.

The build outputs will be placed under a `BuildOutput` subfolder, e.g. `C:\Repos\ProjFS-Managed\BuildOutput`.

**Note:** The Windows Projected File System optional component must be enabled in Windows before
you can run SimpleProviderManaged.exe or a provider of your own devising.  Refer to
[this page](https://docs.microsoft.com/en-us/windows/desktop/projfs/enabling-windows-projected-file-system)
for instructions.

### Dealing with BadImageFormatExceptions
The simplest cause for BadImageFormatExceptions is that you still need to [enable ProjFS](#enabling-projfs).

For .Net Core specific consumers, this can also occur when the .NET Core loader attempts to find Ijwhost.dll from the .NET Core runtime. To force this to be deployed with your application under MSBuild, add the following property to each csproj file that is importing the Microsoft.Windows.ProjFS package:

    <PropertyGroup>
      <UseIJWHost>True</UseIJWHost>
    </PropertyGroup>


## Contributing

For details on how to contribute to this project, see the CONTRIBUTING.md file in this repository.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
