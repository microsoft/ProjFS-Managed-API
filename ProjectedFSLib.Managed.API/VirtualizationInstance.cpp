// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "stdafx.h"
#include "VirtualizationInstance.h"
#include "Utils.h"

using namespace System;
using namespace System::ComponentModel;
using namespace System::Globalization;
using namespace System::IO;
using namespace Microsoft::Windows::ProjFS;

namespace {
#pragma region Prototypes

_Function_class_(PRJ_START_DIRECTORY_ENUMERATION_CB)
HRESULT PrjStartDirectoryEnumerationCB(
    _In_ PRJ_CALLBACK_DATA*               callbackData,
    _In_ LPCGUID                          enumerationId);

_Function_class_(PRJ_END_DIRECTORY_ENUMERATION_CB)
HRESULT PrjEndDirectoryEnumerationCB(
    _In_ PRJ_CALLBACK_DATA*               callbackData,
    _In_ LPCGUID                          enumerationId);

_Function_class_(PRJ_GET_DIRECTORY_ENUMERATION_CB)
HRESULT PrjGetDirectoryEnumerationCB(
    _In_       PRJ_CALLBACK_DATA*          callbackData,
    _In_       LPCGUID                     enumerationId,
    _In_opt_z_ LPCWSTR                     searchExpression,
    _In_       PRJ_DIR_ENTRY_BUFFER_HANDLE dirEntryBufferHandle);

_Function_class_(PRJ_QUERY_FILE_NAME_CB)
HRESULT PrjQueryFileNameCB(
    _In_     PRJ_CALLBACK_DATA*           callbackData);

_Function_class_(PRJ_GET_PLACEHOLDER_INFO_CB)
HRESULT PrjGetPlaceholderInfoCB(
    _In_ const PRJ_CALLBACK_DATA*         callbackData);

_Function_class_(PRJ_GET_FILE_DATA_CB)
HRESULT PrjGetFileDataCB(
    _In_ const PRJ_CALLBACK_DATA*         callbackData,
    _In_ UINT64                           byteOffset,
    _In_ UINT32                           length);

_Function_class_(PRJ_NOTIFICATION_CB)
HRESULT PrjNotificationCB(
    _In_     const PRJ_CALLBACK_DATA*     callbackData,
    _In_     BOOLEAN                      isDirectory,
    _In_     PRJ_NOTIFICATION             notification,
    _In_opt_ PCWSTR                       destinationFileName,
    _Inout_  PRJ_NOTIFICATION_PARAMETERS* operationParameters);

_Function_class_(PRJ_CANCEL_COMMAND_CB)
void PrjCancelCommandCB(
    _In_     PRJ_CALLBACK_DATA*           callbackData);

#pragma region Windows 10 1803 support

HRESULT PrjGetPlaceholderInformationCB(
    _In_ PRJ_CALLBACK_DATA*               callbackData,
    _In_ DWORD                            desiredAccess,
    _In_ DWORD                            shareMode,
    _In_ DWORD                            createDisposition,
    _In_ DWORD                            createOptions,
    _In_ LPCWSTR                          destinationFileName);

HRESULT PrjGetFileStreamCB(
    _In_ PRJ_CALLBACK_DATA*               callbackData,
    _In_ LARGE_INTEGER                    byteOffset,
    _In_ DWORD                            length);

HRESULT PrjNotifyOperationCB(
    _In_     PRJ_CALLBACK_DATA*           callbackData,
    _In_     BOOLEAN                      isDirectory,
    _In_     PRJ_NOTIFICATION_TYPE        notificationType,
    _In_opt_ LPCWSTR                      destinationFileName,
    _Inout_  PRJ_OPERATION_PARAMETERS*    operationParameters);

std::shared_ptr<PRJ_PLACEHOLDER_INFORMATION> CreatePlaceholderInformation(
    DateTime creationTime,
    DateTime lastAccessTime,
    DateTime lastWriteTime,
    DateTime changeTime,
    FileAttributes fileAttributes,
    long long endOfFile,
    bool directory,
    array<Byte>^ contentId,
    array<Byte>^ providerId);

#pragma endregion

#pragma region Utility routines

array<Byte>^ MarshalPlaceholderId(UCHAR* sourceId);

void CopyPlaceholderId(UCHAR* destinationId, array<Byte>^ contentId);

bool IsPowerOf2(unsigned long num);

Guid GUIDtoGuid(const GUID& guid);

std::shared_ptr<PRJ_PLACEHOLDER_INFO> CreatePlaceholderInfo(
    DateTime creationTime,
    DateTime lastAccessTime,
    DateTime lastWriteTime,
    DateTime changeTime,
    FileAttributes fileAttributes,
    long long endOfFile,
    bool directory,
    array<Byte>^ contentId,
    array<Byte>^ providerId);

String^ GetTriggeringProcessNameSafe(const PRJ_CALLBACK_DATA* callbackData);

#pragma endregion

#pragma endregion

// Converts a strongly typed enum to its underlying type
template <typename T>
constexpr std::underlying_type_t<T> CastToUnderlyingType(T e) noexcept
{
    return static_cast<std::underlying_type_t<T>>(e);
}

// Converts a Win32-derived HRESULT back to a Win32 error code.  Note that the general WIN32_FROM_HRESULT
// is not possible to make.  See https://blogs.msdn.microsoft.com/oldnewthing/20061103-07/?p=29133
_Success_(return)
bool Win32FromHRESULT(
    _In_ HRESULT hr,
    _Out_ DWORD* win32Error
)
{
    // If the high word is 0x8007, then we have a Win32 error HRESULT.
    if ((hr & 0xFFFF0000) == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, 0))
    {
        // Could have come from many values, but we choose this one
        *win32Error = HRESULT_CODE(hr);
        return true;
    }

    if (hr == S_OK)
    {
        *win32Error = HRESULT_CODE(hr);
        return true;
    }

    // Otherwise, we got a value we can't convert.
    return false;
}
}

VirtualizationInstance::VirtualizationInstance(
    String^ virtualizationRootPath,
    unsigned int poolThreadCount,
    unsigned int concurrentThreadCount,
    bool enableNegativePathCache,
    System::Collections::Generic::IReadOnlyCollection<NotificationMapping^>^ notificationMappings
)
    : m_virtualizationRootPath(virtualizationRootPath)
    , m_poolThreadCount(poolThreadCount)
    , m_concurrentThreadCount(concurrentThreadCount)
    , m_enableNegativePathCache(enableNegativePathCache)
    , m_notificationMappings(notificationMappings)
    , m_bytesPerSector(0)
    , m_writeBufferAlignmentRequirement(0)
    , m_virtualizationContext(nullptr)
{
    bool markAsRoot = false;

    // This will throw a FileLoadException if ProjectedFSLib.dll is not available.
    m_apiHelper = gcnew ApiHelper();

    // We need the root path in a form usable by native code.
    pin_ptr<const WCHAR> rootPath = PtrToStringChars(m_virtualizationRootPath);

    DirectoryInfo^ dirInfo = gcnew DirectoryInfo(m_virtualizationRootPath);
    System::Guid virtualizationInstanceID;
    if (!dirInfo->Exists)
    {
        // Generate a new instance ID and create the root.  We'll mark it later.
        virtualizationInstanceID = Guid::NewGuid();
        dirInfo->Create();

        markAsRoot = true;
    }
    else
    {
        // Open the directory and query for a ProjFS reparse point.
        std::unique_ptr<HANDLE, HFileDeleter> rootHandle(::CreateFile(rootPath,
                                                                      FILE_READ_ATTRIBUTES,
                                                                      FILE_SHARE_WRITE | FILE_SHARE_READ,
                                                                      0,
                                                                      OPEN_EXISTING,
                                                                      FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                                                      nullptr));

        if (rootHandle.get() == INVALID_HANDLE_VALUE)
        {
            auto lastError = ::GetLastError();
            throw gcnew Win32Exception(lastError, String::Format(CultureInfo::InvariantCulture,
                                                                 "Failed to open root directory {0}.",
                                                                 m_virtualizationRootPath));
        }

        std::vector<BYTE> buffer(MAXIMUM_REPARSE_DATA_BUFFER_SIZE, 0);
        REPARSE_DATA_BUFFER* reparseBuffer = reinterpret_cast<REPARSE_DATA_BUFFER*>(&buffer[0]);

        auto querySuccess = ::DeviceIoControl(rootHandle.get(),
                                              FSCTL_GET_REPARSE_POINT,
                                              nullptr,
                                              0,
                                              reparseBuffer,
                                              MAXIMUM_REPARSE_DATA_BUFFER_SIZE,
                                              nullptr,
                                              nullptr);

        if (!querySuccess)
        {
            auto lastError = ::GetLastError();

            if (lastError == ERROR_NOT_A_REPARSE_POINT)
            {
                // If this directory doesn't have a reparse point on it we'll need to mark it as a
                // root.  We need an ID for that.
                virtualizationInstanceID = Guid::NewGuid();
                markAsRoot = true;
            }
            else
            {
                throw gcnew Win32Exception(lastError, String::Format(CultureInfo::InvariantCulture,
                                                                     "Failed to query for ProjFS reparse point on {0}.",
                                                                     m_virtualizationRootPath));
            }
        }

        // If we did find a reparse point, see if it is one of ours.
        if (!markAsRoot)
        {
            if (reparseBuffer->ReparseTag != IO_REPARSE_TAG_PROJFS)
            {
                throw gcnew Win32Exception(ERROR_REPARSE_TAG_MISMATCH,
                                           String::Format(CultureInfo::InvariantCulture,
                                                          "Root directory {0} already has a different reparse point.",
                                                          m_virtualizationRootPath));
            }
        }
    }

    // If we created the root or found one without a ProjFS reparse point, we'll mark it as the root.
    if (markAsRoot)
    {
        HResult markResult = this->MarkDirectoryAsVirtualizationRoot(m_virtualizationRootPath,
                                                                     virtualizationInstanceID);

        if (markResult != HResult::Ok)
        {
            DWORD error;
            if (!Win32FromHRESULT(CastToUnderlyingType(markResult), &error))
            {
                // This should not happen.  The ProjFS APIs always return HRESULTs that can be
                // expressed as Win32 error codes.
                error = ERROR_INTERNAL_ERROR;
            }

            throw gcnew Win32Exception(error, String::Format(CultureInfo::InvariantCulture,
                                                             "Failed to mark directory {0} as virtualization root.",
                                                             m_virtualizationRootPath));
        }
    }
}

