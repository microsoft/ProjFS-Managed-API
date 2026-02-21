#nullable disable
using System;
using System.Collections.Generic;
using System.IO;

namespace Microsoft.Windows.ProjFS
{
    /// <summary>
    /// HRESULT values used by the ProjFS managed API.
    /// Values match the original Microsoft.Windows.ProjFS library exactly.
    /// </summary>
    public enum HResult : int
    {
        Ok = 0,
        Pending = -2147023899,
        InternalError = -2147023537,
        Handle = -2147024890,
        FileNotFound = -2147024894,
        PathNotFound = -2147024893,
        DirNotEmpty = -2147024751,
        VirtualizationInvalidOp = -2147024511,
        VirtualizationUnavaliable = -2147024527,
        AccessDenied = -2147024891,
        AlreadyInitialized = -2147023649,
        CannotDelete = -805306079,
        Directory = -2147024629,
        InsufficientBuffer = -2147024774,
        InvalidArg = -2147024809,
        OutOfMemory = -2147024882,
        ReparsePointEncountered = -2147020501,
    }

    [Flags]
    public enum NotificationType : uint
    {
        None = 0x00000001,
        FileOpened = 0x00000002,
        NewFileCreated = 0x00000004,
        FileOverwritten = 0x00000008,
        PreDelete = 0x00000010,
        PreRename = 0x00000020,
        PreCreateHardlink = 0x00000040,
        FileRenamed = 0x00000080,
        HardlinkCreated = 0x00000100,
        FileHandleClosedNoModification = 0x00000200,
        FileHandleClosedFileModified = 0x00000400,
        FileHandleClosedFileDeleted = 0x00000800,
        FilePreConvertToFull = 0x00001000,
        UseExistingMask = 0xFFFFFFFF,
    }

    [Flags]
    public enum UpdateType : uint
    {
        AllowDirtyMetadata = 1,
        AllowDirtyData = 2,
        AllowTombstone = 4,
        AllowReadOnly = 32,
    }

    [Flags]
    public enum UpdateFailureCause : uint
    {
        NoFailure = 0,
        DirtyMetadata = 1,
        DirtyData = 2,
        Tombstone = 4,
        ReadOnly = 8,
    }

    [Flags]
    public enum OnDiskFileState : uint
    {
        Placeholder = 1,
        HydratedPlaceholder = 2,
        DirtyPlaceholder = 4,
        Full = 8,
        Tombstone = 16,
    }

    public class NotificationMapping
    {
        /// <summary>
        /// Initializes a new instance with <see cref="NotificationMask"/> set to
        /// <see cref="NotificationType.None"/> and <see cref="NotificationRoot"/> set to null.
        /// </summary>
        public NotificationMapping()
        {
            NotificationMask = NotificationType.None;
        }

        /// <summary>
        /// Initializes a new instance with the specified notification mask and root path.
        /// </summary>
        /// <param name="notificationMask">The set of notifications for this root.</param>
        /// <param name="notificationRoot">Path relative to the virtualization root. Use empty string for the root itself.</param>
        /// <exception cref="ArgumentException">
        /// <paramref name="notificationRoot"/> is "." or begins with ".\".
        /// </exception>
        public NotificationMapping(NotificationType notificationMask, string notificationRoot)
        {
            NotificationMask = notificationMask;
            ValidateNotificationRoot(notificationRoot);
            NotificationRoot = notificationRoot;
        }

        /// <summary>A bit vector of <see cref="NotificationType"/> values.</summary>
        public NotificationType NotificationMask { get; set; }

        /// <summary>
        /// A path to a directory, relative to the virtualization root.
        /// The virtualization root itself must be specified as an empty string.
        /// </summary>
        /// <exception cref="ArgumentException">
        /// The value is "." or begins with ".\".
        /// </exception>
        public string NotificationRoot
        {
            get => _notificationRoot;
            set
            {
                ValidateNotificationRoot(value);
                _notificationRoot = value;
            }
        }

        private string _notificationRoot;

        private static void ValidateNotificationRoot(string root)
        {
            if (root == "." || (root != null && root.StartsWith(".\\")))
            {
                throw new ArgumentException(
                    "notificationRoot cannot be \".\" or begin with \".\\\"");
            }
        }
    }

