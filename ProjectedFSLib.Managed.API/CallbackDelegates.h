#pragma once

#include "DirectoryEnumerationResults.h"
#include "WriteBuffer.h"

using namespace System::Runtime::InteropServices;

namespace Microsoft {
namespace Windows {
namespace ProjFS {

    /// <summary>Determines whether a given file path exists in the provider's store.</summary>
    /// <remarks>
    ///     <para>
    ///     The provider sets its implementation of this delegate into the <c>OnQueryFileName</c>
    ///     property of <c>ProjFS.VirtualizationInstance</c>.
    ///     </para>
    ///     <para>
    ///     If the provider does not implement this callback, ProjFS will call the enumeration callbacks
    ///     when it needs to find out whether a file path exists in the provider’s store.
    ///     </para>
    /// </remarks>
    /// <param name="relativePath">The path, relative to the virtualization root, of the file being queried.</param>
    /// <returns>
    ///     <para><see cref="HResult::Ok"/> if <paramref name="relativePath"/> exists in the provider's store.</para>
    ///     <para><see cref="HResult::FileNotFound"/> if <paramref name="relativePath"/> does not exist in the provider's store.</para>
    ///     <para>An appropriate error code if the provider fails the operation.</para>
    /// </returns>
    public delegate HResult QueryFileNameCallback(
        System::String^ relativePath);

    /// <summary>Informs the provider that an operation begun by an earlier invocation of a callback
    /// is to be canceled.</summary>
    /// <remarks>
    ///     <para>
    ///     The provider sets its implementation of this delegate into the <c>OnCancelCommand</c>
    ///     property of <c>ProjFS.VirtualizationInstance</c>.
    ///     </para>
    ///     <para>
    ///     ProjFS invokes this callback to indicate that the I/O that caused the earlier callback
    ///     to be invoked was canceled, either explicitly or because the thread it was issued on terminated.
    ///     </para>
    ///     <para>
    ///     Calling <c>ProjFS.VirtualizationInstance.CompleteCommand</c> for the <paramref name="commandId"/>
    ///     passed by this callback is not an error, however it is a no-op because the I/O that caused
    ///     the callback invocation identified by <paramref name="commandId"/> has already ended.
    ///     </para>
    ///     <para>
    ///     ProjFS will invoke this callback for a given <paramref name="commandId"/> only after
    ///     the callback to be canceled is invoked.  However if the provider is configured to allow
    ///     more than one concurrently running worker thread, the cancellation and original invocation
    ///     may run concurrently.  The provider must be able to handle this situation.
    ///     </para>
    ///     <para>
    ///     A provider that does not return <see cref="HResult::Pending"/> to any of its callbacks does
    ///     not need to handle this callback.
    ///     </para>
    /// </remarks>
    /// <param name="commandId">A value that identifies the callback invocation to be canceled.
    /// Corresponds to the <paramref name="commandId"/> parameter of callbacks whose processing
    /// can be canceled.</param>
    public delegate void CancelCommandCallback(
        int commandId);

    /// <summary>Indicates that a handle has been created to an existing file or directory.</summary>
    /// <remarks>
    ///     <para>
    ///     The provider sets its implementation of this delegate into the <c>OnNotifyFileOpened</c>
    ///     property of <c>ProjFS.VirtualizationInstance</c>.
    ///     </para>
    ///     <para>
    ///     ProjFS will invoke this callback if the provider registered for <see cref="NotificationType::FileOpened"/>
    ///     when it started the virtualization instance.
    ///     </para>
    ///     <para>
    ///     If the provider returns <c>false</c>, then the file system will cancel the open of the file and
    ///     return STATUS_ACCESS_DENIED to the caller trying to open the file.
    ///     </para>
    /// </remarks>
    /// <param name="relativePath">The path, relative to the virtualization root, of the file or directory.</param>
    /// <param name="isDirectory"><c>true</c> if <paramref name="relativePath"/> is for a directory,
    ///     <c>false</c> if <paramref name="relativePath"/> is for a file.</param>
    /// <param name="triggeringProcessId">The PID of the process that triggered this callback. If this
    ///     information is not available, this will be 0.</param>
    /// <param name="triggeringProcessImageFileName">The image file name corresponding to <paramref name="triggeringProcessId"/>.
    ///     If <paramref name="triggeringProcessId"/> is 0 this will be null.</param>
    /// <param name="notificationMask">Upon return from this callback specifies a bitwise-OR of
    ///     <see cref="NotificationType"/> values indicating the set of notifications the provider
    ///     wishes to receive for this file.
    ///     <para>If the provider sets this value to 0, it is equivalent to specifying <see cref="NotificationType::UseExistingMask"/>.</para>
    /// </param>
    /// <returns>
    ///     <para><c>true</c> if the provider wants to allow the opened file to be returned to the
    ///     caller, <c>false</c> otherwise.</para>
    /// </returns>
    public delegate bool NotifyFileOpenedCallback(
        System::String^ relativePath,
        bool isDirectory,
        unsigned int triggeringProcessId,
        System::String^ triggeringProcessImageFileName,
        [Out] NotificationType% notificationMask);