#pragma region Callback properties

QueryFileNameCallback^ VirtualizationInstance::OnQueryFileName::get(void)
{
    return m_queryFileNameCallback;
}

void VirtualizationInstance::OnQueryFileName::set(QueryFileNameCallback^ callbackDelegate)
{
    ConfirmNotStarted();
    m_queryFileNameCallback = callbackDelegate;
}

CancelCommandCallback^ VirtualizationInstance::OnCancelCommand::get(void)
{
    return m_cancelCommandCallback;
}

void VirtualizationInstance::OnCancelCommand::set(CancelCommandCallback^ callbackDelegate)
{
    ConfirmNotStarted();
    m_cancelCommandCallback = callbackDelegate;
}

NotifyFileOpenedCallback^ VirtualizationInstance::OnNotifyFileOpened::get(void)
{
    return m_notifyFileOpenedCallback;
}

void VirtualizationInstance::OnNotifyFileOpened::set(NotifyFileOpenedCallback^ callbackDelegate)
{
    ConfirmNotStarted();
    m_notifyFileOpenedCallback = callbackDelegate;
}

NotifyNewFileCreatedCallback^ VirtualizationInstance::OnNotifyNewFileCreated::get(void)
{
    return m_notifyNewFileCreatedCallback;
}

void VirtualizationInstance::OnNotifyNewFileCreated::set(NotifyNewFileCreatedCallback^ callbackDelegate)
{
    ConfirmNotStarted();
    m_notifyNewFileCreatedCallback = callbackDelegate;
}

NotifyFileOverwrittenCallback^ VirtualizationInstance::OnNotifyFileOverwritten::get(void)
{
    return m_notifyFileOverwrittenCallback;
}

void VirtualizationInstance::OnNotifyFileOverwritten::set(NotifyFileOverwrittenCallback^ callbackDelegate)
{
    ConfirmNotStarted();
    m_notifyFileOverwrittenCallback = callbackDelegate;
}

NotifyPreDeleteCallback^ VirtualizationInstance::OnNotifyPreDelete::get(void)
{
    return m_notifyPreDeleteCallback;
}

void VirtualizationInstance::OnNotifyPreDelete::set(NotifyPreDeleteCallback^ callbackDelegate)
{
    ConfirmNotStarted();
    m_notifyPreDeleteCallback = callbackDelegate;
}

NotifyPreRenameCallback^ VirtualizationInstance::OnNotifyPreRename::get(void)
{
    return m_notifyPreRenameCallback;
}

void VirtualizationInstance::OnNotifyPreRename::set(NotifyPreRenameCallback^ callbackDelegate)
{
    ConfirmNotStarted();
    m_notifyPreRenameCallback = callbackDelegate;
}

NotifyPreCreateHardlinkCallback^ VirtualizationInstance::OnNotifyPreCreateHardlink::get(void)
{
    return m_notifyPreCreateHardlinkCallback;
}

void VirtualizationInstance::OnNotifyPreCreateHardlink::set(NotifyPreCreateHardlinkCallback^ callbackDelegate)
{
    ConfirmNotStarted();
    m_notifyPreCreateHardlinkCallback = callbackDelegate;
}

NotifyFileRenamedCallback^ VirtualizationInstance::OnNotifyFileRenamed::get(void)
{
    return m_notifyFileRenamedCallback;
}

void VirtualizationInstance::OnNotifyFileRenamed::set(NotifyFileRenamedCallback^ callbackDelegate)
{
    ConfirmNotStarted();
    m_notifyFileRenamedCallback = callbackDelegate;
}

NotifyHardlinkCreatedCallback^ VirtualizationInstance::OnNotifyHardlinkCreated::get(void)
{
    return m_notifyHardlinkCreatedCallback;
}

void VirtualizationInstance::OnNotifyHardlinkCreated::set(NotifyHardlinkCreatedCallback^ callbackDelegate)
{
    ConfirmNotStarted();
    m_notifyHardlinkCreatedCallback = callbackDelegate;
}

NotifyFileHandleClosedNoModificationCallback^ VirtualizationInstance::OnNotifyFileHandleClosedNoModification::get(void)
{
    return m_notifyFileHandleClosedNoModificationCallback;
}

void VirtualizationInstance::OnNotifyFileHandleClosedNoModification::set(NotifyFileHandleClosedNoModificationCallback^ callbackDelegate)
{
    ConfirmNotStarted();
    m_notifyFileHandleClosedNoModificationCallback = callbackDelegate;
}

NotifyFileHandleClosedFileModifiedOrDeletedCallback^ VirtualizationInstance::OnNotifyFileHandleClosedFileModifiedOrDeleted::get(void)
{
    return m_notifyFileHandleClosedFileModifiedOrDeletedCallback;
}

void VirtualizationInstance::OnNotifyFileHandleClosedFileModifiedOrDeleted::set(NotifyFileHandleClosedFileModifiedOrDeletedCallback^ callbackDelegate)
{
    ConfirmNotStarted();
    m_notifyFileHandleClosedFileModifiedOrDeletedCallback = callbackDelegate;
}

NotifyFilePreConvertToFullCallback^ VirtualizationInstance::OnNotifyFilePreConvertToFull::get(void)
{
    return m_notifyFilePreConvertToFullCallback;
}

void VirtualizationInstance::OnNotifyFilePreConvertToFull::set(NotifyFilePreConvertToFullCallback^ callbackDelegate)
{
    ConfirmNotStarted();
    m_notifyFilePreConvertToFullCallback = callbackDelegate;
}

IRequiredCallbacks^ VirtualizationInstance::RequiredCallbacks::get(void)
{
    return m_requiredCallbacks;
}

ApiHelper^ VirtualizationInstance::ApiHelperObject::get(void)
{
    return m_apiHelper;
}

#pragma endregion

#pragma region Other properties

Guid VirtualizationInstance::VirtualizationInstanceId::get(void)
{
    ConfirmStarted();
    return m_virtualizationInstanceID;
}

int VirtualizationInstance::PlaceholderIdLength::get(void)
{
    return PRJ_PLACEHOLDER_ID_LENGTH;
}

#pragma endregion

#pragma region Public method implementations

