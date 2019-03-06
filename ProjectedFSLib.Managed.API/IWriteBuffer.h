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
/// This class defines the interface implemented by the <c>Microsoft.Windows.ProjFS.WriteBuffer</c>
/// class.  This interface class is provided for use by unit tests to mock up the interface to ProjFS.
/// </remarks>
public interface class IWriteBuffer : System::IDisposable
{
public:

    /// <summary>
    /// When overridden in a derived class, gets the allocated length of the buffer.
    /// </summary>
    property long long Length
    {
        long long get(void);
    };

    /// <summary>
    /// When overridden in a derived class, gets a <see cref="System::IO::UnmanagedMemoryStream"/>
    /// representing the internal buffer.
    /// </summary>
    property System::IO::UnmanagedMemoryStream^ Stream
    {
        System::IO::UnmanagedMemoryStream^ get(void);
    };

    /// <summary>
    /// When overridden in a derived class, gets a pointer to the internal buffer.
    /// </summary>
    property System::IntPtr Pointer
    {
        System::IntPtr get(void);
    }
};

}}} // namespace Microsoft.Windows.ProjFS