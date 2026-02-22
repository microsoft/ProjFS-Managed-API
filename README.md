# ProjFS Managed API

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

ProjFS enablement is **required** for this library to work correctly.  ProjFS ships as an
[optional component](https://docs.microsoft.com/en-us/windows/desktop/projfs/enabling-windows-projected-file-system)
starting in Windows 10 version 1809.

To enable ProjFS, run the following in an elevated PowerShell prompt:

```powershell
Enable-WindowsOptionalFeature -Online -FeatureName Client-ProjFS -NoRestart
```

## About the ProjFS Managed API

The Windows SDK contains a native C API for ProjFS.  The ProjFS Managed API provides a pure C#
P/Invoke wrapper around the native API so that developers can write ProjFS providers using managed code.

This is a complete rewrite of the original C++/CLI wrapper.  Key improvements:

- **No C++ toolchain required** — builds with `dotnet build`, no Visual Studio C++ workload needed
- **NativeAOT compatible** — fully supports ahead-of-time compilation and trimming
- **Cross-compilation friendly** — can be built on any machine with the .NET SDK
- **Same API surface** — drop-in replacement using the same `Microsoft.Windows.ProjFS` namespace

### Prerequisites

- [.NET 8 SDK](https://dotnet.microsoft.com/download/dotnet/8.0) or later
- Windows 10 version 1809 or later with ProjFS enabled

## Solution Layout

### ProjectedFSLib.Managed.CSharp project

This project contains the pure C# P/Invoke implementation of the ProjFS managed wrapper,
producing `ProjectedFSLib.Managed.dll`.  It targets net8.0 and net10.0.

### SimpleProviderManaged project

This project builds a simple ProjFS provider, `SimpleProviderManaged.exe`, that uses the managed API.
It projects the contents of one directory (the "source") into another one (the "virtualization root").

### ProjectedFSLib.Managed.Test project

This project builds an NUnit test, `ProjectedFSLib.Managed.Test.exe`, that uses the SimpleProviderManaged
provider to exercise the API wrapper.

## Building

```bash
dotnet build ProjectedFSLib.Managed.sln -c Release
```

Or use the build script:

```bash
scripts\BuildProjFS-Managed.bat Release
```

## Running Tests

```bash
dotnet test ProjectedFSLib.Managed.sln -c Release
```

Or use the test script:

```bash
scripts\RunTests.bat Release
```

**Note:** The Windows Projected File System optional component must be enabled before
you can run SimpleProviderManaged.exe or a provider of your own devising.  Refer to
[Enabling ProjFS](#enabling-projfs) above for instructions.

### Known Filesystem Limitations

**Symlink placeholders require NTFS.**  ProjFS symlink support (`WritePlaceholderInfo2`,
`PrjFillDirEntryBuffer2` with `PRJ_EXT_INFO_TYPE_SYMLINK`) uses the NTFS atomic create
ECP internally.  **ReFS does not support this**, and `PrjWritePlaceholderInfo2` will return
`HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED)` (`0x80070032`) when the virtualization root is on
a ReFS volume.  Non-symlink operations (regular placeholders, file hydration, directory
enumeration, notifications) work correctly on both NTFS and ReFS.

If you encounter `ERROR_NOT_SUPPORTED` from `WritePlaceholderInfo2`, verify the virtualization
root is on an NTFS volume:

```powershell
Get-Volume -DriveLetter D | Select-Object FileSystem  # Should be "NTFS"
```

## Contributing

For details on how to contribute to this project, see the CONTRIBUTING.md file in this repository.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
