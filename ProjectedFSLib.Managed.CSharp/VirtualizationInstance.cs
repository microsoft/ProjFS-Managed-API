#nullable disable
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using static Microsoft.Windows.ProjFS.ProjFSNative;

namespace Microsoft.Windows.ProjFS
{
    /// <summary>
    /// Pure C# P/Invoke implementation of the ProjFS VirtualizationInstance,
    /// replacing the C++/CLI mixed-mode ProjectedFSLib.Managed.dll for NativeAOT compatibility.
    /// </summary>
    public class VirtualizationInstance : IVirtualizationInstance
    {
        public const int PlaceholderIdLength = ProjFSNative.PlaceholderIdLength;

        /// <summary>Returns the maximum allowed length of a placeholder's contentID or provider ID.</summary>
        int IVirtualizationInstance.PlaceholderIdLength => PlaceholderIdLength;

        private readonly string _rootPath;
        private readonly uint _poolThreadCount;
        private readonly uint _concurrentThreadCount;
        private readonly bool _enableNegativePathCache;
        private readonly List<NotificationMapping> _notificationMappings;

        private IntPtr _context; // PRJ_NAMESPACE_VIRTUALIZATION_CONTEXT
        private GCHandle _selfHandle;
        private Guid _instanceId;
        private IRequiredCallbacks _requiredCallbacks;
        private GCHandle _notificationMappingsHandle;
        private IntPtr[] _notificationRootStrings;

        // Keep delegates alive to prevent GC while native code holds function pointers
        private StartDirectoryEnumerationDelegate _startDirEnumDelegate;
        private EndDirectoryEnumerationDelegate _endDirEnumDelegate;
        private GetDirectoryEnumerationDelegate _getDirEnumDelegate;
        private GetPlaceholderInfoDelegate _getPlaceholderInfoDelegate;
        private GetFileDataDelegate _getFileDataDelegate;
        private QueryFileNameDelegate _queryFileNameDelegate;
        private NotificationDelegate _notificationDelegate;
        private CancelCommandDelegate _cancelCommandDelegate;

        public VirtualizationInstance(
            string virtualizationRootPath,
            uint poolThreadCount,
            uint concurrentThreadCount,
            bool enableNegativePathCache,
            IReadOnlyCollection<NotificationMapping> notificationMappings)
        {
            _rootPath = virtualizationRootPath ?? throw new ArgumentNullException(nameof(virtualizationRootPath));
            _poolThreadCount = poolThreadCount;
            _concurrentThreadCount = concurrentThreadCount;
            _enableNegativePathCache = enableNegativePathCache;
            _notificationMappings = new List<NotificationMapping>(notificationMappings ?? Array.Empty<NotificationMapping>());

            // Match C++/CLI behavior: create the directory and mark as virtualization root
            bool markAsRoot = false;
            var dirInfo = new DirectoryInfo(_rootPath);
            if (!dirInfo.Exists)
            {
                _instanceId = Guid.NewGuid();
                dirInfo.Create();
                markAsRoot = true;
            }
            else
            {
                // Check if the directory already has a ProjFS reparse point.
                // If not, we need to mark it as a root.
                // Use PrjGetOnDiskFileState to detect if it's already a ProjFS placeholder/root.
                int hr = ProjFSNative.PrjGetOnDiskFileState(_rootPath, out uint fileState);
                if (hr < 0 || fileState == 0)
                {
                    // Not a ProjFS virtualization root yet — need to mark it
                    _instanceId = Guid.NewGuid();
                    markAsRoot = true;
                }
                else
                {
                    // Already marked. Get the instance ID via PrjGetVirtualizationInstanceInfo
                    // after StartVirtualizing. For now, generate a new one.
                    _instanceId = Guid.NewGuid();
                }
            }

            if (markAsRoot)
            {
                HResult markResult = MarkDirectoryAsVirtualizationRoot(_rootPath, _instanceId);
                if (markResult != HResult.Ok)
                {
                    int errorCode = unchecked((int)markResult) & 0xFFFF;
                    throw new System.ComponentModel.Win32Exception(errorCode,
                        $"Failed to mark directory {_rootPath} as virtualization root. HRESULT: 0x{unchecked((uint)markResult):X8}");
                }
            }
        }

