# ProjFS Managed API

<!--
|Branch|Functional Tests|
|:--:|:--:|
|**main**|[![Build status](https://dev.azure.com/projfs/ci/_apis/build/status/PR%20-%20Build%20and%20Functional%20Test%20-%202022?branchName=main)](https://dev.azure.com/projfs/ci/_build/latest?definitionId=7)|
-->

| | |
|---|---|
| **Package** | `Microsoft.Windows.ProjFS` |
| **Version** | 2.0.0 |
| **Targets** | netstandard2.0, net8.0, net9.0, net10.0 |
| **License** | MIT |

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
- **LibraryImport on .NET 7+** — uses source-generated P/Invoke marshalling for better AOT perf
- **DllImport fallback** — netstandard2.0 target retains traditional P/Invoke for broad compatibility
- **Same API surface** — drop-in replacement using the same `Microsoft.Windows.ProjFS` namespace
- **NuGet ready** — publishes to nuget.org as `Microsoft.Windows.ProjFS`

### Prerequisites

- [.NET 10 SDK](https://dotnet.microsoft.com/download/dotnet/10.0) (required for net10.0 TFM; also builds net8.0/net9.0/netstandard2.0)
- Windows 10 version 1809 or later with ProjFS enabled

## Solution Layout

### ProjectedFSLib.Managed project

This project contains the pure C# P/Invoke implementation of the ProjFS managed wrapper,
producing `ProjectedFSLib.Managed.dll`.  It targets netstandard2.0, net8.0, and net10.0.

The netstandard2.0 target allows use from .NET Framework 4.8 and .NET Core 3.1+ projects,
providing a migration path from the original C++/CLI package without requiring a TFM upgrade.

### P/Invoke Strategy

The library uses two P/Invoke strategies depending on the target framework:

| Target | Strategy | Benefit |
|--------|----------|---------|
| net8.0, net9.0, net10.0 | `LibraryImport` (source generator) | AOT-compatible, no runtime marshalling overhead |
| netstandard2.0 | `DllImport` (traditional) | Broad compatibility (.NET Framework 4.8, .NET Core 3.1+) |

The declarations live in two partial class files:
- `ProjFSNative.cs` — shared structs/constants + DllImport fallback
- `ProjFSNative.LibraryImport.cs` — LibraryImport declarations (compiled only on .NET 7+)

### SimpleProviderManaged project

This project builds a simple ProjFS provider, `SimpleProviderManaged.exe`, that uses the managed API.
It projects the contents of one directory (the "source") into another one (the "virtualization root").

### ProjectedFSLib.Managed.Test project

This project builds an NUnit test, `ProjectedFSLib.Managed.Test.exe`, that uses the SimpleProviderManaged
provider to exercise the API wrapper.

## Building

```powershell
dotnet build ProjectedFSLib.Managed.slnx -c Release
```

Or use the build script:

```powershell
.\scripts\BuildProjFS-Managed.bat Release
```

## Running Tests

```powershell
dotnet test ProjectedFSLib.Managed.slnx -c Release
```

Or use the test script:

```powershell
.\scripts\RunTests.bat Release
```

## NuGet Package

### Installing

```powershell
dotnet add package Microsoft.Windows.ProjFS
```

### Creating a Package Locally

```powershell
.\scripts\Pack-NuGet.ps1
```

This produces `.nupkg` and `.snupkg` files in `artifacts/packages/`.

Official NuGet publishing is handled by the internal CI/CD pipeline.

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

