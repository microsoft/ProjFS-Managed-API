#pragma once

#include "HResult.h"
#include "DirectoryEnumerationResults.h"

namespace Microsoft {
namespace Windows {
namespace ProjFS {

/// <summary>
/// Defines callbacks that a provider is required to implement.
/// </summary>
/// <remarks>
/// <para>
/// A provider must implement the methods in this class to supply basic file system functionality.
/// The provider passes a reference to its implementation in the <c>Microsoft.Windows.ProjFS.StartVirtualizing</c>
/// method.
/// </para>
/// </remarks>
public interface class IRequiredCallbacks {
public:
    /// <summary>
    /// Informs the provider that a directory enumeration is starting.
    /// </summary>
    /// <seealso cref="GetDirectoryEnumerationCallback"/>
    /// <seealso cref="EndDirectoryEnumerationCallback"/>
    /// <remarks>
    ///     <para>
    ///     ProjFS requests a directory enumeration from the provider by first invoking this callback,
    ///     then the <see cref="GetDirectoryEnumerationCallback"/> callback one or more times, then
    ///     the <see cref="EndDirectoryEnumerationCallback"/> callback.  Because multiple enumerations
    ///     may occur in parallel in the same location, ProjFS uses the <paramref name="enumerationId"/>
    ///     argument to associate the callback invocations into a single enumeration, meaning that
    ///     a given set of calls to the enumeration callbacks will use the same value for
    ///     <paramref name="enumerationId"/> for the same session.
    ///     </para>
    /// </remarks>
    /// <param name="commandId">A value that uniquely identifies an invocation of the callback.
    ///     <para>If the provider returns <see cref="HResult::Pending"/> from this method, then it must pass
    ///     this value to <c>ProjFS.VirtualizationInstance.CompleteCommand</c> to signal that it has
    ///     finished processing this invocation of the callback.</para>
    /// </param>
    /// <param name="enumerationId">Identifies this enumeration session.</param>
    /// <param name="relativePath">Identifies the directory to be enumerated.  The path is specified
    ///     relative to the virtualization root.
    /// </param>
    /// <param name="triggeringProcessId">The PID of the process that triggered this callback. If this
    ///     information is not available, this will be 0.</param>
    /// <param name="triggeringProcessImageFileName">The image file name corresponding to <paramref name="triggeringProcessId"/>.
    ///     If <paramref name="triggeringProcessId"/> is 0 this will be null.</param>
    /// <returns>
    ///     <para><see cref="HResult::Ok"/> if the provider successfully completes the operation.</para>
    ///     <para><see cref="HResult::Pending"/> if the provider wishes to complete the operation at a later time.</para>
    ///     <para>An appropriate error code if the provider fails the operation.</para>
    /// </returns>
    HResult StartDirectoryEnumerationCallback(
        int commandId,
        System::Guid enumerationId,
        System::String^ relativePath,
        unsigned int triggeringProcessId,
        System::String^ triggeringProcessImageFileName
    );