        public CancelCommandCallback OnCancelCommand { get; set; }
        public NotifyFileOpenedCallback OnNotifyFileOpened { get; set; }
        public NotifyNewFileCreatedCallback OnNotifyNewFileCreated { get; set; }
        public NotifyFileOverwrittenCallback OnNotifyFileOverwritten { get; set; }
        public NotifyFileHandleClosedNoModificationCallback OnNotifyFileHandleClosedNoModification { get; set; }
        public NotifyFileHandleClosedFileModifiedOrDeletedCallback OnNotifyFileHandleClosedFileModifiedOrDeleted { get; set; }
        public NotifyFilePreConvertToFullCallback OnNotifyFilePreConvertToFull { get; set; }
        public NotifyFileRenamedCallback OnNotifyFileRenamed { get; set; }
        public NotifyHardlinkCreatedCallback OnNotifyHardlinkCreated { get; set; }
        public NotifyPreDeleteCallback OnNotifyPreDelete { get; set; }
        public NotifyPreRenameCallback OnNotifyPreRename { get; set; }
        public NotifyPreCreateHardlinkCallback OnNotifyPreCreateHardlink { get; set; }
        public QueryFileNameCallback OnQueryFileName { get; set; }

        public Guid VirtualizationInstanceId => _instanceId;
        public IRequiredCallbacks RequiredCallbacks => _requiredCallbacks;

        /// <summary>
        /// Marks the specified directory as a virtualization root.
        /// </summary>
        public static HResult MarkDirectoryAsVirtualizationRoot(string rootPath, Guid virtualizationInstanceGuid)
        {
            int hr = ProjFSNative.PrjMarkDirectoryAsVirtualizationRoot(
                rootPath,
                null,
                IntPtr.Zero,
                ref virtualizationInstanceGuid);
            return (HResult)hr;
        }