    /// <summary>Indicates that a new file or directory has been created.</summary>
    /// <remarks>
    ///     <para>
    ///     The provider sets its implementation of this delegate into the <c>OnNotifyNewFileCreated</c>
    ///     property of <c>ProjFS.VirtualizationInstance</c>.
    ///     </para>
    ///     <para>ProjFS will invoke this callback if the provider registered for <see cref="NotificationType::NewFileCreated"/>
    ///     when it started the virtualization instance.</para>
    /// </remarks>
    /// <param name="relativePath">The path, relative to the virtualization root, of the file or directory.</param>
    /// <param name="isDirectory"><c>true</c> if <paramref name="relativePath"/> is for a directory,
    ///     <c>false</c> if <paramref name="relativePath"/> is for a file.</param>
    /// <param name="triggeringProcessId">The PID of the process that triggered this callback. If this
    ///     information is not available, this will be 0.</param>
    /// <param name="triggeringProcessImageFileName">The image file name corresponding to <paramref name="triggeringProcessId"/>.
    ///     If <paramref name="triggeringProcessId"/> is 0 this will be null.</param>
    /// <param name="notificationMask">Upon return from this callback specifies a bitwise-OR of
    ///     <see cref="NotificationType"/> values indicating the set of notifications the provider
    ///     wishes to receive for this file.
    ///     <para>If the provider sets this value to 0, it is equivalent to specifying <see cref="NotificationType::UseExistingMask"/>.</para>
    /// </param>
    public delegate void NotifyNewFileCreatedCallback(
        System::String^ relativePath,
        bool isDirectory,
        unsigned int triggeringProcessId,
        System::String^ triggeringProcessImageFileName,
        [Out] NotificationType% notificationMask);

    /// <summary>Indicates that an existing file has been superseded or overwritten.</summary>
    /// <remarks>
    ///     <para>
    ///     The provider sets its implementation of this delegate into the <c>OnNotifyFileOverwritten</c>
    ///     property of <c>ProjFS.VirtualizationInstance</c>.
    ///     </para>
    ///     <para>ProjFS will invoke this callback if the provider registered for <see cref="NotificationType::FileOverwritten"/>
    ///     when it started the virtualization instance.</para>
    /// </remarks>
    /// <param name="relativePath">The path, relative to the virtualization root, of the file or directory.</param>
    /// <param name="isDirectory"><c>true</c> if <paramref name="relativePath"/> is for a directory,
    ///     <c>false</c> if <paramref name="relativePath"/> is for a file.</param>
    /// <param name="triggeringProcessId">The PID of the process that triggered this callback. If this
    ///     information is not available, this will be 0.</param>
    /// <param name="triggeringProcessImageFileName">The image file name corresponding to <paramref name="triggeringProcessId"/>.
    ///     If <paramref name="triggeringProcessId"/> is 0 this will be null.</param>
    /// <param name="notificationMask">Upon return from this callback specifies a bitwise-OR of
    ///     <see cref="NotificationType"/> values indicating the set of notifications the provider
    ///     wishes to receive for this file.
    ///     <para>If the provider sets this value to 0, it is equivalent to specifying <see cref="NotificationType::UseExistingMask"/>.</para>
    /// </param>
    public delegate void NotifyFileOverwrittenCallback(
        System::String^ relativePath,
        bool isDirectory,
        unsigned int triggeringProcessId,
        System::String^ triggeringProcessImageFileName,
        [Out] NotificationType% notificationMask);

