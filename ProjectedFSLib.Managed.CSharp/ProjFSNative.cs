using System;
using System.Runtime.InteropServices;

namespace Microsoft.Windows.ProjFS
{
    /// <summary>
    /// Native P/Invoke declarations for ProjectedFSLib.dll.
    /// This replaces the C++/CLI mixed-mode ProjectedFSLib.Managed.dll with pure C# P/Invoke.
    /// </summary>
    internal static class ProjFSNative
    {
        private const string ProjFSLib = "ProjectedFSLib.dll";

        internal const int PlaceholderIdLength = 128;

        // ============================
        // Core virtualization lifetime
        // ============================

        [DllImport(ProjFSLib, CharSet = CharSet.Unicode, ExactSpelling = true)]
        internal static extern int PrjStartVirtualizing(
            string virtualizationRootPath,
            ref PRJ_CALLBACKS callbacks,
            IntPtr instanceContext,
            ref PRJ_STARTVIRTUALIZING_OPTIONS options,
            out IntPtr namespaceVirtualizationContext);

        [DllImport(ProjFSLib, ExactSpelling = true)]
        internal static extern void PrjStopVirtualizing(IntPtr namespaceVirtualizationContext);

        // ============================
        // Placeholder management
        // ============================

        [DllImport(ProjFSLib, CharSet = CharSet.Unicode, ExactSpelling = true)]
        internal static extern int PrjWritePlaceholderInfo(
            IntPtr namespaceVirtualizationContext,
            string destinationFileName,
            ref PRJ_PLACEHOLDER_INFO placeholderInfo,
            uint length);

        [DllImport(ProjFSLib, CharSet = CharSet.Unicode, ExactSpelling = true)]
        internal static extern int PrjWritePlaceholderInfo2(
            IntPtr namespaceVirtualizationContext,
            string destinationFileName,
            ref PRJ_PLACEHOLDER_INFO placeholderInfo,
            uint placeholderInfoSize,
            ref PRJ_EXTENDED_INFO extendedInfo);

        [DllImport(ProjFSLib, CharSet = CharSet.Unicode, ExactSpelling = true, EntryPoint = "PrjWritePlaceholderInfo2")]
        internal static extern int PrjWritePlaceholderInfo2Raw(
            IntPtr namespaceVirtualizationContext,
            IntPtr destinationFileName,
            IntPtr placeholderInfo,
            uint placeholderInfoSize,
            IntPtr extendedInfo);

        [DllImport(ProjFSLib, CharSet = CharSet.Unicode, ExactSpelling = true)]
        internal static extern int PrjUpdateFileIfNeeded(
            IntPtr namespaceVirtualizationContext,
            string destinationFileName,
            ref PRJ_PLACEHOLDER_INFO placeholderInfo,
            uint length,
            uint updateFlags,
            out uint failureReason);

        [DllImport(ProjFSLib, CharSet = CharSet.Unicode, ExactSpelling = true)]
        internal static extern int PrjDeleteFile(
            IntPtr namespaceVirtualizationContext,
            string destinationFileName,
            uint updateFlags,
            out uint failureReason);

        [DllImport(ProjFSLib, CharSet = CharSet.Unicode, ExactSpelling = true)]
        internal static extern int PrjMarkDirectoryAsPlaceholder(
            string rootPathName,
            string targetPathName,
            ref PRJ_PLACEHOLDER_VERSION_INFO versionInfo,
            ref Guid virtualizationInstanceID);

        // Overload for MarkDirectoryAsVirtualizationRoot (versionInfo = null)
        [DllImport(ProjFSLib, CharSet = CharSet.Unicode, ExactSpelling = true, EntryPoint = "PrjMarkDirectoryAsPlaceholder")]
        internal static extern int PrjMarkDirectoryAsVirtualizationRoot(
            string rootPathName,
            [MarshalAs(UnmanagedType.LPWStr)] string targetPathName,
            IntPtr versionInfo,
            ref Guid virtualizationInstanceID);