        public unsafe HResult StartVirtualizing(IRequiredCallbacks requiredCallbacks)
        {
            _requiredCallbacks = requiredCallbacks ?? throw new ArgumentNullException(nameof(requiredCallbacks));

            _selfHandle = GCHandle.Alloc(this);

            // Create managed delegates for native callbacks (prevents GC)
            _startDirEnumDelegate = NativeStartDirectoryEnumeration;
            _endDirEnumDelegate = NativeEndDirectoryEnumeration;
            _getDirEnumDelegate = NativeGetDirectoryEnumeration;
            _getPlaceholderInfoDelegate = NativeGetPlaceholderInfo;
            _getFileDataDelegate = NativeGetFileData;
            _queryFileNameDelegate = NativeQueryFileName;
            _notificationDelegate = NativeNotification;
            _cancelCommandDelegate = NativeCancelCommand;

            var callbacks = new PRJ_CALLBACKS
            {
                StartDirectoryEnumerationCallback = Marshal.GetFunctionPointerForDelegate(_startDirEnumDelegate),
                EndDirectoryEnumerationCallback = Marshal.GetFunctionPointerForDelegate(_endDirEnumDelegate),
                GetDirectoryEnumerationCallback = Marshal.GetFunctionPointerForDelegate(_getDirEnumDelegate),
                GetPlaceholderInfoCallback = Marshal.GetFunctionPointerForDelegate(_getPlaceholderInfoDelegate),
                GetFileDataCallback = Marshal.GetFunctionPointerForDelegate(_getFileDataDelegate),
                QueryFileNameCallback = Marshal.GetFunctionPointerForDelegate(_queryFileNameDelegate),
                NotificationCallback = Marshal.GetFunctionPointerForDelegate(_notificationDelegate),
                CancelCommandCallback = Marshal.GetFunctionPointerForDelegate(_cancelCommandDelegate),
            };

            // Set up notification mappings
            var nativeMappings = new PRJ_NOTIFICATION_MAPPING_NATIVE[_notificationMappings.Count];
            var allocatedStrings = new IntPtr[_notificationMappings.Count];

            for (int i = 0; i < _notificationMappings.Count; i++)
                {
                    string root = _notificationMappings[i].NotificationRoot ?? string.Empty;
                    allocatedStrings[i] = Marshal.StringToHGlobalUni(root);
                    nativeMappings[i] = new PRJ_NOTIFICATION_MAPPING_NATIVE
                    {
                        NotificationBitMask = (uint)_notificationMappings[i].NotificationMask,
                        NotificationRoot = allocatedStrings[i],
                    };
                }

                var options = new PRJ_STARTVIRTUALIZING_OPTIONS
                {
                    Flags = _enableNegativePathCache ? PRJ_FLAG_USE_NEGATIVE_PATH_CACHE : PRJ_FLAG_NONE,
                    PoolThreadCount = _poolThreadCount,
                    ConcurrentThreadCount = _concurrentThreadCount,
                    NotificationMappingsCount = (uint)nativeMappings.Length,
                };

                GCHandle mappingsHandle = default;
                try
                {
                    if (nativeMappings.Length > 0)
                    {
                        mappingsHandle = GCHandle.Alloc(nativeMappings, GCHandleType.Pinned);
                        options.NotificationMappings = mappingsHandle.AddrOfPinnedObject();
                    }

                    int hr = ProjFSNative.PrjStartVirtualizing(
                        _rootPath,
                        ref callbacks,
                        GCHandle.ToIntPtr(_selfHandle),
                        ref options,
                        out _context);

                    if (hr < 0)
                    {
                        _selfHandle.Free();
                    }

                    return (HResult)hr;
                }
                finally
                {
                    // Do NOT free mappingsHandle or allocatedStrings here!
                    // ProjFS may cache the notification mapping pointers.
                    // Store them for cleanup in StopVirtualizing.
                    if (mappingsHandle.IsAllocated)
                    {
                        _notificationMappingsHandle = mappingsHandle;
                    }
                    _notificationRootStrings = allocatedStrings;
                }
            // NOTE: Do NOT free allocatedStrings here — ProjFS may cache notification
            // mapping pointers. They are freed in StopVirtualizing.
        }
        public void StopVirtualizing()
        {
            if (_context != IntPtr.Zero)
            {
                ProjFSNative.PrjStopVirtualizing(_context);
                _context = IntPtr.Zero;
            }

            if (_selfHandle.IsAllocated)
            {
                _selfHandle.Free();
            }
        }

        public HResult ClearNegativePathCache(out uint totalEntryNumber)
        {
            int hr = ProjFSNative.PrjClearNegativePathCache(_context, out totalEntryNumber);
            return (HResult)hr;
        }

        public HResult DeleteFile(string relativePath, UpdateType updateFlags, out UpdateFailureCause failureReason)
        {
            int hr = ProjFSNative.PrjDeleteFile(_context, relativePath, (uint)updateFlags, out uint cause);
            failureReason = (UpdateFailureCause)cause;
            return (HResult)hr;
        }

        public unsafe HResult WritePlaceholderInfo(
            string relativePath,
            DateTime creationTime,
            DateTime lastAccessTime,
            DateTime lastWriteTime,
            DateTime changeTime,
            FileAttributes fileAttributes,
            long endOfFile,
            bool isDirectory,
            byte[] contentId,
            byte[] providerId)
        {
            var info = new PRJ_PLACEHOLDER_INFO();
            info.FileBasicInfo.IsDirectory = isDirectory ? (byte)1 : (byte)0;
            info.FileBasicInfo.FileSize = endOfFile;
            info.FileBasicInfo.CreationTime = creationTime.ToFileTime();
            info.FileBasicInfo.LastAccessTime = lastAccessTime.ToFileTime();
            info.FileBasicInfo.LastWriteTime = lastWriteTime.ToFileTime();
            info.FileBasicInfo.ChangeTime = changeTime.ToFileTime();
            info.FileBasicInfo.FileAttributes = (uint)fileAttributes;

            CopyIdToVersionInfo(contentId, providerId, ref info.VersionInfo);

            int hr = ProjFSNative.PrjWritePlaceholderInfo(
                _context,
                relativePath,
                ref info,
                (uint)Marshal.SizeOf<PRJ_PLACEHOLDER_INFO>());

            return (HResult)hr;
        }