    /// <summary>Indicates that a file or directory is about to be deleted.</summary>
    /// <remarks>
    ///     <para>
    ///     The provider sets its implementation of this delegate into the <c>OnNotifyPreDelete</c>
    ///     property of <c>ProjFS.VirtualizationInstance</c>.
    ///     </para>
    ///     <para>ProjFS will invoke this callback if the provider registered for <see cref="NotificationType::PreDelete"/>
    ///     when it started the virtualization instance.</para>
    ///     <para>If the provider returns <c>false</c> then the file system will return STATUS_CANNOT_DELETE
    ///     from the operation that triggered the delete, and the delete will not take place.</para>
    /// </remarks>
    /// <param name="relativePath">The path, relative to the virtualization root, of the file or directory.</param>
    /// <param name="isDirectory"><c>true</c> if <paramref name="relativePath"/> is for a directory,
    ///     <c>false</c> if <paramref name="relativePath"/> is for a file.</param>
    /// <param name="triggeringProcessId">The PID of the process that triggered this callback. If this
    ///     information is not available, this will be 0.</param>
    /// <param name="triggeringProcessImageFileName">The image file name corresponding to <paramref name="triggeringProcessId"/>.
    ///     If <paramref name="triggeringProcessId"/> is 0 this will be null.</param>
    /// <returns>
    ///     <para><c>true</c> if the provider wants to allow the delete to happen, false if it wants
    ///     to prevent the delete.</para>
    /// </returns>
    public delegate bool NotifyPreDeleteCallback(
        System::String^ relativePath,
        bool isDirectory,
        unsigned int triggeringProcessId,
        System::String^ triggeringProcessImageFileName);

    /// <summary>Indicates that a file or directory is about to be renamed.</summary>
    /// <remarks>
    ///     <para>
    ///     The provider sets its implementation of this delegate into the <c>OnNotifyPreRename</c>
    ///     property of <c>ProjFS.VirtualizationInstance</c>.
    ///     </para>
    ///     <para>ProjFS will invoke this callback if the provider registered for <see cref="NotificationType::PreRename"/>
    ///     when it started the virtualization instance.</para>
    ///     <para>If both the <paramref name="relativePath"/> and <paramref name="destinationPath"/>
    ///     parameters of this callback are non-empty strings, this indicates that the source and
    ///     destination of the rename are under the virtualization root.  If the provider specified
    ///     different notification masks in the <paramref name="notificationMappings"/> parameter of
    ///     <c>ProjFS.VirtualizationInstance.StartVirtualizing</c> for the source and destination
    ///     paths, then ProjFS will send this notification if the provider specified
    ///     <see cref="NotificationType::PreRename"/> when registering either the source or destination
    ///     paths.</para>
    ///     <para>If the provider returns <c>false</c>, then the file system will return STATUS_ACCESS_DENIED
    ///     from the rename operation, and the rename will not take effect.</para>
    /// </remarks>
    /// <param name="relativePath">The path, relative to the virtualization root, of the file or directory
    ///     to be renamed.
    ///     <para>This parameter will be "" to indicate that the rename will move the file or directory
    ///     from a location not under the virtualization root.  In that case ProjFS will always send
    ///     this notification if the provider has implemented this callback, even if the provider did
    ///     not specify <see cref="NotificationType::PreRename"/> when registering the subtree containing
    ///     the destination path.
    ///     </para>
    /// </param>
    /// <param name="destinationPath">The path, relative to the virtualization root, to which the file
    ///     or directory will be renamed.
    ///     <para>This parameter will be "" to indicate that the rename will move the file or directory
    ///     out of the virtualization instance.
    ///     </para>
    /// </param>
    /// <param name="triggeringProcessId">The PID of the process that triggered this callback. If this
    ///     information is not available, this will be 0.</param>
    /// <param name="triggeringProcessImageFileName">The image file name corresponding to <paramref name="triggeringProcessId"/>.
    ///     If <paramref name="triggeringProcessId"/> is 0 this will be null.</param>
    /// <returns>
    ///     <para><c>true</c> if the provider wants to allow the rename to happen, false if it wants
    ///     to prevent the rename.</para>
    /// </returns>
    public delegate bool NotifyPreRenameCallback(
        System::String^ relativePath,
        System::String^ destinationPath,
        unsigned int triggeringProcessId,
        System::String^ triggeringProcessImageFileName);