        // ============================
        // File data streaming
        // ============================

        [DllImport(ProjFSLib, ExactSpelling = true)]
        internal static extern int PrjWriteFileData(
            IntPtr namespaceVirtualizationContext,
            ref Guid dataStreamId,
            IntPtr buffer,
            ulong byteOffset,
            uint length);

        [DllImport(ProjFSLib, ExactSpelling = true)]
        internal static extern IntPtr PrjAllocateAlignedBuffer(
            IntPtr namespaceVirtualizationContext,
            UIntPtr size);

        [DllImport(ProjFSLib, ExactSpelling = true)]
        internal static extern void PrjFreeAlignedBuffer(IntPtr buffer);

        // ============================
        // Command completion
        // ============================

        [DllImport(ProjFSLib, ExactSpelling = true)]
        internal static extern int PrjCompleteCommand(
            IntPtr namespaceVirtualizationContext,
            int commandId,
            int completionResult,
            IntPtr extendedParameters);

        [DllImport(ProjFSLib, ExactSpelling = true, EntryPoint = "PrjCompleteCommand")]
        internal static extern int PrjCompleteCommandWithNotification(
            IntPtr namespaceVirtualizationContext,
            int commandId,
            int completionResult,
            ref PRJ_COMPLETE_COMMAND_EXTENDED_PARAMETERS extendedParameters);

        // ============================
        // Cache management
        // ============================

        [DllImport(ProjFSLib, ExactSpelling = true)]
        internal static extern int PrjClearNegativePathCache(
            IntPtr namespaceVirtualizationContext,
            out uint totalEntryNumber);

        // ============================
        // Directory enumeration
        // ============================

        [DllImport(ProjFSLib, CharSet = CharSet.Unicode, ExactSpelling = true)]
        internal static extern int PrjFillDirEntryBuffer(
            string fileName,
            ref PRJ_FILE_BASIC_INFO fileBasicInfo,
            IntPtr dirEntryBufferHandle);

        [DllImport(ProjFSLib, CharSet = CharSet.Unicode, ExactSpelling = true)]
        internal static extern int PrjFillDirEntryBuffer2(
            IntPtr dirEntryBufferHandle,
            string fileName,
            ref PRJ_FILE_BASIC_INFO fileBasicInfo,
            ref PRJ_EXTENDED_INFO extendedInfo);

        [DllImport(ProjFSLib, CharSet = CharSet.Unicode, ExactSpelling = true, EntryPoint = "PrjFillDirEntryBuffer2")]
        internal static extern int PrjFillDirEntryBuffer2NoExtInfo(
            IntPtr dirEntryBufferHandle,
            string fileName,
            ref PRJ_FILE_BASIC_INFO fileBasicInfo,
            IntPtr extendedInfo);

        // ============================
        // Filename utilities
        // ============================

