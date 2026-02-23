#nullable disable
using System;
using System.IO;
using System.Runtime.InteropServices;
using static Microsoft.Windows.ProjFS.ProjFSNative;

namespace Microsoft.Windows.ProjFS
{
    /// <summary>
    /// Pure C# P/Invoke implementation of IDirectoryEnumerationResults,
    /// wrapping PrjFillDirEntryBuffer for the given PRJ_DIR_ENTRY_BUFFER_HANDLE.
    /// </summary>
    public class DirectoryEnumerationResults : IDirectoryEnumerationResults
    {
        private readonly IntPtr _dirEntryBufferHandle;

        internal DirectoryEnumerationResults(IntPtr dirEntryBufferHandle)
        {
            _dirEntryBufferHandle = dirEntryBufferHandle;
        }

        /// <summary>Gets the native directory entry buffer handle for use by CompleteCommand.</summary>
        internal IntPtr DirEntryBufferHandle => _dirEntryBufferHandle;

        /// <inheritdoc/>
        public bool Add(
            string fileName,
            long fileSize,
            bool isDirectory,
            FileAttributes fileAttributes,
            DateTime creationTime,
            DateTime lastAccessTime,
            DateTime lastWriteTime,
            DateTime changeTime)
        {
            ValidateFileName(fileName);

            var basicInfo = new PRJ_FILE_BASIC_INFO
            {
                IsDirectory = isDirectory ? (byte)1 : (byte)0,
                FileSize = fileSize,
                CreationTime = creationTime.ToFileTime(),
                LastAccessTime = lastAccessTime.ToFileTime(),
                LastWriteTime = lastWriteTime.ToFileTime(),
                ChangeTime = changeTime.ToFileTime(),
                FileAttributes = (uint)fileAttributes,
            };

            int hr = ProjFSNative.PrjFillDirEntryBuffer(fileName, ref basicInfo, _dirEntryBufferHandle);
            return hr >= 0; // S_OK = success; negative HRESULT (e.g. INSUFFICIENT_BUFFER) = buffer full
        }

        /// <inheritdoc/>
        public bool Add(string fileName, long fileSize, bool isDirectory)
        {
            ValidateFileName(fileName);

            var basicInfo = new PRJ_FILE_BASIC_INFO
            {
                IsDirectory = isDirectory ? (byte)1 : (byte)0,
                FileSize = fileSize,
            };

            int hr = ProjFSNative.PrjFillDirEntryBuffer(fileName, ref basicInfo, _dirEntryBufferHandle);
            return hr >= 0;
        }

        /// <summary>Adds one entry to a directory enumeration result, with optional symlink target.</summary>
        /// <remarks>
        /// <para>
        /// <b>NTFS Required for Symlinks:</b> When <paramref name=\"symlinkTargetOrNull\"/> is non-null,
        /// this calls PrjFillDirEntryBuffer2 which only works on NTFS volumes. ReFS does not support
        /// the atomic create ECP used by ProjFS to create symlink placeholders. On ReFS, the subsequent
        /// WritePlaceholderInfo2 call will fail with ERROR_NOT_SUPPORTED (0x80070032).
        /// </para>
        /// </remarks>
        /// <exception cref="ArgumentException"><paramref name="fileName"/> is null or empty.</exception>
        public bool Add(
            string fileName,
            long fileSize,
            bool isDirectory,
            FileAttributes fileAttributes,
            DateTime creationTime,
            DateTime lastAccessTime,
            DateTime lastWriteTime,
            DateTime changeTime,
            string symlinkTargetOrNull)
        {
            ValidateFileName(fileName);

            var basicInfo = new PRJ_FILE_BASIC_INFO
            {
                IsDirectory = isDirectory ? (byte)1 : (byte)0,
                FileSize = fileSize,
                CreationTime = creationTime.ToFileTime(),
                LastAccessTime = lastAccessTime.ToFileTime(),
                LastWriteTime = lastWriteTime.ToFileTime(),
                ChangeTime = changeTime.ToFileTime(),
                FileAttributes = (uint)fileAttributes,
            };

            if (symlinkTargetOrNull != null)
            {
                var extendedInfo = new PRJ_EXTENDED_INFO
                {
                    InfoType = PRJ_EXT_INFO_TYPE_SYMLINK,
                    NextInfoOffset = 0,
                };

                IntPtr targetPtr = Marshal.StringToHGlobalUni(symlinkTargetOrNull);
                try
                {
                    extendedInfo.SymlinkTargetName = targetPtr;
                    int hr = ProjFSNative.PrjFillDirEntryBuffer2(
                        _dirEntryBufferHandle, fileName, ref basicInfo, ref extendedInfo);
                    return hr >= 0;
                }
                finally
                {
                    Marshal.FreeHGlobal(targetPtr);
                }
            }
            else
            {
                int hr = ProjFSNative.PrjFillDirEntryBuffer2NoExtInfo(
                    _dirEntryBufferHandle, fileName, ref basicInfo, IntPtr.Zero);
                return hr >= 0;
            }
        }

        private static void ValidateFileName(string fileName)
        {
            if (string.IsNullOrEmpty(fileName))
            {
                throw new ArgumentException("fileName cannot be empty.");
            }
        }
    }
}