    /// <summary>
    /// Requests directory enumeration information from the provider.
    /// </summary>
    /// <seealso cref="StartDirectoryEnumerationCallback"/>
    /// <seealso cref="EndDirectoryEnumerationCallback"/>
    /// <remarks>
    ///     <para>
    ///     ProjFS requests a directory enumeration from the provider by first invoking
    ///     <see cref="StartDirectoryEnumerationCallback"/>, then this callback one or more times,
    ///     then the <see cref="EndDirectoryEnumerationCallback"/> callback.  Because multiple
    ///     enumerations may occur in parallel in the same location, ProjFS uses the
    ///     <paramref name="enumerationId"/> argument to associate the callback invocations into a
    ///     single enumeration, meaning that a given set of calls to the enumeration callbacks will
    ///     use the same value for <paramref name="enumerationId"/> for the same session.
    ///     </para>
    ///     <para>
    ///     The provider must store the value of <paramref name="filterFileName"/> across calls
    ///     to this callback.  The provider replaces the value of <paramref name="filterFileName"/>
    ///     if <paramref name="restartScan"/> in a subsequent invocation of the callback is <c>true</c>
    ///     </para>
    ///     <para>If no entries match the search expression specified in <paramref name="filterFileName"/>,
    ///     or if all the entries in the directory were added in a previous invocation of this callback,
    ///     the provider must return <see cref="HResult::Ok"/>.</para>
    /// </remarks>
    /// <param name="commandId">
    ///     <para>A value that uniquely identifies an invocation of the callback.</para>
    ///     <para>If the provider returns <see cref="HResult::Pending"/> from this method, then it must pass
    ///     this value to <c>ProjFS.VirtualizationInstance.CompleteCommand</c> to signal that it has
    ///     finished processing this invocation of the callback.</para>
    /// </param>
    /// <param name="enumerationId">Identifies this enumeration session.</param>
    /// <param name="filterFileName">
    ///     <para>An optional string specifying a search expression.  This parameter may be <c>null</c>.</para>
    ///     <para>The search expression may include wildcard characters.  The provider should use the
    ///     <c>ProjFS.Utils.DoesNameContainWildCards</c> method routine to determine whether wildcards
    ///     are present in the search expression.  The provider should use the <c>ProjFS.Utils.IsFileNameMatch</c>
    ///     method to determine whether a directory entry in its store matches the search expression.</para>
    ///     <para>If this parameter is not <c>null</c>, only files whose names match the search expression
    ///     should be included in the directory scan.</para>
    ///     <para>If this parameter is <c>null</c>, all entries in the directory must be included.</para>
    /// </param>
    /// <param name="restartScan">
    ///     <para><c>true</c> if the scan is to start at the first entry in the directory.</para>
    ///     <para><c>false</c> if resuming the scan from a previous call.</para>
    ///     <para>On the first invocation of this callback for an enumeration session the provider must
    ///     treat this as <c>true</c>, regardless of its value (i.e. all enumerations must start at the
    ///     first entry).  On subsequent invocations of this callback the provider must honor this value.</para>
    /// </param>
    /// <param name="result">
    ///     <para>Receives the results of the enumeration from the provider.</para>
    ///     <para>The provider uses one of the <see cref="IDirectoryEnumerationResults"/>::<c>Add</c>
    ///     methods of this object to provide the enumeration results.</para>
    ///     <para>If the provider returns <see cref="HResult::Pending"/> from this method, then it must pass
    ///     this value to <c>ProjFS.VirtualizationInstance.CompleteCommand</c> to provide the
    ///     enumeration results.</para>
    /// </param>
    /// <returns>
    ///     <para><see cref="HResult::Ok"/> if the provider successfully completes the operation.</para>
    ///     <para><see cref="HResult::Pending"/> if the provider wishes to complete the operation at a later time.</para>
    ///     <para><see cref="HResult::InsufficientBuffer"/> if <paramref name="result"/><c>.Add</c> returned
    ///     <see cref="HResult::InsufficientBuffer"/> for the first matching file or directory in the enumeration.</para>
    ///     <para>An appropriate error code if the provider fails the operation.</para>
    /// </returns>
    HResult GetDirectoryEnumerationCallback(
        int commandId,
        System::Guid enumerationId,
        System::String^ filterFileName,
        bool restartScan,
        IDirectoryEnumerationResults^ result
    );

    /// <summary>
    /// Informs the provider that a directory enumeration is over.
    /// </summary>
    /// <seealso cref="StartDirectoryEnumerationCallback"/>
    /// <seealso cref="GetDirectoryEnumerationCallback"/>
    /// <remarks>
    ///     <para>
    ///     ProjFS requests a directory enumeration from the provider by first invoking
    ///     <see cref="StartDirectoryEnumerationCallback"/>, then the
    ///     <see cref="GetDirectoryEnumerationCallback"/> callback one or more times, then this
    ///     callback.  Because multiple enumerations may occur in parallel in the same location,
    ///     ProjFS uses the <paramref name="enumerationId"/> argument to associate the callback
    ///     invocations into a single enumeration, meaning that a given set of calls to the enumeration
    ///     callbacks will use the same value for <paramref name="enumerationId"/> for the same session.
    ///     </para>
    /// </remarks>
    /// <param name="enumerationId">Identifies this enumeration session.</param>
    /// <returns>
    ///     <para><see cref="HResult::Ok"/> if the provider successfully completes the operation.</para>
    ///     <para>An appropriate error code if the provider fails the operation.</para>
    /// </returns>
    HResult EndDirectoryEnumerationCallback(
        System::Guid enumerationId
    );

