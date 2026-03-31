using System;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

namespace Microsoft.Windows.ProjFS
{
    /// <summary>
    /// SafeHandle wrapper for the PRJ_NAMESPACE_VIRTUALIZATION_CONTEXT returned
    /// by PrjStartVirtualizing. Guarantees PrjStopVirtualizing is called even
    /// during rude app domain unloads, Environment.Exit, or finalizer-only cleanup.
    /// </summary>
    /// <remarks>
    /// <para>
    /// SafeHandle is a CriticalFinalizerObject — the CLR guarantees its
    /// ReleaseHandle runs after all normal finalizers and during constrained
    /// execution regions. This provides the strongest possible guarantee that
    /// the ProjFS virtualization root is released, preventing zombie processes.
    /// </para>
    /// </remarks>
    internal sealed class SafeProjFsHandle : SafeHandleZeroOrMinusOneIsInvalid
    {
        /// <summary>
        /// Parameterless constructor required by P/Invoke marshaler for out-parameter usage.
        /// </summary>
        public SafeProjFsHandle() : base(ownsHandle: true) { }

        protected override bool ReleaseHandle()
        {
            // Must use the raw 'handle' field (IntPtr) here, not 'this'.
            // Inside ReleaseHandle, the SafeHandle is already marked as closed —
            // passing 'this' to a P/Invoke taking SafeProjFsHandle would fail
            // because the marshaler refuses to marshal a closed SafeHandle.
            ProjFSNative.PrjStopVirtualizing(handle);
            return true;
        }
    }
}
