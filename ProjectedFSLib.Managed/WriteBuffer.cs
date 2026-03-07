#nullable disable
using System;
using System.IO;
using System.Runtime.InteropServices;

namespace Microsoft.Windows.ProjFS
{
    /// <summary>
    /// Pure C# P/Invoke implementation of IWriteBuffer, using PrjAllocateAlignedBuffer/PrjFreeAlignedBuffer.
    /// Replaces the C++/CLI WriteBuffer from ProjectedFSLib.Managed.dll for NativeAOT compatibility.
    /// </summary>
    public class WriteBuffer : IWriteBuffer
    {
        private IntPtr _buffer;
        private bool _disposed;

        internal unsafe WriteBuffer(SafeProjFsHandle virtualizationContext, uint desiredBufferSize)
        {
            _buffer = ProjFSNative.PrjAllocateAlignedBuffer(virtualizationContext, new UIntPtr(desiredBufferSize));
            if (_buffer == IntPtr.Zero)
            {
                throw new InvalidOperationException("PrjAllocateAlignedBuffer returned null — insufficient memory or invalid context.");
            }

            Length = desiredBufferSize;
            byte* pBuf = (byte*)_buffer.ToPointer();
            Stream = new UnmanagedMemoryStream(pBuf, desiredBufferSize, desiredBufferSize, FileAccess.Write);
        }

        ~WriteBuffer()
        {
            Dispose(false);
        }

#pragma warning disable CA1720 // Identifier contains type name — established public API
        public IntPtr Pointer => _buffer;
#pragma warning restore CA1720

        public UnmanagedMemoryStream Stream { get; private set; }

        public long Length { get; }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected void Dispose(bool disposing)
        {
            if (_disposed)
            {
                return;
            }

            if (disposing)
            {
                Stream?.Dispose();
                Stream = null;
            }

            if (_buffer != IntPtr.Zero)
            {
                ProjFSNative.PrjFreeAlignedBuffer(_buffer);
                _buffer = IntPtr.Zero;
            }

            _disposed = true;
        }
    }
}
