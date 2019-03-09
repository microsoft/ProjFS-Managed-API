# ProjFS Managed API
|Branch|Functional Tests|
|:--:|:--:|
|**master**|[![Build status](https://dev.azure.com/projfs/ci/_apis/build/status/PR%20-%20Build%20and%20Functional%20Test%20-%202019?branchName=master)](https://dev.azure.com/projfs/ci/_build/latest?definitionId=5)|
## What is ProjFS?

ProjFS is short for Windows Projected File System.  ProjFS allows a user-mode application called a
"provider" to project hierarchical data into the file system, making it appear as files and directories
in the file system. For example, a simple provider could project the Windows registry into the file
system, making registry keys and values appear as files and directories, respectively. An example of
a more complex provider is [VFS for Git](https://github.com/Microsoft/VFSForGit), used to virtualize
very large git repos.

Conceptual documentation for ProjFS along with documentation of its Win32 API is at
[docs.microsoft.com](https://docs.microsoft.com/en-us/windows/desktop/projfs/projected-file-system).

ProjFS ships as an [optional component](https://docs.microsoft.com/en-us/windows/desktop/projfs/enabling-windows-projected-file-system)
starting in Windows 10 version 1809.

## What is the ProjFS Managed API?

The Windows SDK contains a native C API for ProjFS.  The ProjFS Managed API provides a wrapper around
the native API so that developers can write ProjFS providers using managed code.

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

* Install Visual Studio 2017 Community Edition or higher (https://www.visualstudio.com/downloads/).
  * Include the following workloads: 
    * **.NET desktop development**
    * **.NET Core cross-platform development**
    * **Desktop development with C++**
  * Include the following individual components:
    * **.NET Framework 4.6.1 SDK**
    * **C++/CLI support**
    * **Windows 10 SDK (10.0.17763.0)**
* Create a folder to clone into, e.g. `C:\Repos\ProjFS-Managed`
* Clone this repo into the `src` subfolder, e.g. `C:\Repos\ProjFS-Managed\src`
* Run `src\scripts\BuildProjFS-Managed.bat`
  * You can also build in Visual Studio by opening `src\ProjectedFSLib.Managed.sln` and building.

The build outputs will be placed under a `BuildOutput` subfolder, e.g. `C:\Repos\ProjFS-Managed\BuildOutput`.

**Note:** The Windows Projected File System optional component must be enabled in Windows before
you can run SimpleProviderManaged.exe or a provider of your own devising.  Refer to
[this page](https://docs.microsoft.com/en-us/windows/desktop/projfs/enabling-windows-projected-file-system)
for instructions.

# Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