HResult VirtualizationInstance::StartVirtualizing(IRequiredCallbacks^ requiredCallbacks)
{
    if (m_virtualizationContextGc != nullptr)
    {
        return HResult::AlreadyInitialized;
    }

    // Store the provider's implementation of the required callbacks.
    m_requiredCallbacks = requiredCallbacks;

    // Create a handle to this VirtualizationInstance object so that it can be passed to the native
    // lib as the InstanceContext parameter to PrjStartVirtualizing().
    m_virtualizationContextGc = new gcroot<VirtualizationInstance^>();
    *(m_virtualizationContextGc) = this;

    HRESULT startHr = S_OK;
    if (m_apiHelper->UseBetaApi)
    {
        // Query the file system for sector alignment info that CreateWriteBuffer() will need.
        FindBytesPerSectorAndAlignment();

        PRJ_COMMAND_CALLBACKS callbacks;
        m_apiHelper->_PrjCommandCallbacksInit(sizeof(callbacks), &callbacks);

        // Set the native wrappers for the required callbacks.
        callbacks.PrjStartDirectoryEnumeration = reinterpret_cast<PRJ_START_DIRECTORY_ENUMERATION_CB*>(PrjStartDirectoryEnumerationCB);
        callbacks.PrjEndDirectoryEnumeration = reinterpret_cast<PRJ_END_DIRECTORY_ENUMERATION_CB*>(PrjEndDirectoryEnumerationCB);
        callbacks.PrjGetDirectoryEnumeration = reinterpret_cast<PRJ_GET_DIRECTORY_ENUMERATION_CB*>(PrjGetDirectoryEnumerationCB);
        callbacks.PrjGetPlaceholderInformation = reinterpret_cast<PRJ_GET_PLACEHOLDER_INFORMATION_CB*>(PrjGetPlaceholderInformationCB);
        callbacks.PrjGetFileStream = reinterpret_cast<PRJ_GET_FILE_STREAM_CB*>(PrjGetFileStreamCB);

        // Set native wrappers for the optional callbacks if the provider has an implementation for them.

        if (OnQueryFileName != nullptr)
        {
            callbacks.PrjQueryFileName = reinterpret_cast<PRJ_QUERY_FILE_NAME_CB*>(PrjQueryFileNameCB);
        }

        if (OnCancelCommand != nullptr)
        {
            callbacks.PrjCancelCommand = reinterpret_cast<PRJ_CANCEL_COMMAND_CB*>(PrjCancelCommandCB);
        }

        // We set the native wrapper for the notify callback if the provider set at least one OnNotify* property.
        if ((OnNotifyFileOpened != nullptr) ||
            (OnNotifyNewFileCreated != nullptr) ||
            (OnNotifyFileOverwritten != nullptr) ||
            (OnNotifyPreDelete != nullptr) ||
            (OnNotifyPreRename != nullptr) ||
            (OnNotifyPreCreateHardlink != nullptr) ||
            (OnNotifyFileRenamed != nullptr) ||
            (OnNotifyHardlinkCreated != nullptr) ||
            (OnNotifyFileHandleClosedNoModification != nullptr) ||
            (OnNotifyFileHandleClosedFileModifiedOrDeleted != nullptr) ||
            (OnNotifyFilePreConvertToFull != nullptr))
        {
            callbacks.PrjNotifyOperation = reinterpret_cast<PRJ_NOTIFY_OPERATION_CB*>(PrjNotifyOperationCB);
        }

        pin_ptr<const WCHAR> rootPath = PtrToStringChars(m_virtualizationRootPath);

        // Use a temp location to avoid e0158.
        pin_ptr<PRJ_NAMESPACE_VIRTUALIZATION_CONTEXT> tempHandle = &(m_virtualizationContext);
        auto instanceHandle = reinterpret_cast<PRJ_VIRTUALIZATIONINSTANCE_HANDLE*>(tempHandle);

        VIRTUALIZATION_INST_EXTENDED_PARAMETERS extendedParameters;
        memset(&extendedParameters, 0, sizeof(VIRTUALIZATION_INST_EXTENDED_PARAMETERS));

        extendedParameters.Size = sizeof(VIRTUALIZATION_INST_EXTENDED_PARAMETERS);
        extendedParameters.Flags = m_enableNegativePathCache ? PRJ_FLAG_INSTANCE_NEGATIVE_PATH_CACHE : 0;
        extendedParameters.PoolThreadCount = m_poolThreadCount;
        extendedParameters.ConcurrentThreadCount = m_concurrentThreadCount;

        if (m_notificationMappings->Count > 0)
        {
            std::unique_ptr<PRJ_NOTIFICATION_MAPPING[]> nativeNotificationMappings;
            // "Pinning pointers can only be declared as non-static local variables on the stack."
            // https://docs.microsoft.com/en-us/cpp/windows/pin-ptr-cpp-cli
            // And so lets keep a copy of the paths alive rather than trying to pin the incoming strings
            std::vector<std::wstring> notificationPaths;

            notificationPaths.reserve(m_notificationMappings->Count);
            nativeNotificationMappings.reset(new PRJ_NOTIFICATION_MAPPING[m_notificationMappings->Count]);

            int index = 0;
            for each(NotificationMapping^ mapping in m_notificationMappings)
            {
                nativeNotificationMappings[index].NotificationBitMask = static_cast<PRJ_NOTIFY_TYPES>(mapping->NotificationMask);

                pin_ptr<const WCHAR> path = PtrToStringChars(mapping->NotificationRoot);
                notificationPaths.push_back(std::wstring(path));
                nativeNotificationMappings[index].NotificationRoot = notificationPaths.rbegin()->c_str();
                ++index;
            }

            extendedParameters.NotificationMappings = nativeNotificationMappings.get();
            extendedParameters.NumNotificationMappingsCount = m_notificationMappings->Count;

            startHr = m_apiHelper->_PrjStartVirtualizationInstanceEx(rootPath,
                                                                     &callbacks,
                                                                     m_virtualizationContextGc,
                                                                     &extendedParameters,
                                                                     instanceHandle);
        }
        else
        {
            // The caller didn't provide any notification mappings.  Use the non-Ex Start routine to get
            // ProjFS to supply the default notification mask.
            startHr = m_apiHelper->_PrjStartVirtualizationInstance(rootPath,
                                                                   &callbacks,
                                                                   extendedParameters.Flags,
                                                                   0, // ProjFS will default to PRJ_DEFAULT_NOTIFICATION_MASK defined in prjlibp.h
                                                                   extendedParameters.PoolThreadCount,
                                                                   extendedParameters.ConcurrentThreadCount,
                                                                   m_virtualizationContextGc,
                                                                   instanceHandle);
        }
    }
    else
    {
        PRJ_CALLBACKS callbacks;
        memset(&callbacks, 0, sizeof(PRJ_CALLBACKS));

        // Set native wrappers for the required callbacks.
        callbacks.StartDirectoryEnumerationCallback = reinterpret_cast<PRJ_START_DIRECTORY_ENUMERATION_CB*>(PrjStartDirectoryEnumerationCB);
        callbacks.EndDirectoryEnumerationCallback = reinterpret_cast<PRJ_END_DIRECTORY_ENUMERATION_CB*>(PrjEndDirectoryEnumerationCB);
        callbacks.GetDirectoryEnumerationCallback = reinterpret_cast<PRJ_GET_DIRECTORY_ENUMERATION_CB*>(PrjGetDirectoryEnumerationCB);
        callbacks.GetPlaceholderInfoCallback = reinterpret_cast<PRJ_GET_PLACEHOLDER_INFO_CB*>(PrjGetPlaceholderInfoCB);
        callbacks.GetFileDataCallback = reinterpret_cast<PRJ_GET_FILE_DATA_CB*>(PrjGetFileDataCB);

        // Set native wrappers for the optional callbacks if the provider has an implementation for them.

        if (OnQueryFileName != nullptr)
        {
            callbacks.QueryFileNameCallback = reinterpret_cast<PRJ_QUERY_FILE_NAME_CB*>(PrjQueryFileNameCB);
        }

        if (OnCancelCommand != nullptr)
        {
            callbacks.CancelCommandCallback = reinterpret_cast<PRJ_CANCEL_COMMAND_CB*>(PrjCancelCommandCB);
        }

        // We set the native wrapper for the notification callback if the provider set at least one OnNotify* property.
        if ((OnNotifyFileOpened != nullptr) ||
            (OnNotifyNewFileCreated != nullptr) ||
            (OnNotifyFileOverwritten != nullptr) ||
            (OnNotifyPreDelete != nullptr) ||
            (OnNotifyPreRename != nullptr) ||
            (OnNotifyPreCreateHardlink != nullptr) ||
            (OnNotifyFileRenamed != nullptr) ||
            (OnNotifyHardlinkCreated != nullptr) ||
            (OnNotifyFileHandleClosedNoModification != nullptr) ||
            (OnNotifyFileHandleClosedFileModifiedOrDeleted != nullptr) ||
            (OnNotifyFilePreConvertToFull != nullptr))
        {
            callbacks.NotificationCallback = reinterpret_cast<PRJ_NOTIFICATION_CB*>(PrjNotificationCB);
        }

        pin_ptr<const WCHAR> rootPath = PtrToStringChars(m_virtualizationRootPath);
        pin_ptr<PRJ_NAMESPACE_VIRTUALIZATION_CONTEXT> namespaceCtx = &(m_virtualizationContext);

        PRJ_STARTVIRTUALIZING_OPTIONS startOptions;
        memset(&startOptions, 0, sizeof(PRJ_STARTVIRTUALIZING_OPTIONS));

        startOptions.Flags = m_enableNegativePathCache ? PRJ_FLAG_USE_NEGATIVE_PATH_CACHE : PRJ_FLAG_NONE;
        startOptions.PoolThreadCount = m_poolThreadCount;
        startOptions.ConcurrentThreadCount = m_concurrentThreadCount;

        // These need to stay in scope until we call PrjStartVirtualizing
        std::unique_ptr<PRJ_NOTIFICATION_MAPPING[]> nativeNotificationMappings;
        std::vector<std::wstring> notificationPaths;

        if (m_notificationMappings->Count > 0)
        {
            // "Pinning pointers can only be declared as non-static local variables on the stack."
            // https://docs.microsoft.com/en-us/cpp/windows/pin-ptr-cpp-cli
            // And so lets keep a copy of the paths alive rather than trying to pin the incoming strings

            notificationPaths.reserve(m_notificationMappings->Count);
            nativeNotificationMappings.reset(new PRJ_NOTIFICATION_MAPPING[m_notificationMappings->Count]);

            int index = 0;
            for each(NotificationMapping^ mapping in m_notificationMappings)
            {
                nativeNotificationMappings[index].NotificationBitMask = static_cast<PRJ_NOTIFY_TYPES>(mapping->NotificationMask);

                pin_ptr<const WCHAR> path = PtrToStringChars(mapping->NotificationRoot);
                notificationPaths.push_back(std::wstring(path));
                nativeNotificationMappings[index].NotificationRoot = notificationPaths.rbegin()->c_str();
                ++index;
            }

            startOptions.NotificationMappings = nativeNotificationMappings.get();
            startOptions.NotificationMappingsCount = m_notificationMappings->Count;
        }
        else
        {
            // ProjFS will supply a default notification mask for the root if there are no mappings.
            startOptions.NotificationMappingsCount = 0;
        }

        startHr = m_apiHelper->_PrjStartVirtualizing(rootPath,
                                                     &callbacks,
                                                     m_virtualizationContextGc,
                                                     &startOptions,
                                                     namespaceCtx);
    }

    if (FAILED(startHr))
    {
        return static_cast<HResult>(startHr);
    }

    // Store the virtualization instance ID.
    GUID instanceId = {};
    Guid^ instanceIDRef;
    HRESULT getInfoHr = S_OK;
    if (m_apiHelper->UseBetaApi)
    {
        getInfoHr = m_apiHelper->_PrjGetVirtualizationInstanceIdFromHandle(reinterpret_cast<PRJ_VIRTUALIZATIONINSTANCE_HANDLE>(m_virtualizationContext),
                                                                           &instanceId);
    }
    else
    {
        PRJ_VIRTUALIZATION_INSTANCE_INFO instanceInfo = {};
        getInfoHr = m_apiHelper->_PrjGetVirtualizationInstanceInfo(m_virtualizationContext,
                                                                   &instanceInfo);
        instanceId = instanceInfo.InstanceID;
    }

    // If we couldn't get the instance ID, shut the instance down and return error (this is super
    // unlikely; it could only happen if somehow the instance successfully started above, but its
    // info is corrupt).
    if (FAILED(getInfoHr))
    {
        StopVirtualizing();
        return static_cast<HResult>(getInfoHr);
    }

    instanceIDRef = gcnew Guid(instanceId.Data1,
                               instanceId.Data2,
                               instanceId.Data3,
                               instanceId.Data4[0],
                               instanceId.Data4[1],
                               instanceId.Data4[2],
                               instanceId.Data4[3],
                               instanceId.Data4[4],
                               instanceId.Data4[5],
                               instanceId.Data4[6],
                               instanceId.Data4[7]);

    m_virtualizationInstanceID = *instanceIDRef;

    return HResult::Ok;
}

