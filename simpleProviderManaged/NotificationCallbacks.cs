// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

using Microsoft.Windows.ProjFS;
using Serilog;
using System;
using System.Collections.Generic;

namespace SimpleProviderManaged
{
    /// <summary>
    /// This class implements the file system operation notification callbacks for <see cref="SimpleProvider"/>.
    /// The callbacks here are basic examples.  Each one simply logs a message when triggered.  If 
    /// the provider is running in test mode, the callback also signals the test that it received
    /// the callback.
    /// </summary>
    class NotificationCallbacks
    {
        private readonly SimpleProvider provider;

        /// <summary>
        /// Instantiates the class.
        /// </summary>
        /// <param name="provider">The <see cref="SimpleProvider"/> instance that will receive the callbacks.</param>
        /// <param name="virtInstance">The <see cref="VirtualizationInstance"/> for <paramref name="provider"/>.</param>
        /// <param name="notificationMappings">A collection of <see cref="NotificationMapping"/> objects
        /// describing which notifications <paramref name="provider"/> will receive and on what paths.</param>
        public NotificationCallbacks(
            SimpleProvider provider,
            VirtualizationInstance virtInstance,
            IReadOnlyCollection<NotificationMapping> notificationMappings)
        {
            this.provider = provider;

            // Look through notificationMappings for all the set notification bits.  Supply a callback
            // for each set bit.
            NotificationType notification = NotificationType.None;
            foreach (NotificationMapping mapping in notificationMappings)
            {
                notification |= mapping.NotificationMask;
            }

            if ((notification & NotificationType.FileOpened) == NotificationType.FileOpened)
            {
                virtInstance.OnNotifyFileOpened = NotifyFileOpenedCallback;
            }

            if ((notification & NotificationType.NewFileCreated) == NotificationType.NewFileCreated)
            {
                virtInstance.OnNotifyNewFileCreated = NotifyNewFileCreatedCallback;
            }

            if ((notification & NotificationType.FileOverwritten) == NotificationType.FileOverwritten)
            {
                virtInstance.OnNotifyFileOverwritten = NotifyFileOverwrittenCallback;
            }

            if ((notification & NotificationType.PreDelete) == NotificationType.PreDelete)
            {
                virtInstance.OnNotifyPreDelete = NotifyPreDeleteCallback;
            }

            if ((notification & NotificationType.PreRename) == NotificationType.PreRename)
            {
                virtInstance.OnNotifyPreRename = NotifyPreRenameCallback;
            }

            if ((notification & NotificationType.PreCreateHardlink) == NotificationType.PreCreateHardlink)
            {
                virtInstance.OnNotifyPreCreateHardlink = NotifyPreCreateHardlinkCallback;
            }

            if ((notification & NotificationType.FileRenamed) == NotificationType.FileRenamed)
            {
                virtInstance.OnNotifyFileRenamed = NotifyFileRenamedCallback;
            }

            if ((notification & NotificationType.HardlinkCreated) == NotificationType.HardlinkCreated)
            {
                virtInstance.OnNotifyHardlinkCreated = NotifyHardlinkCreatedCallback;
            }

            if ((notification & NotificationType.FileHandleClosedNoModification) == NotificationType.FileHandleClosedNoModification)
            {
                virtInstance.OnNotifyFileHandleClosedNoModification = NotifyFileHandleClosedNoModificationCallback;
            }

            if (((notification & NotificationType.FileHandleClosedFileModified) == NotificationType.FileHandleClosedFileModified) ||
                ((notification & NotificationType.FileHandleClosedFileDeleted) == NotificationType.FileHandleClosedFileDeleted))
            {
                virtInstance.OnNotifyFileHandleClosedFileModifiedOrDeleted = NotifyFileHandleClosedFileModifiedOrDeletedCallback;
            }

            if ((notification & NotificationType.FilePreConvertToFull) == NotificationType.FilePreConvertToFull)
            {
                virtInstance.OnNotifyFilePreConvertToFull = NotifyFilePreConvertToFullCallback;
            }
        }

        public bool NotifyFileOpenedCallback(
            string relativePath,
            bool isDirectory,
            uint triggeringProcessId,
            string triggeringProcessImageFileName,
            out NotificationType notificationMask)
        {
            Log.Information("NotifyFileOpenedCallback [{relativePath}]", relativePath);
            Log.Information("  Notification triggered by [{triggeringProcessImageFileName} {triggeringProcessId}]",
                triggeringProcessImageFileName, triggeringProcessId);

            notificationMask = NotificationType.UseExistingMask;
            provider.SignalIfTestMode("FileOpened");
            return true;
        }


        public void NotifyNewFileCreatedCallback(
            string relativePath,
            bool isDirectory,
            uint triggeringProcessId,
            string triggeringProcessImageFileName,
            out NotificationType notificationMask)
        {
            Log.Information("NotifyNewFileCreatedCallback [{relativePath}]", relativePath);
            Log.Information("  Notification triggered by [{triggeringProcessImageFileName} {triggeringProcessId}]",
                triggeringProcessImageFileName, triggeringProcessId);

            notificationMask = NotificationType.UseExistingMask;
            provider.SignalIfTestMode("NewFileCreated");
        }