    // Callback delegates matching the original Microsoft.Windows.ProjFS signatures
    public delegate void CancelCommandCallback(int commandId);

    public delegate bool NotifyFileOpenedCallback(
        string relativePath,
        bool isDirectory,
        uint triggeringProcessId,
        string triggeringProcessImageFileName,
        out NotificationType notificationMask);

    public delegate void NotifyNewFileCreatedCallback(
        string relativePath,
        bool isDirectory,
        uint triggeringProcessId,
        string triggeringProcessImageFileName,
        out NotificationType notificationMask);

    public delegate void NotifyFileOverwrittenCallback(
        string relativePath,
        bool isDirectory,
        uint triggeringProcessId,
        string triggeringProcessImageFileName,
        out NotificationType notificationMask);

    public delegate void NotifyFileHandleClosedNoModificationCallback(
        string relativePath,
        bool isDirectory,
        uint triggeringProcessId,
        string triggeringProcessImageFileName);

    public delegate void NotifyFileHandleClosedFileModifiedOrDeletedCallback(
        string relativePath,
        bool isDirectory,
        bool isFileModified,
        bool isFileDeleted,
        uint triggeringProcessId,
        string triggeringProcessImageFileName);

    public delegate bool NotifyFilePreConvertToFullCallback(
        string relativePath,
        uint triggeringProcessId,
        string triggeringProcessImageFileName);

    public delegate void NotifyFileRenamedCallback(
        string relativePath,
        string destinationPath,
        bool isDirectory,
        uint triggeringProcessId,
        string triggeringProcessImageFileName,
        out NotificationType notificationMask);

    public delegate void NotifyHardlinkCreatedCallback(
        string relativePath,
        string destinationPath,
        uint triggeringProcessId,
        string triggeringProcessImageFileName);

    public delegate bool NotifyPreDeleteCallback(
        string relativePath,
        bool isDirectory,
        uint triggeringProcessId,
        string triggeringProcessImageFileName);

    public delegate bool NotifyPreRenameCallback(
        string relativePath,
        string destinationPath,
        uint triggeringProcessId,
        string triggeringProcessImageFileName);

    public delegate bool NotifyPreCreateHardlinkCallback(
        string relativePath,
        string destinationPath,
        uint triggeringProcessId,
        string triggeringProcessImageFileName);

    public delegate HResult QueryFileNameCallback(string relativePath);

    // Interfaces
    public interface IWriteBuffer : IDisposable
    {
        IntPtr Pointer { get; }
        UnmanagedMemoryStream Stream { get; }
        long Length { get; }
    }

    public interface IDirectoryEnumerationResults
    {
        bool Add(
            string fileName,
            long fileSize,
            bool isDirectory,
            FileAttributes fileAttributes,
            DateTime creationTime,
            DateTime lastAccessTime,
            DateTime lastWriteTime,
            DateTime changeTime);

        bool Add(string fileName, long fileSize, bool isDirectory);

        /// <summary>Adds one entry to a directory enumeration result, with optional symlink target.</summary>
        /// <param name="symlinkTargetOrNull">The symlink target path, or null if this is not a symlink.</param>
        bool Add(
            string fileName,
            long fileSize,
            bool isDirectory,
            FileAttributes fileAttributes,
            DateTime creationTime,
            DateTime lastAccessTime,
            DateTime lastWriteTime,
            DateTime changeTime,
            string symlinkTargetOrNull);
    }

    public interface IRequiredCallbacks
    {
        HResult StartDirectoryEnumerationCallback(
            int commandId,
            Guid enumerationId,
            string relativePath,
            uint triggeringProcessId,
            string triggeringProcessImageFileName);

        HResult EndDirectoryEnumerationCallback(Guid enumerationId);

        HResult GetDirectoryEnumerationCallback(
            int commandId,
            Guid enumerationId,
            string filterFileName,
            bool restartScan,
            IDirectoryEnumerationResults result);

        HResult GetPlaceholderInfoCallback(
            int commandId,
            string relativePath,
            uint triggeringProcessId,
            string triggeringProcessImageFileName);

        HResult GetFileDataCallback(
            int commandId,
            string relativePath,
            ulong byteOffset,
            uint length,
            Guid dataStreamId,
            byte[] contentId,
            byte[] providerId,
            uint triggeringProcessId,
            string triggeringProcessImageFileName);
    }

