// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "IDirectoryEnumerationResults.h"

using namespace System;
using namespace System::Globalization;
using namespace System::IO;

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

        auto projFsLib = ::LoadLibraryW(L"ProjectedFSLib.dll");
        if (!projFsLib)
        {
            throw gcnew FileLoadException(String::Format(CultureInfo::InvariantCulture, "Could not load ProjectedFSLib.dll to set up entry points."));
        }

        if (::GetProcAddress(projFsLib, "PrjWritePlaceholderInfo2") != nullptr)
        {
            // We have the API introduced in Windows 10 version 2004.
            this->_PrjFillDirEntryBuffer2 = reinterpret_cast<t_PrjFillDirEntryBuffer2>(::GetProcAddress(projFsLib,
                "PrjFillDirEntryBuffer2"));
        }

        ::FreeLibrary(projFsLib);
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
        ValidateFileName(fileName);

        pin_ptr<const WCHAR> pFileName = PtrToStringChars(fileName);
        PRJ_FILE_BASIC_INFO basicInfo = BuildFileBasicInfo(fileSize,
            isDirectory,
            fileAttributes,
            creationTime,
            lastAccessTime,
            lastWriteTime,
            changeTime);

        auto hr = ::PrjFillDirEntryBuffer(pFileName,
                                          &basicInfo,
                                          m_dirEntryBufferHandle);

        if FAILED(hr)
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
    /// <param name="symlinkTargetOrNull">Specifies the symlink target path if the file is a symlink.</param>
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
        System::DateTime changeTime,
        System::String^ symlinkTargetOrNull) sealed
    {
        ValidateFileName(fileName);

        pin_ptr<const WCHAR> pFileName = PtrToStringChars(fileName);
        PRJ_FILE_BASIC_INFO basicInfo = BuildFileBasicInfo(fileSize,
            isDirectory,
            fileAttributes,
            creationTime,
            lastAccessTime,
            lastWriteTime,
            changeTime);

        HRESULT hr;
        if (symlinkTargetOrNull != nullptr && this->_PrjFillDirEntryBuffer2 != nullptr)
        {
            PRJ_EXTENDED_INFO extendedInfo = {};

            extendedInfo.InfoType = PRJ_EXT_INFO_TYPE_SYMLINK;
            pin_ptr<const WCHAR> targetPath = PtrToStringChars(symlinkTargetOrNull);
            extendedInfo.Symlink.TargetName = targetPath;

            hr = this->_PrjFillDirEntryBuffer2(m_dirEntryBufferHandle,
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

    void ValidateFileName(System::String^ fileName)
    {
        if (System::String::IsNullOrEmpty(fileName))
        {
            throw gcnew System::ArgumentException(System::String::Format(System::Globalization::CultureInfo::InvariantCulture,
                "fileName cannot be empty."));
        }
    }

    PRJ_FILE_BASIC_INFO BuildFileBasicInfo(long long fileSize,
        bool isDirectory,
        System::IO::FileAttributes fileAttributes,
        System::DateTime creationTime,
        System::DateTime lastAccessTime,
        System::DateTime lastWriteTime,
        System::DateTime changeTime)
    {
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

        return basicInfo;
    }

    typedef HRESULT(__stdcall* t_PrjFillDirEntryBuffer2)(
        _In_ PRJ_DIR_ENTRY_BUFFER_HANDLE dirEntryBufferHandle,
        _In_ PCWSTR fileName,
        _In_opt_ PRJ_FILE_BASIC_INFO* fileBasicInfo,
        _In_opt_ PRJ_EXTENDED_INFO* extendedInfo
        );

    t_PrjFillDirEntryBuffer2 _PrjFillDirEntryBuffer2 = nullptr;
};
}}} // namespace Microsoft.Windows.ProjFS