void VirtualizationInstance::StopVirtualizing()
{
    HRESULT hr = S_OK;
    if (m_apiHelper->UseBetaApi)
    {
        hr = m_apiHelper->_PrjStopVirtualizationInstance(m_virtualizationContext);
    }
    else
    {
        try
        {
            // The underlying native API throws a structured exception if the instance is in an invalid
            // state.
            m_apiHelper->_PrjStopVirtualizing(m_virtualizationContext);
        }
        catch (...)
        {
            // Catch the structured exception and remember that we had a failure.
            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr))
    {
        m_virtualizationContext = nullptr;

        delete m_virtualizationContextGc;
        m_virtualizationContextGc = nullptr;
    }
    else
    {
        // Since this is a resource-releasing routine we don't return an error code.  Instead we throw.
        throw gcnew InvalidOperationException("Virtualization instance in invalid state.");
    }
}

HResult VirtualizationInstance::ClearNegativePathCache([Out] unsigned int% totalEntryNumber)
{
    UINT32 entryCount = 0;
    HResult result = static_cast<HResult>(::PrjClearNegativePathCache(m_virtualizationContext,
                                                                      &entryCount));
    totalEntryNumber = entryCount;

    return result;
}

HResult VirtualizationInstance::WriteFileData(
    Guid dataStreamId,
    IWriteBuffer^ buffer,
    unsigned long long byteOffset,
    unsigned long length)
{
    if (buffer == nullptr)
    {
        return HResult::InvalidArg;
    }

    array<Byte>^ guidData = dataStreamId.ToByteArray();
    pin_ptr<Byte> data = &(guidData[0]);
    if (m_apiHelper->UseBetaApi)
    {
        return static_cast<HResult>(m_apiHelper->_PrjWriteFile(reinterpret_cast<PRJ_VIRTUALIZATIONINSTANCE_HANDLE>(m_virtualizationContext),
                                                               reinterpret_cast<GUID*>(data),
                                                               buffer->Pointer.ToPointer(),
                                                               byteOffset,
                                                               length));
    }
    else
    {
        return static_cast<HResult>(m_apiHelper->_PrjWriteFileData(m_virtualizationContext,
                                                                   reinterpret_cast<GUID*>(data),
                                                                   buffer->Pointer.ToPointer(),
                                                                   byteOffset,
                                                                   length));
    }
}

HResult VirtualizationInstance::DeleteFile(
    String^ relativePath,
    UpdateType updateFlags,
    [Out] UpdateFailureCause% failureReason)
{
    pin_ptr<const WCHAR> path = PtrToStringChars(relativePath);
    PRJ_UPDATE_FAILURE_CAUSES deleteFailureReason = PRJ_UPDATE_FAILURE_CAUSE_NONE;
    HResult result = static_cast<HResult>(::PrjDeleteFile(m_virtualizationContext,
                                                          path,
                                                          static_cast<PRJ_UPDATE_TYPES>(updateFlags),
                                                          &deleteFailureReason));
    failureReason = static_cast<UpdateFailureCause>(deleteFailureReason);
    return result;
}

HResult VirtualizationInstance::WritePlaceholderInfo(String^ relativePath,
                                                     DateTime creationTime,
                                                     DateTime lastAccessTime,
                                                     DateTime lastWriteTime,
                                                     DateTime changeTime,
                                                     FileAttributes fileAttributes,
                                                     long long endOfFile,
                                                     bool isDirectory,
                                                     array<Byte>^ contentId,
                                                     array<Byte>^ providerId)
{
    if (relativePath == nullptr)
    {
        return HResult::InvalidArg;
    }

    if (m_apiHelper->UseBetaApi)
    {
        std::shared_ptr<PRJ_PLACEHOLDER_INFORMATION> fileInformation = CreatePlaceholderInformation(creationTime,
                                                                                                    lastAccessTime,
                                                                                                    lastWriteTime,
                                                                                                    changeTime,
                                                                                                    fileAttributes,
                                                                                                    isDirectory ? 0 : endOfFile,
                                                                                                    isDirectory,
                                                                                                    contentId,
                                                                                                    providerId);

        pin_ptr<const WCHAR> path = PtrToStringChars(relativePath);
        return static_cast<HResult>(m_apiHelper->_PrjWritePlaceholderInformation(reinterpret_cast<PRJ_VIRTUALIZATIONINSTANCE_HANDLE>(m_virtualizationContext),
                                                                                 path,
                                                                                 fileInformation.get(),
                                                                                 sizeof(PRJ_PLACEHOLDER_INFORMATION)));
    }
    else
    {
        std::shared_ptr<PRJ_PLACEHOLDER_INFO> placeholderInfo = CreatePlaceholderInfo(creationTime,
                                                                                      lastAccessTime,
                                                                                      lastWriteTime,
                                                                                      changeTime,
                                                                                      fileAttributes,
                                                                                      isDirectory ? 0 : endOfFile,
                                                                                      isDirectory,
                                                                                      contentId,
                                                                                      providerId);

        pin_ptr<const WCHAR> path = PtrToStringChars(relativePath);
        return static_cast<HResult>(m_apiHelper->_PrjWritePlaceholderInfo(m_virtualizationContext,
                                                                          path,
                                                                          placeholderInfo.get(),
                                                                          sizeof(PRJ_PLACEHOLDER_INFO)));
    }
}


HResult VirtualizationInstance::WritePlaceholderInfo2(
    String^ relativePath,
    DateTime creationTime,
    DateTime lastAccessTime,
    DateTime lastWriteTime,
    DateTime changeTime,
    FileAttributes fileAttributes,
    long long endOfFile,
    bool isDirectory,
    String^ symlinkTargetOrNull,
    array<Byte>^ contentId,
    array<Byte>^ providerId)
{
    // This API is supported in Windows 10 version 2004 and above.
    if (m_apiHelper->SupportedApi < ApiLevel::v2004)
    {
        throw gcnew NotImplementedException("PrjWritePlaceholderInfo2 is not supported in this version of Windows.");
    }

    if (relativePath == nullptr)
    {
        return HResult::InvalidArg;
    }

    std::shared_ptr<PRJ_PLACEHOLDER_INFO> placeholderInfo = CreatePlaceholderInfo(creationTime,
        lastAccessTime,
        lastWriteTime,
        changeTime,
        fileAttributes,
        isDirectory ? 0 : endOfFile,
        isDirectory,
        contentId,
        providerId);

    pin_ptr<const WCHAR> path = PtrToStringChars(relativePath);

    if (symlinkTargetOrNull != nullptr)
    {
        PRJ_EXTENDED_INFO extendedInfo = {};

        extendedInfo.InfoType = PRJ_EXT_INFO_TYPE_SYMLINK;
        pin_ptr<const WCHAR> targetPath = PtrToStringChars(symlinkTargetOrNull);
        extendedInfo.Symlink.TargetName = targetPath;

        return static_cast<HResult>(m_apiHelper->_PrjWritePlaceholderInfo2(m_virtualizationContext,
            path,
            placeholderInfo.get(),
            sizeof(PRJ_PLACEHOLDER_INFO),
            &extendedInfo));
    }
    else
    {
        return static_cast<HResult>(m_apiHelper->_PrjWritePlaceholderInfo(m_virtualizationContext,
            path,
            placeholderInfo.get(),
            sizeof(PRJ_PLACEHOLDER_INFO)));
    }
}