        [DllImport(ProjFSLib, CharSet = CharSet.Unicode, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        internal static extern bool PrjDoesNameContainWildCards(string fileName);

        [DllImport(ProjFSLib, CharSet = CharSet.Unicode, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.U1)]
        internal static extern bool PrjFileNameMatch(string fileNameToCheck, string pattern);

        [DllImport(ProjFSLib, CharSet = CharSet.Unicode, ExactSpelling = true)]
        internal static extern int PrjFileNameCompare(string fileName1, string fileName2);

        // ============================
        // File state query
        // ============================

        [DllImport(ProjFSLib, CharSet = CharSet.Unicode, ExactSpelling = true)]
        internal static extern int PrjGetOnDiskFileState(string destinationFileName, out uint fileState);

        // ============================
        // Virtualization instance info
        // ============================

        [DllImport(ProjFSLib, ExactSpelling = true)]
        internal static extern int PrjGetVirtualizationInstanceInfo(
            IntPtr namespaceVirtualizationContext,
            ref PRJ_VIRTUALIZATION_INSTANCE_INFO virtualizationInstanceInfo);

        // ============================
        // Native structures
        // ============================

        [StructLayout(LayoutKind.Sequential)]
        internal struct PRJ_CALLBACK_DATA
        {
            public uint Size;
            public uint Flags;
            public IntPtr NamespaceVirtualizationContext;
            public int CommandId;
            public Guid FileId;
            public Guid DataStreamId;
            public IntPtr FilePathName;  // PCWSTR
            public IntPtr VersionInfo;   // PRJ_PLACEHOLDER_VERSION_INFO*
            public uint TriggeringProcessId;
            public IntPtr TriggeringProcessImageFileName; // PCWSTR
            public IntPtr InstanceContext;
        }

        internal const uint PRJ_CB_DATA_FLAG_ENUM_RESTART_SCAN = 0x00000001;
        internal const uint PRJ_CB_DATA_FLAG_ENUM_RETURN_SINGLE_ENTRY = 0x00000002;

        [StructLayout(LayoutKind.Sequential)]
        internal unsafe struct PRJ_PLACEHOLDER_VERSION_INFO
        {
            public fixed byte ProviderID[PlaceholderIdLength];
            public fixed byte ContentID[PlaceholderIdLength];
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct PRJ_FILE_BASIC_INFO
        {
            public byte IsDirectory;       // BOOLEAN (1 byte)
            private byte _pad1;
            private byte _pad2;
            private byte _pad3;
            private int _pad4;             // Pad to 8-byte alignment for FileSize
            public long FileSize;          // INT64
            public long CreationTime;      // LARGE_INTEGER (FileTime)
            public long LastAccessTime;    // LARGE_INTEGER (FileTime)
            public long LastWriteTime;     // LARGE_INTEGER (FileTime)
            public long ChangeTime;        // LARGE_INTEGER (FileTime)
            public uint FileAttributes;    // UINT32
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct PRJ_PLACEHOLDER_INFO
        {
            public PRJ_FILE_BASIC_INFO FileBasicInfo;
            // EaInformation
            public uint EaBufferSize;
            public uint OffsetToFirstEa;
            // SecurityInformation
            public uint SecurityBufferSize;
            public uint OffsetToSecurityDescriptor;
            // StreamsInformation
            public uint StreamsInfoBufferSize;
            public uint OffsetToFirstStreamInfo;
            // VersionInfo
            public PRJ_PLACEHOLDER_VERSION_INFO VersionInfo;
            // VariableData[1] — flexible array member in the native struct
            // Must be included so that sizeof matches the native sizeof(PRJ_PLACEHOLDER_INFO) = 344
            public byte VariableData;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct PRJ_STARTVIRTUALIZING_OPTIONS
        {
            public uint Flags;
            public uint PoolThreadCount;
            public uint ConcurrentThreadCount;
            public IntPtr NotificationMappings;     // PRJ_NOTIFICATION_MAPPING*
            public uint NotificationMappingsCount;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct PRJ_VIRTUALIZATION_INSTANCE_INFO
        {
            public Guid InstanceID;
            public uint WriteAlignment;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct PRJ_EXTENDED_INFO
        {
            public uint InfoType;
            public uint NextInfoOffset;
            public IntPtr SymlinkTargetName;  // PCWSTR
        }

        internal const uint PRJ_EXT_INFO_TYPE_SYMLINK = 1;

        [StructLayout(LayoutKind.Sequential)]
        internal struct PRJ_NOTIFICATION_MAPPING_NATIVE
        {
            public uint NotificationBitMask;   // PRJ_NOTIFY_TYPES
            public IntPtr NotificationRoot;    // PCWSTR
        }

        internal const uint PRJ_FLAG_NONE = 0;
        internal const uint PRJ_FLAG_USE_NEGATIVE_PATH_CACHE = 1;

        // PRJ_NOTIFICATION values (for the notification callback)
        internal const int PRJ_NOTIFICATION_FILE_OPENED = 0x00000002;
        internal const int PRJ_NOTIFICATION_NEW_FILE_CREATED = 0x00000004;
        internal const int PRJ_NOTIFICATION_FILE_OVERWRITTEN = 0x00000008;
        internal const int PRJ_NOTIFICATION_PRE_DELETE = 0x00000010;
        internal const int PRJ_NOTIFICATION_PRE_RENAME = 0x00000020;
        internal const int PRJ_NOTIFICATION_PRE_SET_HARDLINK = 0x00000040;
        internal const int PRJ_NOTIFICATION_FILE_RENAMED = 0x00000080;
        internal const int PRJ_NOTIFICATION_HARDLINK_CREATED = 0x00000100;
        internal const int PRJ_NOTIFICATION_FILE_HANDLE_CLOSED_NO_MODIFICATION = 0x00000200;
        internal const int PRJ_NOTIFICATION_FILE_HANDLE_CLOSED_FILE_MODIFIED = 0x00000400;
        internal const int PRJ_NOTIFICATION_FILE_HANDLE_CLOSED_FILE_DELETED = 0x00000800;
        internal const int PRJ_NOTIFICATION_FILE_PRE_CONVERT_TO_FULL = 0x00001000;

        // PRJ_COMPLETE_COMMAND_TYPE
        internal const int PRJ_COMPLETE_COMMAND_TYPE_NOTIFICATION = 1;
        internal const int PRJ_COMPLETE_COMMAND_TYPE_ENUMERATION = 2;

        [StructLayout(LayoutKind.Explicit)]
        internal struct PRJ_COMPLETE_COMMAND_EXTENDED_PARAMETERS
        {
            [FieldOffset(0)]
            public int CommandType;

            // For Notification type: notification mask at offset 8
            [FieldOffset(8)]
            public uint NotificationMask;

            // For Enumeration type: dir entry buffer handle at offset 8
            [FieldOffset(8)]
            public IntPtr DirEntryBufferHandle;
        }

        // ============================
        // Callback function pointer struct
        // ============================

        [StructLayout(LayoutKind.Sequential)]
        internal struct PRJ_CALLBACKS
        {
            public IntPtr StartDirectoryEnumerationCallback;
            public IntPtr EndDirectoryEnumerationCallback;
            public IntPtr GetDirectoryEnumerationCallback;
            public IntPtr GetPlaceholderInfoCallback;
            public IntPtr GetFileDataCallback;
            public IntPtr QueryFileNameCallback;
            public IntPtr NotificationCallback;
            public IntPtr CancelCommandCallback;
        }

        // Managed delegate types for callbacks (these get marshaled to native function pointers)
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        internal unsafe delegate int StartDirectoryEnumerationDelegate(PRJ_CALLBACK_DATA* callbackData, Guid* enumerationId);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        internal unsafe delegate int EndDirectoryEnumerationDelegate(PRJ_CALLBACK_DATA* callbackData, Guid* enumerationId);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        internal unsafe delegate int GetDirectoryEnumerationDelegate(PRJ_CALLBACK_DATA* callbackData, Guid* enumerationId, IntPtr searchExpression, IntPtr dirEntryBufferHandle);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        internal unsafe delegate int GetPlaceholderInfoDelegate(PRJ_CALLBACK_DATA* callbackData);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        internal unsafe delegate int GetFileDataDelegate(PRJ_CALLBACK_DATA* callbackData, ulong byteOffset, uint length);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        internal unsafe delegate int QueryFileNameDelegate(PRJ_CALLBACK_DATA* callbackData);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        internal unsafe delegate int NotificationDelegate(PRJ_CALLBACK_DATA* callbackData, byte isDirectory, int notification, IntPtr destinationFileName, IntPtr operationParameters);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        internal unsafe delegate void CancelCommandDelegate(PRJ_CALLBACK_DATA* callbackData);
    }
}
