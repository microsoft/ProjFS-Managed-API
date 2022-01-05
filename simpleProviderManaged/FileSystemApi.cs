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
        public enum SymbolicLinkOptions : uint
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
            IO_REPARSE_TAG_MOUNT_POINT = 0xA0000003, // Used for mount point support, specified in section 2.1.2.5.
            IO_REPARSE_TAG_SYMLINK = 0xA000000C, // Used for symbolic link support. See section 2.1.2.4.
        }

        [Flags]
        public enum FileDesiredAccess : uint
        {
            /// <summary>
            /// See http://msdn.microsoft.com/en-us/library/windows/desktop/aa364399(v=vs.85).aspx
            /// </summary>
            GenericRead = 0x80000000
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
            /// The file does not have other attributes set. This attribute is valid only if used alone.
            /// </summary>
            FileAttributeNormal = 0x80,

            /// <summary>
            /// The file is being opened or created for a backup or restore operation. The system ensures that the calling process
            /// overrides file security checks when the process has SE_BACKUP_NAME and SE_RESTORE_NAME privileges. For more
            /// information, see Changing Privileges in a Token.
            /// You must set this flag to obtain a handle to a directory. A directory handle can be passed to some functions instead of
            /// a file handle.
            /// </summary>
            FileFlagBackupSemantics = 0x02000000,

            /// <summary>
            /// Normal reparse point processing will not occur; CreateFile will attempt to open the reparse point. When a file is
            /// opened, a file handle is returned, whether or not the filter that controls the reparse point is operational.
            /// This flag cannot be used with the CREATE_ALWAYS flag.
            /// If the file is not a reparse point, then this flag is ignored.
            /// </summary>
            FileFlagOpenReparsePoint = 0x00200000,
        }

        public static class NativeIOConstants
        {
            /// <summary>
            /// ERROR_SUCCESS
            /// </summary>
            public const int ErrorSuccess = 0x0;
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
        private static extern int CreateSymbolicLinkW(string lpSymlinkFileName, string lpTargetFileName, SymbolicLinkOptions dwFlags);

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
            SymbolicLinkOptions creationFlag = isTargetFile ? SymbolicLinkOptions.File : SymbolicLinkOptions.Directory;
            creationFlag |= SymbolicLinkOptions.AllowUnprivilegedCreate;

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

            string pathFrom;
            if (!root.EndsWith(Path.DirectorySeparatorChar.ToString()))
            {
                // PathRelativePathToW expects the root to have a trailing slash.
                pathFrom = root + Path.DirectorySeparatorChar;
            }
            else
            {
                pathFrom = root;
            }

            bool result = PathRelativePathToW(relativePathBuilder,
                pathFrom,
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
