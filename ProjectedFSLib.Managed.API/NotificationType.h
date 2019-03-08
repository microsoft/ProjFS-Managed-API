// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

namespace Microsoft {
namespace Windows {
namespace ProjFS {
    /// <summary>
    /// Defines values for file system operation notifications ProjFS can send to a provider.
    /// </summary>
    /// <remarks>
    ///     <para>
    ///     ProjFS can send notifications of file system activity to a provider.  When the provider
    ///     starts a virtualization instance it specifies which notifications it wishes to receive.
    ///     It may also specify a new set of notifications for a file when it is created or renamed.
    ///     The provider must set implementations of <c>Notify...Callback</c> delegates in the <c>OnNotify...</c>
    ///     properties of <c>ProjFS.VirtualizationInstance</c> in order to receive the notifications
    ///     for which it registers.
    ///     </para>
    ///     <para>
    ///     ProjFS sends notifications for files and directories managed by an active virtualization
    ///     instance. That is, ProjFS will send notifications for the virtualization root and its
    ///     descendants.  Symbolic links and junctions within the virtualization root are not traversed
    ///     when determining what constitutes a descendant of the virtualization root.
    ///     </para>
    ///     <para>
    ///     ProjFS sends notifications only for the primary data stream of a file.  ProjFS does not
    ///     send notifications for operations on alternate data streams.
    ///     </para>
    ///     <para>
    ///     ProjFS does not send notifications for an inactive virtualization instance.  A virtualization
    ///     instance is inactive if any one of the following is true:
    ///         <list type="bullet">
    ///             <item>
    ///                 <description>
    ///                 The provider has not yet started it by calling <c>ProjFS.VirtualizationInstance.StartVirtualizing</c>.
    ///                 </description>
    ///             </item>
    ///             <item>
    ///                 <description>
    ///                 The provider has stopped the instance by calling <c>ProjFS.VirtualizationInstance.StopVirtualizing</c>.
    ///                 </description>
    ///             </item>
    ///             <item>
    ///                 <description>
    ///                 The provider process has exited.
    ///                 </description>
    ///             </item>
    ///         </list>
    ///     </para>
    ///     <para>
    ///     The provider may specify which notifications it wishes to receive when starting a virtualization
    ///     instance, or in response to a notification that allows a new notification mask to be set.
    ///     The provider specifies a default set of notifications that it wants ProjFS to send for the
    ///     virtualization instance when it starts the instance.  The provider specifies the default
    ///     notifications via the <paramref name="notificationMappings"/> parameter of the
    ///     <c>ProjFS.VirtualizationInstance</c> constructor, which may specify different notification
    ///     masks for different subtrees of the virtualization instance.
    ///     </para>
    ///     <para>
    ///     The provider may choose to supply a different notification mask in response to a notification
    ///     of file open, create, overwrite, or rename.  ProjFS will continue to send these notifications
    ///     for the given file until all handles to the file are closed.  After that it will revert
    ///     to the default set of notifications.  Naturally if the default set of notifications does
    ///     not specify that ProjFS should notify for open, create, etc., the provider will not get
    ///     the opportunity to specify a different mask for those operations.
    ///     </para>
    /// </remarks>
    [System::FlagsAttribute]
    public enum class NotificationType : unsigned long
    {
        /// <summary>
        /// Indicates that the provider does not want any notifications.  This value overrides all others.
        /// </summary>
        None = PRJ_NOTIFY_SUPPRESS_NOTIFICATIONS,

        /// <summary>
        /// Indicates that ProjFS should call the provider's <c>OnNotifyFileOpened</c> callback when a handle is created to an existing file or directory.
        /// </summary>
        FileOpened = PRJ_NOTIFY_FILE_OPENED,

        /// <summary>
        /// Indicates that ProjFS should call the provider's <c>OnNotifyNewFileCreated</c> callback when a new file or directory is created.
        /// </summary>
        NewFileCreated = PRJ_NOTIFY_NEW_FILE_CREATED,

        /// <summary>
        /// Indicates that ProjFS should call the provider's <c>OnNotifyFileOverwritten</c> callback when an existing file is superseded or overwritten.
        /// </summary>
        FileOverwritten = PRJ_NOTIFY_FILE_OVERWRITTEN,

        /// <summary>
        /// Indicates that ProjFS should call the provider's <c>OnNotifyPreDelete</c> callback when a file or directory is about to be deleted.
        /// </summary>
        PreDelete = PRJ_NOTIFY_PRE_DELETE,

        /// <summary>
        /// Indicates that ProjFS should call the provider's <c>OnNotifyPreRename</c> callback when a file or directory is about to be renamed.
        /// </summary>
        PreRename = PRJ_NOTIFY_PRE_RENAME,

        /// <summary>
        /// Indicates that ProjFS should call the provider's <c>OnNotifyPreCreateHardlink</c> callback when a hard link is about to be created for a file.
        /// </summary>
        PreCreateHardlink = PRJ_NOTIFY_PRE_SET_HARDLINK,

        /// <summary>
        /// Indicates that ProjFS should call the provider's <c>OnNotifyFileRenamed</c> callback when a file or directory has been renamed.
        /// </summary>
        FileRenamed = PRJ_NOTIFY_FILE_RENAMED,

        /// <summary>
        /// Indicates that ProjFS should call the provider's <c>OnNotifyHardlinkCreated</c> callback when a hard link has been created for a file.
        /// </summary>
        HardlinkCreated = PRJ_NOTIFY_HARDLINK_CREATED,

        /// <summary>
        /// Indicates that ProjFS should call the provider's <c>OnNotifyFileHandleClosedNoModification</c> callback when a handle is closed on a file or directory
        /// and the closing handle neither modified nor deleted it.
        /// </summary>
        FileHandleClosedNoModification = PRJ_NOTIFY_FILE_HANDLE_CLOSED_NO_MODIFICATION,

        /// <summary>
        /// Indicates that ProjFS should call the provider's <c>OnNotifyFileHandleClosedFileModifiedOrDeleted</c> callback when a handle is closed on a file or
        /// directory and the closing handle was used to modify it.
        /// </summary>
        FileHandleClosedFileModified = PRJ_NOTIFY_FILE_HANDLE_CLOSED_FILE_MODIFIED,

        /// <summary>
        /// Indicates that ProjFS should call the provider's <c>OnNotifyFileHandleClosedFileModifiedOrDeleted</c> callback when a handle is closed on a file or
        /// directory and it is deleted as part of closing the handle.
        /// </summary>
        FileHandleClosedFileDeleted = PRJ_NOTIFY_FILE_HANDLE_CLOSED_FILE_DELETED,

        /// <summary>
        /// Indicates that ProjFS should call the provider's <c>OnNotifyFilePreConvertToFull</c> callback when it is about to convert a placeholder to a full file.
        /// </summary>
        FilePreConvertToFull = PRJ_NOTIFY_FILE_PRE_CONVERT_TO_FULL,

        /// <summary>
        /// This value is not used when calling the <c>VirtualizationInstance</c> constructor.  It
        /// is only returned from <c>OnNotify...</c> callbacks that have a <paramref name="notificationMask"/>
        /// parameter, and indicates that the provider wants to continue to receive the notifications
        /// it registered for when starting the virtualization instance.
        /// </summary>
        UseExistingMask = static_cast<std::underlying_type_t<NotificationType>>(PRJ_NOTIFY_USE_EXISTING_MASK)
    };
}}} // namespace Microsoft.Windows.ProjFS
