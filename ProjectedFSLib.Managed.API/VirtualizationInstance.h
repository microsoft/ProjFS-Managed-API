// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "IVirtualizationInstance.h"
#include "ApiHelper.h"

namespace Microsoft {
namespace Windows {
namespace ProjFS {
    /// <summary>
    /// Provides methods and callbacks that allow a provider to interact with a virtualization instance.
    /// </summary>
    /// <remarks>
    /// <para>
    /// The provider creates one instance of this class for each virtualization root that it manages.
    /// The provider uses this class's properties and methods to receive and respond to callbacks from
    /// ProjFS for its virtualization instance, and to send commands that control the virtualization
    /// instance's state.
    /// </para>
    /// </remarks>
    public ref class VirtualizationInstance : public IVirtualizationInstance
    {
    public:

        /// <summary>
        /// Initializes an object that manages communication between a provider and ProjFS.
        /// </summary>
        /// <remarks>
        ///     <para>
        ///     If <paramref name="virtualizationRootPath"/> doesn't already exist, this constructor
        ///     will create it and mark it as the virtualization root.  The constructor will generate
        ///     a GUID to serve as the virtualization instance ID.
        ///     </para>
        ///     <para>
        ///     If <paramref name="virtualizationRootPath"/> does exist, this constructor will check
        ///     for a ProjFS reparse point.  If the reparse point does not exist, <c>virtualizationRootPath</c>
        ///     will be marked as the virtualization root.  If it has a different reparse point then
        ///     the constructor will throw a <see cref="System::ComponentModel::Win32Exception"/> for
        ///     ERROR_REPARSE_TAG_MISMATCH.
        ///     </para>
        ///     <para>
        ///     For providers that create their virtualization root separately from instantiating the
        ///     <c>VirtualizationInstance</c> class, the static method
        ///     <see cref="MarkDirectoryAsVirtualizationRoot"/> is provided.
        ///     </para>
        /// </remarks>
        /// <param name="virtualizationRootPath">
        ///     The full path to the virtualization root directory.  If this directory does not already
        ///     exist, it will be created.  See the Remarks section for further details.
        /// </param>
        /// <param name="poolThreadCount">
        ///     <para>
        ///     The number of threads the provider will have available to process callbacks from ProjFS.
        ///     </para>
        ///     <para>
        ///     If the provider specifies 0, ProjFS will use a default value of 2 * <paramref name="concurrentThreadCount"/>.
        ///     </para>
        /// </param>
        /// <param name="concurrentThreadCount">
        ///     <para>
        ///     The maximum number of threads the provider wants to run concurrently to process callbacks
        ///     from ProjFS.
        ///     </para>
        ///     <para>
        ///     If the provider specifies 0, ProjFS will use a default value equal to the number of
        ///     CPU cores in the system.
        ///     </para>
        /// </param>
        /// <param name="enableNegativePathCache">
        ///     <para>
        ///     If <c>true</c>, specifies that the virtualization instance should maintain a "negative
        ///     path cache".  If the negative path cache is active, then if the provider indicates
        ///     that a file path does not exist by returning <see cref="HResult::FileNotFound"/> from its
        ///     implementation of <see cref="IRequiredCallbacks::GetPlaceholderInfoCallback"/>, then ProjFS will
        ///     fail subsequent opens of that path without calling <see cref="IRequiredCallbacks::GetPlaceholderInfoCallback"/>
        ///     again.
        ///     </para>
        ///     <para>
        ///     To resume receiving <see cref="IRequiredCallbacks::GetPlaceholderInfoCallback"/> for paths the provider has
        ///     indicated do not exist, the provider must call <see cref="ClearNegativePathCache"/>.
        ///     </para>
        /// </param>
        /// <param name="notificationMappings">
        ///     <para>
        ///     A collection of zero or more <see cref="NotificationMapping"/> objects that describe the
        ///     notifications the provider wishes to receive for the virtualization root.
        ///     </para>
        ///     <para>
        ///     If the collection is empty, ProjFS will send the notifications <see cref="NotificationType::FileOpened"/>,
        ///     <see cref="NotificationType::NewFileCreated"/>, and <see cref="NotificationType::FileOverwritten"/>
        ///     for all files and directories under the virtualization root.
        ///     </para>
        /// </param>
        /// <exception cref="System::IO::FileLoadException">
        /// The native ProjFS library (ProjectedFSLib.dll) is not available.
        /// </exception>
        /// <exception cref="System::EntryPointNotFoundException">
        /// An expected entry point cannot be found in ProjectedFSLib.dll.
        /// </exception>
        /// <exception cref="System::ComponentModel::Win32Exception">
        /// An error occurred in setting up the virtualization root.
        /// </exception>
        VirtualizationInstance(
            System::String^ virtualizationRootPath,
            unsigned int poolThreadCount,
            unsigned int concurrentThreadCount,
            bool enableNegativePathCache,
            System::Collections::Generic::IReadOnlyCollection<NotificationMapping^>^ notificationMappings
        );

#pragma region Callback properties

        /// <summary>
        /// Stores the provider's implementation of <see cref="QueryFileNameCallback"/>.
        /// </summary>
        /// <seealso cref="QueryFileNameCallback"/>
        /// <remarks>The provider must set this property prior to calling <see cref="StartVirtualizing"/>.</remarks>
        virtual property QueryFileNameCallback^ OnQueryFileName
        {
            QueryFileNameCallback^ get(void) sealed;

            /// <exception cref="System::InvalidOperationException">
            /// The provider has already called <see cref="StartVirtualizing"/>.
            /// </exception>
            void set(QueryFileNameCallback^ callbackDelegate) sealed;
        };

        /// <summary>Stores the provider's implementation of <see cref="CancelCommandCallback"/>.</summary>
        /// <seealso cref="CancelCommandCallback"/>
        /// <remarks>
        /// <para>
        /// If the provider wishes to support asynchronous processing of callbacks (that is, if it
        /// intends to return <see cref="HResult::Pending"/> from any of its callbacks), then the provider
        /// must set this property prior to calling <see cref="StartVirtualizing"/>.
        /// </para>
        /// <para>
        /// If the provider does not wish to support asynchronous processing of callbacks, then it
        /// is not required to provide an implementation of this callback.
        /// </para>
        /// <para>If the provider does implement this callback, then it must set this property prior to
        /// calling <see cref="StartVirtualizing"/>.</para>
        /// </remarks>
        virtual property CancelCommandCallback^ OnCancelCommand
        {
            CancelCommandCallback^ get(void) sealed;

            /// <exception cref="System::InvalidOperationException">
            /// The provider has already called <see cref="StartVirtualizing"/>.
            /// </exception>
            void set(CancelCommandCallback^ callbackDelegate) sealed;
        };
        
        /// <summary>Stores the provider's implementation of <see cref="NotifyFileOpenedCallback"/>.</summary>
        /// <seealso cref="NotifyFileOpenedCallback"/>
        /// <remarks>
        /// <para>The provider is not required to provide an implementation of this callback.
        /// If it does not provide this callback, the provider will not receive notifications when
        /// a file has been opened.</para>
        /// <para>If the provider does implement this callback, then it must set this property prior to
        /// calling <see cref="StartVirtualizing"/>.</para>
        /// </remarks>
        virtual property NotifyFileOpenedCallback^ OnNotifyFileOpened
        {
            NotifyFileOpenedCallback^ get(void) sealed;

            /// <exception cref="System::InvalidOperationException">
            /// The provider has already called <see cref="StartVirtualizing"/>.
            /// </exception>
            void set(NotifyFileOpenedCallback^ callbackDelegate) sealed;
        };

        /// <summary>Stores the provider's implementation of <see cref="NotifyNewFileCreatedCallback"/>.</summary>
        /// <seealso cref="NotifyNewFileCreatedCallback"/>
        /// <remarks>
        /// <para>The provider is not required to provide an implementation of this callback.
        /// If it does not provide this callback, the provider will not receive notifications when
        /// a new file has been created.</para>
        /// <para>If the provider does implement this callback, then it must set this property prior to
        /// calling <see cref="StartVirtualizing"/>.</para>
        /// </remarks>
        virtual property NotifyNewFileCreatedCallback^ OnNotifyNewFileCreated
        {
            NotifyNewFileCreatedCallback^ get(void) sealed;

            /// <exception cref="System::InvalidOperationException">
            /// The provider has already called <see cref="StartVirtualizing"/>.
            /// </exception>
            void set(NotifyNewFileCreatedCallback^ callbackDelegate) sealed;
        };

        /// <summary>Stores the provider's implementation of <see cref="NotifyFileOverwrittenCallback"/>.</summary>
        /// <seealso cref="NotifyFileOverwrittenCallback"/>
        /// <remarks>
        /// <para>The provider is not required to provide an implementation of this callback.
        /// If it does not provide this callback, the provider will not receive notifications when
        /// a file has been superseded or overwritten.</para>
        /// <para>If the provider does implement this callback, then it must set this property prior to
        /// calling <see cref="StartVirtualizing"/>.</para>
        /// </remarks>
        virtual property NotifyFileOverwrittenCallback^ OnNotifyFileOverwritten
        {
            NotifyFileOverwrittenCallback^ get(void) sealed;

            /// <exception cref="System::InvalidOperationException">
            /// The provider has already called <see cref="StartVirtualizing"/>.
            /// </exception>
            void set(NotifyFileOverwrittenCallback^ callbackDelegate) sealed;
        };

        /// <summary>Stores the provider's implementation of <see cref="NotifyPreDeleteCallback"/>.</summary>
        /// <seealso cref="NotifyPreDeleteCallback"/>
        /// <remarks>
        /// <para>The provider is not required to provide an implementation of this callback.
        /// If it does not provide this callback, the provider will not receive notifications when
        /// a file is about to be deleted.</para>
        /// <para>If the provider does implement this callback, then it must set this property prior to
        /// calling <see cref="StartVirtualizing"/>.</para>
        /// </remarks>
        virtual property NotifyPreDeleteCallback^ OnNotifyPreDelete
        {
            NotifyPreDeleteCallback^ get(void) sealed;

            /// <exception cref="System::InvalidOperationException">
            /// The provider has already called <see cref="StartVirtualizing"/>.
            /// </exception>
            void set(NotifyPreDeleteCallback^ callbackDelegate) sealed;
        };

        /// <summary>Stores the provider's implementation of <see cref="NotifyPreRenameCallback"/>.</summary>
        /// <seealso cref="NotifyPreRenameCallback"/>
        /// <remarks>
        /// <para>The provider is not required to provide an implementation of this callback.
        /// If it does not provide this callback, the provider will not receive notifications when
        /// a file is about to be renamed.</para>
        /// <para>If the provider does implement this callback, then it must set this property prior to
        /// calling <see cref="StartVirtualizing"/>.</para>
        /// </remarks>
        virtual property NotifyPreRenameCallback^ OnNotifyPreRename
        {
            NotifyPreRenameCallback^ get(void) sealed;

            /// <exception cref="System::InvalidOperationException">
            /// The provider has already called <see cref="StartVirtualizing"/>.
            /// </exception>
            void set(NotifyPreRenameCallback^ callbackDelegate) sealed;
        };

        /// <summary>Stores the provider's implementation of <see cref="NotifyPreCreateHardlinkCallback"/>.</summary>
        /// <seealso cref="NotifyPreCreateHardlinkCallback"/>
        /// <remarks>
        /// <para>The provider is not required to provide an implementation of this callback.
        /// If it does not provide this callback, the provider will not receive notifications when
        /// a hard link is about to be created for a file.</para>
        /// <para>If the provider does implement this callback, then it must set this property prior to
        /// calling <see cref="StartVirtualizing"/>.</para>
        /// </remarks>
        virtual property NotifyPreCreateHardlinkCallback^ OnNotifyPreCreateHardlink
        {
            NotifyPreCreateHardlinkCallback^ get(void) sealed;

            /// <exception cref="System::InvalidOperationException">
            /// The provider has already called <see cref="StartVirtualizing"/>.
            /// </exception>
            void set(NotifyPreCreateHardlinkCallback^ callbackDelegate) sealed;
        };

        /// <summary>Stores the provider's implementation of <see cref="NotifyFileRenamedCallback"/>.</summary>
        /// <seealso cref="NotifyFileRenamedCallback"/>
        /// <remarks>
        /// <para>The provider is not required to provide an implementation of this callback.
        /// If it does not provide this callback, the provider will not receive notifications when
        /// a file has been renamed.</para>
        /// <para>If the provider does implement this callback, then it must set this property prior to
        /// calling <see cref="StartVirtualizing"/>.</para>
        /// </remarks>
        virtual property NotifyFileRenamedCallback^ OnNotifyFileRenamed
        {
            NotifyFileRenamedCallback^ get(void) sealed;

            /// <exception cref="System::InvalidOperationException">
            /// The provider has already called <see cref="StartVirtualizing"/>.
            /// </exception>
            void set(NotifyFileRenamedCallback^ callbackDelegate) sealed;
        };

        /// <summary>Stores the provider's implementation of <see cref="NotifyHardlinkCreatedCallback"/>.</summary>
        /// <seealso cref="NotifyHardlinkCreatedCallback"/>
        /// <remarks>
        /// <para>The provider is not required to provide an implementation of this callback.
        /// If it does not provide this callback, the provider will not receive notifications when
        /// a hard link has been created for a file.</para>
        /// <para>If the provider does implement this callback, then it must set this property prior to
        /// calling <see cref="StartVirtualizing"/>.</para>
        /// </remarks>
        virtual property NotifyHardlinkCreatedCallback^ OnNotifyHardlinkCreated
        {
            NotifyHardlinkCreatedCallback^ get(void) sealed;

            /// <exception cref="System::InvalidOperationException">
            /// The provider has already called <see cref="StartVirtualizing"/>.
            /// </exception>
            void set(NotifyHardlinkCreatedCallback^ callbackDelegate) sealed;
        };

        /// <summary>Stores the provider's implementation of <see cref="NotifyFileHandleClosedNoModificationCallback"/>.</summary>
        /// <seealso cref="NotifyFileHandleClosedNoModificationCallback"/>
        /// <remarks>
        /// <para>The provider is not required to provide an implementation of this callback.
        /// If it does not provide this callback, the provider will not receive notifications when
        /// a file handle has been closed and the file has not been modified.</para>
        /// <para>If the provider does implement this callback, then it must set this property prior to
        /// calling <see cref="StartVirtualizing"/>.</para>
        /// </remarks>
        virtual property NotifyFileHandleClosedNoModificationCallback^ OnNotifyFileHandleClosedNoModification
        {
            NotifyFileHandleClosedNoModificationCallback^ get(void) sealed;

            /// <exception cref="System::InvalidOperationException">
            /// The provider has already called <see cref="StartVirtualizing"/>.
            /// </exception>
            void set(NotifyFileHandleClosedNoModificationCallback^ callbackDelegate) sealed;
        };

        /// <summary>Stores the provider's implementation of <see cref="NotifyFileHandleClosedFileModifiedOrDeletedCallback"/>.</summary>
        /// <seealso cref="NotifyFileHandleClosedFileModifiedOrDeletedCallback"/>
        /// <remarks>
        /// <para>The provider is not required to provide an implementation of this callback.
        /// If it does not provide this callback, the provider will not receive notifications when
        /// a file handle has been closed on a modified file, or the file was deleted as a result
        /// of closing the handle.</para>
        /// <para>If the provider does implement this callback, then it must set this property prior to
        /// calling <see cref="StartVirtualizing"/>.</para>
        /// </remarks>
        virtual property NotifyFileHandleClosedFileModifiedOrDeletedCallback^ OnNotifyFileHandleClosedFileModifiedOrDeleted
        {
            NotifyFileHandleClosedFileModifiedOrDeletedCallback^ get(void) sealed;

            /// <exception cref="System::InvalidOperationException">
            /// The provider has already called <see cref="StartVirtualizing"/>.
            /// </exception>
            void set(NotifyFileHandleClosedFileModifiedOrDeletedCallback^ callbackDelegate) sealed;
        };

        /// <summary>Stores the provider's implementation of <see cref="NotifyFilePreConvertToFullCallback"/>.</summary>
        /// <seealso cref="NotifyFilePreConvertToFullCallback"/>
        /// <remarks>
        /// <para>The provider is not required to provide an implementation of this callback.
        /// If it does not provide this callback, the provider will not receive notifications when
        /// a file is about to be converted from a placeholder to a full file. </para>
        /// <para>If the provider does implement this callback, then it must set this property prior to
        /// calling <see cref="StartVirtualizing"/>.</para>
        /// </remarks>
        virtual property NotifyFilePreConvertToFullCallback^ OnNotifyFilePreConvertToFull
        {
            NotifyFilePreConvertToFullCallback^ get(void) sealed;

            /// <exception cref="System::InvalidOperationException">
            /// The provider has already called <see cref="StartVirtualizing"/>.
            /// </exception>
            void set(NotifyFilePreConvertToFullCallback^ callbackDelegate) sealed;
        };

        /// <summary>Retrieves the <see cref="IRequiredCallbacks"/> interface.</summary>
        virtual property IRequiredCallbacks^ RequiredCallbacks
        {
            IRequiredCallbacks^ get(void) sealed;
        };

#pragma endregion

#pragma region Other properties

        /// <summary>Allows the provider to retrieve the virtualization instance GUID.</summary>
        /// <remarks>
        /// A virtualization instance is identified with a GUID.  If the provider did not generate
        /// and store a GUID itself using the <see cref="MarkDirectoryAsVirtualizationRoot"/> method,
        /// then the VirtualizationInstance class generates one for it.  Either way, the provider
        /// can retrieve the GUID via this property.
        /// </remarks>
        virtual property System::Guid VirtualizationInstanceId
        {
            System::Guid get(void) sealed;
        };

        /// <summary>Returns the maximum allowed length of a placeholder's contentID or provider ID.</summary>
        /// <remarks>
        /// See <see cref="WritePlaceholderInfo"/> or <see cref="UpdateFileIfNeeded"/> for more information.
        /// </remarks>
        virtual property int PlaceholderIdLength
        {
            int get(void) sealed;
        };

#pragma endregion

#pragma region Public methods

        /// <summary>
        /// Starts the virtualization instance, making it available to service I/O and invoke callbacks
        /// on the provider.
        /// </summary>
        /// <remarks>
        ///     <para>
        ///     If the provider has implemented any optional callback delegates, it must set their
        ///     implementations into the <c>On...</c> properties prior to calling this method.
        ///     </para>
        ///     <para>
        ///     On Windows 10 version 1803 this method attempts to determine the sector alignment
        ///     requirements of the underlying storage device and stores that information internally
        ///     in the <see cref="VirtualizationInstance"/> instance.  This information is required
        ///     by the <see cref="CreateWriteBuffer(unsigned int)"/> method to ensure that it can return data in
        ///     the <see cref="WriteFileData"/> method when the original reader is using unbuffered
        ///     I/O.  If this method cannot determine the sector alignment requirements of the
        ///     underlying storage device, it will throw a <see cref="System::IO::IOException"/>
        ///     exception.
        ///     </para>
        ///     <para>
        ///     On Windows 10 version 1809 and later versions the alignment requirements are determined
        ///     by the system.
        ///     </para>
        /// </remarks>
        /// <param name="requiredCallbacks">
        ///     <para>
        ///     The provider's implementation of the <see cref="IRequiredCallbacks"/> interface.
        ///     </para>
        /// </param>
        /// <returns>
        ///     <para><see cref="HResult::Ok"/> if the virtualization instance started successfully.</para>
        ///     <para><see cref="HResult::OutOfMemory"/> if a buffer could not be allocated to communicate with ProjFS.</para>
        ///     <para><see cref="HResult::VirtualizationInvalidOp"/> if the virtualization root is an ancestor or descendant of an existing virtualization root.</para>
        ///     <para><see cref="HResult::AlreadyInitialized"/> if the virtualization instance is already running.</para>
        /// </returns>
        /// <exception cref="System::IO::IOException">
        /// The sector alignment requirements of the volume could not be determined.  See the Remarks section.
        /// </exception> 
        virtual HResult StartVirtualizing(
            IRequiredCallbacks^ requiredCallbacks) sealed;

        /// <summary>
        /// Stops the virtualization instance, making it unavailable to service I/O or invoke callbacks
        /// on the provider.
        /// </summary>
        /// <exception cref="System::InvalidOperationException">
        /// The virtualization instance is in an invalid state (it may already be stopped).
        /// </exception>
        virtual void StopVirtualizing() sealed;

        /// <summary>
        /// Purges the virtualization instance's negative path cache, if it is active.
        /// </summary>
        /// <remarks>
        ///     <para>
        ///     If the negative path cache is active, then if the provider indicates that a file path does
        ///     not exist by returning <see cref="HResult::FileNotFound"/> from its implementation of
        ///     <see cref="IRequiredCallbacks::GetPlaceholderInfoCallback"/>
        ///     then ProjFS will fail subsequent opens of that path without calling
        ///     <see cref="IRequiredCallbacks::GetPlaceholderInfoCallback"/> again.
        ///     </para>
        ///     <para>
        ///     To resume receiving <see cref="IRequiredCallbacks::GetPlaceholderInfoCallback"/> for
        ///     paths the provider has indicated do not exist, the provider must call this method.
        ///     </para>
        /// </remarks>
        /// <param name="totalEntryNumber">
        /// Returns the number of paths that were in the cache before it was purged.
        /// </param>
        /// <returns>
        ///     <para><see cref="HResult::Ok"/> if the the cache was successfully purged.</para>
        ///     <para><see cref="HResult::OutOfMemory"/> if a buffer could not be allocated to communicate with ProjFS.</para>
        /// </returns>
        virtual HResult ClearNegativePathCache(
            [Out] unsigned int% totalEntryNumber) sealed;

        /// <summary>
        /// Sends file contents to ProjFS.
        /// </summary>
        /// <remarks>
        ///     <para>
        ///     The provider uses this method to provide the data requested when ProjFS calls the provider's
        ///     implementation of <see cref="IRequiredCallbacks::GetFileDataCallback"/>.
        ///     </para>
        ///     <para>
        ///     The provider calls <see cref="CreateWriteBuffer(unsigned int)"/> to create an instance of <see cref="WriteBuffer"/>
        ///     to contain the data to be written.  The <see cref="WriteBuffer"/> ensures that any alignment
        ///     requirements of the underlying storage device are met.
        ///     </para>
        /// </remarks>
        /// <param name="dataStreamId">
        /// Identifier for the data stream to write to.  The provider must use the value of
        /// <paramref name="dataStreamId"/> passed in its <see cref="IRequiredCallbacks::GetFileDataCallback"/>
        /// callback.
        /// </param>
        /// <param name="buffer">
        /// A <see cref="WriteBuffer"/> created using <see cref="CreateWriteBuffer(unsigned int)"/> that contains
        /// the data to write.
        /// </param>
        /// <param name="byteOffset">
        /// Byte offset from the beginning of the file at which to write the data.
        /// </param>
        /// <param name="length">
        /// The number of bytes to write to the file.
        /// </param>
        /// <returns>
        ///     <para><see cref="HResult::Ok"/> if the data was successfully written.</para>
        ///     <para><see cref="HResult::OutOfMemory"/> if a buffer could not be allocated to communicate with ProjFS.</para>
        ///     <para><see cref="HResult::InvalidArg"/> if <paramref name="buffer"/> is not specified,
        ///     <paramref name="length"/> is 0, or <paramref name="byteOffset"/> is greater than the
        ///     length of the file.</para>
        ///     <para><see cref="HResult::Handle"/> if <paramref name="dataStreamId"/> does not
        ///     correspond to a placeholder that is expecting data to be provided.</para>
        /// </returns>
        /// <seealso cref="WriteBuffer"/>
        virtual HResult WriteFileData(
            System::Guid dataStreamId,
            IWriteBuffer^ buffer,
            unsigned long long byteOffset,
            unsigned long length) sealed;

        /// <summary>
        /// Enables a provider to delete a file or directory that has been cached on the local file system.
        /// </summary>
        /// <remarks>
        ///     <para>
        ///     If the item is still in the provider's store, deleting it from the local file system changes
        ///     it to a virtual item.
        ///     </para>
        ///     <para>
        ///     This routine will fail if called on a file or directory that is already virtual.
        ///     </para>
        ///     <para>
        ///     If the file or directory to be deleted is in any state other than "placeholder", the
        ///     provider must specify an appropriate combination of <see cref="UpdateType"/> values
        ///     in the <paramref name="updateFlags"/> parameter.  This helps guard against accidental
        ///     loss of data. If the provider did not specify a combination of <see cref="UpdateType"/>
        ///     values in the <paramref name="updateFlags"/> parameter that would allow the delete
        ///     to happen, the method fails with <see cref="HResult::VirtualizationInvalidOp"/>.
        ///     </para>
        ///     <para>
        ///     If a directory contains only tombstones, it may be deleted using this method and
        ///     specifying <see cref="UpdateType::AllowTombstone"/> in <paramref name="updateFlags"/>.
        ///     If the directory contains non-tombstone files, then this method will return <see cref="HResult::DirNotEmpty"/>.
        ///     </para>
        /// </remarks>
        /// <param name="relativePath">
        /// The path, relative to the virtualization root, to the file or directory to delete.
        /// </param>
        /// <param name="updateFlags">
        ///     <para>
        ///     A combination of 0 or more <see cref="UpdateType"/> values to control whether ProjFS
        ///     should allow the delete given the state of the file or directory on disk.  See the documentation
        ///     of <see cref="UpdateType"/> for a description of each flag and what it will allow.
        ///     </para>
        ///     <para>
        ///     If the item is a dirty placeholder, full file, or tombstone, and the provider does not
        ///     specify the appropriate flag(s), this routine will fail to delete the placeholder.
        ///     </para>
        /// </param>
        /// <param name="failureReason">
        /// If this method fails with <see cref="HResult::VirtualizationInvalidOp"/>, this receives a
        /// <see cref="UpdateFailureCause"/> value that describes the reason the delete failed.
        /// </param>
        /// <returns>
        ///     <para><see cref="HResult::Ok"/> if the delete succeeded.</para>
        ///     <para><see cref="HResult::OutOfMemory"/> if a buffer could not be allocated to communicate with ProjFS.</para>
        ///     <para><see cref="HResult::InvalidArg"/> if <paramref name="relativePath"/> is an empty string.</para>
        ///     <para><see cref="HResult::FileNotFound"/> if <paramref name="relativePath"/> cannot
        ///     be found.  It may be for a virtual file or directory.</para>
        ///     <para><see cref="HResult::PathNotFound"/> if <paramref name="relativePath"/> contains
        ///     an intermediate component that cannot be found.  The path may terminate beneath a
        ///     directory that has been replaced with a tombstone.</para>
        ///     <para><see cref="HResult::DirNotEmpty"/> if <paramref name="relativePath"/> is a
        ///     directory and is not empty.</para>
        ///     <para><see cref="HResult::VirtualizationInvalidOp"/> if the input value of <paramref name="updateFlags"/>
        ///     does not allow the delete given the state of the file or directory on disk.  The value
        ///     of <paramref name="failureReason"/> indicates the cause of the failure.</para>
        /// </returns>
        virtual HResult DeleteFile(
             System::String^ relativePath, 
             UpdateType updateFlags, 
             [Out] UpdateFailureCause% failureReason) sealed;
         
        /// <summary>
        /// Sends file or directory metadata to ProjFS.
        /// </summary>
        /// <remarks>
        ///     <para>
        ///     The provider uses this method to create a placeholder on disk.  It does this when ProjFS
        ///     calls the provider's implementation of <see cref="IRequiredCallbacks::GetPlaceholderInfoCallback"/>,
        ///     or the provider may use this method to proactively lay down a placeholder.
        ///     </para>
        ///     <para>
        ///     Note that the timestamps the provider specifies in the <paramref name="creationTime"/>,
        ///     <paramref name="lastAccessTime"/>, <paramref name="lastWriteTime"/>, and <paramref name="changeTime"/>
        ///     parameters may be any values the provider wishes.  This allows the provider to preserve
        ///     the illusion of files and directories that already exist on the user's system even before they
        ///     are physically created on the user's disk.
        ///     </para>
        /// </remarks>
        /// <param name="relativePath">
        ///     <para>
        ///     The path, relative to the virtualization root, of the file or directory.
        ///     </para>
        ///     <para>
        ///     If the provider is processing a call to <see cref="IRequiredCallbacks::GetPlaceholderInfoCallback"/>,
        ///     then this must be a match to the <c>relativePath</c> value passed in that call.  The
        ///     provider should use the <see cref="Utils::FileNameCompare"/> method to determine whether
        ///     the two names match.
        ///     </para>
        ///     <para>
        ///     For example, if <see cref="IRequiredCallbacks::GetPlaceholderInfoCallback"/> specifies
        ///     <c>dir1\dir1\FILE.TXT</c> in <c>relativePath</c>, and the provider’s backing store contains
        ///     a file called <c>File.txt</c> in the <c>dir1\dir2</c> directory, and <see cref="Utils::FileNameCompare"/>
        ///     returns 0 when comparing the names <c>FILE.TXT</c> and <c>File.txt</c>, then the provider
        ///     specifies <c>dir1\dir2\File.txt</c> as the value of this parameter.
        ///     </para>
        /// </param>
        /// <param name="creationTime">
        /// The time the file was created.
        /// </param>
        /// <param name="lastAccessTime">
        /// The time the file was last accessed.
        /// </param>
        /// <param name="lastWriteTime">
        /// The time the file was last written to.
        /// </param>
        /// <param name="changeTime">
        /// The time the file was last changed.
        /// </param>
        /// <param name="fileAttributes">
        /// The file attributes.
        /// </param>
        /// <param name="endOfFile">
        /// The size of the file in bytes.
        /// </param>
        /// <param name="isDirectory">
        /// <c>true</c> if <paramref name="relativePath"/> is for a directory, <c>false</c> otherwise.
        /// </param>
        /// <param name="contentId">
        ///     <para>
        ///     A content identifier, generated by the provider.  ProjFS will pass this value back to the
        ///     provider when requesting file contents in the <see cref="IRequiredCallbacks::GetFileDataCallback"/> callback.
        ///     This allows the provider to distinguish between different versions of the same file, i.e.
        ///     different file contents and/or metadata for the same file path.
        ///     </para>
        ///     <para>
        ///     This value must be at most <see cref="PlaceholderIdLength"/> bytes in size.  Any data
        ///     beyond that length will be discarded.
        ///     </para>
        /// </param>
        /// <param name="providerId">
        ///     <para>
        ///     Optional provider-specific data.  ProjFS will pass this value back to the provider
        ///     when requesting file contents in the <see cref="IRequiredCallbacks::GetFileDataCallback"/> callback.  The
        ///     provider may use this value as its own unique identifier, for example as a version number
        ///     for the format of the <paramref name="contentId"/> value.
        ///     </para>
        ///     <para>
        ///     This value must be at most <see cref="PlaceholderIdLength"/> bytes in size.  Any data
        ///     beyond that length will be discarded.
        ///     </para>
        /// </param>
        /// <returns>    
        ///     <para><see cref="HResult::Ok"/> if the placeholder information was successfully written.</para>
        ///     <para><see cref="HResult::OutOfMemory"/> if a buffer could not be allocated to communicate with ProjFS.</para>
        ///     <para><see cref="HResult::InvalidArg"/> if <paramref name="relativePath"/> is an empty string.</para>
        /// </returns>
        virtual HResult WritePlaceholderInfo(
            System::String^ relativePath,
            System::DateTime creationTime,
            System::DateTime lastAccessTime,
            System::DateTime lastWriteTime,
            System::DateTime changeTime,
            System::IO::FileAttributes fileAttributes,
            long long endOfFile,
            bool isDirectory,
            array<System::Byte>^ contentId,
            array<System::Byte>^ providerId) sealed;

        virtual HResult WritePlaceholderInfoSymlink(
            System::String^ relativePath,
            System::DateTime creationTime,
            System::DateTime lastAccessTime,
            System::DateTime lastWriteTime,
            System::DateTime changeTime,
            System::IO::FileAttributes fileAttributes,
            long long endOfFile,
            bool isDirectory,
            array<System::Byte>^ contentId,
            array<System::Byte>^ providerId,
            System::String^ targetName) sealed;

        /// <summary>
        /// Updates an item that has been cached on the local file system.
        /// </summary>
        /// <remarks>
        /// <para>
        /// This routine cannot be called on a virtual file or directory.
        /// </para>
        /// <para>
        /// If the file or directory to be updated is in any state other than "placeholder", the provider
        /// must specify an appropriate combination of <see cref="UpdateType"/> values in the
        /// <paramref name="updateFlags"/> parameter.  This helps guard against accidental loss of
        /// data, since upon successful return from this routine the item becomes a placeholder with
        /// the updated metadata.  Any metadata that had been changed since the placeholder was created,
        /// or any file data it contained is discarded.
        /// </para>
        /// <para>
        /// Note that the timestamps the provider specifies in the <paramref name="creationTime"/>,
        /// <paramref name="lastAccessTime"/>, <paramref name="lastWriteTime"/>, and <paramref name="changeTime"/>
        /// parameters may be any values the provider wishes.  This allows the provider to preserve
        /// the illusion of files and directories that already exist on the user's system even before they
        /// are physically created on the user's disk.
        /// </para>
        /// </remarks>
        /// <param name="relativePath">
        /// The path, relative to the virtualization root, to the file or directory to updated.
        /// </param>
        /// <param name="creationTime">
        /// The time the file was created.
        /// </param>
        /// <param name="lastAccessTime">
        /// The time the file was last accessed.
        /// </param>
        /// <param name="lastWriteTime">
        /// The time the file was last written to.
        /// </param>
        /// <param name="changeTime">
        /// The time the file was last changed.
        /// </param>
        /// <param name="fileAttributes">
        /// The file attributes.
        /// </param>
        /// <param name="endOfFile">
        /// The size of the file in bytes.
        /// </param>
        /// <param name="contentId">
        /// <para>
        /// A content identifier, generated by the provider.  ProjFS will pass this value back to the
        /// provider when requesting file contents in the <see cref="IRequiredCallbacks::GetFileDataCallback"/> callback.
        /// This allows the provider to distinguish between different versions of the same file, i.e.
        /// different file contents and/or metadata for the same file path.
        /// </para>
        /// <para>
        /// If this parameter specifies a content identifier that is the same as the content identifier
        /// already on the file or directory, the call succeeds and no update takes place.  Otherwise,
        /// if the call succeeds then the value of this parameter replaces the existing content identifier
        /// on the file or directory.
        /// </para>
        /// <para>
        /// This value must be at most <see cref="PlaceholderIdLength"/> bytes in size.  Any data
        /// beyond that length will be discarded.
        /// </para>
        /// </param>
        /// <param name="providerId">
        /// <para>
        /// Optional provider-specific data.  ProjFS will pass this value back to the provider
        /// when requesting file contents in the <see cref="IRequiredCallbacks::GetFileDataCallback"/> callback.  The
        /// provider may use this value as its own unique identifier, for example as a version number
        /// for the format of the <paramref name="contentId"/> value.
        /// </para>
        /// <para>
        /// This value must be at most <see cref="PlaceholderIdLength"/> bytes in size.  Any data
        /// beyond that length will be discarded.
        /// </para>
        /// </param>
        /// <param name="updateFlags">
        /// <para>
        /// A combination of 0 or more <see cref="UpdateType"/> values to control whether ProjFS
        /// should allow the update given the state of the file or directory on disk.  See the documentation
        /// of <see cref="UpdateType"/> for a description of each flag and what it will allow.
        /// </para>
        /// <para>
        /// If the item is a dirty placeholder, full file, or tombstone, and the provider does not
        /// specify the appropriate flag(s), this routine will fail to update the item.
        /// </para>
        /// </param>
        /// <param name="failureReason">
        /// If this method fails with <see cref="HResult::VirtualizationInvalidOp"/>, this receives a
        /// <see cref="UpdateFailureCause"/> value that describes the reason the update failed.
        /// </param>
        /// <returns>
        ///     <para><see cref="HResult::Ok"/> if the update succeeded.</para>
        ///     <para><see cref="HResult::OutOfMemory"/> if a buffer could not be allocated to communicate with ProjFS.</para>
        ///     <para><see cref="HResult::InvalidArg"/> if <paramref name="relativePath"/> is an empty string.</para>
        ///     <para><see cref="HResult::FileNotFound"/> if <paramref name="relativePath"/> cannot
        ///     be found.  It may be for a virtual file or directory.</para>
        ///     <para><see cref="HResult::PathNotFound"/> if <paramref name="relativePath"/> contains
        ///     an intermediate component that cannot be found.  The path may terminate beneath a
        ///     directory that has been replaced with a tombstone.</para>
        ///     <para><see cref="HResult::DirNotEmpty"/> if <paramref name="relativePath"/> is a
        ///     directory and is not empty.</para>
        ///     <para><see cref="HResult::VirtualizationInvalidOp"/> if the input value of <paramref name="updateFlags"/>
        ///     does not allow the update given the state of the file or directory on disk.  The value
        ///     of <paramref name="failureReason"/> indicates the cause of the failure.</para>
        /// </returns>
        virtual HResult UpdateFileIfNeeded(
            System::String^ relativePath,
            System::DateTime creationTime,
            System::DateTime lastAccessTime,
            System::DateTime lastWriteTime,
            System::DateTime changeTime,
            System::IO::FileAttributes fileAttributes,
            long long endOfFile,
            array<System::Byte>^ contentId,
            array<System::Byte>^ providerId,
            UpdateType updateFlags,
            [Out] UpdateFailureCause% failureReason) sealed;

        /// <summary>
        /// Signals to ProjFS that the provider has completed processing a callback from which it
        /// previously returned <see cref="HResult::Pending"/>.
        /// </summary>
        /// <remarks>
        /// If the provider calls this method for the <paramref name="commandId"/> passed by the
        /// <see cref="CancelCommandCallback"/> callback it is not an error, however it is a no-op
        /// because the I/O that caused the callback invocation identified by <paramref name="commandId"/>
        /// has already ended.
        /// </remarks>
        /// <param name="commandId">
        /// A value that uniquely identifies the callback invocation to complete.  This value must be
        /// equal to the value of the <paramref name="commandId"/> parameter of the callback from
        /// which the provider previously returned <see cref="HResult::Pending"/>.
        /// </param>
        /// <returns>
        ///     <para><see cref="HResult::Ok"/> if completion succeeded.</para>
        ///     <para><see cref="HResult::InvalidArg"/> if <paramref name="commandId"/> does not specify a pended callback.</para>
        /// </returns>
        virtual HResult CompleteCommand(
            int commandId) sealed;

        /// <summary>
        /// Signals to ProjFS that the provider has completed processing a callback from which it
        /// previously returned <see cref="HResult::Pending"/>.
        /// </summary>
        /// <remarks>
        /// If the provider calls this method for the <paramref name="commandId"/> passed by the
        /// <see cref="CancelCommandCallback"/> callback it is not an error, however it is a no-op
        /// because the I/O that caused the callback invocation identified by <paramref name="commandId"/>
        /// has already ended.
        /// </remarks>
        /// <param name="commandId">
        /// A value that uniquely identifies the callback invocation to complete.  This value must be
        /// equal to the value of the <paramref name="commandId"/> parameter of the callback from
        /// which the provider previously returned <see cref="HResult::Pending"/>.
        /// </param>
        /// <param name="completionResult">
        /// The final status of the operation.  See the descriptions for the callback delegates for
        /// appropriate return codes.
        /// </param>
        /// <returns>
        ///     <para><see cref="HResult::Ok"/> if completion succeeded.</para>
        ///     <para><see cref="HResult::InvalidArg"/> if <paramref name="commandId"/> does not specify a pended callback.</para>
        /// </returns>
        virtual HResult CompleteCommand(
            int commandId,
            HResult completionResult) sealed;

        /// <summary>
        /// Signals to ProjFS that the provider has completed processing a callback from which it
        /// previously returned <see cref="HResult::Pending"/>.
        /// </summary>
        /// <remarks>
        /// If the provider calls this method for the <paramref name="commandId"/> passed by the
        /// <see cref="CancelCommandCallback"/> callback it is not an error, however it is a no-op
        /// because the I/O that caused the callback invocation identified by <paramref name="commandId"/>
        /// has already ended.
        /// </remarks>
        /// <param name="commandId">
        /// A value that uniquely identifies the callback invocation to complete.  This value must be
        /// equal to the value of the <paramref name="commandId"/> parameter of the callback from
        /// which the provider previously returned <see cref="HResult::Pending"/>.
        /// </param>
        /// <param name="results">
        /// Used when completing <see cref="IRequiredCallbacks::GetDirectoryEnumerationCallback"/>.  Receives
        /// the results of the enumeration.
        /// </param>
        /// <returns>
        ///     <para><see cref="HResult::Ok"/> if completion succeeded.</para>
        ///     <para><see cref="HResult::InvalidArg"/> if <paramref name="commandId"/> does not specify a pended callback or
        ///     if <paramref name="results"/> is not a valid <see cref="IDirectoryEnumerationResults"/>.</para>
        /// </returns>
        virtual HResult CompleteCommand(
            int commandId,
            IDirectoryEnumerationResults^ results) sealed;

        /// <summary>
        /// Signals to ProjFS that the provider has completed processing a callback from which it
        /// previously returned <see cref="HResult::Pending"/>.
        /// </summary>
        /// <remarks>
        /// If the provider calls this method for the <paramref name="commandId"/> passed by the
        /// <see cref="CancelCommandCallback"/> callback it is not an error, however it is a no-op
        /// because the I/O that caused the callback invocation identified by <paramref name="commandId"/>
        /// has already ended.
        /// </remarks>
        /// <param name="commandId">
        /// A value that uniquely identifies the callback invocation to complete.  This value must be
        /// equal to the value of the <paramref name="commandId"/> parameter of the callback from
        /// which the provider previously returned <see cref="HResult::Pending"/>.
        /// </param>
        /// <param name="newNotificationMask">
        /// <para>
        /// Used when completing <c>Microsoft.Windows.ProjFS.Notify*</c> callbacks that have a
        /// <c>notificationMask</c> parameter.  Specifies a bitwise-OR of <see cref="NotificationType"/>
        /// values indicating the set of notifications the provider wishes to receive for this file.
        /// </para>
        /// <para>
        /// If the provider sets this value to 0, it is equivalent to specifying
        /// <see cref="NotificationType::UseExistingMask"/>.
        /// </para>
        /// </param>
        /// <returns>
        ///     <para><see cref="HResult::Ok"/> if completion succeeded.</para>
        ///     <para><see cref="HResult::InvalidArg"/> if <paramref name="commandId"/> does not specify a pended callback
        ///     or if <paramref name="newNotificationMask"/> is not a valid combination of <see cref="NotificationType"/>
        ///     values.</para>
        /// </returns>
        virtual HResult CompleteCommand(
            int commandId,
            NotificationType newNotificationMask) sealed;

        /// <summary>
        /// Creates a <see cref="WriteBuffer"/> for use with <see cref="WriteFileData"/>.
        /// </summary>
        /// <remarks>
        ///     <para>
        ///     The <see cref="WriteBuffer"/> object ensures that any alignment requirements of the
        ///     underlying storage device are met when writing data with the <see cref="WriteFileData"/>
        ///     method.
        ///     </para>
        ///     <para>
        ///     Note that unlike most methods on <see cref="VirtualizationInstance"/>, this method
        ///     throws rather than return <see cref="HResult"/>.  This makes it convenient to use
        ///     in constructions like a <c>using</c> clause.
        ///     </para>
        /// </remarks>
        /// <param name="desiredBufferSize">
        /// The size in bytes of the buffer required to write the data.
        /// </param>
        /// <returns>
        /// A <see cref="WriteBuffer"/> that provides at least <paramref name="desiredBufferSize"/>
        /// bytes of capacity.
        /// </returns>
        /// <exception cref="System::OutOfMemoryException">
        /// A buffer could not be allocated.
        /// </exception>
        virtual IWriteBuffer^ CreateWriteBuffer(
            unsigned int desiredBufferSize) sealed;

        /// <summary>
        /// Creates a <see cref="WriteBuffer"/> for use with <see cref="WriteFileData"/>.
        /// </summary>
        /// <remarks>
        ///     <para>
        ///     The <see cref="WriteBuffer"/> object ensures that any alignment requirements of the
        ///     underlying storage device are met when writing data with the <see cref="WriteFileData"/>
        ///     method.
        ///     </para>
        ///     <para>
        ///     This overload allows a provider to get sector-aligned values for the start offset and
        ///     length of the write.  The provider uses <paramref name="alignedByteOffset"/> and
        ///     <paramref name="alignedLength"/> to copy the correct data out of its backing store
        ///     into the <see cref="WriteBuffer"/> and transfer it when calling <see cref="WriteFileData"/>.
        ///     </para>
        ///     <para>
        ///     Note that unlike most methods on <see cref="VirtualizationInstance"/>, this method
        ///     throws rather than return <see cref="HResult"/>.  This makes it convenient to use
        ///     in constructions like a <c>using</c> clause.
        ///     </para>
        /// </remarks>
        /// <param name="byteOffset">
        /// Byte offset from the beginning of the file at which the provider wants to write data.
        /// </param>
        /// <param name="length">
        /// The number of bytes to write to the file.
        /// </param>
        /// <param name="alignedByteOffset">
        /// <paramref name="byteOffset"/>, aligned to the sector size of the storage device.  The
        /// provider uses this value as the <c>byteOffset</c> parameter to <see cref="WriteFileData"/>.
        /// </param>
        /// <param name="alignedLength">
        /// <paramref name="length"/>, aligned to the sector size of the storage device.  The
        /// provider uses this value as the <c>length</c> parameter to <see cref="WriteFileData"/>.
        /// </param>
        /// <returns>
        /// A <see cref="WriteBuffer"/> that provides the needed capacity.
        /// </returns>
        /// <exception cref="System::ComponentModel::Win32Exception">
        /// An error occurred retrieving the sector size from ProjFS.
        /// </exception>
        /// <exception cref="System::OutOfMemoryException">
        /// A buffer could not be allocated.
        /// </exception>
        virtual IWriteBuffer^ CreateWriteBuffer(
            unsigned long long byteOffset,
            unsigned int length,
            [Out] unsigned long long% alignedByteOffset,
            [Out] unsigned int% alignedLength) sealed;

        /// <summary>
        /// Converts an existing directory to a hydrated directory placeholder.
        /// </summary>
        /// <remarks>
        /// Children of the directory are not affected.
        /// </remarks>
        /// <param name="targetDirectoryPath">
        /// The full path (i.e. not relative to the virtualization root) to the directory to convert
        /// to a placeholder.
        /// </param>
        /// <param name="contentId">
        /// <para>
        /// A content identifier, generated by the provider. This value is used to distinguish between
        /// different versions of the same file, for example different file contents and/or metadata
        /// (e.g. timestamps) for the same file path.
        /// </para>
        /// <para>
        /// This value must be at most <see cref="PlaceholderIdLength"/> bytes in size.  Any data
        /// beyond that length will be discarded.
        /// </para>
        /// </param>
        /// <param name="providerId">
        /// <para>
        /// Optional provider-specific data.  The provider may use this value as its own unique identifier,
        /// for example as a version number for the format of the <paramref name="contentId"/> value.
        /// </para>
        /// <para>
        /// This value must be at most <see cref="PlaceholderIdLength"/> bytes in size.  Any data
        /// beyond that length will be discarded.
        /// </para>
        /// </param>
        /// <returns>
        /// <para><see cref="HResult::Ok"/> if the conversion succeeded.</para>
        /// <para><see cref="HResult::OutOfMemory"/> if a buffer could not be allocated to communicate with ProjFS.</para>
        /// <para><see cref="HResult::InvalidArg"/> if <paramref name="targetDirectoryPath"/> is an empty string
        /// or if it is not a directory path.</para>
        /// <para><see cref="HResult::ReparsePointEncountered"/> if <paramref name="targetDirectoryPath"/>
        /// is already a placeholder or some other kind of reparse point.</para>
        /// </returns>
        virtual HResult MarkDirectoryAsPlaceholder(
            System::String^ targetDirectoryPath,
            array<System::Byte>^ contentId,
            array<System::Byte>^ providerId) sealed;

        /// <summary>
        /// Marks an existing directory as the provider's virtualization root.
        /// </summary>
        /// <remarks>
        /// A provider may wish to designate its virtualization root before it is ready or able to
        /// instantiate the <see cref="VirtualizationInstance"/> class.  In that case it may use this
        /// method to designate the root.  The provider must generate a GUID to identify the virtualization
        /// instance and pass it in <paramref name="virtualizationInstanceGuid"/>.  The
        /// <see cref="VirtualizationInstance"/> constructor will use that
        /// value to identify the provider to ProjFS.
        /// </remarks>
        /// <param name="rootPath">
        /// The full path to the directory to mark as the virtualization root.
        /// </param>
        /// <param name="virtualizationInstanceGuid">
        /// A GUID generated by the provider.  ProjFS uses this value to internally identify the provider.
        /// </param>
        /// <returns>
        /// <para><see cref="HResult::Ok"/> if the conversion succeeded.</para>
        /// <para><see cref="HResult::InvalidArg"/> if <paramref name="rootPath"/> is an empty string.</para>
        /// <para><see cref="HResult::Directory"/> if <paramref name="rootPath"/> does not specify a directory.</para>
        /// <para><see cref="HResult::ReparsePointEncountered"/> if <paramref name="rootPath"/> is already a placeholder or some other kind of reparse point.</para>
        /// <para><see cref="HResult::VirtualizationInvalidOp"/> if <paramref name="rootPath"/> is an ancestor or descendant of an existing virtualization root.</para>
        /// </returns>
        static HResult MarkDirectoryAsVirtualizationRoot(
            System::String^ rootPath,
            System::Guid virtualizationInstanceGuid);

#pragma endregion

    private:

#pragma region Private methods

        void ConfirmStarted();
        void ConfirmNotStarted();
        void FindBytesPerSectorAndAlignment();

#pragma endregion

#pragma region Callback property storage

        IRequiredCallbacks^ m_requiredCallbacks;

        QueryFileNameCallback^ m_queryFileNameCallback;
        CancelCommandCallback^ m_cancelCommandCallback;

        NotifyFileOpenedCallback^ m_notifyFileOpenedCallback;
        NotifyNewFileCreatedCallback^ m_notifyNewFileCreatedCallback;
        NotifyFileOverwrittenCallback^ m_notifyFileOverwrittenCallback;
        NotifyPreDeleteCallback^ m_notifyPreDeleteCallback;
        NotifyPreRenameCallback^ m_notifyPreRenameCallback;
        NotifyPreCreateHardlinkCallback^ m_notifyPreCreateHardlinkCallback;
        NotifyFileRenamedCallback^ m_notifyFileRenamedCallback;
        NotifyHardlinkCreatedCallback^ m_notifyHardlinkCreatedCallback;
        NotifyFileHandleClosedNoModificationCallback^ m_notifyFileHandleClosedNoModificationCallback;
        NotifyFileHandleClosedFileModifiedOrDeletedCallback^ m_notifyFileHandleClosedFileModifiedOrDeletedCallback;
        NotifyFilePreConvertToFullCallback^ m_notifyFilePreConvertToFullCallback;

#pragma endregion

#pragma region Member data variables

        // Values provided by the constructor.
        System::String^ m_virtualizationRootPath;
        unsigned int m_poolThreadCount;
        unsigned int m_concurrentThreadCount;
        bool m_enableNegativePathCache;
        System::Collections::Generic::IReadOnlyCollection<NotificationMapping^>^ m_notificationMappings;

        // We keep a GC handle to the VirtualizationInstance object to pass through ProjFS as the
        // instance context.
        gcroot<VirtualizationInstance^>* m_virtualizationContextGc = nullptr;

        // Variables to support aligned I/O in Windows 10 version 1803.
        unsigned long m_bytesPerSector;
        unsigned long m_writeBufferAlignmentRequirement;

        PRJ_NAMESPACE_VIRTUALIZATION_CONTEXT m_virtualizationContext;
        System::Guid m_virtualizationInstanceID;
        ApiHelper^ m_apiHelper;

#pragma endregion
    };

#pragma region Support structures

