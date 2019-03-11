// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

namespace Microsoft {
namespace Windows {
namespace ProjFS {
    /// <summary>
    /// HRESULT values that ProjFS may report to a provider, or that a provider may return to ProjFS.
    /// </summary>
    /// <remarks>
    /// <para>
    /// .NET methods normally do not return error codes, preferring to throw exceptions.  For the most
    /// part this API does not throw exceptions, preferring instead to return error codes.  We do this
    /// for few reasons:
    /// <list type="bullet">
    ///     <item>
    ///     <description>
    ///     This API is a relatively thin wrapper around a native API that itself returns HRESULT codes.
    ///     This managed library would have to translate those error codes into exceptions to throw.
    ///     </description>
    ///     </item>
    ///     <item>
    ///     <description>
    ///     Errors that a provider returns are sent through the file system, back to the user who is
    ///     performing the I/O.  If the provider callbacks threw exceptions, the managed library would
    ///     just have to catch them and turn them into HRESULT codes.
    ///     </description>
    ///     </item>
    ///     <item>
    ///     <description>
    ///     If the API methods described here threw exceptions, either the provider would have to catch
    ///     them and turn them into error codes to return from its callbacks, or it would allow those
    ///     exceptions to propagate and this managed library would still have to deal with them as
    ///     described in the preceding bullet.
    ///     </description>
    ///     </item>
    /// </list>
    /// So rather than deal with the overhead of exceptions just to try to conform to .NET conventions,
    /// this API largely dispenses with them and uses HRESULT codes.
    /// </para>
    /// <para>
    /// Note that for the convenience of C# developers the <c>VirtualizationInstance::CreateWriteBuffer</c>
    /// method does throw <c>System::OutOfMemoryException</c> if it cannot allocate the buffer.  This
    /// makes the method convenient to use with the <c>using</c> keyword.
    /// </para>
    /// <para>
    /// Note that when HRESULT codes returned from the provider are sent to the file system, the ProjFS
    /// library translates them into NTSTATUS codes.  Because there is not a 1-to-1 mapping of HRESULT
    /// codes to NTSTATUS codes, the set of HRESULT codes that a provider is allowed to return is
    /// necessarily constrained.
    /// </para>
    /// <para>
    /// A provider's <c>IRequiredCallbacks</c> method and <c>On...</c> delegate implementations may
    /// return any <c>HResult</c> value returned from a <c>VirtualizationInstance</c>, as well as the
    /// following <c>HResult</c> values:
    /// <list type="bullet">
    ///     <item>
    ///     <description><see cref="HResult::Ok"/></description>
    ///     </item>
    ///     <item>
    ///     <description><see cref="HResult::Pending"/></description>
    ///     </item>
    ///     <item>
    ///     <description><see cref="HResult::OutOfMemory"/></description>
    ///     </item>
    ///     <item>
    ///     <description><see cref="HResult::InsufficientBuffer"/></description>
    ///     </item>
    ///     <item>
    ///     <description><see cref="HResult::FileNotFound"/></description>
    ///     </item>
    ///     <item>
    ///     <description><see cref="HResult::VirtualizationUnavaliable"/></description>
    ///     </item>
    ///     <item>
    ///     <description><see cref="HResult::InternalError"/></description>
    ///     </item>
    /// </list>
    /// </para>
    /// <para>
    /// The remaining values in the <c>HResult</c> enum may be returned to a provider from ProjFS APIs
    /// and are primarily intended to communicate information to the provider.  As noted above, if
    /// such a value is returned to a provider in its implementation of a callback or delegate, it may
    /// return the value to ProjFS.
    /// </para>
    /// </remarks>
    public enum class HResult : long
    {
        // In addition to any HResult value returned from a VirtualizationInstance method, a provider
        // may return any of the following HResult values.

        ///<summary>Success.</summary>
        Ok = S_OK,

        ///<summary>The data necessary to complete this operation is not yet available.</summary>
        Pending = HRESULT_FROM_WIN32(ERROR_IO_PENDING),

        ///<summary>Ran out of memory.</summary>
        OutOfMemory = E_OUTOFMEMORY,

        ///<summary>The data area passed to a system call is too small.</summary>
        InsufficientBuffer = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER),

        ///<summary>The system cannot find the file specified.</summary>
        FileNotFound = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),

        ///<summary>The provider that supports file system virtualization is temporarily unavailable.</summary>
        VirtualizationUnavaliable = HRESULT_FROM_WIN32(ERROR_FILE_SYSTEM_VIRTUALIZATION_UNAVAILABLE),

        ///<summary>The provider is in an invalid state that prevents it from servicing the callback
        ///(only use this if none of the other error codes is a better match).</summary>
        InternalError = HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR),

        //////////////////////////////////////////////////////////////////////////
        //
        // The following HResult values are intended for use by the ProjFS library only. They are
        // for communicating information to the provider.
        //
        // ProjFS internally returns NTSTATUS codes to the I/O system.  Because the mapping from NTSTATUS
        // to Win32 error/HRESULT codes is not 1:1, ProjFS's user-mode library performs a reverse
        // mapping for selected error codes.  ProjFS performs such a reverse mapping for the codes
        // listed above.  Those listed below may not have a reverse mapping.  Any code for which ProjFS
        // does not have a reverse mapping will be returned to the I/O system as
        // STATUS_INTERNAL_ERROR.
        // 
        //////////////////////////////////////////////////////////////////////////

        ///<summary>An attempt was made to perform an initialization operation when initialization
        ///has already been completed.</summary>
        AlreadyInitialized = HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED),

        ///<summary>Access is denied.</summary>
        AccessDenied = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED),

        ///<summary>An attempt has been made to remove a file or directory that cannot be deleted.</summary>
        // There is not a Win32 error code that can reverse-map back to STATUS_CANNOT_DELETE.  However
        // the I/O system expects the file system to return that status code as an indication that a
        // delete is not allowed. To ensure the I/O system gets the correct code we define it here
        // by hand.  This is equivalent to HRESULT_FROM_NT(STATUS_CANNOT_DELETE).  Doing it this way
        // saves us from having to do weird things with #include and #define.
        CannotDelete = ((HRESULT) (0xc0000121 | 0x10000000)),

        ///<summary>The directory name is invalid (it may not be a directory).</summary>
        Directory = HRESULT_FROM_WIN32(ERROR_DIRECTORY),

        ///<summary>The directory is not empty.</summary>
        DirNotEmpty = HRESULT_FROM_WIN32(ERROR_DIR_NOT_EMPTY),

        ///<summary>Invalid handle (it may already be closed).</summary>
        Handle = E_HANDLE,

        ///<summary>One or more arguments are invalid.</summary>
        InvalidArg = E_INVALIDARG,

        ///<summary>The system cannot find the path specified.</summary>
        PathNotFound = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND),

        ///<summary>The object manager encountered a reparse point while retrieving an object.</summary>
        ReparsePointEncountered = HRESULT_FROM_WIN32(ERROR_REPARSE_POINT_ENCOUNTERED),

        ///<summary>The virtualization operation is not allowed on the file in its current state.</summary>
        VirtualizationInvalidOp = HRESULT_FROM_WIN32(ERROR_FILE_SYSTEM_VIRTUALIZATION_INVALID_OPERATION),
    };
}}} // namespace Microsoft.Windows.ProjFS