HResult VirtualizationInstance::UpdateFileIfNeeded(String^ relativePath,
                                                   DateTime creationTime,
                                                   DateTime lastAccessTime,
                                                   DateTime lastWriteTime,
                                                   DateTime changeTime,
                                                   FileAttributes fileAttributes,
                                                   long long endOfFile,
                                                   array<Byte>^ contentId,
                                                   array<Byte>^ providerId,
                                                   UpdateType updateFlags,
                                                   [Out] UpdateFailureCause% failureReason)
{
    HResult result;
    if (m_apiHelper->UseBetaApi)
    {
        std::shared_ptr<PRJ_PLACEHOLDER_INFORMATION> fileInformation = CreatePlaceholderInformation(
            creationTime,
            lastAccessTime,
            lastWriteTime,
            changeTime,
            fileAttributes,
            endOfFile,
            false, // directory
            contentId,
            providerId);

        unsigned long updateFailureReason = 0;
        pin_ptr<const WCHAR> path = PtrToStringChars(relativePath);
        result = static_cast<HResult>(m_apiHelper->_PrjUpdatePlaceholderIfNeeded(reinterpret_cast<PRJ_VIRTUALIZATIONINSTANCE_HANDLE>(m_virtualizationContext),
                                                                                 path,
                                                                                 fileInformation.get(),
                                                                                 FIELD_OFFSET(PRJ_PLACEHOLDER_INFORMATION, VariableData), // We have written no variable data
                                                                                 CastToUnderlyingType(updateFlags),
                                                                                 &updateFailureReason));

        failureReason = static_cast<UpdateFailureCause>(updateFailureReason);
    }
    else
    {
        std::shared_ptr<PRJ_PLACEHOLDER_INFO> placeholderInfo = CreatePlaceholderInfo(creationTime,
                                                                                      lastAccessTime,
                                                                                      lastWriteTime,
                                                                                      changeTime,
                                                                                      fileAttributes,
                                                                                      endOfFile,
                                                                                      false, // directory
                                                                                      contentId,
                                                                                      providerId);

        PRJ_UPDATE_FAILURE_CAUSES updateFailureCause = PRJ_UPDATE_FAILURE_CAUSE_NONE;
        pin_ptr<const WCHAR> path = PtrToStringChars(relativePath);
        result = static_cast<HResult>(m_apiHelper->_PrjUpdateFileIfNeeded(m_virtualizationContext,
                                                                          path,
                                                                          placeholderInfo.get(),
                                                                          sizeof(PRJ_PLACEHOLDER_INFO),
                                                                          static_cast<PRJ_UPDATE_TYPES>(updateFlags),
                                                                          &updateFailureCause));

        failureReason = static_cast<UpdateFailureCause>(updateFailureCause);
    }

    return result;
}

HResult VirtualizationInstance::CompleteCommand(int commandId)
{
    return CompleteCommand(commandId, HResult::Ok);
}

HResult VirtualizationInstance::CompleteCommand(
    int commandId,
    HResult completionResult)
{
    return static_cast<HResult>(::PrjCompleteCommand(
        m_virtualizationContext,
        commandId,
        static_cast<HRESULT>(completionResult),
        nullptr));
}

HResult VirtualizationInstance::CompleteCommand(
    int commandId,
    IDirectoryEnumerationResults^ results)
{
    PRJ_COMPLETE_COMMAND_EXTENDED_PARAMETERS extendedParams = { };

    extendedParams.CommandType = PRJ_COMPLETE_COMMAND_TYPE_ENUMERATION;
    // results has to be a concrete DirectoryEnumerationResults.
    extendedParams.Enumeration.DirEntryBufferHandle = safe_cast<DirectoryEnumerationResults^>(results)->DirEntryBufferHandle;

    return static_cast<HResult>(::PrjCompleteCommand(
        m_virtualizationContext,
        commandId,
        static_cast<HRESULT>(HResult::Ok),
        &extendedParams));
}

HResult VirtualizationInstance::CompleteCommand(
    int commandId,
    NotificationType newNotificationMask)
{
    PRJ_COMPLETE_COMMAND_EXTENDED_PARAMETERS extendedParams = { };

    extendedParams.CommandType = PRJ_COMPLETE_COMMAND_TYPE_NOTIFICATION;
    extendedParams.Notification.NotificationMask = static_cast<PRJ_NOTIFY_TYPES>(newNotificationMask);

    return static_cast<HResult>(::PrjCompleteCommand(
        m_virtualizationContext,
        commandId,
        static_cast<HRESULT>(HResult::Ok),
        &extendedParams));
}

IWriteBuffer^ VirtualizationInstance::CreateWriteBuffer(
    unsigned int desiredBufferSize)
{
    WriteBuffer^ buffer;

    if (m_apiHelper->UseBetaApi)
    {
        if (desiredBufferSize < m_bytesPerSector)
        {
            desiredBufferSize = m_bytesPerSector;
        }
        else
        {
            unsigned int bufferRemainder = desiredBufferSize % m_bytesPerSector;
            if (bufferRemainder != 0)
            {
                // Round up to nearest multiple of m_bytesPerSector
                desiredBufferSize += (m_bytesPerSector - bufferRemainder);
            }
        }

        buffer = gcnew WriteBuffer(desiredBufferSize, m_writeBufferAlignmentRequirement);
    }
    else
    {
        // On Windows 10 version 1809 and above the alignment requirements are stored in the
        // namespace virtualization context.
        buffer = gcnew WriteBuffer(desiredBufferSize,
                                   m_virtualizationContext,
                                   m_apiHelper);
    }

    return buffer;
}

IWriteBuffer^ VirtualizationInstance::CreateWriteBuffer(
    unsigned long long byteOffset,
    unsigned int length,
    [Out] unsigned long long% alignedByteOffset,
    [Out] unsigned int% alignedLength)
{
    // Get the sector size so we can compute the aligned versions of byteOffset and length to return
    // to the user.  If we're on Windows 10 version 1803 the sector size is stored on the class.
    // Otherwise it's available from the namespace virtualization context.
    unsigned long bytesPerSector;
    if (m_apiHelper->UseBetaApi)
    {
        bytesPerSector = m_bytesPerSector;
    }
    else
    {
        PRJ_VIRTUALIZATION_INSTANCE_INFO instanceInfo = {};
        auto result = m_apiHelper->_PrjGetVirtualizationInstanceInfo(m_virtualizationContext,
                                                                     &instanceInfo);

        if (FAILED(result))
        {
            DWORD error;
            if (!Win32FromHRESULT(result, &error))
            {
                // This should not happen.  The ProjFS APIs always return HRESULTs that can be
                // expressed as Win32 error codes.
                error = ERROR_INTERNAL_ERROR;
            }

            throw gcnew Win32Exception(error, String::Format(CultureInfo::InvariantCulture,
                                                             "Failed to retrieve virtualization instance info for directory {0}.",
                                                             m_virtualizationRootPath));

        }

        bytesPerSector = instanceInfo.WriteAlignment;
    }

    // alignedByteOffset is byteOffset, rounded down to the nearest bytesPerSector boundary.
    alignedByteOffset = byteOffset & (0 - static_cast<unsigned long long>(bytesPerSector));

    // alignedLength is the end offset of the requested range, rounded up to the nearest bytesPerSector
    // boundary.
    unsigned long long rangeEndOffset = byteOffset + static_cast<unsigned long long>(length);
    unsigned long long alignedRangeEndOffset = (rangeEndOffset + (bytesPerSector - 1)) & (0 - bytesPerSector);
    alignedLength = static_cast<unsigned int>(alignedRangeEndOffset - alignedByteOffset);

    // Now that we've got the adjusted length, create the buffer itself.
    return CreateWriteBuffer(alignedLength);
}

HResult VirtualizationInstance::MarkDirectoryAsPlaceholder(
    String^ targetDirectoryPath,
    array<Byte>^ contentId,
    array<Byte>^ providerId)
{
    GUID virtualizationInstanceId;
    HRESULT hr = S_OK;

    if (m_apiHelper->UseBetaApi)
    {
        hr = m_apiHelper->_PrjGetVirtualizationInstanceIdFromHandle(reinterpret_cast<PRJ_VIRTUALIZATIONINSTANCE_HANDLE>(m_virtualizationContext),
                                                                    &virtualizationInstanceId);

        if (hr == S_OK)
        {
            PRJ_PLACEHOLDER_VERSION_INFO versionInfo;
            memset(&versionInfo, 0, sizeof(PRJ_PLACEHOLDER_VERSION_INFO));
            CopyPlaceholderId(versionInfo.ProviderID, providerId);
            CopyPlaceholderId(versionInfo.ContentID, contentId);

            pin_ptr<const WCHAR> rootPath = PtrToStringChars(m_virtualizationRootPath);
            pin_ptr<const WCHAR> targetPath = PtrToStringChars(targetDirectoryPath);
            hr = m_apiHelper->_PrjConvertDirectoryToPlaceholder(rootPath,
                                                                targetPath,
                                                                &versionInfo,
                                                                0,
                                                                &virtualizationInstanceId);
        }
    }
    else
    {
        PRJ_VIRTUALIZATION_INSTANCE_INFO instanceInfo;
        hr = m_apiHelper->_PrjGetVirtualizationInstanceInfo(m_virtualizationContext,
                                                            &instanceInfo);

        if (SUCCEEDED(hr))
        {
            PRJ_PLACEHOLDER_VERSION_INFO versionInfo;
            memset(&versionInfo, 0, sizeof(PRJ_PLACEHOLDER_VERSION_INFO));
            CopyPlaceholderId(versionInfo.ProviderID, providerId);
            CopyPlaceholderId(versionInfo.ContentID, contentId);

            pin_ptr<const WCHAR> rootPath = PtrToStringChars(m_virtualizationRootPath);
            pin_ptr<const WCHAR> targetPath = PtrToStringChars(targetDirectoryPath);
            hr = m_apiHelper->_PrjMarkDirectoryAsPlaceholder(rootPath,
                                                             targetPath,
                                                             &versionInfo,
                                                             &instanceInfo.InstanceID);
        }
    }

    return static_cast<HResult>(hr);
}

