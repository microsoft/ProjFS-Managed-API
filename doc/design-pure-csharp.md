# Design: Pure C# ProjFS Managed API

## Summary

This document describes the design and motivation for replacing the C++/CLI mixed-mode
`ProjectedFSLib.Managed.dll` with a pure C# P/Invoke implementation that maintains
100% API compatibility while enabling NativeAOT compilation, trimming, and simplified builds.

## Background

The original ProjFS Managed API is a C++/CLI mixed-mode assembly that wraps the native
`ProjectedFSLib.dll` APIs. While functional, this approach has several limitations:

| Concern | C++/CLI | Pure C# |
|---------|---------|---------|
| Build toolchain | Requires VS C++ workload + C++/CLI support | `dotnet build` only |
| Runtime dependency | Visual C++ redistributable (Ijwhost.dll) | None |
| NativeAOT | Not supported (mixed-mode incompatible) | Fully supported |
| Trimming | Not supported | `IsAotCompatible=true` |
| Cross-compilation | Requires matching native toolchain | Any machine with .NET SDK |
| TFMs | net48, netcoreapp3.1 | net8.0, net9.0, net10.0+ |
| Maintenance | Dual C++/C# expertise needed | C# only |

## Design Goals

1. **API compatibility** — Same `Microsoft.Windows.ProjFS` namespace, same types, same signatures
2. **Drop-in replacement** — Consumers change only the project reference, no code changes
3. **AOT-safe** — No reflection, no dynamic code generation, `IsAotCompatible=true`
4. **Correctness** — Byte-identical struct layouts verified against Windows SDK headers
5. **Complete feature coverage** — All v1809 APIs + v2004 symlink APIs

## Architecture

### P/Invoke Layer (`ProjFSNative.cs`)

Direct `[DllImport]` declarations for all `ProjectedFSLib.dll` exports:

- Core: `PrjStartVirtualizing`, `PrjStopVirtualizing`
- Placeholders: `PrjWritePlaceholderInfo`, `PrjWritePlaceholderInfo2`, `PrjUpdateFileIfNeeded`
- Enumeration: `PrjFillDirEntryBuffer`, `PrjFillDirEntryBuffer2`
- Data: `PrjWriteFileData`, `PrjAllocateAlignedBuffer`, `PrjFreeAlignedBuffer`
- Utilities: `PrjFileNameMatch`, `PrjFileNameCompare`, `PrjDoesNameContainWildCards`

All native structs use `[StructLayout(LayoutKind.Sequential)]` with exact field-level
padding verified against `sizeof()` and `offsetof()` from the Windows SDK:

| Struct | C sizeof | C# Marshal.SizeOf |
|--------|----------|--------------------|
| `PRJ_FILE_BASIC_INFO` | 56 | 56 |
| `PRJ_PLACEHOLDER_INFO` | 344 | 344 |
| `PRJ_EXTENDED_INFO` | 16 | 16 |
| `PRJ_CALLBACK_DATA` | 96 | 96 |
| `PRJ_CALLBACKS` | 64 | 64 |
| `PRJ_STARTVIRTUALIZING_OPTIONS` | 32 | 32 |
| `PRJ_NOTIFICATION_MAPPING` | 16 | 16 |

**Key lesson learned:** `PRJ_PLACEHOLDER_INFO` includes a `UINT8 VariableData[1]` flexible
array member at offset 336. Omitting this field causes `Marshal.SizeOf` to return 336 instead
of the native 344, and `PrjWritePlaceholderInfo` returns `ERROR_INSUFFICIENT_BUFFER`.

### Callback Routing (`VirtualizationInstance.cs`)

Native ProjFS callbacks (registered via `PRJ_CALLBACKS` function pointers) are routed to
managed code through `[UnmanagedFunctionPointer(CallingConvention.StdCall)]` delegates.
The instance context passed to `PrjStartVirtualizing` is a `GCHandle` that allows the
static callback methods to recover the `VirtualizationInstance` object.

### Constructor Behavior

The constructor matches C++/CLI behavior exactly:
1. Creates the virtualization root directory if it doesn't exist
2. Checks for existing ProjFS reparse point via `PrjGetOnDiskFileState`
3. Marks the directory as a virtualization root via `PrjMarkDirectoryAsPlaceholder`
4. Throws `Win32Exception` on failure

### String Marshaling

All `PCWSTR` parameters in ProjFS structs are passed as `IntPtr` with
`Marshal.StringToHGlobalUni()`. Using `GCHandle.Alloc(string, GCHandleType.Pinned)`
is **incorrect** — `AddrOfPinnedObject()` on a pinned string returns the object header
address, not the character data pointer.

### Notification Mapping Lifetime

Notification mapping data (the `PRJ_NOTIFICATION_MAPPING` array and the `NotificationRoot`
string pointers) must remain valid for the lifetime of the virtualization instance.
ProjFS may cache these pointers internally. They are freed only in `StopVirtualizing`.

## Known Limitations

### ReFS Symlink Restriction

ProjFS symlink placeholders (`WritePlaceholderInfo2`, `PrjFillDirEntryBuffer2` with
`PRJ_EXT_INFO_TYPE_SYMLINK`) require an **NTFS** volume. The ProjFS kernel minifilter
(`PrjFlt.sys`) creates symlinks via the NTFS atomic create ECP (`GUID_ECP_ATOMIC_CREATE`),
which ReFS does not support. On ReFS volumes, `PrjWritePlaceholderInfo2` returns
`ERROR_NOT_SUPPORTED` (`0x80070032`).

This was diagnosed via WinDbg: the user-mode `ProjectedFSLib.dll` correctly sends a
`FilterSendMessage` to the kernel, but `PrjFlt.sys` returns `STATUS_NOT_SUPPORTED`
(`0xC00000BB`) because `FltCreateFileEx2` with the symlink atomic create ECP fails on ReFS.

Non-symlink operations work correctly on both NTFS and ReFS.

### No Windows 10 1803 Beta API Support

The pure C# implementation targets the v1809 (final) ProjFS API only. The v1803 beta API
(`PrjStartVirtualizationInstance`, `PrjWritePlaceholderInformation`, etc.) is not supported.
This matches the minimum supported Windows version for ProjFS as a shipped component.

## Test Results

All 16 tests pass on both net8.0 and net10.0:

- 10 core tests: placeholder creation, file hydration, directory enumeration, notifications
- 6 symlink tests: file symlinks, directory symlinks, relative paths (require NTFS + elevation)

## Migration Guide

To switch from the C++/CLI package to the pure C# implementation:

1. Remove the `Microsoft.Windows.ProjFS` NuGet package reference
2. Add a project reference to `ProjectedFSLib.Managed.CSharp.csproj`
3. Remove `<UseIJWHost>True</UseIJWHost>` from your project file (no longer needed)
4. Remove the Visual C++ redistributable from your installer
5. No code changes required — same namespace, same API surface
