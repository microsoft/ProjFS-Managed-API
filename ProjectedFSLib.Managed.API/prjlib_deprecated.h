/*++

Copyright (c) Microsoft Corporation.
Licensed under the MIT license.


Module Name:

    prjlib_deprecated.h

Abstract:

   This module defines pre-release ProjFS constants, data structures, and functions that were deprecated
   in Windows 10 version 1809.  We keep their definitions here to separate them from the final public
   API which is available in the Windows 10 SDK.  To use the final public API #include ProjectedFSLib.h
   and link against ProjectedFSLib.lib.

   The structures and APIs declared in this file will be removed from a future version of Windows.

--*/

#ifndef PRJLIB_DEPRECATED_H
#define PRJLIB_DEPRECATED_H

#if _MSC_VER > 1000
#pragma once
#endif

#pragma warning(disable:4201) // nameless struct/union

#include <winapifamily.h>

#define STDAPI_DEPRECATED(_MSG) EXTERN_C __declspec(deprecated(_MSG)) HRESULT STDAPICALLTYPE

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN10_RS4)

#pragma region Common structures

typedef PRJ_NOTIFICATION PRJ_NOTIFICATION_TYPE;

typedef HANDLE PRJ_VIRTUALIZATIONINSTANCE_HANDLE;

#pragma endregion

#pragma region Virtualization instance APIs

//
//  Forward definitions.
//  

typedef struct _PRJ_COMMAND_CALLBACKS PRJ_COMMAND_CALLBACKS, *PPRJ_COMMAND_CALLBACKS;

STDAPI_DEPRECATED("Use PrjStartVirtualizing instead of PrjStartVirtualizationInstance.")
PrjStartVirtualizationInstance (
    _In_     LPCWSTR                            VirtualizationRootPath,
    _In_     PPRJ_COMMAND_CALLBACKS             Callbacks,
    _In_opt_ DWORD                              Flags,
    _In_opt_ DWORD                              GlobalNotificationMask,
    _In_opt_ DWORD                              PoolThreadCount,
    _In_opt_ DWORD                              ConcurrentThreadCount,
    _In_opt_ PVOID                              InstanceContext,
    _Outptr_ PRJ_VIRTUALIZATIONINSTANCE_HANDLE* VirtualizationInstanceHandle
    );

typedef struct _VIRTUALIZATION_INST_EXTENDED_PARAMETERS
{
    DWORD Size;
    DWORD Flags;
    DWORD PoolThreadCount;
    DWORD ConcurrentThreadCount;
    PRJ_NOTIFICATION_MAPPING* NotificationMappings;
    DWORD NumNotificationMappingsCount;
} VIRTUALIZATION_INST_EXTENDED_PARAMETERS, *PVIRTUALIZATION_INST_EXTENDED_PARAMETERS;

#define PRJ_FLAG_INSTANCE_NEGATIVE_PATH_CACHE   0x00000002

STDAPI_DEPRECATED("Use PrjStartVirtualizing instead of PrjStartVirtualizationInstanceEx.")
PrjStartVirtualizationInstanceEx (
    _In_     LPCWSTR                                    VirtualizationRootPath,
    _In_     PPRJ_COMMAND_CALLBACKS                     Callbacks,
    _In_opt_ PVOID                                      InstanceContext,
    _In_opt_ PVIRTUALIZATION_INST_EXTENDED_PARAMETERS   ExtendedParameters,
    _Outptr_ PRJ_VIRTUALIZATIONINSTANCE_HANDLE*         VirtualizationInstanceHandle
    );

STDAPI_DEPRECATED("Use PrjStopVirtualizing instead of PrjStopVirtualizationInstance.")
PrjStopVirtualizationInstance (
    _In_  PRJ_VIRTUALIZATIONINSTANCE_HANDLE   VirtualizationInstanceHandle
    );

