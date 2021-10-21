// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

using Microsoft.Win32.SafeHandles;
using System;
using System.ComponentModel;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace SimpleProviderManaged
{
    public sealed class FileSystemApi
    {
        [Flags]
        public enum SymbolicLinkTarget : uint
        {
            /// <summary>
            /// The link target is a file.
            /// </summary>
            File = 0x0,

            /// <summary>
            /// The link target is a directory.
            /// </summary>
            Directory = 0x1,

            /// <summary>
            /// Specify this flag to allow creation of symbolic links when the process is not elevated.
            /// </summary>
            AllowUnprivilegedCreate = 0x2
        }

        public enum DwReserved0Flag : uint
        {
            IO_REPARSE_TAG_RESERVED_ZERO = 0x00000000, // Reserved reparse tag value.
            IO_REPARSE_TAG_RESERVED_ONE = 0x00000001, // Reserved reparse tag value.
            IO_REPARSE_TAG_MOUNT_POINT = 0xA0000003, // Used for mount point support, specified in section 2.1.2.5.
            IO_REPARSE_TAG_HSM = 0xC0000004, // Obsolete.Used by legacy Hierarchical Storage Manager Product.
            IO_REPARSE_TAG_HSM2 = 0x80000006, // Obsolete.Used by legacy Hierarchical Storage Manager Product.
            IO_REPARSE_TAG_DRIVER_EXTENDER = 0x80000005, // Home server drive extender.<3>
            IO_REPARSE_TAG_SIS = 0x80000007, // Used by single-instance storage (SIS) filter driver.Server-side interpretation only, not meaningful over the wire.
            IO_REPARSE_TAG_DFS = 0x8000000A, // Used by the DFS filter.The DFS is described in the Distributed File System (DFS): Referral Protocol Specification[MS - DFSC]. Server-side interpretation only, not meaningful over the wire.
            IO_REPARSE_TAG_DFSR = 0x80000012, // Used by the DFS filter.The DFS is described in [MS-DFSC]. Server-side interpretation only, not meaningful over the wire.
            IO_REPARSE_TAG_FILTER_MANAGER = 0x8000000B, // Used by filter manager test harness.<4>
            IO_REPARSE_TAG_SYMLINK = 0xA000000C, // Used for symbolic link support. See section 2.1.2.4.
            IO_REPARSE_TAG_WCIFS = 0x80000018, // The tag for a WCI reparse point
            IO_REPARSE_TAG_WCIFS_TOMBSTONE = 0xA000001F, // The tag for a WCI tombstone file
        }

        [Flags]
        public enum FileDesiredAccess : uint
        {
            /// <summary>
            /// No access requested.
            /// </summary>
            None = 0,

            /// <summary>
            /// Waitable handle (always required by CreateFile?)
            /// </summary>
            Synchronize = 0x00100000,

            /// <summary>
            /// Object can be deleted.
            /// </summary>
            Delete = 0x00010000,

            /// <summary>
            /// See http://msdn.microsoft.com/en-us/library/windows/desktop/aa364399(v=vs.85).aspx
            /// </summary>
            GenericRead = 0x80000000,

            /// <summary>
            /// See http://msdn.microsoft.com/en-us/library/windows/desktop/aa364399(v=vs.85).aspx
            /// </summary>
            GenericWrite = 0x40000000,

            /// <summary>
            /// Can read file or directory attributes.
            /// </summary>
            FileReadAttributes = 0x0080,

            /// <summary>
            /// The right to write file attributes.
            /// </summary>
            FileWriteAttributes = 0x00100,
        }

        [Flags]
        public enum FileFlagsAndAttributes : uint
        {
            /// <summary>
            /// No flags.
            /// </summary>
            None = 0,

            /// <summary>
            /// The handle that identifies a directory.
            /// </summary>
            FileAttributeDirectory = 0x20,

            /// <summary>
            /// The file should be archived. Applications use this attribute to mark files for backup or removal.
            /// </summary>
            FileAttributeArchive = 0x20,

            /// <summary>
            /// The file or directory is encrypted. For a file, this means that all data in the file is encrypted. For a directory,
            /// this means that encryption is the default for newly created files and subdirectories. For more information, see File
            /// Encryption.
            /// This flag has no effect if FILE_ATTRIBUTE_SYSTEM is also specified.
            /// </summary>
            FileAttributeEncrypted = 0x4000,

            /// <summary>
            /// The file is hidden. Do not include it in an ordinary directory listing.
            /// </summary>
            FileAttributeHidden = 0x2,

            /// <summary>
            /// The file does not have other attributes set. This attribute is valid only if used alone.
            /// </summary>
            FileAttributeNormal = 0x80,

            /// <summary>
            /// The data of a file is not immediately available. This attribute indicates that file data is physically moved to offline
            /// storage. This attribute is used by Remote Storage, the hierarchical storage management software. Applications should
            /// not arbitrarily change this attribute.
            /// </summary>
            FileAttributeOffline = 0x1000,

            /// <summary>
            /// The file is read only. Applications can read the file, but cannot write to or delete it.
            /// </summary>
            FileAttributeReadOnly = 0x1,

            /// <summary>
            /// The file is part of or used exclusively by an operating system.
            /// </summary>
            FileAttributeSystem = 0x4,

            /// <summary>
            /// The file is being used for temporary storage.
            /// </summary>
            FileAttributeTemporary = 0x100,

            /// <summary>
            /// The file is being opened or created for a backup or restore operation. The system ensures that the calling process
            /// overrides file security checks when the process has SE_BACKUP_NAME and SE_RESTORE_NAME privileges. For more
            /// information, see Changing Privileges in a Token.
            /// You must set this flag to obtain a handle to a directory. A directory handle can be passed to some functions instead of
            /// a file handle.
            /// </summary>
            FileFlagBackupSemantics = 0x02000000,

            /// <summary>
            /// The file is to be deleted immediately after all of its handles are closed, which includes the specified handle and any
            /// other open or duplicated handles.
            /// If there are existing open handles to a file, the call fails unless they were all opened with the FILE_SHARE_DELETE
            /// share mode.
            /// Subsequent open requests for the file fail, unless the FILE_SHARE_DELETE share mode is specified.
            /// </summary>
            FileFlagDeleteOnClose = 0x04000000,

            /// <summary>
            /// The file or device is being opened with no system caching for data reads and writes. This flag does not affect hard
            /// disk caching or memory mapped files.
            /// </summary>
            FileFlagNoBuffering = 0x20000000,

            /// <summary>
            /// The file data is requested, but it should continue to be located in remote storage. It should not be transported back
            /// to local storage. This flag is for use by remote storage systems.
            /// </summary>
            FileFlagOpenNoRecall = 0x00100000,

            /// <summary>
            /// Normal reparse point processing will not occur; CreateFile will attempt to open the reparse point. When a file is
            /// opened, a file handle is returned, whether or not the filter that controls the reparse point is operational.
            /// This flag cannot be used with the CREATE_ALWAYS flag.
            /// If the file is not a reparse point, then this flag is ignored.
            /// </summary>
            FileFlagOpenReparsePoint = 0x00200000,

            /// <summary>
            /// The file or device is being opened or created for asynchronous I/O.
            /// When subsequent I/O operations are completed on this handle, the event specified in the OVERLAPPED structure will be
            /// set to the signaled state.
            /// If this flag is specified, the file can be used for simultaneous read and write operations.
            /// If this flag is not specified, then I/O operations are serialized, even if the calls to the read and write functions
            /// specify an OVERLAPPED structure.
            /// </summary>
            FileFlagOverlapped = 0x40000000,

            /// <summary>
            /// Access will occur according to POSIX rules. This includes allowing multiple files with names, differing only in case,
            /// for file systems that support that naming.
            /// Use care when using this option, because files created with this flag may not be accessible by applications that are
            /// written for MS-DOS or 16-bit Windows.
            /// </summary>
            FileFlagPosixSemantics = 0x01000000,

            /// <summary>
            /// Access is intended to be random. The system can use this as a hint to optimize file caching.
            /// This flag has no effect if the file system does not support cached I/O and FILE_FLAG_NO_BUFFERING.
            /// </summary>
            FileFlagRandomAccess = 0x10000000,

            /// <summary>
            /// The file or device is being opened with session awareness. If this flag is not specified, then per-session devices
            /// (such as a redirected USB device) cannot be opened by processes running in session 0.
            /// </summary>
            FileFlagSessionAware = 0x00800000,

            /// <summary>
            /// Access is intended to be sequential from beginning to end. The system can use this as a hint to optimize file caching.
            /// This flag should not be used if read-behind (that is, reverse scans) will be used.
            /// This flag has no effect if the file system does not support cached I/O and FILE_FLAG_NO_BUFFERING.
            /// For more information, see the Caching Behavior section of this topic.
            /// </summary>
            FileFlagSequentialScan = 0x08000000,

            /// <summary>
            /// Write operations will not go through any intermediate cache, they will go directly to disk.
            /// </summary>
            FileFlagWriteThrough = 0x80000000,

            /// <summary>
            /// When opening a named pipe, the pipe server can only impersonate this client at the 'anonymous' level (i.e., no privilege is made available).
            /// </summary>
            /// <remarks>
            /// This is actually <c>SECURITY_SQOS_PRESENT</c> which makes <c>CreateFile</c> respect SQQS flags; those flags are ignored unless this is specified.
            /// But <c>SECURITY_ANONYMOUS</c> is zero; so think of this as those two flags together (much easier to use correctly).
            ///
            /// Please also note that SECURITY_SQOS_PRESENT is the same value as FILE_FLAG_OPEN_NO_RECALL.
            /// See the comment here for example: https://github.com/rust-lang/rust/blob/master/library/std/src/sys/windows/ext/fs.rs
            /// </remarks>
            SecurityAnonymous = 0x00100000,
        }

        public static class NativeIOConstants
        {
            /// <summary>
            /// FSCTL_READ_FILE_USN_DATA
            /// </summary>
            public const uint FsctlReadFileUsnData = 0x900eb;

            /// <summary>
            /// FSCTL_WRITE_USN_CLOSE_RECORD
            /// </summary>
            public const uint FsctlWriteUsnCloseRecord = 0x900ef;

            /// <summary>
            /// FSCTL_QUERY_USN_JOURNAL
            /// </summary>
            public const uint FsctlQueryUsnJournal = 0x900f4;

            /// <summary>
            /// FSCTL_READ_USN_JOURNAL
            /// </summary>
            public const uint FsctlReadUsnJournal = 0x900bb;

            /// <summary>
            /// FSCTL_READ_UNPRIVILEGED_USN_JOURNAL
            /// </summary>
            public const uint FsctlReadUnprivilegedUsnJournal = 0x903ab;

            /// <summary>
            /// FVE_LOCKED_VOLUME
            /// </summary>
#pragma warning disable SA1139 // Use literal suffix notation instead of casting
            public const int FveLockedVolume = unchecked((int)0x80310000);
#pragma warning restore SA1139

            /// <summary>
            /// INVALID_FILE_ATTRIBUTES
            /// </summary>
            public const uint InvalidFileAttributes = 0xFFFFFFFF;

            /// <summary>
            /// ERROR_JOURNAL_NOT_ACTIVE
            /// </summary>
            public const uint ErrorJournalNotActive = 0x49B;

            /// <summary>
            ///  ERROR_JOURNAL_DELETE_IN_PROGRESS
            /// </summary>
            public const uint ErrorJournalDeleteInProgress = 0x49A;

            /// <summary>
            ///  ERROR_JOURNAL_ENTRY_DELETED
            /// </summary>
            public const uint ErrorJournalEntryDeleted = 0x49D;

            /// <summary>
            /// ERROR_NO_MORE_FILES
            /// </summary>
            public const uint ErrorNoMoreFiles = 0x12;

            /// <summary>
            /// ERROR_WRITE_PROTECT
            /// </summary>
            public const uint ErrorWriteProtect = 0x13;

            /// <summary>
            /// ERROR_INVALID_PARAMETER
            /// </summary>
            public const int ErrorInvalidParameter = 0x57;

            /// <summary>
            /// ERROR_INVALID_FUNCTION
            /// </summary>
            public const uint ErrorInvalidFunction = 0x1;

            /// <summary>
            /// ERROR_ONLY_IF_CONNECTED
            /// </summary>
            public const uint ErrorOnlyIfConnected = 0x4E3;

            /// <summary>
            /// ERROR_SUCCESS
            /// </summary>
            public const int ErrorSuccess = 0x0;

            /// <summary>
            /// ERROR_ACCESS_DENIED
            /// </summary>
            public const int ErrorAccessDenied = 0x5;

            /// <summary>
            /// ERROR_SHARING_VIOLATION
            /// </summary>
            public const int ErrorSharingViolation = 0x20;

            /// <summary>
            /// ERROR_TOO_MANY_LINKS
            /// </summary>
            public const int ErrorTooManyLinks = 0x476;

            /// <summary>
            /// ERROR_NOT_SAME_DEVICE
            /// </summary>
            public const int ErrorNotSameDevice = 0x11;

            /// <summary>
            /// ERROR_NOT_SUPPORTED
            /// </summary>
            public const int ErrorNotSupported = 0x32;

            /// <summary>
            /// ERROR_FILE_NOT_FOUND
            /// </summary>
            public const int ErrorFileNotFound = 0x2;

            /// <summary>
            /// ERROR_FILE_EXISTS
            /// </summary>
            public const int ErrorFileExists = 0x50;

            /// <summary>
            /// ERROR_FILE_ALREADY_EXISTS
            /// </summary>
            public const int ErrorAlreadyExists = 0xB7;

            /// <summary>
            /// ERROR_PATH_NOT_FOUND
            /// </summary>
            public const int ErrorPathNotFound = 0x3;

            /// <summary>
            /// ERROR_NOT_READY
            /// </summary>
            public const int ErrorNotReady = 0x15;

            /// <summary>
            /// ERROR_DIR_NOT_EMPTY
            /// </summary>
            public const int ErrorDirNotEmpty = 0x91;

            /// <summary>
            /// ERROR_DIRECTORY
            /// </summary>
            public const int ErrorDirectory = 0x10b;

            /// <summary>
            /// ERROR_PARTIAL_COPY
            /// </summary>
            public const int ErrorPartialCopy = 0x12b;

            /// <summary>
            /// ERROR_IO_PENDING
            /// </summary>
            public const int ErrorIOPending = 0x3E5;

            /// <summary>
            /// ERROR_IO_INCOMPLETE
            /// </summary>
            public const int ErrorIOIncomplete = 0x3E4;

            /// <summary>
            /// ERROR_ABANDONED_WAIT_0
            /// </summary>
            public const int ErrorAbandonedWait0 = 0x2DF;

            /// <summary>
            /// ERROR_HANDLE_EOF
            /// </summary>
            public const int ErrorHandleEof = 0x26;

            /// <summary>
            /// ERROR_TIMEOUT
            /// </summary>
            public const int ErrorTimeout = 0x5B4;

            /// <summary> 
            /// ERROR_PIPE_BUSY. 
            /// </summary> 
            public const int ErrorPipeBusy = 0xE7;

            /// <summary>
            /// Infinite timeout.
            /// </summary>
            public const int Infinite = -1;

            /// <summary>
            /// Maximum path length.
            /// </summary>
            public const int MaxPath = 260;

            /// <summary>
            /// Maximum path length for \\?\ style paths.
            /// </summary>
            public const int MaxLongPath = 32767;

            /// <summary>
            /// Maximum path length for directory.
            /// </summary>
            public const int MaxDirectoryPath = 248;

            /// <summary>
            /// ERROR_CANT_ACCESS_FILE
            /// </summary>
            public const int ErrorCantAccessFile = 0x780;

            /// <summary>
            /// ERROR_BAD_PATHNAME
            /// </summary>
            public const int ErrorBadPathname = 0xA1;

            /// <summary>
            /// ERROR_INVALID_NAME
            /// </summary>
            public const int ErrorInvalidName = 0x7B;
        }

        private const int INITIAL_REPARSE_DATA_BUFFER_SIZE = 1024;
        private const int FSCTL_GET_REPARSE_POINT = 0x000900a8;

        private const int ERROR_INSUFFICIENT_BUFFER = 0x7A;
        private const int ERROR_MORE_DATA = 0xEA;
        private const int ERROR_SUCCESS = 0x0;
        private const int SYMLINK_FLAG_RELATIVE = 0x1;

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool DeviceIoControl(
            SafeFileHandle deviceHandle,
            uint ioControlCode,
            IntPtr inputBuffer,
            int inputBufferSize,
            IntPtr outputBuffer,
            int outputBufferSize,
            out int bytesReturned,
            IntPtr overlapped);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode, ExactSpelling = true)]
        private static extern SafeFileHandle CreateFileW(
            string lpFileName,
            FileDesiredAccess dwDesiredAccess,
            FileShare dwShareMode,
            IntPtr lpSecurityAttributes,
            FileMode dwCreationDisposition,
            FileFlagsAndAttributes dwFlagsAndAttributes,
            IntPtr hTemplateFile);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode, ExactSpelling = true)]
        private static extern int CreateSymbolicLinkW(string lpSymlinkFileName, string lpTargetFileName, SymbolicLinkTarget dwFlags);

        [DllImport("shlwapi.dll", SetLastError = true, CharSet = CharSet.Unicode, ExactSpelling = true)]
        private static extern bool PathRelativePathToW(
            System.Text.StringBuilder pszPath,
            string pszFrom,
            FileFlagsAndAttributes dwAttrFrom,
            string pszTo,
            FileFlagsAndAttributes dwAttrTo);

        private static unsafe string GetReparsePointTarget(SafeFileHandle handle)
        {
            string targetPath = string.Empty;

            int bufferSize = INITIAL_REPARSE_DATA_BUFFER_SIZE;
            int errorCode = ERROR_INSUFFICIENT_BUFFER;

            byte[] buffer = null;
            while (errorCode == ERROR_MORE_DATA || errorCode == ERROR_INSUFFICIENT_BUFFER)
            {
                buffer = new byte[bufferSize];
                bool success = false;

                fixed (byte* pBuffer = buffer)
                {
                    int bufferReturnedSize;
                    success = DeviceIoControl(
                        handle,
                        FSCTL_GET_REPARSE_POINT,
                        IntPtr.Zero,
                        0,
                        (IntPtr)pBuffer,
                        bufferSize,
                        out bufferReturnedSize,
                        IntPtr.Zero);
                }

                bufferSize *= 2;
                errorCode = success ? 0 : Marshal.GetLastWin32Error();
            }

            if (errorCode != 0)
            {
                throw new Win32Exception(errorCode, "DeviceIoControl(FSCTL_GET_REPARSE_POINT)");
            }

            // Now get the offsets in the REPARSE_DATA_BUFFER buffer string based on
            // the offsets for the different type of reparse points.

            const uint PrintNameOffsetIndex = 12;
            const uint PrintNameLengthIndex = 14;
            const uint SubsNameOffsetIndex = 8;
            const uint SubsNameLengthIndex = 10;

            fixed (byte* pBuffer = buffer)
            {
                uint reparsePointTag = *(uint*)(pBuffer);

                if (reparsePointTag != (uint)DwReserved0Flag.IO_REPARSE_TAG_SYMLINK
                    && reparsePointTag != (uint)DwReserved0Flag.IO_REPARSE_TAG_MOUNT_POINT)
                {
                    throw new NotSupportedException($"Reparse point tag {reparsePointTag:X} not supported");
                }

                uint pathBufferOffsetIndex = (uint)((reparsePointTag == (uint)DwReserved0Flag.IO_REPARSE_TAG_SYMLINK) ? 20 : 16);
                char* nameStartPtr = (char*)(pBuffer + pathBufferOffsetIndex);
                int nameOffset = *(short*)(pBuffer + PrintNameOffsetIndex) / 2;
                int nameLength = *(short*)(pBuffer + PrintNameLengthIndex) / 2;
                targetPath = new string(nameStartPtr, nameOffset, nameLength);

                if (string.IsNullOrWhiteSpace(targetPath))
                {
                    nameOffset = *(short*)(pBuffer + SubsNameOffsetIndex) / 2;
                    nameLength = *(short*)(pBuffer + SubsNameLengthIndex) / 2;
                    targetPath = new string(nameStartPtr, nameOffset, nameLength);
                }
            }

            return targetPath;
        }

        public static bool TryCreateSymbolicLink(string symLinkFileName, string targetFileName, bool isTargetFile)
        {
            SymbolicLinkTarget creationFlag = isTargetFile ? SymbolicLinkTarget.File : SymbolicLinkTarget.Directory;
            creationFlag |= SymbolicLinkTarget.AllowUnprivilegedCreate;

            int res = CreateSymbolicLinkW(symLinkFileName, targetFileName, creationFlag);

            // The return value of CreateSymbolicLinkW is underspecified in its documentation.
            // In non-admin mode where Developer mode is not enabled, the return value can be greater than zero, but the last error
            // is ERROR_PRIVILEGE_NOT_HELD, and consequently the symlink is not created. We strenghten the return value by
            // also checking that the last error is ERROR_SUCCESS.
            int lastError = Marshal.GetLastWin32Error();
            return res > 0 && lastError == NativeIOConstants.ErrorSuccess;
        }

        public static bool TryGetReparsePointTarget(string source, out string target)
        {
            target = null;
            using (SafeFileHandle handle = CreateFileW(
                source,
                FileDesiredAccess.GenericRead,
                FileShare.Read | FileShare.Delete,
                IntPtr.Zero,
                FileMode.Open,
                FileFlagsAndAttributes.FileFlagOpenReparsePoint | FileFlagsAndAttributes.FileFlagBackupSemantics,
                IntPtr.Zero))
            {
                if (handle.IsInvalid)
                {
                    return false;
                }

                target = GetReparsePointTarget(handle);
                return true;
            }
        }

        public static string TryGetPathRelativeToRoot(string root, string path, bool isPathToADirectory)
        {
            const Int32 MaxPath = 260;
            StringBuilder relativePathBuilder = new StringBuilder(MaxPath);

            bool result = PathRelativePathToW(relativePathBuilder,
                root,
                FileFlagsAndAttributes.FileAttributeDirectory,
                path,
                isPathToADirectory ? FileFlagsAndAttributes.FileAttributeDirectory : FileFlagsAndAttributes.FileAttributeNormal);

            if (!result)
            {
                throw new Exception($"Failed to get relative path of {path} with root {root}.");
            }

            return relativePathBuilder.ToString();
        }
    }
}