//static 
HResult VirtualizationInstance::MarkDirectoryAsVirtualizationRoot(
    String^ rootPath,
    Guid virtualizationInstanceGuid)
{
    PRJ_PLACEHOLDER_VERSION_INFO versionInfo;
    memset(&versionInfo, 0, sizeof(PRJ_PLACEHOLDER_VERSION_INFO));

    // We need our own ApiHelper because this is a static method.
    ApiHelper^ apiHelper = gcnew ApiHelper();

    array<Byte>^ guidArray = virtualizationInstanceGuid.ToByteArray();
    pin_ptr<Byte> guidData = &(guidArray[0]);
    pin_ptr<const WCHAR> root = PtrToStringChars(rootPath);
    if (apiHelper->UseBetaApi)
    {
        return static_cast<HResult>(apiHelper->_PrjConvertDirectoryToPlaceholder(root,
                                                                                 L"",
                                                                                 &versionInfo,
                                                                                 PRJ_FLAG_VIRTUALIZATION_ROOT,
                                                                                 (GUID*) guidData));
    }
    else
    {
        return static_cast<HResult>(apiHelper->_PrjMarkDirectoryAsPlaceholder(root,
                                                                              nullptr,
                                                                              &versionInfo,
                                                                              reinterpret_cast<GUID*>(guidData)));
    }
}

#pragma endregion

#pragma region Private method implementations

void VirtualizationInstance::ConfirmStarted()
{
    if (!m_virtualizationContext)
    {
        throw gcnew InvalidOperationException("Operation invalid before virtualization instance is started");
    }
}

void VirtualizationInstance::ConfirmNotStarted()
{
    if (m_virtualizationContext)
    {
        throw gcnew InvalidOperationException("Operation invalid after virtualization instance is started");
    }
}

void VirtualizationInstance::FindBytesPerSectorAndAlignment()
{
    WCHAR volumePath[MAX_PATH];
    pin_ptr<const WCHAR> rootPath = PtrToStringChars(m_virtualizationRootPath);
    if (!GetVolumePathName(rootPath,
                           volumePath,
                           ARRAYSIZE(volumePath)))
    {
        DWORD lastError = ::GetLastError();
        throw gcnew IOException(String::Format(CultureInfo::InvariantCulture,
                                               "Failed to get volume path name, Error: {0}",
                                               lastError));
    }

    WCHAR volumeName[VOLUME_PATH_LENGTH + 1];
    if (!GetVolumeNameForVolumeMountPoint(volumePath,
                                          volumeName,
                                          ARRAYSIZE(volumeName)))
    {
        DWORD lastError = ::GetLastError();
        throw gcnew IOException(String::Format(CultureInfo::InvariantCulture,
                                               "Failed to get volume name for volume mount point: {0}, Error: {1}",
                                               gcnew String(volumeName),
                                               lastError));
    }

    if (wcslen(volumeName) != VOLUME_PATH_LENGTH || volumeName[VOLUME_PATH_LENGTH - 1] != L'\\')
    {
        throw gcnew IOException(String::Format(CultureInfo::InvariantCulture,
                                               "Volume name {0} is not in expected format",
                                               gcnew String(volumeName)));
    }

    HANDLE rootHandle = CreateFile(volumeName,
                                   0,
                                   0,
                                   NULL,
                                   OPEN_EXISTING,
                                   FILE_FLAG_BACKUP_SEMANTICS,
                                   NULL);
    if (rootHandle == INVALID_HANDLE_VALUE)
    {
        DWORD lastError = ::GetLastError();
        throw gcnew IOException(String::Format(CultureInfo::InvariantCulture,
                                               "Failed to get handle to {0}, Error: {1}", m_virtualizationRootPath,
                                               lastError));
    }

    FILE_STORAGE_INFO storageInfo = {};
    if (!GetFileInformationByHandleEx(rootHandle,
                                      FileStorageInfo,
                                      &storageInfo,
                                      sizeof(storageInfo)))
    {
        DWORD lastError = ::GetLastError();
        CloseHandle(rootHandle);
        throw gcnew IOException(String::Format(CultureInfo::InvariantCulture,
                                               "Failed to query sector size of volume, Error: {0}",
                                               lastError));
    }

    FILE_ALIGNMENT_INFO alignmentInfo = {};
    if (!GetFileInformationByHandleEx(rootHandle,
                                      FileAlignmentInfo,
                                      &alignmentInfo,
                                      sizeof(alignmentInfo)))
    {
        DWORD lastError = ::GetLastError();
        CloseHandle(rootHandle);
        throw gcnew IOException(String::Format(CultureInfo::InvariantCulture,
                                               "Failed to query device alignment, Error: {0}",
                                               lastError));
    }

    m_bytesPerSector = storageInfo.LogicalBytesPerSector;

    // AlignmentRequirement returns the required alignment minus 1 
    // https://msdn.microsoft.com/en-us/library/cc232065.aspx
    // https://docs.microsoft.com/en-us/windows-hardware/drivers/kernel/initializing-a-device-object
    m_writeBufferAlignmentRequirement = alignmentInfo.AlignmentRequirement + 1;

    CloseHandle(rootHandle);

    if (!IsPowerOf2(m_writeBufferAlignmentRequirement))
    {
        throw gcnew IOException(String::Format(CultureInfo::InvariantCulture,
                                               "Failed to determine write buffer alignment requirement: {0} is not a power of 2",
                                               m_writeBufferAlignmentRequirement));
    }
}

#pragma endregion

namespace {
#pragma region C callbacks

_Function_class_(PRJ_START_DIRECTORY_ENUMERATION_CB)
HRESULT PrjStartDirectoryEnumerationCB(_In_ PRJ_CALLBACK_DATA* callbackData,
                                       _In_ LPCGUID enumerationId)
{

    if (callbackData->InstanceContext != NULL)
    {
        gcroot<VirtualizationInstance^>& pVirtualizationInstanceObj = *((gcroot<VirtualizationInstance^>*)callbackData->InstanceContext);

        return static_cast<HRESULT>(pVirtualizationInstanceObj->RequiredCallbacks->StartDirectoryEnumerationCallback(
            callbackData->CommandId,
            GUIDtoGuid(*enumerationId),
            gcnew String(callbackData->FilePathName),
            callbackData->TriggeringProcessId,
            (callbackData->TriggeringProcessImageFileName != NULL) ? gcnew String(callbackData->TriggeringProcessImageFileName) : String::Empty));
    }

    return HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR);
}

_Function_class_(PRJ_END_DIRECTORY_ENUMERATION_CB)
HRESULT PrjEndDirectoryEnumerationCB(_In_ PRJ_CALLBACK_DATA* callbackData,
                                     _In_ LPCGUID enumerationId)
{

    if (callbackData->InstanceContext != NULL)
    {
        gcroot<VirtualizationInstance^>& pVirtualizationInstanceObj = *((gcroot<VirtualizationInstance^>*)callbackData->InstanceContext);

        return static_cast<HRESULT>(pVirtualizationInstanceObj->RequiredCallbacks->EndDirectoryEnumerationCallback(
            GUIDtoGuid(*enumerationId)));
    }

    return HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR);
}

_Function_class_(PRJ_GET_DIRECTORY_ENUMERATION_CB)
HRESULT PrjGetDirectoryEnumerationCB(_In_ PRJ_CALLBACK_DATA* callbackData,
                                     _In_ LPCGUID enumerationId,
                                     _In_opt_z_ LPCWSTR searchExpression,
                                     _In_ PRJ_DIR_ENTRY_BUFFER_HANDLE dirEntryBufferHandle)
{

    if (callbackData->InstanceContext != NULL)
    {
        gcroot<VirtualizationInstance^>& pVirtualizationInstanceObj = *((gcroot<VirtualizationInstance^>*)callbackData->InstanceContext);

        IDirectoryEnumerationResults^ enumerationData = gcnew DirectoryEnumerationResults(dirEntryBufferHandle, pVirtualizationInstanceObj->ApiHelperObject);
        return static_cast<HRESULT>(pVirtualizationInstanceObj->RequiredCallbacks->GetDirectoryEnumerationCallback(
            callbackData->CommandId,
            GUIDtoGuid(*enumerationId),
            (searchExpression != NULL) ? gcnew String(searchExpression) : nullptr,
            (callbackData->Flags & PRJ_CB_DATA_FLAG_ENUM_RESTART_SCAN),
            enumerationData));
    }
    return HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR);
}

_Function_class_(PRJ_GET_PLACEHOLDER_INFO_CB)
HRESULT PrjGetPlaceholderInfoCB(_In_ const PRJ_CALLBACK_DATA* callbackData)
{
    if (callbackData->InstanceContext != NULL)
    {
        gcroot<VirtualizationInstance^>& pVirtualizationInstanceObj = *((gcroot<VirtualizationInstance^>*)callbackData->InstanceContext);

        return static_cast<HRESULT>(pVirtualizationInstanceObj->RequiredCallbacks->GetPlaceholderInfoCallback(
            callbackData->CommandId,
            gcnew String(callbackData->FilePathName),
            callbackData->TriggeringProcessId,
            GetTriggeringProcessNameSafe(callbackData)));
    }

    return HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR);
}