        /// <summary>
        /// Sends file or directory metadata to ProjFS, with optional symlink extended info.
        /// </summary>
        public unsafe HResult WritePlaceholderInfo2(
            string relativePath,
            DateTime creationTime,
            DateTime lastAccessTime,
            DateTime lastWriteTime,
            DateTime changeTime,
            FileAttributes fileAttributes,
            long endOfFile,
            bool isDirectory,
            string symlinkTargetOrNull,
            byte[] contentId,
            byte[] providerId)
        {
            var info = new PRJ_PLACEHOLDER_INFO();
            info.FileBasicInfo.IsDirectory = isDirectory ? (byte)1 : (byte)0;
            info.FileBasicInfo.FileSize = isDirectory ? 0 : endOfFile;
            info.FileBasicInfo.CreationTime = creationTime.ToFileTime();
            info.FileBasicInfo.LastAccessTime = lastAccessTime.ToFileTime();
            info.FileBasicInfo.LastWriteTime = lastWriteTime.ToFileTime();
            info.FileBasicInfo.ChangeTime = changeTime.ToFileTime();
            info.FileBasicInfo.FileAttributes = (uint)fileAttributes;

            CopyIdToVersionInfo(contentId, providerId, ref info.VersionInfo);

            if (!string.IsNullOrEmpty(symlinkTargetOrNull))
            {
                int hr;
                fixed (char* pTarget = symlinkTargetOrNull)
                fixed (char* pPath = relativePath)
                {
                    var extendedInfo = new PRJ_EXTENDED_INFO
                    {
                        InfoType = PRJ_EXT_INFO_TYPE_SYMLINK,
                        NextInfoOffset = 0,
                        SymlinkTargetName = (IntPtr)pTarget,
                    };

                    PRJ_EXTENDED_INFO* pExt = &extendedInfo;

                    hr = ProjFSNative.PrjWritePlaceholderInfo2Raw(
                        _context,
                        (IntPtr)pPath,
                        (IntPtr)System.Runtime.CompilerServices.Unsafe.AsPointer(ref info),
                        (uint)sizeof(PRJ_PLACEHOLDER_INFO),
                        (IntPtr)pExt);
                }

                return (HResult)hr;
            }
            else
            {
                int hr = ProjFSNative.PrjWritePlaceholderInfo(
                    _context,
                    relativePath,
                    ref info,
                    (uint)Marshal.SizeOf<PRJ_PLACEHOLDER_INFO>());
                return (HResult)hr;
            }
        }

        public unsafe HResult UpdateFileIfNeeded(
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
            out UpdateFailureCause failureReason)
        {
            var info = new PRJ_PLACEHOLDER_INFO();
            info.FileBasicInfo.IsDirectory = 0;
            info.FileBasicInfo.FileSize = endOfFile;
            info.FileBasicInfo.CreationTime = creationTime.ToFileTime();
            info.FileBasicInfo.LastAccessTime = lastAccessTime.ToFileTime();
            info.FileBasicInfo.LastWriteTime = lastWriteTime.ToFileTime();
            info.FileBasicInfo.ChangeTime = changeTime.ToFileTime();
            info.FileBasicInfo.FileAttributes = (uint)fileAttributes;

            CopyIdToVersionInfo(contentId, providerId, ref info.VersionInfo);

            int hr = ProjFSNative.PrjUpdateFileIfNeeded(
                _context,
                relativePath,
                ref info,
                (uint)Marshal.SizeOf<PRJ_PLACEHOLDER_INFO>(),
                (uint)updateFlags,
                out uint cause);

            failureReason = (UpdateFailureCause)cause;
            return (HResult)hr;
        }

