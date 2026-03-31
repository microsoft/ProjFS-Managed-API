// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

using Microsoft.Windows.ProjFS;
using NUnit.Framework;
using System;

namespace ProjectedFSLib.Managed.Test
{
    /// <summary>
    /// Tests for VirtualizationInstance IDisposable implementation.
    /// These tests verify the dispose pattern mechanics without requiring
    /// the ProjFS optional feature to be enabled on the machine.
    /// </summary>
    public class DisposeTests
    {
        [Test]
        public void VirtualizationInstance_ImplementsIDisposable()
        {
            // VirtualizationInstance must implement IDisposable to prevent zombie processes.
            var instance = new VirtualizationInstance(
                "C:\\nonexistent",
                poolThreadCount: 0,
                concurrentThreadCount: 0,
                enableNegativePathCache: false,
                notificationMappings: new System.Collections.Generic.List<NotificationMapping>());

            Assert.That(instance, Is.InstanceOf<IDisposable>());
        }

        [Test]
        public void IVirtualizationInstance_ExtendsIDisposable()
        {
            // The interface itself must extend IDisposable so all implementations are required
            // to support disposal.
            Assert.That(typeof(IDisposable).IsAssignableFrom(typeof(IVirtualizationInstance)));
        }

        [Test]
        public void Dispose_CanBeCalledMultipleTimes()
        {
            var instance = new VirtualizationInstance(
                "C:\\nonexistent",
                poolThreadCount: 0,
                concurrentThreadCount: 0,
                enableNegativePathCache: false,
                notificationMappings: new System.Collections.Generic.List<NotificationMapping>());

            // Should not throw on any call.
            instance.Dispose();
            instance.Dispose();
            instance.Dispose();
        }

        [Test]
        public void StopVirtualizing_CanBeCalledMultipleTimes()
        {
            var instance = new VirtualizationInstance(
                "C:\\nonexistent",
                poolThreadCount: 0,
                concurrentThreadCount: 0,
                enableNegativePathCache: false,
                notificationMappings: new System.Collections.Generic.List<NotificationMapping>());

            // Should not throw on any call.
            instance.StopVirtualizing();
            instance.StopVirtualizing();
        }

        [Test]
        public void StopVirtualizing_ThenDispose_DoesNotThrow()
        {
            var instance = new VirtualizationInstance(
                "C:\\nonexistent",
                poolThreadCount: 0,
                concurrentThreadCount: 0,
                enableNegativePathCache: false,
                notificationMappings: new System.Collections.Generic.List<NotificationMapping>());

            instance.StopVirtualizing();
            instance.Dispose();
        }

        [Test]
        public void AfterDispose_MethodsThrowObjectDisposedException()
        {
            var instance = new VirtualizationInstance(
                "C:\\nonexistent",
                poolThreadCount: 0,
                concurrentThreadCount: 0,
                enableNegativePathCache: false,
                notificationMappings: new System.Collections.Generic.List<NotificationMapping>());

            instance.Dispose();

            Assert.Throws<ObjectDisposedException>(() =>
                instance.ClearNegativePathCache(out _));

            Assert.Throws<ObjectDisposedException>(() =>
                instance.DeleteFile("test.txt", UpdateType.AllowDirtyMetadata, out _));

            Assert.Throws<ObjectDisposedException>(() =>
                instance.WritePlaceholderInfo(
                    "test.txt", DateTime.Now, DateTime.Now, DateTime.Now, DateTime.Now,
                    System.IO.FileAttributes.Normal, 0, false, new byte[128], new byte[128]));

            Assert.Throws<ObjectDisposedException>(() =>
                instance.CreateWriteBuffer(4096));

            Assert.Throws<ObjectDisposedException>(() =>
                instance.CompleteCommand(0));

            Assert.Throws<ObjectDisposedException>(() =>
                instance.StartVirtualizing(null!));
        }

        [Test]
        public void AfterStopVirtualizing_MethodsThrowObjectDisposedException()
        {
            var instance = new VirtualizationInstance(
                "C:\\nonexistent",
                poolThreadCount: 0,
                concurrentThreadCount: 0,
                enableNegativePathCache: false,
                notificationMappings: new System.Collections.Generic.List<NotificationMapping>());

            instance.StopVirtualizing();

            // StopVirtualizing should have the same effect as Dispose.
            Assert.Throws<ObjectDisposedException>(() =>
                instance.ClearNegativePathCache(out _));

            Assert.Throws<ObjectDisposedException>(() =>
                instance.CreateWriteBuffer(4096));
        }

        [Test]
        public void UsingStatement_DisposesAutomatically()
        {
            VirtualizationInstance instance;
            using (instance = new VirtualizationInstance(
                "C:\\nonexistent",
                poolThreadCount: 0,
                concurrentThreadCount: 0,
                enableNegativePathCache: false,
                notificationMappings: new System.Collections.Generic.List<NotificationMapping>()))
            {
                // Instance is alive here.
            }

            // After using block, instance should be disposed.
            Assert.Throws<ObjectDisposedException>(() =>
                instance.ClearNegativePathCache(out _));
        }
    }
}
