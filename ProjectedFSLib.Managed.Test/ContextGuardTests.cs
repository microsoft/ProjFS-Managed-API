// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

using Microsoft.Windows.ProjFS;
using NUnit.Framework;
using System;

namespace ProjectedFSLib.Managed.Test
{
    /// <summary>
    /// Tests that methods requiring a running virtualization context fail with a clear
    /// <see cref="InvalidOperationException"/> when called before <c>StartVirtualizing</c>
    /// has established the native context, rather than throwing an opaque
    /// <see cref="NullReferenceException"/> from inside the P/Invoke marshaling stub.
    ///
    /// This guards the failure mode observed in VFS for Git telemetry, where a
    /// <c>StartVirtualizing</c> that threw under memory pressure (before assigning the
    /// context) left subsequent <c>CompleteCommand</c> / <c>DeleteFile</c> calls
    /// dereferencing a null SafeHandle. The resulting crash was reported against the
    /// native entry point (PrjCompleteCommand / PrjDeleteFile) and was indistinguishable
    /// from a genuine native access violation.
    ///
    /// These tests do not require the ProjFS optional feature to be enabled.
    /// </summary>
    public class ContextGuardTests
    {
        private static VirtualizationInstance CreateInstance()
        {
            return new VirtualizationInstance(
                "C:\\nonexistent",
                poolThreadCount: 0,
                concurrentThreadCount: 0,
                enableNegativePathCache: false,
                notificationMappings: new System.Collections.Generic.List<NotificationMapping>());
        }

        [Test]
        public void BeforeStartVirtualizing_ContextMethodsThrowInvalidOperationException()
        {
            using var instance = CreateInstance();

            // StartVirtualizing has never been called, so the native context is null.
            // Each of these must surface a clear InvalidOperationException instead of a
            // NullReferenceException from the marshaling stub.
            Assert.Throws<InvalidOperationException>(() =>
                instance.ClearNegativePathCache(out _));

            Assert.Throws<InvalidOperationException>(() =>
                instance.DeleteFile("test.txt", UpdateType.AllowDirtyMetadata, out _));

            Assert.Throws<InvalidOperationException>(() =>
                instance.WritePlaceholderInfo(
                    "test.txt", DateTime.Now, DateTime.Now, DateTime.Now, DateTime.Now,
                    System.IO.FileAttributes.Normal, 0, false, new byte[128], new byte[128]));

            Assert.Throws<InvalidOperationException>(() =>
                instance.UpdateFileIfNeeded(
                    "test.txt", DateTime.Now, DateTime.Now, DateTime.Now, DateTime.Now,
                    System.IO.FileAttributes.Normal, 0, new byte[128], new byte[128],
                    UpdateType.AllowDirtyMetadata, out _));

            Assert.Throws<InvalidOperationException>(() =>
                instance.CreateWriteBuffer(4096));

            Assert.Throws<InvalidOperationException>(() =>
                instance.CompleteCommand(0));

            Assert.Throws<InvalidOperationException>(() =>
                instance.CompleteCommand(0, HResult.Ok));
        }

        [Test]
        public void DisposedTakesPrecedenceOverMissingContext()
        {
            var instance = CreateInstance();
            instance.Dispose();

            // Once disposed, the disposed check must win: callers get ObjectDisposedException,
            // not InvalidOperationException, even though the context was never established.
            Assert.Throws<ObjectDisposedException>(() =>
                instance.CompleteCommand(0));

            Assert.Throws<ObjectDisposedException>(() =>
                instance.DeleteFile("test.txt", UpdateType.AllowDirtyMetadata, out _));
        }
    }
}