    /// <summary>Indicates that a hard link is about to be created for the file.</summary>
    /// <remarks>
    ///     <para>
    ///     The provider sets its implementation of this delegate into the <c>OnNotifyPreCreateHardlink</c>
    ///     property of <c>ProjFS.VirtualizationInstance</c>.
    ///     </para>
    ///     <para>ProjFS will invoke this callback if the provider registered for <see cref="NotificationType::PreCreateHardlink"/>
    ///     when it started the virtualization instance.</para>
    ///     <para>If both the <paramref name="relativePath"/> and <paramref name="destinationPath"/>
    ///     parameters of this callback are non-empty strings, this indicates that the new hard link
    ///     will be created under the virtualization root for a file that is under the virtualization
    ///     root.  If the provider specified different notification masks in the
    ///     <paramref name="notificationMappings"/> parameter of <c>ProjFS.VirtualizationInstance.StartVirtualizing</c>
    ///     for the source and destination paths, then ProjFS will send this notification if the provider
    ///     specified <see cref="NotificationType::PreCreateHardlink"/> when registering either the
    ///     source or destination paths.</para>
    ///     <para>If the provider returns <c>false</c>, then the file system will return STATUS_ACCESS_DENIED
    ///     from the hard link operation, and the hard link will not be created.</para>
    /// </remarks>
    /// <param name="relativePath">The path, relative to the virtualization root, of the file or directory
    ///     for which the hard link is to be created.
    ///     <para>This parameter will be "" to indicate that the hard link name will be created under
    ///     the virtualization root, i.e. a new hard link is being created under the virtualization
    ///     root to a file whose path is not under the virtualization root.</para>
    /// </param>
    /// <param name="destinationPath">The path, relative to the virtualization root, of the new hard
    ///     link name.
    ///     <para>This parameter will be "" to indicate that the hard link name will be created in
    ///     a location not under the virtualization, i.e. a new hard link is being created in a location
    ///     not under the virtualization for a file is under the virtualization root.</para>
    /// </param>
    /// <param name="triggeringProcessId">The PID of the process that triggered this callback. If this
    ///     information is not available, this will be 0.</param>
    /// <param name="triggeringProcessImageFileName">The image file name corresponding to <paramref name="triggeringProcessId"/>.
    ///     If <paramref name="triggeringProcessId"/> is 0 this will be null.</param>
    /// <returns>
    ///     <para><c>true</c> if the provider wants to allow the hard link operation to happen, false
    ///     if it wants to prevent the hard link operation.</para>
    /// </returns>
    public delegate bool NotifyPreCreateHardlinkCallback(
        System::String^ relativePath,
        System::String^ destinationPath,
        unsigned int triggeringProcessId,
        System::String^ triggeringProcessImageFileName);

    /// <summary>
    /// Indicates that a file or directory has been renamed.  The file or directory may have been moved
    /// into the virtualization instance.
    /// </summary>
    /// <remarks>
    ///     <para>
    ///     The provider sets its implementation of this delegate into the <c>OnNotifyFileRenamed</c>
    ///     property of <c>ProjFS.VirtualizationInstance</c>.
    ///     </para>
    ///     <para>ProjFS will invoke this callback if the provider registered for <see cref="NotificationType::FileRenamed"/>
    ///     when it started the virtualization instance.</para>
    ///     <para>If both the <paramref name="relativePath"/> and <paramref name="destinationPath"/>
    ///     parameters of this callback are non-empty strings, this indicates that the source and
    ///     destination of the rename were both under the virtualization root.  If the provider specified
    ///     different notification masks in the <paramref name="notificationMappings"/> parameter of
    ///     <c>ProjFS.VirtualizationInstance.StartVirtualizing</c> for the source and destination
    ///     paths, then ProjFS will send this notification if the provider specified
    ///     <see cref="NotificationType::FileRenamed"/> when registering either the source or destination
    ///     paths.</para>
    /// </remarks>
    /// <param name="relativePath">The original path, relative to the virtualization root, of the file
    ///     or directory that was renamed.
    ///     <para>This parameter will be "" to indicate that the rename moved the file or directory
    ///     from outside the virtualization instance.  In that case ProjFS will always send this notification
    ///     if the provider has implemented this callback, even if the provider did not specify <see cref="NotificationType::FileRenamed"/>
    ///     when registering the subtree containing the destination path.
    ///     </para>
    /// </param>
    /// <param name="destinationPath">The path, relative to the virtualization root, to which the file
    ///     or directory was renamed.
    ///     <para>This parameter will be "" to indicate that the rename moved the file or directory
    ///     out of the virtualization instance.
    ///     </para>
    /// </param>
    /// <param name="isDirectory"><c>true</c> if <paramref name="relativePath"/> is for a directory,
    ///     <c>false</c> if <paramref name="relativePath"/> is for a file.</param>
    /// <param name="triggeringProcessId">The PID of the process that triggered this callback. If this
    ///     information is not available, this will be 0.</param>
    /// <param name="triggeringProcessImageFileName">The image file name corresponding to <paramref name="triggeringProcessId"/>.
    ///     If <paramref name="triggeringProcessId"/> is 0 this will be null.</param>
    /// <param name="notificationMask">Upon return from this callback specifies a bitwise-OR of
    ///     <see cref="NotificationType"/> values indicating the set of notifications the provider
    ///     wishes to receive for this file.
    ///     <para>If the provider sets this value to 0, it is equivalent to specifying <see cref="NotificationType::UseExistingMask"/>.</para>
    /// </param>
    public delegate void NotifyFileRenamedCallback(
        System::String^ relativePath,
        System::String^ destinationPath,
        bool isDirectory,
        unsigned int triggeringProcessId,
        System::String^ triggeringProcessImageFileName,
        [Out] NotificationType% notificationMask);