    /// <summary>Requests metadata information for a file or directory from the provider.</summary>
    /// <remarks>
    ///     <para>ProjFS uses the information the provider provides in this callback to create a
    ///     placeholder for the requested item.</para>
    ///     <para>To handle this callback, the provider typically calls
    ///     <c>ProjFS.VirtualizationInstance.WritePlaceholderInfo</c> to give ProjFS the information
    ///     for the requested file name.  Then the provider completes the callback.</para>
    ///     <para>This callback tells the provider that it must ensure that <paramref name="relativePath"/>
    ///     exists.  Although calling <c>ProjFS.VirtualizationInstance.WritePlaceholderInfo</c>
    ///     is the normal way of handling this callback, a provider may satisfy the request for
    ///     <paramref name="relativePath"/> in another way.  For example, it may instead create a
    ///     normal file or a symbolic link to an existing file.</para>
    /// </remarks>
    /// <param name="commandId">
    ///     <para>A value that uniquely identifies an invocation of the callback.</para>
    ///     <para>If the provider returns <see cref="HResult::Pending"/> from this method, then it must pass
    ///     this value to <c>ProjFS.VirtualizationInstance.CompleteCommand</c> to signal that it has
    ///     finished processing this invocation of the callback.</para>
    /// </param>
    /// <param name="relativePath">The path, relative to the virtualization root, of the file for
    /// which to return information.</param>
    /// <param name="triggeringProcessId">The PID for the process that triggered this callback.  If
    /// this information is not available, this will be 0.</param>
    /// <param name="triggeringProcessImageFileName">The image file name corresponding to
    /// <paramref name="triggeringProcessId"/>. If <paramref name="triggeringProcessId"/> is 0 this
    /// will be <c>null</c>.</param>
    /// <returns>
    ///     <para><see cref="HResult::Ok"/> if the file exists in the provider's store and it successfully
    ///     gave the file's information to ProjFS.</para>
    ///     <para><see cref="HResult::FileNotFound"/> if <paramref name="relativePath"/> does not exist in the provider's store.</para>
    ///     <para><see cref="HResult::Pending"/> if the provider wishes to complete the operation at a later time.</para>
    ///     <para>An appropriate error code if the provider fails the operation.</para>
    /// </returns>
    HResult GetPlaceholderInfoCallback(
        int commandId,
        System::String^ relativePath,
        unsigned int triggeringProcessId,
        System::String^ triggeringProcessImageFileName
    );

    /// <summary>Requests the contents of a file's primary data stream.</summary>
    /// <remarks>
    ///     <para>ProjFS uses the data the provider provides in this callback to convert the file into
    ///     a hydrated placeholder.</para>
    ///     <para>To handle this callback, the provider issues one or more calls to
    ///     <c>ProjFS.VirtualizationInstance.WriteFile</c> to give ProjFS the contents of the file's
    ///     primary data stream.  Then the provider completes the callback.</para>
    /// </remarks>
    /// <param name="commandId">A value that uniquely identifies an invocation of the callback.
    ///     <para>If the provider returns <see cref="HResult::Pending"/> from this method, then it must pass
    ///     this value to <c>ProjFS.VirtualizationInstance.CompleteCommand</c> to signal that it has
    ///     finished processing this invocation of the callback.</para>
    /// </param>
    /// <param name="relativePath">The path, relative to the virtualization root, of the file for
    /// which to provide data.</param>
    /// <param name="byteOffset">Offset in bytes from the beginning of the file at which the provider
    ///     must start returning data.  The provider must return file data starting at or before this
    ///     offset.</param>
    /// <param name="length">Number of bytes of file data requested.  The provider must return at least
    ///     this many bytes of file data beginning at <paramref name="byteOffset"/>.</param>
    /// <param name="dataStreamId">The unique value to associate with this file stream.  The provider
    ///     must pass this value to <c>ProjFS.VirtualizationInstance.WriteFile</c> when providing
    ///     file data as part of handling this callback.</param>
    /// <param name="contentId">The <paramref name="contentId"/> value specified by the provider when
    ///     it created the placeholder for this file.  See <c>ProjFS.VirtualizationInstance.WritePlaceholderInfo</c>.</param>
    /// <param name="providerId">The <paramref name="providerId"/> value specified by the provider when
    ///     it created the placeholder for this file.  See <c>ProjFS.VirtualizationInstance.WritePlaceholderInfo</c>.</param>
    /// <param name="triggeringProcessId">The PID for the process that triggered this callback.  If
    /// this information is not available, this will be 0.</param>
    /// <param name="triggeringProcessImageFileName">The image file name corresponding to
    /// <paramref name="triggeringProcessId"/>.  If <paramref name="triggeringProcessId"/> is 0 this
    /// will be <c>null</c>.</param>
    /// <returns>
    ///     <para><see cref="HResult::Ok"/> if the provider successfully wrote all the requested data.</para>
    ///     <para><see cref="HResult::Pending"/> if the provider wishes to complete the operation at a later time.</para>
    ///     <para>An appropriate error code if the provider fails the operation.</para>
    /// </returns>
    HResult GetFileDataCallback(
        int commandId,
        System::String^ relativePath,
        unsigned long long byteOffset,
        unsigned int length,
        System::Guid dataStreamId,
        array<System::Byte>^ contentId,
        array<System::Byte>^ providerId,
        unsigned int triggeringProcessId,
        System::String^ triggeringProcessImageFileName
    );
};

}}} // namespace Microsoft.Windows.ProjFS