        public HResult CompleteCommand(int commandId, HResult completionResult)
        {
            int hr = ProjFSNative.PrjCompleteCommand(_context, commandId, (int)completionResult, IntPtr.Zero);
            return (HResult)hr;
        }

        public HResult CompleteCommand(int commandId, NotificationType newNotificationMask)
        {
            var extParams = new PRJ_COMPLETE_COMMAND_EXTENDED_PARAMETERS
            {
                CommandType = PRJ_COMPLETE_COMMAND_TYPE_NOTIFICATION,
                NotificationMask = (uint)newNotificationMask,
            };

            int hr = ProjFSNative.PrjCompleteCommandWithNotification(_context, commandId, 0, ref extParams);
            return (HResult)hr;
        }

        public HResult CompleteCommand(int commandId, IDirectoryEnumerationResults results)
        {
            var dirResults = (DirectoryEnumerationResults)results;
            var extParams = new PRJ_COMPLETE_COMMAND_EXTENDED_PARAMETERS
            {
                CommandType = PRJ_COMPLETE_COMMAND_TYPE_ENUMERATION,
                DirEntryBufferHandle = dirResults.DirEntryBufferHandle,
            };

            int hr = ProjFSNative.PrjCompleteCommandWithNotification(_context, commandId, 0, ref extParams);
            return (HResult)hr;
        }

        public HResult CompleteCommand(int commandId)
        {
            int hr = ProjFSNative.PrjCompleteCommand(_context, commandId, 0, IntPtr.Zero);
            return (HResult)hr;
        }

        public IWriteBuffer CreateWriteBuffer(uint desiredBufferSize)
        {
            return new WriteBuffer(_context, desiredBufferSize);
        }

        public IWriteBuffer CreateWriteBuffer(ulong byteOffset, uint length, out ulong alignedByteOffset, out uint alignedLength)
        {
            // Get the sector size from PrjGetVirtualizationInstanceInfo so we can
            // compute aligned values for byteOffset and length.
            var instanceInfo = new PRJ_VIRTUALIZATION_INSTANCE_INFO();
            int hr = ProjFSNative.PrjGetVirtualizationInstanceInfo(_context, ref instanceInfo);
            if (hr < 0)
            {
                throw new System.ComponentModel.Win32Exception(hr,
                    $"Failed to retrieve virtualization instance info for directory {_rootPath}.");
            }

            uint bytesPerSector = instanceInfo.WriteAlignment;

            // alignedByteOffset is byteOffset, rounded down to the nearest bytesPerSector boundary.
            alignedByteOffset = byteOffset & ~((ulong)bytesPerSector - 1);

            // alignedLength is the end offset of the requested range, rounded up to the nearest
            // bytesPerSector boundary, minus the aligned start offset.
            ulong rangeEndOffset = byteOffset + (ulong)length;
            ulong alignedRangeEndOffset = (rangeEndOffset + ((ulong)bytesPerSector - 1)) & ~((ulong)bytesPerSector - 1);
            alignedLength = (uint)(alignedRangeEndOffset - alignedByteOffset);

            // Create a buffer of the aligned length.
            return CreateWriteBuffer(alignedLength);
        }

        public HResult WriteFileData(Guid dataStreamId, IWriteBuffer buffer, ulong byteOffset, uint length)
        {
            int hr = ProjFSNative.PrjWriteFileData(_context, ref dataStreamId, buffer.Pointer, byteOffset, length);
            return (HResult)hr;
        }

        public unsafe HResult MarkDirectoryAsPlaceholder(string targetDirectoryPath, byte[] contentId, byte[] providerId)
        {
            var versionInfo = new PRJ_PLACEHOLDER_VERSION_INFO();
            CopyIdToVersionInfo(contentId, providerId, ref versionInfo);

            int hr = ProjFSNative.PrjMarkDirectoryAsPlaceholder(
                _rootPath,
                targetDirectoryPath,
                ref versionInfo,
                ref _instanceId);

            return (HResult)hr;
        }