    /// <summary>Indicates that a hard link has been created for the file.</summary>
    /// <remarks>
    ///     <para>
    ///     The provider sets its implementation of this delegate into the <c>OnNotifyHardlinkCreated</c>
    ///     property of <c>ProjFS.VirtualizationInstance</c>.
    ///     </para>
    ///     <para>ProjFS will invoke this callback if the provider registered for <see cref="NotificationType::HardlinkCreated"/>
    ///     when it started the virtualization instance.</para>
    ///     <para>If both the <paramref name="relativePath"/> and <paramref name="destinationPath"/>
    ///     parameters of this callback are non-empty strings, this indicates that the new hard link
    ///     was created under the virtualization root for a file that exists under the virtualization
    ///     root.  If the provider specified different notification masks in the <paramref name="notificationMappings"/>
    ///     parameter of <c>ProjFS.VirtualizationInstance.StartVirtualizing</c> for the source and
    ///     destination paths, then ProjFS will send this notification if the provider specified
    ///     <see cref="NotificationType::HardlinkCreated"/> when registering either the source or destination
    ///     paths.</para>
    /// </remarks>
    /// <param name="relativePath">The path, relative to the virtualization root, of the file for
    ///     which the hard link was created.
    ///     <para>This parameter will be "" to indicate that the hard link name was created under the
    ///     virtualization root, i.e. a new hard link was created under the virtualization to a file
    ///     that exists in a location not under the virtualization root.</para>
    /// </param>
    /// <param name="destinationPath">The path, relative to the virtualization root, of the new hard
    ///     link name.
    ///     <para>This parameter will be "" to indicate that the hard link name was created in a
    ///     location not under the virtualization root, i.e. a new hard link was created in a location
    ///     not under the virtualization root for a file that is under the virtualization root.</para>
    /// </param>
    /// <param name="triggeringProcessId">The PID of the process that triggered this callback. If this
    ///     information is not available, this will be 0.</param>
    /// <param name="triggeringProcessImageFileName">The image file name corresponding to <paramref name="triggeringProcessId"/>.
    ///     If <paramref name="triggeringProcessId"/> is 0 this will be null.</param>
    public delegate void NotifyHardlinkCreatedCallback(
        System::String^ relativePath,
        System::String^ destinationPath,
        unsigned int triggeringProcessId,
        System::String^ triggeringProcessImageFileName);

    /// <summary>
    /// Indicates that a handle has been closed on the file or directory, and that the file was not modified
    /// while that handle was open, and that the file or directory was not deleted as part of closing the handle.
    /// </summary>
    /// <remarks>
    ///     <para>
    ///     The provider sets its implementation of this delegate into the <c>OnNotifyFileHandleClosedNoModification</c>
    ///     property of <c>ProjFS.VirtualizationInstance</c>.
    ///     </para>
    ///     <para>ProjFS will invoke this callback if the provider registered for <see cref="NotificationType::FileHandleClosedNoModification"/>
    ///     when it started the virtualization instance.</para>
    /// </remarks>
    /// <param name="relativePath">The path, relative to the virtualization root, of the file or directory.</param>
    /// <param name="isDirectory"><c>true</c> if <paramref name="relativePath"/> is for a directory,
    ///     <c>false</c> if <paramref name="relativePath"/> is for a file.</param>
    /// <param name="triggeringProcessId">The PID of the process that triggered this callback. If this
    ///     information is not available, this will be 0.</param>
    /// <param name="triggeringProcessImageFileName">The image file name corresponding to <paramref name="triggeringProcessId"/>.
    ///     If <paramref name="triggeringProcessId"/> is 0 this will be null.</param>
    public delegate void NotifyFileHandleClosedNoModificationCallback(
        System::String^ relativePath,
        bool isDirectory,
        unsigned int triggeringProcessId,
        System::String^ triggeringProcessImageFileName);

