// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

namespace Microsoft {
namespace Windows {
namespace ProjFS {

/// <summary>
/// Interface to allow for easier unit testing of a virtualization provider.
/// </summary>
/// <remarks>
/// This class defines the interface implemented by the <c>Microsoft.Windows.ProjFS.DirectoryEnumerationResults</c>
/// class.  This interface class is provided for use by unit tests to mock up the interface to ProjFS.
/// </remarks>
public interface class IDirectoryEnumerationResults
{
public:

    /// <summary>
    /// When overridden in a derived class, adds one entry to a directory enumeration result.
    /// </summary>
    /// <remarks>
    /// <para>
    /// In its implementation of a <c>GetDirectoryEnumerationCallback</c> delegate the provider
    /// calls this method for each matching file or directory in the enumeration.
    /// </para>
    /// <para>
    /// If this method returns <c>false</c>, the provider returns <c>HResult.Ok</c> and waits for
    /// the next <c>GetDirectoryEnumerationCallback</c>.  Then it resumes filling the enumeration with
    /// the entry it was trying to add when it got <c>false</c>. 
    /// </para>
    /// <para>
    /// If the function returns <c>false</c> for the first file or directory in the enumeration, the
    /// provider returns <c>HResult.InsufficientBuffer</c> from the <c>GetDirectoryEnumerationCallback</c>
    /// method.
    /// </para>
    /// <para>
    /// IMPORTANT: File and directory names passed to this method must be in the sort
    /// specified by <c>PrjFileNameCompare</c>
    /// (see https://docs.microsoft.com/en-us/windows/win32/api/projectedfslib/nf-projectedfslib-prjfilenamecompare ),
    /// or else names can be duplicated or missing from the enumeration results presented to the
    /// process enumerating the filesystem.
    /// </para>
    /// </remarks>
    /// <param name="fileName">The name of the file or directory.</param>
    /// <param name="fileSize">The size of the file.</param>
    /// <param name="isDirectory"><c>true</c> if this item is a directory, <c>false</c> if it is a file.</param>
    /// <returns>
    /// <para>
    /// <c>true</c> if the entry was successfully added to the enumeration buffer, <c>false</c> otherwise.
    /// </para>
    /// </returns>
    bool Add(
        System::String^ fileName,
        long long fileSize,
        bool isDirectory
    );

    /// <summary>
    /// When overridden in a derived class, adds one entry to a directory enumeration result.
    /// </summary>
    /// <remarks>
    /// <para>
    /// In its implementation of a <c>GetDirectoryEnumerationCallback</c> delegate the provider
    /// calls this method for each matching file or directory in the enumeration.
    /// </para>
    /// <para>
    /// If this method returns <c>false</c>, the provider returns <c>HResult.Ok</c> and waits for
    /// the next <c>GetDirectoryEnumerationCallback</c>.  Then it resumes filling the enumeration with
    /// the entry it was trying to add when it got <c>false</c>. 
    /// </para>
    /// <para>
    /// If the function returns <c>false</c> for the first file or directory in the enumeration, the
    /// provider returns <c>HResult.InsufficientBuffer</c> from the <c>GetDirectoryEnumerationCallback</c>
    /// method.
    /// </para>
    /// <para>
    /// IMPORTANT: File and directory names passed to this method must be in the sort
    /// specified by <c>PrjFileNameCompare</c>
    /// (see https://docs.microsoft.com/en-us/windows/win32/api/projectedfslib/nf-projectedfslib-prjfilenamecompare ),
    /// or else names can be duplicated or missing from the enumeration results presented to the
    /// process enumerating the filesystem.
    /// </para>
    /// </remarks>
    /// <param name="fileName">The name of the file or directory.</param>
    /// <param name="fileSize">The size of the file.</param>
    /// <param name="isDirectory"><c>true</c> if this item is a directory, <c>false</c> if it is a file.</param>
    /// <param name="fileAttributes">The file attributes.</param>
    /// <param name="creationTime">Specifies the time that the file was created.</param>
    /// <param name="lastAccessTime">Specifies the time that the file was last accessed.</param>
    /// <param name="lastWriteTime">Specifies the time that the file was last written to.</param>
    /// <param name="changeTime">Specifies the last time the file was changed.</param>
    /// <returns>
    /// <para>
    /// <c>true</c> if the entry was successfully added to the enumeration buffer, <c>false</c> otherwise.
    /// </para>
    /// </returns>
    bool Add(
        System::String^ fileName,
        long long fileSize,
        bool isDirectory,
        System::IO::FileAttributes fileAttributes,
        System::DateTime creationTime,
        System::DateTime lastAccessTime,
        System::DateTime lastWriteTime,
        System::DateTime changeTime
    );

    /// <summary>
    /// When overridden in a derived class, adds one entry to a directory enumeration result.
    /// </summary>
    /// <remarks>
    /// <para>
    /// In its implementation of a <c>GetDirectoryEnumerationCallback</c> delegate the provider
    /// calls this method for each matching file or directory in the enumeration.
    /// </para>
    /// <para>
    /// If this method returns <c>false</c>, the provider returns <c>HResult.Ok</c> and waits for
    /// the next <c>GetDirectoryEnumerationCallback</c>.  Then it resumes filling the enumeration with
    /// the entry it was trying to add when it got <c>false</c>. 
    /// </para>
    /// <para>
    /// If the function returns <c>false</c> for the first file or directory in the enumeration, the
    /// provider returns <c>HResult.InsufficientBuffer</c> from the <c>GetDirectoryEnumerationCallback</c>
    /// method.
    /// </para>
    /// <para>
    /// IMPORTANT: File and directory names passed to this method must be in the sort
    /// specified by <c>PrjFileNameCompare</c>
    /// (see https://docs.microsoft.com/en-us/windows/win32/api/projectedfslib/nf-projectedfslib-prjfilenamecompare ),
    /// or else names can be duplicated or missing from the enumeration results presented to the
    /// process enumerating the filesystem.
    /// </para>
    /// <para>
    /// This overload is incompatible with .NET Framework clients. 
    /// See https://github.com/microsoft/ProjFS-Managed-API/issues/81 for details.
    /// </para>
    /// </remarks>
    /// <param name="fileName">The name of the file or directory.</param>
    /// <param name="fileSize">The size of the file.</param>
    /// <param name="isDirectory"><c>true</c> if this item is a directory, <c>false</c> if it is a file.</param>
    /// <param name="fileAttributes">The file attributes.</param>
    /// <param name="creationTime">Specifies the time that the file was created.</param>
    /// <param name="lastAccessTime">Specifies the time that the file was last accessed.</param>
    /// <param name="lastWriteTime">Specifies the time that the file was last written to.</param>
    /// <param name="changeTime">Specifies the last time the file was changed.</param>
    /// <param name="symlinkTargetOrNull">Specifies the symlink target path if the file is a symlink.</param>
    /// <returns>
    /// <para>
    /// <c>true</c> if the entry was successfully added to the enumeration buffer, <c>false</c> otherwise.
    /// </para>
    /// </returns>
    bool Add(
        System::String ^ fileName,
        long long fileSize,
        bool isDirectory,
        System::IO::FileAttributes fileAttributes,
        System::DateTime creationTime,
        System::DateTime lastAccessTime,
        System::DateTime lastWriteTime,
        System::DateTime changeTime,
        System::String ^ symlinkTargetOrNull
    );
};

}}} // namespace Microsoft.Windows.ProjFS