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

        internal unsafe WriteBuffer(IntPtr virtualizationContext, uint desiredBufferSize)
        {
            _buffer = ProjFSNative.PrjAllocateAlignedBuffer(virtualizationContext, new UIntPtr(desiredBufferSize));
            if (_buffer == IntPtr.Zero)
            {
                throw new OutOfMemoryException("PrjAllocateAlignedBuffer returned null");
            }

            Length = desiredBufferSize;
            byte* pBuf = (byte*)_buffer.ToPointer();
            Stream = new UnmanagedMemoryStream(pBuf, desiredBufferSize, desiredBufferSize, FileAccess.Write);
        }

        ~WriteBuffer()
        {
            Dispose(false);
        }

        public IntPtr Pointer => _buffer;

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