        // ===========================================================
        // Native callback implementations
        // ===========================================================

        private static VirtualizationInstance GetInstance(IntPtr instanceContext)
        {
            return (VirtualizationInstance)GCHandle.FromIntPtr(instanceContext).Target;
        }

        private static unsafe int NativeStartDirectoryEnumeration(PRJ_CALLBACK_DATA* pData, Guid* pEnumId)
        {
            try
            {
                var inst = GetInstance(pData->InstanceContext);
                string virtualPath = Marshal.PtrToStringUni(pData->FilePathName);
                string processName = Marshal.PtrToStringUni(pData->TriggeringProcessImageFileName);
                return (int)inst._requiredCallbacks.StartDirectoryEnumerationCallback(
                    pData->CommandId, *pEnumId, virtualPath, pData->TriggeringProcessId, processName);
            }
            catch (Exception ex)
            {
                System.Diagnostics.Trace.WriteLine($"[ProjFS] StartDirEnum EXCEPTION: {ex}");
                return (int)HResult.InternalError;
            }
        }

        private static unsafe int NativeEndDirectoryEnumeration(PRJ_CALLBACK_DATA* pData, Guid* pEnumId)
        {
            try
            {
                var inst = GetInstance(pData->InstanceContext);
                return (int)inst._requiredCallbacks.EndDirectoryEnumerationCallback(*pEnumId);
            }
            catch
            {
                return (int)HResult.InternalError;
            }
        }

        private static unsafe int NativeGetDirectoryEnumeration(PRJ_CALLBACK_DATA* pData, Guid* pEnumId, IntPtr searchExpression, IntPtr dirEntryBufferHandle)
        {
            try
            {
                var inst = GetInstance(pData->InstanceContext);
                string filterFileName = searchExpression != IntPtr.Zero ? Marshal.PtrToStringUni(searchExpression) : null;
                bool restartScan = (pData->Flags & PRJ_CB_DATA_FLAG_ENUM_RESTART_SCAN) != 0;
                var results = new DirectoryEnumerationResults(dirEntryBufferHandle);
                return (int)inst._requiredCallbacks.GetDirectoryEnumerationCallback(
                    pData->CommandId, *pEnumId, filterFileName, restartScan, results);
            }
            catch (Exception ex)
            {
                System.Diagnostics.Trace.WriteLine($"[ProjFS] GetDirEnum EXCEPTION: {ex}");
                return (int)HResult.InternalError;
            }
        }

        private static unsafe int NativeGetPlaceholderInfo(PRJ_CALLBACK_DATA* pData)
        {
            try
            {
                var inst = GetInstance(pData->InstanceContext);
                string virtualPath = Marshal.PtrToStringUni(pData->FilePathName);
                string processName = Marshal.PtrToStringUni(pData->TriggeringProcessImageFileName);
                return (int)inst._requiredCallbacks.GetPlaceholderInfoCallback(
                    pData->CommandId, virtualPath, pData->TriggeringProcessId, processName);
            }
            catch (Exception ex)
            {
                System.Diagnostics.Trace.WriteLine($"[ProjFS] GetPlaceholderInfo EXCEPTION: {ex}");
                return (int)HResult.InternalError;
            }
        }