_Function_class_(PRJ_GET_FILE_DATA_CB)
HRESULT PrjGetFileDataCB(_In_ const PRJ_CALLBACK_DATA* callbackData,
                         _In_ UINT64 byteOffset,
                         _In_ UINT32 length)
{
    if (callbackData->InstanceContext != NULL)
    {
        gcroot<VirtualizationInstance^>& pVirtualizationInstanceObj = *((gcroot<VirtualizationInstance^>*)callbackData->InstanceContext);

        return static_cast<HRESULT>(pVirtualizationInstanceObj->RequiredCallbacks->GetFileDataCallback(
            callbackData->CommandId,
            gcnew String(callbackData->FilePathName),
            byteOffset,
            length,
            GUIDtoGuid(callbackData->DataStreamId),
            (callbackData->VersionInfo != NULL) ? MarshalPlaceholderId(callbackData->VersionInfo->ContentID) : nullptr,
            (callbackData->VersionInfo != NULL) ? MarshalPlaceholderId(callbackData->VersionInfo->ProviderID) : nullptr,
            callbackData->TriggeringProcessId,
            GetTriggeringProcessNameSafe(callbackData)));
    }

    return HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR);
}

_Function_class_(PRJ_QUERY_FILE_NAME_CB)
HRESULT PrjQueryFileNameCB(_In_ PRJ_CALLBACK_DATA* callbackData)
{
    if (callbackData->InstanceContext != NULL)
    {
        gcroot<VirtualizationInstance^>& pVirtualizationInstanceObj = *((gcroot<VirtualizationInstance^>*)callbackData->InstanceContext);

        if (pVirtualizationInstanceObj->OnQueryFileName != nullptr)
        {
            return static_cast<HRESULT>(pVirtualizationInstanceObj->OnQueryFileName(gcnew String(callbackData->FilePathName)));
        }
    }

    return HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR);
}

_Function_class_(PRJ_CANCEL_COMMAND_CB)
void PrjCancelCommandCB(_In_ PRJ_CALLBACK_DATA* callbackData)
{
    if (callbackData->InstanceContext != NULL)
    {
        gcroot<VirtualizationInstance^>& pVirtualizationInstanceObj = *((gcroot<VirtualizationInstance^>*)callbackData->InstanceContext);

        if (pVirtualizationInstanceObj->OnCancelCommand != nullptr)
        {
            pVirtualizationInstanceObj->OnCancelCommand(callbackData->CommandId);
        }
    }
}

_Function_class_(PRJ_NOTIFICATION_CB)
HRESULT PrjNotificationCB(_In_ const PRJ_CALLBACK_DATA* callbackData,
                          _In_ BOOLEAN isDirectory,
                          _In_ PRJ_NOTIFICATION notification,
                          _In_opt_ PCWSTR destinationFileName,
                          _Inout_ PRJ_NOTIFICATION_PARAMETERS* notificationParameters)
{
    if (callbackData->InstanceContext != nullptr)
    {
        gcroot<VirtualizationInstance^>& pVirtualizationInstanceObj = *((gcroot<VirtualizationInstance^>*)callbackData->InstanceContext);

        // Our pre-operation callback handlers return HResult, most of the post-operation callback
        // handlers have void return type.  Declare this for the ones that have a return code.
        HResult notificationResult = HResult::Ok;

        switch (notification)
        {
        case PRJ_NOTIFICATION_FILE_OPENED:
            if (pVirtualizationInstanceObj->OnNotifyFileOpened != nullptr)
            {
                NotificationType notificationMask;

                // The provider can deny the open by returning false.
                if (!pVirtualizationInstanceObj->OnNotifyFileOpened(gcnew String(callbackData->FilePathName),
                                                                    isDirectory != FALSE,
                                                                    callbackData->TriggeringProcessId,
                                                                    GetTriggeringProcessNameSafe(callbackData),
                                                                    notificationMask))
                {
                    notificationResult = HResult::AccessDenied;
                }
                else
                {
                    notificationParameters->PostCreate.NotificationMask = static_cast<PRJ_NOTIFY_TYPES>(notificationMask);
                }
            }
            break;

        case PRJ_NOTIFICATION_NEW_FILE_CREATED:
            if (pVirtualizationInstanceObj->OnNotifyNewFileCreated != nullptr)
            {
                NotificationType notificationMask;
                pVirtualizationInstanceObj->OnNotifyNewFileCreated(gcnew String(callbackData->FilePathName),
                                                                   isDirectory != FALSE,
                                                                   callbackData->TriggeringProcessId,
                                                                   GetTriggeringProcessNameSafe(callbackData),
                                                                   notificationMask);

                notificationParameters->PostCreate.NotificationMask = static_cast<PRJ_NOTIFY_TYPES>(notificationMask);
            }
            break;

        case PRJ_NOTIFICATION_FILE_OVERWRITTEN:
            if (pVirtualizationInstanceObj->OnNotifyFileOverwritten != nullptr)
            {
                NotificationType notificationMask;
                pVirtualizationInstanceObj->OnNotifyFileOverwritten(gcnew String(callbackData->FilePathName),
                                                                    isDirectory != FALSE,
                                                                    callbackData->TriggeringProcessId,
                                                                    GetTriggeringProcessNameSafe(callbackData),
                                                                    notificationMask);

                notificationParameters->PostCreate.NotificationMask = static_cast<PRJ_NOTIFY_TYPES>(notificationMask);
            }
            break;

        case PRJ_NOTIFICATION_PRE_DELETE:
            if (pVirtualizationInstanceObj->OnNotifyPreDelete != nullptr)
            {
                if (!pVirtualizationInstanceObj->OnNotifyPreDelete(gcnew String(callbackData->FilePathName),
                                                                   isDirectory != FALSE,
                                                                   callbackData->TriggeringProcessId,
                                                                   GetTriggeringProcessNameSafe(callbackData)))
                {
                    notificationResult = HResult::CannotDelete;
                }
            }
            break;

        case PRJ_NOTIFICATION_PRE_RENAME:
            if (pVirtualizationInstanceObj->OnNotifyPreRename != nullptr)
            {
                if (!pVirtualizationInstanceObj->OnNotifyPreRename(gcnew String(callbackData->FilePathName),
                                                                   gcnew String(destinationFileName),
                                                                   callbackData->TriggeringProcessId,
                                                                   GetTriggeringProcessNameSafe(callbackData)))
                {
                    notificationResult = HResult::AccessDenied;
                }
            }
            break;

        case PRJ_NOTIFICATION_PRE_SET_HARDLINK:
            if (pVirtualizationInstanceObj->OnNotifyPreCreateHardlink != nullptr)
            {
                if (!pVirtualizationInstanceObj->OnNotifyPreCreateHardlink(gcnew String(callbackData->FilePathName),
                                                                           gcnew String(destinationFileName),
                                                                           callbackData->TriggeringProcessId,
                                                                           GetTriggeringProcessNameSafe(callbackData)))
                {
                    notificationResult = HResult::AccessDenied;
                }
            }
            break;

        case PRJ_NOTIFICATION_FILE_RENAMED:
            if (pVirtualizationInstanceObj->OnNotifyFileRenamed != nullptr)
            {
                NotificationType notificationMask;
                pVirtualizationInstanceObj->OnNotifyFileRenamed(gcnew String(callbackData->FilePathName),
                                                                gcnew String(destinationFileName),
                                                                isDirectory != FALSE,
                                                                callbackData->TriggeringProcessId,
                                                                GetTriggeringProcessNameSafe(callbackData),
                                                                notificationMask);

                notificationParameters->FileRenamed.NotificationMask = static_cast<PRJ_NOTIFY_TYPES>(notificationMask);
            }
            break;

        case PRJ_NOTIFICATION_HARDLINK_CREATED:
            if (pVirtualizationInstanceObj->OnNotifyHardlinkCreated != nullptr)
            {
                pVirtualizationInstanceObj->OnNotifyHardlinkCreated(gcnew String(callbackData->FilePathName),
                                                                    gcnew String(destinationFileName),
                                                                    callbackData->TriggeringProcessId,
                                                                    GetTriggeringProcessNameSafe(callbackData));
            }
            break;

        case PRJ_NOTIFICATION_FILE_HANDLE_CLOSED_NO_MODIFICATION:
            if (pVirtualizationInstanceObj->OnNotifyFileHandleClosedNoModification != nullptr)
            {
                pVirtualizationInstanceObj->OnNotifyFileHandleClosedNoModification(gcnew String(callbackData->FilePathName),
                                                                                   isDirectory != FALSE,
                                                                                   callbackData->TriggeringProcessId,
                                                                                   GetTriggeringProcessNameSafe(callbackData));
            }
            break;

        case PRJ_NOTIFICATION_FILE_HANDLE_CLOSED_FILE_MODIFIED:
            if (pVirtualizationInstanceObj->OnNotifyFileHandleClosedFileModifiedOrDeleted != nullptr)
            {
                pVirtualizationInstanceObj->OnNotifyFileHandleClosedFileModifiedOrDeleted(gcnew String(callbackData->FilePathName),
                                                                                          isDirectory != FALSE,
                                                                                          true,   // isFileModified
                                                                                          false,  // isFileDeleted
                                                                                          callbackData->TriggeringProcessId,
                                                                                          GetTriggeringProcessNameSafe(callbackData));
            }
            break;

        case PRJ_NOTIFICATION_FILE_HANDLE_CLOSED_FILE_DELETED:
            if (pVirtualizationInstanceObj->OnNotifyFileHandleClosedFileModifiedOrDeleted != nullptr)
            {
                pVirtualizationInstanceObj->OnNotifyFileHandleClosedFileModifiedOrDeleted(gcnew String(callbackData->FilePathName),
                                                                                          isDirectory != FALSE,
                                                                                          notificationParameters->FileDeletedOnHandleClose.IsFileModified != FALSE,   // isFileModified
                                                                                          true,                                                                       // isFileDeleted
                                                                                          callbackData->TriggeringProcessId,
                                                                                          GetTriggeringProcessNameSafe(callbackData));
            }
            break;

        case PRJ_NOTIFICATION_FILE_PRE_CONVERT_TO_FULL:
            if (pVirtualizationInstanceObj->OnNotifyFilePreConvertToFull != nullptr)
            {
                if (!pVirtualizationInstanceObj->OnNotifyFilePreConvertToFull(gcnew String(callbackData->FilePathName),
                                                                              callbackData->TriggeringProcessId,
                                                                              GetTriggeringProcessNameSafe(callbackData)))
                {
                    notificationResult = HResult::AccessDenied;
                }
            }
            break;

        default:
            // Unexpected notification type
            break;
        }

        return static_cast<HRESULT>(notificationResult);
    }

    return HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR);
}