    /// <summary>
    /// Indicates that a handle has been closed on the file or directory, and whether the file was modified
    /// while that handle was open, or that the file or directory was deleted as part of closing the handle.
    /// </summary>
    /// <remarks>
    ///     <para>
    ///     The provider sets its implementation of this delegate into the <c>OnNotifyFileHandleClosedFileModifiedOrDeleted</c>
    ///     property of <c>ProjFS.VirtualizationInstance</c>.
    ///     </para>
    ///     <para>ProjFS will invoke this callback if the provider registered for <see cref="NotificationType::FileHandleClosedFileModified"/>
    ///     or <see cref="NotificationType::FileHandleClosedFileDeleted"/> when it started the virtualization instance.</para>
    /// </remarks>
    /// <param name="relativePath">The path, relative to the virtualization root, of the file or directory.</param>
    /// <param name="isDirectory"><c>true</c> if <paramref name="relativePath"/> is for a directory,
    ///     <c>false</c> if <paramref name="relativePath"/> is for a file.</param>
    /// <param name="isFileModified"><c>true</c> if the file or directory was modified while the handle
    ///     was open, <c>false</c> otherwise.</param>
    /// <param name="isFileDeleted"><c>true</c> if the file or directory was deleted as part of closing
    ///     the handle, <c>false</c> otherwise.</param>
    /// <param name="triggeringProcessId">The PID of the process that triggered this callback. If this
    ///     information is not available, this will be 0.</param>
    /// <param name="triggeringProcessImageFileName">The image file name corresponding to <paramref name="triggeringProcessId"/>.
    ///     If <paramref name="triggeringProcessId"/> is 0 this will be null.</param>
    public delegate void NotifyFileHandleClosedFileModifiedOrDeletedCallback(
        System::String^ relativePath,
        bool isDirectory,
        bool isFileModified,
        bool isFileDeleted,
        unsigned int triggeringProcessId,
        System::String^ triggeringProcessImageFileName);

    /// <summary>
    /// Indicates that the file is about to be converted from a placeholder to a full file, i.e. its
    /// contents are likely to be modified.
    /// </summary>
    /// <remarks>
    ///     <para>
    ///     The provider sets its implementation of this delegate into the <c>OnNotifyFilePreConvertToFull</c>
    ///     property of <c>ProjFS.VirtualizationInstance</c>.
    ///     </para>
    ///     <para>ProjFS will invoke this callback if the provider registered for <see cref="NotificationType::FilePreConvertToFull"/>
    ///     when it started the virtualization instance.</para>
    ///     <para>If the provider returns <c>false</c>, then the file system will return
    ///     STATUS_ACCESS_DENIED from the operation that triggered the conversion, and the placeholder
    ///     will not be converted to a full file.</para>
    /// </remarks>
    /// <param name="relativePath">The path, relative to the virtualization root, of the file or directory.</param>
    /// <param name="triggeringProcessId">The PID of the process that triggered this callback. If this
    ///     information is not available, this will be 0.</param>
    /// <param name="triggeringProcessImageFileName">The image file name corresponding to <paramref name="triggeringProcessId"/>.
    ///     If <paramref name="triggeringProcessId"/> is 0 this will be null.</param>
    /// <returns>
    ///     <para><c>true</c> if the provider wants to allow the file to be converted to full, <c>false</c>
    ///     if it wants to prevent the file from being converted to full.</para>
    /// </returns>
    public delegate bool NotifyFilePreConvertToFullCallback(
        System::String^ relativePath,
        unsigned int triggeringProcessId,
        System::String^ triggeringProcessImageFileName);

}}} // namespace Microsoft.Windows.ProjFS