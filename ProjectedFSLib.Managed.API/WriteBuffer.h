// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "ApiHelper.h"

#pragma once

namespace Microsoft {
namespace Windows {
namespace ProjFS {
    /// <summary>
    /// Helper class to ensure correct alignment when providing file contents for a placeholder.
    /// </summary>
    /// <remarks>
    /// <para>
    /// The provider does not instantiate this class directly.  It uses the 
    /// <c>ProjFS.VirtualizationInstance.CreateWriteBuffer</c> method to obtain a properly initialized
    /// instance of this class.
    /// </para>
    /// <para>
    /// The <c>ProjFS.VirtualizationInstance.WriteFileData</c> method requires a data buffer containing
    /// file data for a placeholder so that ProjFS can convert the placeholder to a hydrated placeholder
    /// (see <c>ProjFS.OnDiskFileState</c> for a discussion of file states).  Internally ProjFS uses
    /// the user's FILE_OBJECT to write this data to the file.  Because the user may have opened the
    /// file for unbuffered I/O, and unbuffered I/O imposes certain alignment requirements, this
    /// class is provided to abstract out those details.
    /// </para>
    /// <para>
    /// When the provider starts its virtualization instance, the <c>VirtualizationInstance</c> class
    /// queries the alignment requirements of the underlying physical storage device and uses this
    /// information to return a properly-initialized instance of this class from its <c>CreateWriteBuffer</c>
    /// method.
    /// </para>
    /// </remarks>
    public ref class WriteBuffer
    {
    internal:
        WriteBuffer(
            unsigned long bufferSize,
            unsigned long alignment);

        WriteBuffer(
            unsigned long bufferSize,
            PRJ_NAMESPACE_VIRTUALIZATION_CONTEXT namespaceCtx,
            ApiHelper^ apiHelper);

    public:
        ~WriteBuffer();

        /// <summary>
        /// Gets the allocated length of the buffer.
        /// </summary>
        property long long Length
        {
            long long get(void);
        };

        /// <summary>
        /// Gets a <see cref="System::IO::UnmanagedMemoryStream"/> representing the internal buffer.
        /// </summary>
        property System::IO::UnmanagedMemoryStream^ Stream
        {
            System::IO::UnmanagedMemoryStream^ get(void);
        };

        /// <summary>
        /// Gets a pointer to the internal buffer.
        /// </summary>
        property System::IntPtr Pointer
        {
            System::IntPtr get(void);
        }

    protected:
        /// <summary>
        /// Frees the internal buffer.
        /// </summary>
        !WriteBuffer();

    private:
        System::IO::UnmanagedMemoryStream^ stream;
        unsigned char* buffer;
        PRJ_NAMESPACE_VIRTUALIZATION_CONTEXT namespaceCtx;
        ApiHelper^ apiHelper;
    };
}}} // namespace Microsoft.Windows.ProjFS