#pragma region Windows 10 1803 support

HRESULT PrjGetPlaceholderInformationCB(_In_ PRJ_CALLBACK_DATA* callbackData,
                                       _In_ DWORD desiredAccess,
                                       _In_ DWORD shareMode,
                                       _In_ DWORD createDisposition,
                                       _In_ DWORD createOptions,
                                       _In_ LPCWSTR destinationFileName)
{
    UNREFERENCED_PARAMETER(desiredAccess);
    UNREFERENCED_PARAMETER(shareMode);
    UNREFERENCED_PARAMETER(createDisposition);
    UNREFERENCED_PARAMETER(createOptions);
    UNREFERENCED_PARAMETER(destinationFileName);

    return PrjGetPlaceholderInfoCB(callbackData);
}

HRESULT PrjGetFileStreamCB(_In_ PRJ_CALLBACK_DATA* callbackData,
                           _In_ LARGE_INTEGER byteOffset,
                           _In_ DWORD length)
{
    return PrjGetFileDataCB(callbackData, byteOffset.QuadPart, length);
}

HRESULT PrjNotifyOperationCB(_In_ PRJ_CALLBACK_DATA* callbackData,
                             _In_ BOOLEAN isDirectory,
                             _In_ PRJ_NOTIFICATION_TYPE notificationType,
                             _In_opt_ LPCWSTR destinationFileName,
                             _Inout_ PRJ_OPERATION_PARAMETERS* operationParameters)
{
    HRESULT hr;
    PRJ_NOTIFICATION_PARAMETERS notificationParameters = {};

    // Transfer input parameters to 1803-style parameter structure.
    switch (notificationType)
    {
    case PRJ_NOTIFICATION_FILE_HANDLE_CLOSED_FILE_DELETED:
        notificationParameters.FileDeletedOnHandleClose.IsFileModified = operationParameters->FileDeletedOnHandleClose.IsFileModified;
        break;
    }

    hr = PrjNotificationCB(callbackData, isDirectory, notificationType, destinationFileName, &notificationParameters);

    // Transfer output parameters from 1803-style parameter structure.
    switch (notificationType)
    {
    case PRJ_NOTIFICATION_FILE_OPENED:
    case PRJ_NOTIFICATION_NEW_FILE_CREATED:
    case PRJ_NOTIFICATION_FILE_OVERWRITTEN:

        operationParameters->PostCreate.NotificationMask = notificationParameters.PostCreate.NotificationMask;
        break;

    case PRJ_NOTIFICATION_FILE_RENAMED:

        operationParameters->FileRenamed.NotificationMask = notificationParameters.FileRenamed.NotificationMask;
        break;
    }

    return hr;
}

#pragma endregion

#pragma endregion

#pragma region Helper methods

inline array<Byte>^ MarshalPlaceholderId(UCHAR* sourceId)
{
    array<Byte>^ marshalledId = gcnew array<Byte>(PRJ_PLACEHOLDER_ID_LENGTH);
    pin_ptr<byte> pinnedId = &marshalledId[0];
    memcpy(pinnedId, sourceId, PRJ_PLACEHOLDER_ID_LENGTH);
    return marshalledId;
}

inline void CopyPlaceholderId(UCHAR* destinationId, array<Byte>^ sourceId)
{
    if (sourceId != nullptr && sourceId->Length > 0)
    {
        pin_ptr<byte> pinnedId = &sourceId[0];
        memcpy(
            destinationId,
            pinnedId,
            min(sourceId->Length * sizeof(byte), PRJ_PLACEHOLDER_ID_LENGTH));
    }
}

inline bool IsPowerOf2(unsigned long num)
{
    return (num & (num - 1)) == 0;
}

inline Guid GUIDtoGuid(const GUID& guid)
{
    return Guid(
        guid.Data1,
        guid.Data2,
        guid.Data3,
        guid.Data4[0],
        guid.Data4[1],
        guid.Data4[2],
        guid.Data4[3],
        guid.Data4[4],
        guid.Data4[5],
        guid.Data4[6],
        guid.Data4[7]);
}

inline std::shared_ptr<PRJ_PLACEHOLDER_INFORMATION> CreatePlaceholderInformation(
    DateTime creationTime,
    DateTime lastAccessTime,
    DateTime lastWriteTime,
    DateTime changeTime,
    FileAttributes fileAttributes,
    long long endOfFile,
    bool directory,
    array<Byte>^ contentId,
    array<Byte>^ providerId)
{
    std::shared_ptr<PRJ_PLACEHOLDER_INFORMATION> fileInformation(static_cast<PRJ_PLACEHOLDER_INFORMATION*>(malloc(sizeof(PRJ_PLACEHOLDER_INFORMATION))), free);
    fileInformation->Size = sizeof(PRJ_PLACEHOLDER_INFORMATION);

    memset(&fileInformation->FileBasicInfo, 0, sizeof(PRJ_FILE_BASIC_INFO));
    fileInformation->FileBasicInfo.FileSize = endOfFile;
    fileInformation->FileBasicInfo.IsDirectory = directory;
    fileInformation->FileBasicInfo.CreationTime.QuadPart = creationTime.ToFileTime();
    fileInformation->FileBasicInfo.LastAccessTime.QuadPart = lastAccessTime.ToFileTime();
    fileInformation->FileBasicInfo.LastWriteTime.QuadPart = lastWriteTime.ToFileTime();
    fileInformation->FileBasicInfo.ChangeTime.QuadPart = changeTime.ToFileTime();
    fileInformation->FileBasicInfo.FileAttributes = static_cast<UINT32>(fileAttributes);

    fileInformation->EaInformation.EaBufferSize = 0;
    fileInformation->EaInformation.OffsetToFirstEa = static_cast<unsigned long>(-1);

    fileInformation->SecurityInformation.SecurityBufferSize = 0;
    fileInformation->SecurityInformation.OffsetToSecurityDescriptor = static_cast<unsigned long>(-1);

    fileInformation->StreamsInformation.StreamsInfoBufferSize = 0;
    fileInformation->StreamsInformation.OffsetToFirstStreamInfo = static_cast<unsigned long>(-1);

    memset(&fileInformation->VersionInfo, 0, sizeof(PRJ_PLACEHOLDER_VERSION_INFO));
    CopyPlaceholderId(fileInformation->VersionInfo.ProviderID, providerId);
    CopyPlaceholderId(fileInformation->VersionInfo.ContentID, contentId);

    return fileInformation;
}

inline std::shared_ptr<PRJ_PLACEHOLDER_INFO> CreatePlaceholderInfo(
    DateTime creationTime,
    DateTime lastAccessTime,
    DateTime lastWriteTime,
    DateTime changeTime,
    FileAttributes fileAttributes,
    long long endOfFile,
    bool directory,
    array<Byte>^ contentId,
    array<Byte>^ providerId)
{
    std::shared_ptr<PRJ_PLACEHOLDER_INFO> placeholderInfo(static_cast<PRJ_PLACEHOLDER_INFO*>(calloc(1,
                                                                                                    sizeof(PRJ_PLACEHOLDER_INFO))),
                                                          free);

    placeholderInfo->FileBasicInfo.IsDirectory = directory;
    placeholderInfo->FileBasicInfo.FileSize = endOfFile;
    placeholderInfo->FileBasicInfo.CreationTime.QuadPart = creationTime.ToFileTime();
    placeholderInfo->FileBasicInfo.LastAccessTime.QuadPart = lastAccessTime.ToFileTime();
    placeholderInfo->FileBasicInfo.LastWriteTime.QuadPart = lastWriteTime.ToFileTime();
    placeholderInfo->FileBasicInfo.ChangeTime.QuadPart = changeTime.ToFileTime();
    placeholderInfo->FileBasicInfo.FileAttributes = static_cast<UINT32>(fileAttributes);

    CopyPlaceholderId(placeholderInfo->VersionInfo.ProviderID, providerId);
    CopyPlaceholderId(placeholderInfo->VersionInfo.ContentID, contentId);

    return placeholderInfo;
}

inline String^ GetTriggeringProcessNameSafe(const PRJ_CALLBACK_DATA* callbackData)
{
    return (callbackData->TriggeringProcessImageFileName != NULL)
        ? gcnew String(callbackData->TriggeringProcessImageFileName)
        : String::Empty;
}
#pragma endregion
}