STDAPI_DEPRECATED("Use PrjGetVirtualizationInstanceInfo instead of PrjGetVirtualizationInstanceIdFromHandle.")
PrjGetVirtualizationInstanceIdFromHandle (
    _In_  PRJ_VIRTUALIZATIONINSTANCE_HANDLE     VirtualizationInstanceHandle,
    _Out_ LPGUID                                VirtualizationInstanceID
    );

#pragma endregion

#pragma region Placeholder and File APIs

#define PRJ_FLAG_VIRTUALIZATION_ROOT   0x00000010 

STDAPI_DEPRECATED("Use PrjMarkDirectoryAsPlaceholder instead of PrjConvertDirectoryToPlaceholder.")
PrjConvertDirectoryToPlaceholder (
    _In_     LPCWSTR                       RootPathName,
    _In_     LPCWSTR                       TargetPathName,
    _In_opt_ PRJ_PLACEHOLDER_VERSION_INFO* VersionInfo,
    _In_opt_ DWORD                         Flags,
    _In_     LPCGUID                       VirtualizationInstanceID
    );

typedef struct _PRJ_EA_INFORMATION
{
    DWORD EaBufferSize;
    DWORD OffsetToFirstEa;
} PRJ_EA_INFORMATION, *PPRJ_EA_INFORMATION;

typedef struct _PRJ_SECURITY_INFORMATION
{
    DWORD SecurityBufferSize;
    DWORD OffsetToSecurityDescriptor;
} PRJ_SECURITY_INFORMATION, *PPRJ_SECURITY_INFORMATION;

typedef struct _PRJ_STREAMS_INFORMATION
{
    DWORD StreamsInfoBufferSize;
    DWORD OffsetToFirstStreamInfo;
} PRJ_STREAMS_INFORMATION, *PPRJ_STREAMS_INFORMATION;

typedef struct _PRJ_PLACEHOLDER_INFORMATION
{
    DWORD                              Size;
    PRJ_FILE_BASIC_INFO                FileBasicInfo;
    PRJ_EA_INFORMATION                 EaInformation;
    PRJ_SECURITY_INFORMATION           SecurityInformation;
    PRJ_STREAMS_INFORMATION            StreamsInformation;
    PRJ_PLACEHOLDER_VERSION_INFO       VersionInfo;
    UCHAR                              VariableData[ANYSIZE_ARRAY];
} PRJ_PLACEHOLDER_INFORMATION, *PPRJ_PLACEHOLDER_INFORMATION;

STDAPI_DEPRECATED("Use PrjWritePlaceholderInfo instead of PrjWritePlaceholderInformation.")
PrjWritePlaceholderInformation  (
    _In_    PRJ_VIRTUALIZATIONINSTANCE_HANDLE  VirtualizationInstanceHandle,
    _In_    LPCWSTR                            DestinationFileName,
    _In_    PPRJ_PLACEHOLDER_INFORMATION       PlaceholderInformation,
    _In_    DWORD                              Length
    );

STDAPI_DEPRECATED("Use PrjUpdateFileIfNeeded instead of PrjUpdatePlaceholderIfNeeded.")
PrjUpdatePlaceholderIfNeeded (
    _In_        PRJ_VIRTUALIZATIONINSTANCE_HANDLE  VirtualizationInstanceHandle,
    _In_        LPCWSTR                            DestinationFileName,
    _In_        PPRJ_PLACEHOLDER_INFORMATION       PlaceholderInformation,
    _In_        DWORD                              Length,
    _In_opt_    DWORD                              UpdateFlags,
    _Out_opt_   PDWORD                             FailureReason
    );

STDAPI_DEPRECATED("Use PrjWriteFileData instead of PrjWriteFile.")
PrjWriteFile (
    _In_ PRJ_VIRTUALIZATIONINSTANCE_HANDLE VirtualizationInstanceHandle,
    _In_ const GUID* StreamId,
    _In_reads_bytes_(Length) void* Buffer,
    _In_ UINT64 ByteOffset,
    _In_ UINT32 Length
    );

