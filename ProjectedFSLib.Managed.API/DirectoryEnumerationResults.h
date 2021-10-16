// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "IDirectoryEnumerationResults.h"

namespace Microsoft {
namespace Windows {
namespace ProjFS {
///<summary>Helper class for providing the results of a directory enumeration.</summary>
///<remarks>
/// ProjFS passes an instance of this class to the provider in the <paramref name="result"/>
/// parameter of its implementation of a <c>GetDirectoryEnumerationCallback</c> delegate.  The provider
/// calls one of its <c>Add</c> methods for each item in the enumeration
/// to add it to the result set.
///</remarks>
public ref class DirectoryEnumerationResults : public IDirectoryEnumerationResults {
internal:

    DirectoryEnumerationResults(PRJ_DIR_ENTRY_BUFFER_HANDLE bufferHandle)
    {
        m_dirEntryBufferHandle = bufferHandle;
    }

    // Provides access to the native handle to the directory entry buffer.
    // Used internally by VirtualizationInstance::CompleteCommand(int, IDirectoryEnumerationResults^).
    property PRJ_DIR_ENTRY_BUFFER_HANDLE DirEntryBufferHandle
    {
        PRJ_DIR_ENTRY_BUFFER_HANDLE get(void)
        {
            return m_dirEntryBufferHandle;
        }
    };

public:

    /// <summary>Adds one entry to a directory enumeration result.</summary>
    /// <remarks>
    ///     <para>
    ///     In its implementation of a <c>GetDirectoryEnumerationCallback</c> delegate the provider
    ///     calls this method for each matching file or directory in the enumeration.
    ///     </para>
    ///     <para>
    ///     If the provider calls this <c>Add</c> overload, then the timestamps reported to the caller
    ///     of the enumeration are the current system time.  If the provider wants the caller to see other
    ///     timestamps, it must use the other <c>Add</c> overload.
    ///     </para>
    ///     <para>
    ///     If this method returns <c>false</c>, the provider returns <see cref="HResult::Ok"/> and waits for
    ///     the next <c>GetDirectoryEnumerationCallback</c>.  Then it resumes filling the enumeration with
    ///     the entry it was trying to add when it got <c>false</c>. 
    ///     </para>
    ///     <para>
    ///     If the method returns <c>false</c> for the first file or directory in the enumeration, the
    ///     provider returns <see cref="HResult::InsufficientBuffer"/> from the <c>GetDirectoryEnumerationCallback</c>
    ///     method.
    ///     </para>
    /// </remarks>
    /// <param name="fileName">The name of the file or directory.</param>
    /// <param name="fileSize">The size of the file.</param>
    /// <param name="isDirectory"><c>true</c> if this item is a directory, <c>false</c> if it is a file.</param>
    /// <returns>
    ///     <para>
    ///     <c>true</c> if the entry was successfully added to the enumeration buffer, <c>false</c> otherwise.
    ///     </para>
    /// </returns>
    /// <exception cref="System::ArgumentException">
    /// <paramref name="fileName"/> is null or empty.
    /// </exception>
    virtual bool Add(
        System::String^ fileName,
        long long fileSize,
        bool isDirectory) sealed
    {
        if (System::String::IsNullOrEmpty(fileName))
        {
            throw gcnew System::ArgumentException(System::String::Format(System::Globalization::CultureInfo::InvariantCulture,
                                                                         "fileName cannot be empty."));
        }

        pin_ptr<const WCHAR> pFileName = PtrToStringChars(fileName);

        PRJ_FILE_BASIC_INFO basicInfo = { 0 };
        basicInfo.IsDirectory = isDirectory;
        basicInfo.FileSize = fileSize;

        auto hr = ::PrjFillDirEntryBuffer(pFileName,
                                          &basicInfo,
                                          m_dirEntryBufferHandle);

        if (FAILED(hr))
        {
            return false;
        }

        return true;
    }

    /// <summary>Adds one entry to a directory enumeration result.</summary>
    /// <remarks>
    ///     <para>
    ///     In its implementation of a <c>GetDirectoryEnumerationCallback</c> delegate the provider
    ///     calls this method for each matching file or directory in the enumeration.
    ///     </para>
    ///     <para>
    ///     If this method returns <c>false</c>, the provider returns <see cref="HResult::Ok"/> and waits for
    ///     the next <c>GetDirectoryEnumerationCallback</c>.  Then it resumes filling the enumeration with
    ///     the entry it was trying to add when it got <c>false</c>. 
    ///     </para>
    ///     <para>
    ///     If the method returns <c>false</c> for the first file or directory in the enumeration, the
    ///     provider returns <see cref="HResult::InsufficientBuffer"/> from the <c>GetDirectoryEnumerationCallback</c>
    ///     method.
    ///     </para>
    /// </remarks>
    /// <param name="fileName">The name of the file or directory.</param>
    /// <param name="fileSize">The size of the file.</param>
    /// <param name="isDirectory"><c>true</c> if this item is a directory, <c>false</c> if it is a file.</param>
    /// <param name="fileAttributes">The file attributes.</param>
    /// <param name="creationTime">The time the file was created.</param>
    /// <param name="lastAccessTime">The time the file was last accessed.</param>
    /// <param name="lastWriteTime">The time the file was last written to.</param>
    /// <param name="changeTime">The time the file was last changed.</param>
    /// <returns>
    ///     <para>
    ///     <c>true</c> if the entry was successfully added to the enumeration buffer, <c>false</c> otherwise.
    ///     </para>
    /// </returns>
    /// <exception cref="System::ArgumentException">
    /// <paramref name="fileName"/> is null or empty.
    /// </exception>
    virtual bool Add(
        System::String^ fileName,
        long long fileSize,
        bool isDirectory,
        System::IO::FileAttributes fileAttributes,
        System::DateTime creationTime,
        System::DateTime lastAccessTime,
        System::DateTime lastWriteTime,
        System::DateTime changeTime) sealed
    {
        if (System::String::IsNullOrEmpty(fileName))
        {
            throw gcnew System::ArgumentException(System::String::Format(System::Globalization::CultureInfo::InvariantCulture,
                                                                         "fileName cannot be empty."));
        }

        pin_ptr<const WCHAR> pFileName = PtrToStringChars(fileName);
        PRJ_FILE_BASIC_INFO basicInfo = { 0 };

        if (creationTime != System::DateTime::MinValue)
        {
            basicInfo.CreationTime.QuadPart = creationTime.ToFileTime();
        }

        if (lastAccessTime != System::DateTime::MinValue)
        {
            basicInfo.LastAccessTime.QuadPart = lastAccessTime.ToFileTime();
        }

        if (lastWriteTime != System::DateTime::MinValue)
        {
            basicInfo.LastWriteTime.QuadPart = lastWriteTime.ToFileTime();
        }

        if (changeTime != System::DateTime::MinValue)
        {
            basicInfo.ChangeTime.QuadPart = changeTime.ToFileTime();
        }

        basicInfo.FileAttributes = static_cast<UINT32>(fileAttributes);
        basicInfo.IsDirectory = isDirectory;
        basicInfo.FileSize = fileSize;

        auto hr = ::PrjFillDirEntryBuffer(pFileName,
                                          &basicInfo,
                                          m_dirEntryBufferHandle);

        if FAILED(hr)
        {
            return false;
        }

        return true;
    }

    virtual bool Add2(
        System::String^ fileName,
        long long fileSize,
        bool isDirectory,
        System::IO::FileAttributes fileAttributes,
        System::DateTime creationTime,
        System::DateTime lastAccessTime,
        System::DateTime lastWriteTime,
        System::DateTime changeTime,
        System::String^ symlinkTargetOrNull) sealed
    {
        if (System::String::IsNullOrEmpty(fileName))
        {
            throw gcnew System::ArgumentException(System::String::Format(System::Globalization::CultureInfo::InvariantCulture,
                "fileName cannot be empty."));
        }

        pin_ptr<const WCHAR> pFileName = PtrToStringChars(fileName);
        PRJ_FILE_BASIC_INFO basicInfo = { 0 };

        if (creationTime != System::DateTime::MinValue)
        {
            basicInfo.CreationTime.QuadPart = creationTime.ToFileTime();
        }

        if (lastAccessTime != System::DateTime::MinValue)
        {
            basicInfo.LastAccessTime.QuadPart = lastAccessTime.ToFileTime();
        }

        if (lastWriteTime != System::DateTime::MinValue)
        {
            basicInfo.LastWriteTime.QuadPart = lastWriteTime.ToFileTime();
        }

        if (changeTime != System::DateTime::MinValue)
        {
            basicInfo.ChangeTime.QuadPart = changeTime.ToFileTime();
        }

        basicInfo.FileAttributes = static_cast<UINT32>(fileAttributes);
        basicInfo.IsDirectory = isDirectory;
        basicInfo.FileSize = fileSize;

        HRESULT hr;
        if (symlinkTargetOrNull != nullptr)
        {
            PRJ_EXTENDED_INFO extendedInfo = {};

            extendedInfo.InfoType = PRJ_EXT_INFO_TYPE_SYMLINK;
            pin_ptr<const WCHAR> targetPath = PtrToStringChars(symlinkTargetOrNull);
            extendedInfo.Symlink.TargetName = targetPath;

            hr = ::PrjFillDirEntryBuffer2(m_dirEntryBufferHandle,
                pFileName,
                &basicInfo,
                &extendedInfo);
        }
        else
        {
            hr = ::PrjFillDirEntryBuffer(pFileName,
                &basicInfo,
                m_dirEntryBufferHandle);
        }

        if FAILED(hr)
        {
            return false;
        }

        return true;
    }

private:

    PRJ_DIR_ENTRY_BUFFER_HANDLE m_dirEntryBufferHandle;
};
}}} // namespace Microsoft.Windows.ProjFS