        private static unsafe int NativeGetFileData(PRJ_CALLBACK_DATA* pData, ulong byteOffset, uint length)
        {
            try
            {
                var inst = GetInstance(pData->InstanceContext);
                string virtualPath = Marshal.PtrToStringUni(pData->FilePathName);
                string processName = Marshal.PtrToStringUni(pData->TriggeringProcessImageFileName);

                byte[] contentId = null;
                byte[] providerId = null;
                if (pData->VersionInfo != IntPtr.Zero)
                {
                    var versionInfo = (PRJ_PLACEHOLDER_VERSION_INFO*)pData->VersionInfo;
                    contentId = new byte[PlaceholderIdLength];
                    providerId = new byte[PlaceholderIdLength];
                    fixed (byte* pContent = contentId, pProvider = providerId)
                    {
                        Buffer.MemoryCopy(versionInfo->ContentID, pContent, PlaceholderIdLength, PlaceholderIdLength);
                        Buffer.MemoryCopy(versionInfo->ProviderID, pProvider, PlaceholderIdLength, PlaceholderIdLength);
                    }
                }

                return (int)inst._requiredCallbacks.GetFileDataCallback(
                    pData->CommandId, virtualPath, byteOffset, length,
                    pData->DataStreamId, contentId, providerId,
                    pData->TriggeringProcessId, processName);
            }
            catch
            {
                return (int)HResult.InternalError;
            }
        }

        private static unsafe int NativeQueryFileName(PRJ_CALLBACK_DATA* pData)
        {
            try
            {
                var inst = GetInstance(pData->InstanceContext);
                if (inst.OnQueryFileName != null)
                {
                    string virtualPath = Marshal.PtrToStringUni(pData->FilePathName);
                    return (int)inst.OnQueryFileName(virtualPath);
                }

                return (int)HResult.Ok;
            }
            catch
            {
                return (int)HResult.InternalError;
            }
        }

        private static unsafe int NativeNotification(PRJ_CALLBACK_DATA* pData, byte isDirectory, int notification, IntPtr destinationFileName, IntPtr operationParameters)
        {
            try
            {
                var inst = GetInstance(pData->InstanceContext);
                string virtualPath = Marshal.PtrToStringUni(pData->FilePathName);
                string destPath = destinationFileName != IntPtr.Zero ? Marshal.PtrToStringUni(destinationFileName) : null;
                bool isDir = isDirectory != 0;
                uint processId = pData->TriggeringProcessId;
                string processName = Marshal.PtrToStringUni(pData->TriggeringProcessImageFileName);

                switch (notification)
                {
                    case PRJ_NOTIFICATION_FILE_OPENED:
                        if (inst.OnNotifyFileOpened != null)
                        {
                            bool allow = inst.OnNotifyFileOpened(virtualPath, isDir, processId, processName, out NotificationType mask);
                            if (!allow)
                            {
                                return (int)HResult.AccessDenied;
                            }

                            WriteNotificationMask(operationParameters, (uint)mask);
                        }

                        break;

                    case PRJ_NOTIFICATION_NEW_FILE_CREATED:
                        if (inst.OnNotifyNewFileCreated != null)
                        {
                            inst.OnNotifyNewFileCreated(virtualPath, isDir, processId, processName, out NotificationType mask);
                            WriteNotificationMask(operationParameters, (uint)mask);
                        }

                        break;

                    case PRJ_NOTIFICATION_FILE_OVERWRITTEN:
                        if (inst.OnNotifyFileOverwritten != null)
                        {
                            inst.OnNotifyFileOverwritten(virtualPath, isDir, processId, processName, out NotificationType mask);
                            WriteNotificationMask(operationParameters, (uint)mask);
                        }

                        break;

                    case PRJ_NOTIFICATION_PRE_DELETE:
                        if (inst.OnNotifyPreDelete != null)
                        {
                            bool allow = inst.OnNotifyPreDelete(virtualPath, isDir, processId, processName);
                            if (!allow)
                            {
                                return (int)HResult.AccessDenied;
                            }
                        }

                        break;

                    case PRJ_NOTIFICATION_PRE_RENAME:
                        if (inst.OnNotifyPreRename != null)
                        {
                            bool allow = inst.OnNotifyPreRename(virtualPath, destPath, processId, processName);
                            if (!allow)
                            {
                                return (int)HResult.AccessDenied;
                            }
                        }

                        break;

                    case PRJ_NOTIFICATION_PRE_SET_HARDLINK:
                        if (inst.OnNotifyPreCreateHardlink != null)
                        {
                            bool allow = inst.OnNotifyPreCreateHardlink(virtualPath, destPath, processId, processName);
                            if (!allow)
                            {
                                return (int)HResult.AccessDenied;
                            }
                        }

                        break;

                    case PRJ_NOTIFICATION_FILE_RENAMED:
                        if (inst.OnNotifyFileRenamed != null)
                        {
                            inst.OnNotifyFileRenamed(virtualPath, destPath, isDir, processId, processName, out NotificationType mask);
                            WriteNotificationMask(operationParameters, (uint)mask);
                        }

                        break;

                    case PRJ_NOTIFICATION_HARDLINK_CREATED:
                        inst.OnNotifyHardlinkCreated?.Invoke(virtualPath, destPath, processId, processName);
                        break;

                    case PRJ_NOTIFICATION_FILE_HANDLE_CLOSED_NO_MODIFICATION:
                        inst.OnNotifyFileHandleClosedNoModification?.Invoke(virtualPath, isDir, processId, processName);
                        break;

                    case PRJ_NOTIFICATION_FILE_HANDLE_CLOSED_FILE_MODIFIED:
                        inst.OnNotifyFileHandleClosedFileModifiedOrDeleted?.Invoke(
                            virtualPath, isDir, isFileModified: true, isFileDeleted: false, processId, processName);
                        break;

                    case PRJ_NOTIFICATION_FILE_HANDLE_CLOSED_FILE_DELETED:
                        {
                            bool isFileModified = false;
                            if (operationParameters != IntPtr.Zero)
                            {
                                // PRJ_NOTIFICATION_PARAMETERS.FileDeletedOnHandleClose.IsFileModified
                                isFileModified = Marshal.ReadByte(operationParameters) != 0;
                            }

                            inst.OnNotifyFileHandleClosedFileModifiedOrDeleted?.Invoke(
                                virtualPath, isDir, isFileModified, isFileDeleted: true, processId, processName);
                        }

                        break;

                    case PRJ_NOTIFICATION_FILE_PRE_CONVERT_TO_FULL:
                        if (inst.OnNotifyFilePreConvertToFull != null)
                        {
                            bool allow = inst.OnNotifyFilePreConvertToFull(virtualPath, processId, processName);
                            if (!allow)
                            {
                                return (int)HResult.AccessDenied;
                            }
                        }

                        break;
                }

                return (int)HResult.Ok;
            }
            catch
            {
                return (int)HResult.InternalError;
            }
        }

