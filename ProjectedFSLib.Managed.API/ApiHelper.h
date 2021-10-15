// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

namespace Microsoft {
namespace Windows {
namespace ProjFS {
/// <summary>Helper class for using the correct native APIs in the managed layer.</summary>
/// <remarks>
/// <para>
/// The final ProjFS native APIs released in Windows 10 version 1809 differ from the now-deprecated
/// beta APIs released in Windows 10 version 1803.  In 1809 the beta APIs are still exported from
/// ProjectedFSLib.dll, in case an experimental provider written against the native 1803 APIs is run
/// on 1809.
/// </para>
/// <para>
/// This managed API wrapper is meant to be usable on 1803 and later, so it is able to use the
/// beta 1803 native APIs and the final 1809 native APIs.  Since the 1809 APIs are not present on
/// 1803, and because we intend to remove the beta 1803 APIs from a later version of Windows, we
/// dynamically load the native APIs here.  If we didn't do that then trying to use this managed
/// wrapper on a version of Windows missing one or the other native API would result in the program
/// dying on startup with an unhandled <c>System::IO::FileLoadException</c>: "A procedure
/// imported by 'ProjectedFSLib.Managed.dll' could not be loaded."
/// </para>
/// <para>
/// It is likely that at some point after removing the beta 1803 native APIs from Windows we will
/// also remove support for them from this managed wrapper.
/// </para>
/// </remarks>
ref class ApiHelper {
internal:
    ApiHelper();

    property bool UseRS5Api
    {
        bool get(void);
    };

private:

#pragma region Signatures for Windows 10 version 1809 APIs

    typedef HRESULT (__stdcall* t_PrjStartVirtualizing)(
        _In_ PCWSTR virtualizationRootPath,
        _In_ const PRJ_CALLBACKS* callbacks,
        _In_opt_ const void* instanceContext,
        _In_opt_ const PRJ_STARTVIRTUALIZING_OPTIONS* options,
        _Outptr_ PRJ_NAMESPACE_VIRTUALIZATION_CONTEXT* namespaceVirtualizationContext
        );

    typedef void (__stdcall* t_PrjStopVirtualizing)(
        _In_ PRJ_NAMESPACE_VIRTUALIZATION_CONTEXT namespaceVirtualizationContext
        );

    typedef HRESULT (__stdcall* t_PrjWriteFileData)(
        _In_ PRJ_NAMESPACE_VIRTUALIZATION_CONTEXT namespaceVirtualizationContext,
        _In_ const GUID* dataStreamId,
        _In_reads_bytes_(length) void* buffer,
        _In_ UINT64 byteOffset,
        _In_ UINT32 length
        );

    typedef HRESULT (__stdcall* t_PrjWritePlaceholderInfo)(
        _In_ PRJ_NAMESPACE_VIRTUALIZATION_CONTEXT namespaceVirtualizationContext,
        _In_ PCWSTR destinationFileName,
        _In_reads_bytes_(placeholderInfoSize) const PRJ_PLACEHOLDER_INFO* placeholderInfo,
        _In_ UINT32 placeholderInfoSize
        );

    typedef HRESULT(__stdcall* t_PrjWritePlaceholderInfo2)(
        _In_ PRJ_NAMESPACE_VIRTUALIZATION_CONTEXT namespaceVirtualizationContext,
        _In_ PCWSTR destinationFileName,
        _In_reads_bytes_(placeholderInfoSize) const PRJ_PLACEHOLDER_INFO* placeholderInfo,
        _In_ UINT32 placeholderInfoSize,
        _In_opt_ const PRJ_EXTENDED_INFO* ExtendedInfo
        );

    typedef void* (__stdcall* t_PrjAllocateAlignedBuffer)(
        _In_ PRJ_NAMESPACE_VIRTUALIZATION_CONTEXT namespaceVirtualizationContext,
        _In_ size_t size
        );

    typedef void (__stdcall* t_PrjFreeAlignedBuffer)(
        _In_ void* buffer
        );

    typedef HRESULT (__stdcall* t_PrjGetVirtualizationInstanceInfo)(
        _In_ PRJ_NAMESPACE_VIRTUALIZATION_CONTEXT namespaceVirtualizationContext,
        _Out_ PRJ_VIRTUALIZATION_INSTANCE_INFO* virtualizationInstanceInfo
        );

    typedef HRESULT (__stdcall* t_PrjUpdateFileIfNeeded)(
        _In_ PRJ_NAMESPACE_VIRTUALIZATION_CONTEXT namespaceVirtualizationContext,
        _In_ PCWSTR destinationFileName,
        _In_reads_bytes_(placeholderInfoSize) const PRJ_PLACEHOLDER_INFO* placeholderInfo,
        _In_ UINT32 placeholderInfoSize,
        _In_opt_ PRJ_UPDATE_TYPES updateFlags,
        _Out_opt_ PRJ_UPDATE_FAILURE_CAUSES* failureReason
        );

    typedef HRESULT (__stdcall* t_PrjMarkDirectoryAsPlaceholder)(
        _In_ PCWSTR rootPathName,
        _In_opt_ PCWSTR targetPathName,
        _In_opt_ const PRJ_PLACEHOLDER_VERSION_INFO* versionInfo,
        _In_ const GUID* virtualizationInstanceID
        );

#pragma endregion

#pragma region Signatures for deprecated Windows 10 version 1803

    typedef HRESULT (__stdcall* t_PrjStartVirtualizationInstance)(
        _In_ LPCWSTR VirtualizationRootPath,
        _In_ PPRJ_COMMAND_CALLBACKS Callbacks,
        _In_opt_ DWORD Flags,
        _In_opt_ DWORD GlobalNotificationMask,
        _In_opt_ DWORD PoolThreadCount,
        _In_opt_ DWORD ConcurrentThreadCount,
        _In_opt_ PVOID InstanceContext,
        _Outptr_ PRJ_VIRTUALIZATIONINSTANCE_HANDLE* VirtualizationInstanceHandle
        );

    typedef HRESULT (__stdcall* t_PrjStartVirtualizationInstanceEx)(
        _In_ LPCWSTR VirtualizationRootPath,
        _In_ PPRJ_COMMAND_CALLBACKS Callbacks,
        _In_opt_ PVOID InstanceContext,
        _In_opt_ PVIRTUALIZATION_INST_EXTENDED_PARAMETERS ExtendedParameters,
        _Outptr_ PRJ_VIRTUALIZATIONINSTANCE_HANDLE* VirtualizationInstanceHandle
        );

    typedef HRESULT (__stdcall* t_PrjStopVirtualizationInstance)(
        _In_ PRJ_VIRTUALIZATIONINSTANCE_HANDLE VirtualizationInstanceHandle
        );

    typedef HRESULT (__stdcall* t_PrjGetVirtualizationInstanceIdFromHandle)(
        _In_ PRJ_VIRTUALIZATIONINSTANCE_HANDLE VirtualizationInstanceHandle,
        _Out_ LPGUID VirtualizationInstanceID
        );

    typedef HRESULT (__stdcall* t_PrjConvertDirectoryToPlaceholder)(
        _In_ LPCWSTR RootPathName,
        _In_ LPCWSTR TargetPathName,
        _In_opt_ PRJ_PLACEHOLDER_VERSION_INFO* VersionInfo,
        _In_opt_ DWORD Flags,
        _In_ LPCGUID VirtualizationInstanceID
        );

    typedef HRESULT (__stdcall* t_PrjWritePlaceholderInformation)(
        _In_ PRJ_VIRTUALIZATIONINSTANCE_HANDLE VirtualizationInstanceHandle,
        _In_ LPCWSTR DestinationFileName,
        _In_ PPRJ_PLACEHOLDER_INFORMATION PlaceholderInformation,
        _In_ DWORD Length
        );

    typedef HRESULT (__stdcall* t_PrjUpdatePlaceholderIfNeeded)(
        _In_ PRJ_VIRTUALIZATIONINSTANCE_HANDLE VirtualizationInstanceHandle,
        _In_ LPCWSTR DestinationFileName,
        _In_ PPRJ_PLACEHOLDER_INFORMATION PlaceholderInformation,
        _In_ DWORD Length,
        _In_opt_ DWORD UpdateFlags,
        _Out_opt_ PDWORD FailureReason
        );

    typedef HRESULT (__stdcall* t_PrjWriteFile)(
        _In_ PRJ_VIRTUALIZATIONINSTANCE_HANDLE VirtualizationInstanceHandle,
        _In_ const GUID* StreamId,
        _In_reads_bytes_(Length) void* Buffer,
        _In_ UINT64 ByteOffset,
        _In_ UINT32 Length
        );

    typedef HRESULT (__stdcall* t_PrjCommandCallbacksInit)(
        _In_ DWORD CallbacksSize,
        _Out_writes_bytes_(CallbacksSize) PPRJ_COMMAND_CALLBACKS Callbacks
        );

#pragma endregion

    bool useRS5Api;

internal:

    // 1809 API
    t_PrjStartVirtualizing _PrjStartVirtualizing = nullptr;
    t_PrjStopVirtualizing _PrjStopVirtualizing = nullptr;
    t_PrjWriteFileData _PrjWriteFileData = nullptr;
    t_PrjWritePlaceholderInfo _PrjWritePlaceholderInfo = nullptr;
    t_PrjWritePlaceholderInfo2 _PrjWritePlaceholderInfo2 = nullptr;
    t_PrjAllocateAlignedBuffer _PrjAllocateAlignedBuffer = nullptr;
    t_PrjFreeAlignedBuffer _PrjFreeAlignedBuffer = nullptr;
    t_PrjGetVirtualizationInstanceInfo _PrjGetVirtualizationInstanceInfo = nullptr;
    t_PrjUpdateFileIfNeeded _PrjUpdateFileIfNeeded = nullptr;
    t_PrjMarkDirectoryAsPlaceholder _PrjMarkDirectoryAsPlaceholder = nullptr;

    // 1803 API
    t_PrjStartVirtualizationInstance _PrjStartVirtualizationInstance = nullptr;
    t_PrjStartVirtualizationInstanceEx _PrjStartVirtualizationInstanceEx = nullptr;
    t_PrjStopVirtualizationInstance _PrjStopVirtualizationInstance = nullptr;
    t_PrjGetVirtualizationInstanceIdFromHandle _PrjGetVirtualizationInstanceIdFromHandle = nullptr;
    t_PrjConvertDirectoryToPlaceholder _PrjConvertDirectoryToPlaceholder = nullptr;
    t_PrjWritePlaceholderInformation _PrjWritePlaceholderInformation = nullptr;
    t_PrjUpdatePlaceholderIfNeeded _PrjUpdatePlaceholderIfNeeded = nullptr;
    t_PrjWriteFile _PrjWriteFile = nullptr;
    t_PrjCommandCallbacksInit _PrjCommandCallbacksInit = nullptr;
};

}}} // namespace Microsoft.Windows.ProjFS