#pragma endregion

#pragma region Callback support

STDAPI_DEPRECATED("PrjCommandCallbacksInit is deprecated and will not exist in future versions of Windows.")
PrjCommandCallbacksInit (
    _In_                                DWORD                   CallbacksSize,
    _Out_writes_bytes_(CallbacksSize)   PPRJ_COMMAND_CALLBACKS  Callbacks
    );

typedef
HRESULT 
(CALLBACK PRJ_GET_PLACEHOLDER_INFORMATION_CB) (
    _In_      PRJ_CALLBACK_DATA*                 CallbackData,
    _In_      DWORD                              DesiredAccess,
    _In_      DWORD                              ShareMode,
    _In_      DWORD                              CreateDisposition,
    _In_      DWORD                              CreateOptions,
    _In_      LPCWSTR                            DestinationFileName
    );

typedef
HRESULT
(CALLBACK PRJ_GET_FILE_STREAM_CB) (
    _In_      PRJ_CALLBACK_DATA*                 CallbackData,
    _In_      LARGE_INTEGER                      ByteOffset,
    _In_      DWORD                              Length
    );

typedef union _PRJ_OPERATION_PARAMETERS {

    struct {
        DWORD  DesiredAccess;
        DWORD  ShareMode;
        DWORD  CreateDisposition;
        DWORD  CreateOptions;
        DWORD  IoStatusInformation;
        DWORD  NotificationMask;
    } PostCreate;

    struct {
        DWORD                          NotificationMask;
    } FileRenamed;

    struct {
        BOOLEAN IsFileModified;
    } FileDeletedOnHandleClose;

} PRJ_OPERATION_PARAMETERS, *PPRJ_OPERATION_PARAMETERS;

typedef
HRESULT 
(CALLBACK PRJ_NOTIFY_OPERATION_CB) (
    _In_ PRJ_CALLBACK_DATA*                 CallbackData,
    _In_ BOOLEAN                            IsDirectory,
    _In_ PRJ_NOTIFICATION_TYPE              NotificationType,
    _In_opt_ LPCWSTR                        DestinationFileName,
    _Inout_ PPRJ_OPERATION_PARAMETERS       OperationParameters
    );

typedef struct _PRJ_COMMAND_CALLBACKS
{

    //
    //  Size of this structure.  Initialized by PrjCommandCallbacksInit().
    //  

    DWORD                                        Size;

    //
    //  The provider must implement the following callbacks.
    //

    PRJ_START_DIRECTORY_ENUMERATION_CB*           PrjStartDirectoryEnumeration;

    PRJ_END_DIRECTORY_ENUMERATION_CB*             PrjEndDirectoryEnumeration;

    PRJ_GET_DIRECTORY_ENUMERATION_CB*             PrjGetDirectoryEnumeration;

    PRJ_GET_PLACEHOLDER_INFORMATION_CB*           PrjGetPlaceholderInformation;

    PRJ_GET_FILE_STREAM_CB*                       PrjGetFileStream;

    //
    //  Optional.  If the provider does not implement this callback, ProjFS will invoke the directory
    //  enumeration callbacks to determine the existence of a file path in the provider's store.
    //

    PRJ_QUERY_FILE_NAME_CB*                       PrjQueryFileName;

    //
    //  Optional.  If the provider does not implement this callback, it will not get any notifications
    //  from ProjFS.
    //

    PRJ_NOTIFY_OPERATION_CB*                      PrjNotifyOperation;

    //
    //  Optional.  If the provider does not implement this callback, operations in ProjFS will not
    //  be able to be canceled.
    //

    PRJ_CANCEL_COMMAND_CB*                        PrjCancelCommand;

} PRJ_COMMAND_CALLBACKS, *PPRJ_COMMAND_CALLBACKS;

#pragma endregion

#endif // _WIN32_WINNT >= _WIN32_WINNT_WIN10_RS4
#endif // WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#endif // PRJLIB_DEPRECATED_H