    // Support for smart HANDLE
    struct HFileDeleter {
        typedef HANDLE pointer;

        void operator()(HANDLE h)
        {
            if (h != INVALID_HANDLE_VALUE)
            {
                ::CloseHandle(h);
            }
        }
    };

    // #include'ing the header that defines this is tricky, so we just have the definition here.
    typedef struct _REPARSE_DATA_BUFFER {
        ULONG  ReparseTag;
        USHORT ReparseDataLength;
        USHORT Reserved;

        _Field_size_bytes_(ReparseDataLength)
        union {
            struct {
                USHORT SubstituteNameOffset;
                USHORT SubstituteNameLength;
                USHORT PrintNameOffset;
                USHORT PrintNameLength;
                ULONG Flags;
                WCHAR PathBuffer[1];
            } SymbolicLinkReparseBuffer;
            struct {
                USHORT SubstituteNameOffset;
                USHORT SubstituteNameLength;
                USHORT PrintNameOffset;
                USHORT PrintNameLength;
                WCHAR PathBuffer[1];
            } MountPointReparseBuffer;
            struct {
                UCHAR  DataBuffer[1];
            } GenericReparseBuffer;
        } DUMMYUNIONNAME;
    } REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

#pragma endregion
}}} // namespace Microsoft.Windows.ProjFS