    public interface IVirtualizationInstance
    {
        /// <summary>Returns the virtualization instance GUID.</summary>
        Guid VirtualizationInstanceId { get; }

        /// <summary>Returns the maximum allowed length of a placeholder's contentID or provider ID.</summary>
        int PlaceholderIdLength { get; }

        /// <summary>Retrieves the <see cref="IRequiredCallbacks"/> interface.</summary>
        IRequiredCallbacks RequiredCallbacks { get; }

        CancelCommandCallback OnCancelCommand { get; set; }
        NotifyFileOpenedCallback OnNotifyFileOpened { get; set; }
        NotifyNewFileCreatedCallback OnNotifyNewFileCreated { get; set; }
        NotifyFileOverwrittenCallback OnNotifyFileOverwritten { get; set; }
        NotifyFileHandleClosedNoModificationCallback OnNotifyFileHandleClosedNoModification { get; set; }
        NotifyFileHandleClosedFileModifiedOrDeletedCallback OnNotifyFileHandleClosedFileModifiedOrDeleted { get; set; }
        NotifyFilePreConvertToFullCallback OnNotifyFilePreConvertToFull { get; set; }
        NotifyFileRenamedCallback OnNotifyFileRenamed { get; set; }
        NotifyHardlinkCreatedCallback OnNotifyHardlinkCreated { get; set; }
        NotifyPreDeleteCallback OnNotifyPreDelete { get; set; }
        NotifyPreRenameCallback OnNotifyPreRename { get; set; }
        NotifyPreCreateHardlinkCallback OnNotifyPreCreateHardlink { get; set; }
        QueryFileNameCallback OnQueryFileName { get; set; }

        HResult StartVirtualizing(IRequiredCallbacks requiredCallbacks);
        void StopVirtualizing();
        HResult ClearNegativePathCache(out uint totalEntryNumber);
        HResult DeleteFile(string relativePath, UpdateType updateFlags, out UpdateFailureCause failureReason);

        HResult UpdateFileIfNeeded(
            string relativePath,
            DateTime creationTime,
            DateTime lastAccessTime,
            DateTime lastWriteTime,
            DateTime changeTime,
            FileAttributes fileAttributes,
            long endOfFile,
            byte[] contentId,
            byte[] providerId,
            UpdateType updateFlags,
            out UpdateFailureCause failureReason);

        HResult WritePlaceholderInfo(
            string relativePath,
            DateTime creationTime,
            DateTime lastAccessTime,
            DateTime lastWriteTime,
            DateTime changeTime,
            FileAttributes fileAttributes,
            long endOfFile,
            bool isDirectory,
            byte[] contentId,
            byte[] providerId);

        HResult CompleteCommand(int commandId, NotificationType newNotificationMask);
        HResult CompleteCommand(int commandId, IDirectoryEnumerationResults results);
        HResult CompleteCommand(int commandId, HResult completionResult);
        HResult CompleteCommand(int commandId);

        IWriteBuffer CreateWriteBuffer(ulong byteOffset, uint length, out ulong alignedByteOffset, out uint alignedLength);
        IWriteBuffer CreateWriteBuffer(uint desiredBufferSize);

        HResult WriteFileData(Guid dataStreamId, IWriteBuffer buffer, ulong byteOffset, uint length);
        HResult MarkDirectoryAsPlaceholder(string targetDirectoryPath, byte[] contentId, byte[] providerId);
    }

    /// <summary>
    /// Static utility methods wrapping native ProjFS functions.
    /// </summary>
    public abstract class Utils
    {
        public static bool DoesNameContainWildCards(string fileName)
        {
            return ProjFSNative.PrjDoesNameContainWildCards(fileName);
        }

        public static bool IsFileNameMatch(string fileNameToCheck, string pattern)
        {
            return ProjFSNative.PrjFileNameMatch(fileNameToCheck, pattern);
        }

        public static int FileNameCompare(string fileName1, string fileName2)
        {
            return ProjFSNative.PrjFileNameCompare(fileName1, fileName2);
        }

        public static bool TryGetOnDiskFileState(string fullPath, out OnDiskFileState fileState)
        {
            int hr = ProjFSNative.PrjGetOnDiskFileState(fullPath, out uint state);
            fileState = (OnDiskFileState)state;
            return hr >= 0;
        }
    }
}