        public void NotifyFileOverwrittenCallback(
            string relativePath,
            bool isDirectory,
            uint triggeringProcessId,
            string triggeringProcessImageFileName,
            out NotificationType notificationMask)
        {
            Log.Information("NotifyFileOverwrittenCallback [{relativePath}]", relativePath);
            Log.Information("  Notification triggered by [{triggeringProcessImageFileName} {triggeringProcessId}]",
                triggeringProcessImageFileName, triggeringProcessId);

            notificationMask = NotificationType.UseExistingMask;
            provider.SignalIfTestMode("FileOverwritten");
        }

        public bool NotifyPreDeleteCallback(
            string relativePath,
            bool isDirectory,
            uint triggeringProcessId,
            string triggeringProcessImageFileName)
        {
            Log.Information("NotifyPreDeleteCallback [{relativePath}]", relativePath);
            Log.Information("  Notification triggered by [{triggeringProcessImageFileName} {triggeringProcessId}]",
                triggeringProcessImageFileName, triggeringProcessId);

            provider.SignalIfTestMode("PreDelete");
            return provider.Options.DenyDeletes ? false : true;
        }

        public bool NotifyPreRenameCallback(
            string relativePath,
            string destinationPath,
            uint triggeringProcessId,
            string triggeringProcessImageFileName)
        {
            Log.Information("NotifyPreRenameCallback [{relativePath}] [{destinationPath}]", relativePath, destinationPath);
            Log.Information("  Notification triggered by [{triggeringProcessImageFileName} {triggeringProcessId}]",
                triggeringProcessImageFileName, triggeringProcessId);

            provider.SignalIfTestMode("PreRename");
            return true;
        }

        public bool NotifyPreCreateHardlinkCallback(
            string relativePath,
            string destinationPath,
            uint triggeringProcessId,
            string triggeringProcessImageFileName)
        {
            Log.Information("NotifyPreCreateHardlinkCallback [{relativePath}] [{destinationPath}]", relativePath, destinationPath);
            Log.Information("  Notification triggered by [{triggeringProcessImageFileName} {triggeringProcessId}]",
                triggeringProcessImageFileName, triggeringProcessId);

            provider.SignalIfTestMode("PreCreateHardlink");
            return true;
        }

        public void NotifyFileRenamedCallback(
            string relativePath,
            string destinationPath,
            bool isDirectory,
            uint triggeringProcessId,
            string triggeringProcessImageFileName,
            out NotificationType notificationMask)
        {
            Log.Information("NotifyFileRenamedCallback [{relativePath}] [{destinationPath}]", relativePath, destinationPath);
            Log.Information("  Notification triggered by [{triggeringProcessImageFileName} {triggeringProcessId}]",
                triggeringProcessImageFileName, triggeringProcessId);

            notificationMask = NotificationType.UseExistingMask;
            provider.SignalIfTestMode("FileRenamed");
        }

        public void NotifyHardlinkCreatedCallback(
            string relativePath,
            string destinationPath,
            uint triggeringProcessId,
            string triggeringProcessImageFileName)
        {
            Log.Information("NotifyHardlinkCreatedCallback [{relativePath}] [{destinationPath}]", relativePath, destinationPath);
            Log.Information("  Notification triggered by [{triggeringProcessImageFileName} {triggeringProcessId}]",
                triggeringProcessImageFileName, triggeringProcessId);

            provider.SignalIfTestMode("HardlinkCreated");
        }

        public void NotifyFileHandleClosedNoModificationCallback(
            string relativePath,
            bool isDirectory,
            uint triggeringProcessId,
            string triggeringProcessImageFileName)
        {
            Log.Information("NotifyFileHandleClosedNoModificationCallback [{relativePath}]", relativePath);
            Log.Information("  Notification triggered by [{triggeringProcessImageFileName} {triggeringProcessId}]",
                triggeringProcessImageFileName, triggeringProcessId);

            provider.SignalIfTestMode("FileHandleClosedNoModification");
        }

        public void NotifyFileHandleClosedFileModifiedOrDeletedCallback(
            string relativePath,
            bool isDirectory,
            bool isFileModified,
            bool isFileDeleted,
            uint triggeringProcessId,
            string triggeringProcessImageFileName)
        {
            Log.Information("NotifyFileHandleClosedFileModifiedOrDeletedCallback [{relativePath}]", relativePath);
            Log.Information("  Modified: {isFileModified}, Deleted: {isFileDeleted} ", isFileModified, isFileDeleted);
            Log.Information("  Notification triggered by [{triggeringProcessImageFileName} {triggeringProcessId}]",
                triggeringProcessImageFileName, triggeringProcessId);

            provider.SignalIfTestMode("FileHandleClosedFileModifiedOrDeleted");
        }

        public bool NotifyFilePreConvertToFullCallback(
            string relativePath,
            uint triggeringProcessId,
            string triggeringProcessImageFileName)
        {
            Log.Information("NotifyFilePreConvertToFullCallback [{relativePath}]", relativePath);
            Log.Information("  Notification triggered by [{triggeringProcessImageFileName} {triggeringProcessId}]",
                triggeringProcessImageFileName, triggeringProcessId);

            provider.SignalIfTestMode("FilePreConvertToFull");
            return true;
        }

    }
}