        private static unsafe void NativeCancelCommand(PRJ_CALLBACK_DATA* pData)
        {
            try
            {
                var inst = GetInstance(pData->InstanceContext);
                inst.OnCancelCommand?.Invoke(pData->CommandId);
            }
            catch
            {
                // CancelCommand is void; nothing to return.
            }
        }

        // ===========================================================
        // Helper methods
        // ===========================================================

        private static unsafe void CopyIdToVersionInfo(byte[] contentId, byte[] providerId, ref PRJ_PLACEHOLDER_VERSION_INFO versionInfo)
        {
            if (contentId != null)
            {
                int len = Math.Min(contentId.Length, PlaceholderIdLength);
                fixed (byte* pDst = versionInfo.ContentID, pSrc = contentId)
                {
                    Buffer.MemoryCopy(pSrc, pDst, PlaceholderIdLength, len);
                }
            }

            if (providerId != null)
            {
                int len = Math.Min(providerId.Length, PlaceholderIdLength);
                fixed (byte* pDst = versionInfo.ProviderID, pSrc = providerId)
                {
                    Buffer.MemoryCopy(pSrc, pDst, PlaceholderIdLength, len);
                }
            }
        }

        private static void WriteNotificationMask(IntPtr operationParameters, uint mask)
        {
            // PRJ_NOTIFICATION_PARAMETERS is a union; for post-create/overwrite/rename,
            // the first field is NotificationMask (PRJ_NOTIFY_TYPES, 4 bytes)
            if (operationParameters != IntPtr.Zero)
            {
                Marshal.WriteInt32(operationParameters, unchecked((int)mask));
            }
        }
    }
}
