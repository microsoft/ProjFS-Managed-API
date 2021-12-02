// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

using NUnit.Framework;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

namespace ProjectedFSLib.Managed.Test
{
    // Set of basic tests to exercise the entry points in the managed code API wrapper.
    //
    // The Microsoft.Windows.ProjFS managed API is a fairly thin wrapper around a set of native
    // APIs for ProjFS.  The native API functionality has its own tests that are routinely executed
    // at Microsoft in the normal course of OS development.
    //
    // The tests in this module are meant to ensure the managed wrapper APIs get coverage.  They 
    // kick off a separate provider process, set up some file system state in the source root, then
    // perform actions in the virtualization root and check whether the results are expected.
    // 
    // Note that these tests are not as comprehensive as the native API tests are.  They also don't
    // have access to a couple of private APIs that the native tests do, in particular one that allows
    // a provider process to receive notifications caused by its own I/O.  Normally ProjFS does not
    // send notifications to a provider of I/O that the provider itself performs, to avoid deadlocks
    // and loops in an unsuspecting provider.  Hence the necessity of the separate provider process
    // in these tests.
    public class BasicTests
    {
        private Helpers helpers;

        [OneTimeSetUp]
        public void ClassSetup()
        {
            // Default timeout for wait handles is 10 seconds
            helpers = new Helpers(10 * 1000);
        }

        [SetUp]
        public void TestSetup()
        {
            helpers.CreateRootsForTest(
                out string sourceRoot,
                out string virtRoot);

            // We defer starting the provider to the test case so that it can specify extra options
            // if it wants to.
        }

        // We stop the virtualization instance in the TearDown fixture, so that exercises the following
        // methods in Microsoft.Windows.ProjFS:
        //  VirtualizationInstance.StopVirtualizing()
        [TearDown]
        public void TestTeardown()
        {
            helpers.GetRootNamesForTest(out string sourceRoot, out string virtRoot);

            // Recursively delete the source root directory.
            try
            {
                DirectoryInfo sourceInfo = new DirectoryInfo(sourceRoot);
                sourceInfo.Delete(true);
            }
            catch (IOException ex)
            {
                Console.Error.WriteLine("Deleting sourceroot {0}: {1}", sourceRoot, ex.Message);
            }

            // Recursively delete the virtualization root directory.
            // 
            // Note that we haven't yet shut down the provider.  That means that everything will get
            // hydrated just before being deleted.  I did this because DirectoryInfo.Delete() doesn't
            // take into account reparse points when it recursively deletes, unlike e.g. DOS 'rmdir /s /q'.
            // So without crafting a custom recursive deleter that works like rmdir, this would throw
            // an IOException with the error ERROR_FILE_SYSTEM_VIRTUALIZATION_UNAVAILABLE if the provider
            // were not running.
            try
            {
                DirectoryInfo virtInfo = new DirectoryInfo(virtRoot);
                virtInfo.Delete(true);
            }
            catch (IOException ex)
            {
                Console.Error.WriteLine("Deleting virtroot {0} (sourceroot {1}): {2}", virtRoot, sourceRoot, ex.Message);
            }

            // Shut down the provider app.
            helpers.StopTestProvider();
        }

        // We start the virtualization instance in each test case, so that exercises the following
        // methods in Microsoft.Windows.ProjFS:
        //  VirtualizationInstance.VirtualizationInstance()
        //  VirtualizationInstance.MarkDirectoryAsVirtualizationRoot()
        //  VirtualizationInstance.StartVirtualizing()

        // This case exercises the following methods in Microsoft.Windows.ProjFS:
        //  VirtualizationInstance.WritePlaceholderInfo()
        //  VirtualizationInstance.CreateWriteBuffer()
        //  VirtualizationInstance.WriteFileData()
        //  
        // It also illustrates the SimpleProvider implementation of the following callbacks:
        //  IRequiredCallbacks.GetPlaceholderInfoCallback()
        //  IRequiredCallbakcs.GetFileDataCallback()
        [TestCase("foo.txt")]
        [TestCase("dir1\\dir2\\dir3\\bar.txt")]
        public void TestCanReadThroughVirtualizationRoot(string destinationFile)
        {
            helpers.StartTestProvider(out string sourceRoot, out string virtRoot);

            // Some contents to write to the file in the source and read out through the virtualization.
            string fileContent = nameof(TestCanReadThroughVirtualizationRoot);

            // Create a file that we can read through the virtualization root.
            helpers.CreateVirtualFile(destinationFile, fileContent);

            // Open the file through the virtualization and read its contents.
            string line = helpers.ReadFileInVirtRoot(destinationFile);

            Assert.That(fileContent, Is.EqualTo(line));
            Assert.That("RandomNonsense", Is.Not.EqualTo(line));
        }

        // We start the virtualization instance in each test case, so that exercises the following
        // methods in Microsoft.Windows.ProjFS:
        //  VirtualizationInstance.VirtualizationInstance()
        //  VirtualizationInstance.MarkDirectoryAsVirtualizationRoot()
        //  VirtualizationInstance.StartVirtualizing()

        // This case exercises the following methods in Microsoft.Windows.ProjFS:
        //  VirtualizationInstance.WritePlaceholderInfo2()
        //  VirtualizationInstance.CreateWriteBuffer()
        //  VirtualizationInstance.WriteFileData()
        //  DirectoryEnumerationResults.Add()
        //  
        // It also illustrates the SimpleProvider implementation of the following callbacks:
        //  IRequiredCallbacks.GetPlaceholderInfoCallback()
        //  IRequiredCallbakcs.GetFileDataCallback()
        //  IRequiredCallbacks.StartDirectoryEnumeration()
        //  IRequiredCallbacks.GetDirectoryEnumeration()
        //  IRequiredCallbacks.EndDirectoryEnumeration()
        [TestCase("sourcefoo.txt", "symfoo.txt", true)]
        [TestCase("sourcefoo.txt", "symfoo.txt", false)]
        [TestCase("dir1\\sourcebar.txt", "dir4\\symbar.txt", true)]
        public void TestCanReadSymlinksThroughVirtualizationRoot(string destinationFile, string symlinkFile, bool useRootedPaths)
        {
            helpers.StartTestProvider(out string sourceRoot, out string virtRoot);
            // Some contents to write to the file in the source and read out through the virtualization.
            string fileContent = nameof(TestCanReadSymlinksThroughVirtualizationRoot);

            // Create a file and a symlink to it.
            helpers.CreateVirtualFile(destinationFile, fileContent);
            helpers.CreateVirtualSymlink(symlinkFile, destinationFile, useRootedPaths);

            // Open the file through the virtualization and read its contents.
            string line = helpers.ReadFileInVirtRoot(destinationFile);
            Assert.That(fileContent, Is.EqualTo(line));

            // Enumerate and ensure the symlink is present.
            var pathToEnumerate = Path.Combine(virtRoot, Path.GetDirectoryName(symlinkFile));
            DirectoryInfo virtDirInfo = new DirectoryInfo(pathToEnumerate);
            List<FileSystemInfo> virtList = new List<FileSystemInfo>(virtDirInfo.EnumerateFileSystemInfos("*", SearchOption.AllDirectories));
            string fullPath = Path.Combine(virtRoot, symlinkFile);
            FileSystemInfo symlink = virtList.Where(x => x.FullName == fullPath).First();
            Assert.That((symlink.Attributes & FileAttributes.ReparsePoint) != 0);

            // Get the symlink target and check that it points to the correct file.
            string reparsePointTarget = helpers.ReadReparsePointTargetInVirtualRoot(symlinkFile);
            string expectedTarget = useRootedPaths ? Path.Combine(virtRoot, destinationFile) : destinationFile;
            Assert.That(reparsePointTarget, Is.EqualTo(expectedTarget));

            // Check if we have the same content if accessing the file through a symlink.
            string lineAccessedThroughSymlink = helpers.ReadFileInVirtRoot(symlinkFile);
            Assert.That(fileContent, Is.EqualTo(lineAccessedThroughSymlink));
        }

        // We start the virtualization instance in each test case, so that exercises the following
        // methods in Microsoft.Windows.ProjFS:
        //  VirtualizationInstance.VirtualizationInstance()
        //  VirtualizationInstance.MarkDirectoryAsVirtualizationRoot()
        //  VirtualizationInstance.StartVirtualizing()

        // This case exercises the following methods in Microsoft.Windows.ProjFS:
        //  VirtualizationInstance.WritePlaceholderInfo2()
        //  VirtualizationInstance.CreateWriteBuffer()
        //  VirtualizationInstance.WriteFileData()
        //  DirectoryEnumerationResults.Add()
        //  
        // It also illustrates the SimpleProvider implementation of the following callbacks:
        //  IRequiredCallbacks.GetPlaceholderInfoCallback()
        //  IRequiredCallbakcs.GetFileDataCallback()
        //  IRequiredCallbacks.StartDirectoryEnumeration()
        //  IRequiredCallbacks.GetDirectoryEnumeration()
        //  IRequiredCallbacks.EndDirectoryEnumeration()
        [TestCase("dir1\\dir2\\dir3\\sourcebar.txt", "dir4\\dir5\\dir6\\symbar.txt", "..\\..\\..\\dir1\\dir2\\dir3\\sourcebar.txt")]
        public void TestCanReadSymlinksWithRelativePathTargetsThroughVirtualizationRoot(string destinationFile, string symlinkFile, string symlinkTarget)
        {
            helpers.StartTestProvider(out string sourceRoot, out string virtRoot);
            // Some contents to write to the file in the source and read out through the virtualization.
            string fileContent = nameof(TestCanReadSymlinksThroughVirtualizationRoot);

            // Create a file and a symlink to it.
            helpers.CreateVirtualFile(destinationFile, fileContent);
            helpers.CreateVirtualSymlink(symlinkFile, symlinkTarget, false);

            // Open the file through the virtualization and read its contents.
            string line = helpers.ReadFileInVirtRoot(destinationFile);
            Assert.That(fileContent, Is.EqualTo(line));

            // Enumerate and ensure the symlink is present.
            var pathToEnumerate = Path.Combine(virtRoot, Path.GetDirectoryName(symlinkFile));
            DirectoryInfo virtDirInfo = new DirectoryInfo(pathToEnumerate);
            List<FileSystemInfo> virtList = new List<FileSystemInfo>(virtDirInfo.EnumerateFileSystemInfos("*", SearchOption.AllDirectories));
            string fullPath = Path.Combine(virtRoot, symlinkFile);
            FileSystemInfo symlink = virtList.Where(x => x.FullName == fullPath).First();
            Assert.That((symlink.Attributes & FileAttributes.ReparsePoint) != 0);

            // Get the symlink target and check that it points to the correct file.
            string reparsePointTarget = helpers.ReadReparsePointTargetInVirtualRoot(symlinkFile);
            Assert.That(reparsePointTarget, Is.EqualTo(symlinkTarget));

            // Check if we have the same content if accessing the file through a symlink.
            string lineAccessedThroughSymlink = helpers.ReadFileInVirtRoot(symlinkFile);
            Assert.That(fileContent, Is.EqualTo(lineAccessedThroughSymlink));
        }

        // We start the virtualization instance in each test case, so that exercises the following
        // methods in Microsoft.Windows.ProjFS:
        //  VirtualizationInstance.VirtualizationInstance()
        //  VirtualizationInstance.MarkDirectoryAsVirtualizationRoot()
        //  VirtualizationInstance.StartVirtualizing()

        // This case exercises the following methods in Microsoft.Windows.ProjFS:
        //  VirtualizationInstance.WritePlaceholderInfo2()
        //  VirtualizationInstance.CreateWriteBuffer()
        //  VirtualizationInstance.WriteFileData()
        //  DirectoryEnumerationResults.Add()
        //  
        // It also illustrates the SimpleProvider implementation of the following callbacks:
        //  IRequiredCallbacks.GetPlaceholderInfoCallback()
        //  IRequiredCallbakcs.GetFileDataCallback()
        //  IRequiredCallbacks.StartDirectoryEnumeration()
        //  IRequiredCallbacks.GetDirectoryEnumeration()
        //  IRequiredCallbacks.EndDirectoryEnumeration()
        [TestCase("dir1\\dir2\\dir3\\", "file.txt", "dir4\\dir5\\sdir6")]
        public void TestCanReadSymlinkDirsThroughVirtualizationRoot(string destinationDir, string destinationFileName, string symlinkDir)
        {
            helpers.StartTestProvider(out string sourceRoot, out string virtRoot);

            // Some contents to write to the file in the source and read out through the virtualization.
            string fileContent = nameof(TestCanReadSymlinkDirsThroughVirtualizationRoot);

            string destinationFile = Path.Combine(destinationDir, destinationFileName);
            helpers.CreateVirtualFile(destinationFile, fileContent);
            helpers.CreateVirtualSymlinkDirectory(symlinkDir, destinationDir, true);

            // Enumerate and ensure the symlink is present.
            var pathToEnumerate = Path.Combine(virtRoot, Path.GetDirectoryName(symlinkDir));
            DirectoryInfo virtDirInfo = new DirectoryInfo(pathToEnumerate);
            List<FileSystemInfo> virtList = new List<FileSystemInfo>(virtDirInfo.EnumerateFileSystemInfos("*", SearchOption.AllDirectories));
            string fullPath = Path.Combine(virtRoot, symlinkDir);

            // Ensure we can access the file through directory symlink.
            string symlinkFile = Path.Combine(virtRoot, symlinkDir, destinationFileName);
            string lineAccessedThroughSymlink = helpers.ReadFileInVirtRoot(symlinkFile);
            Assert.That(fileContent, Is.EqualTo(lineAccessedThroughSymlink));
        }

        // We start the virtualization instance in each test case, so that exercises the following
        // methods in Microsoft.Windows.ProjFS:
        //  VirtualizationInstance.VirtualizationInstance()
        //  VirtualizationInstance.MarkDirectoryAsVirtualizationRoot()
        //  VirtualizationInstance.StartVirtualizing()

        // This case exercises the following methods in Microsoft.Windows.ProjFS:
        //  VirtualizationInstance.WritePlaceholderInfo2()
        //  VirtualizationInstance.CreateWriteBuffer()
        //  VirtualizationInstance.WriteFileData()
        //  DirectoryEnumerationResults.Add()
        //  
        // It also illustrates the SimpleProvider implementation of the following callbacks:
        //  IRequiredCallbacks.GetPlaceholderInfoCallback()
        //  IRequiredCallbakcs.GetFileDataCallback()
        [Test]
        public void TestCanReadSymlinkFilesAndirsThroughVirtualizationRoot()
        {
            helpers.StartTestProvider(out string sourceRoot, out string virtRoot);

            //|-- Dir.lnk->Dir
            //|-- other.txt
            //|-- Dir
            //|   -- file.lnk->file.txt
            //|   -- file.txt
            //|   -- other.lnk->..\\other.txt
            //|-- file.lnk->Dir.lnk\file.lnk
            string fileContent = nameof(TestCanReadSymlinkFilesAndirsThroughVirtualizationRoot);
            string otherFileContent = nameof(TestCanReadSymlinkFilesAndirsThroughVirtualizationRoot) + "_other";

            string destinationDir = "Dir";
            string destinationFile = Path.Combine(destinationDir, "file.txt");
            string otherDestinationFile = "other.txt";

            string fileSymlinkInDestinationDir = Path.Combine(destinationDir, "file.lnk");
            string dirSymlink = "Dir.lnk";
            string otherFileSymlink = Path.Combine(destinationDir, "other.lnk");
            string fileSymlinkInDirSymlink = Path.Combine(dirSymlink, "file.lnk.lnk");

            helpers.CreateVirtualFile(destinationFile, fileContent);
            helpers.CreateVirtualFile(otherDestinationFile, otherFileContent);

            helpers.CreateVirtualSymlinkDirectory(dirSymlink, destinationDir, true);
            helpers.CreateVirtualSymlink(fileSymlinkInDestinationDir, destinationFile, true);
            helpers.CreateVirtualSymlink(otherFileSymlink, "..\\other.txt", false);
            helpers.CreateVirtualSymlink(fileSymlinkInDirSymlink, fileSymlinkInDestinationDir, true);

            // Ensure we can access the file through directory and file symlink.
            string symlinkFile = Path.Combine(virtRoot, fileSymlinkInDirSymlink);
            string lineAccessedThroughSymlink = helpers.ReadFileInVirtRoot(symlinkFile);
            Assert.That(fileContent, Is.EqualTo(lineAccessedThroughSymlink));

            string otherSymlinkFile = Path.Combine(virtRoot, otherFileSymlink);
            string lineAccessedThroughOtherSymlink = helpers.ReadFileInVirtRoot(otherSymlinkFile);
            Assert.That(otherFileContent, Is.EqualTo(lineAccessedThroughOtherSymlink));
        }

        // This case exercises the following methods in Microsoft.Windows.ProjFS:
        //  DirectoryEnumerationResults.Add()
        //  
        // It also illustrates the SimpleProvider implementation of the following callbacks:
        //  IRequiredCallbacks.StartDirectoryEnumeration()
        //  IRequiredCallbacks.GetDirectoryEnumeration()
        //  IRequiredCallbacks.EndDirectoryEnumeration()
        [Test]
        public void TestEnumerationInVirtualizationRoot()
        {
            helpers.StartTestProvider(out string sourceRoot, out string virtRoot);

            Random random = new Random();

            // Generate some randomly-named directories in the source.
            for (int i = 1; i < 10; i++)
            {
                string dirName = Path.Combine(sourceRoot, helpers.RandomString(10) + i);
                DirectoryInfo dirInfo = new DirectoryInfo(dirName);
                dirInfo.Create();

                // Make the time stamps something other than "now".
                FileSystemInfo fsInfo = dirInfo as FileSystemInfo;
                fsInfo.CreationTime = fsInfo.CreationTime
                    .AddDays(-random.Next(1, 30))
                    .AddHours(random.Next(0,23))
                    .AddMinutes(random.Next(0, 59));
                fsInfo.LastAccessTime = fsInfo.LastAccessTime
                    .AddDays(-random.Next(1, 30))
                    .AddHours(random.Next(0, 23))
                    .AddMinutes(random.Next(0, 59));
                fsInfo.LastWriteTime = fsInfo.LastWriteTime
                    .AddDays(-random.Next(1, 30))
                    .AddHours(random.Next(0, 23))
                    .AddMinutes(random.Next(0, 59));
            }

            // Generate some randomly-named files with random sizes in the source.
            for (int i = 1; i < 10; i++)
            {
                string fileName = Path.Combine(sourceRoot, helpers.RandomString(10) + i);
                FileInfo fileInfo = new FileInfo(fileName);
                using (FileStream fs = fileInfo.OpenWrite())
                {
                    Byte[] contents =
                        new UTF8Encoding(true).GetBytes(helpers.RandomString(random.Next(10, 100)));

                    fs.Write(contents, 0, contents.Length);
                }

                // Make the time stamps something other than "now".
                FileSystemInfo fsInfo = fileInfo as FileSystemInfo;
                fsInfo.CreationTime = fsInfo.CreationTime
                    .AddDays(-random.Next(1, 30))
                    .AddHours(random.Next(0, 23))
                    .AddMinutes(random.Next(0, 59));
                fsInfo.LastAccessTime = fsInfo.LastAccessTime
                    .AddDays(-random.Next(1, 30))
                    .AddHours(random.Next(0, 23))
                    .AddMinutes(random.Next(0, 59));
                fsInfo.LastWriteTime = fsInfo.LastWriteTime
                    .AddDays(-random.Next(1, 30))
                    .AddHours(random.Next(0, 23))
                    .AddMinutes(random.Next(0, 59));
            }

            // Enumerate the source to build a list of its contents.
            DirectoryInfo sourceDirInfo = new DirectoryInfo(sourceRoot);
            List<FileSystemInfo> sourceList = new List<FileSystemInfo>(sourceDirInfo.EnumerateFileSystemInfos());

            // Now enumerate the virtualization root.
            DirectoryInfo virtDirInfo = new DirectoryInfo(virtRoot);
            List<FileSystemInfo> virtList = new List<FileSystemInfo>(virtDirInfo.EnumerateFileSystemInfos());

            // Compare the enumerations.  They should be the same.
            Assert.That(sourceList.Count, Is.EqualTo(virtList.Count));

            for (int i = 0; i < sourceList.Count; i++)
            {
                Assert.That(sourceList[i].Name, Is.EqualTo(virtList[i].Name), "Name");
                Assert.That(sourceList[i].CreationTime, Is.EqualTo(virtList[i].CreationTime), "CreationTime");
                Assert.That(sourceList[i].LastAccessTime, Is.EqualTo(virtList[i].LastAccessTime), "LastAccessTime");
                Assert.That(sourceList[i].LastWriteTime, Is.EqualTo(virtList[i].LastWriteTime), "LastWriteTime");

                bool isSourceDirectory = (sourceList[i].Attributes & FileAttributes.Directory) == FileAttributes.Directory;
                bool isVirtDirectory = (virtList[i].Attributes & FileAttributes.Directory) == FileAttributes.Directory;
                Assert.That(isSourceDirectory, Is.EqualTo(isVirtDirectory), "IsDirectory");

                if (!isSourceDirectory)
                {
                    FileInfo sourceInfo = sourceList[i] as FileInfo;
                    FileInfo virtInfo = virtList[i] as FileInfo;

                    Assert.That(sourceInfo.Length, Is.EqualTo(virtInfo.Length), "Length");
                }
            }
        }

        [Test]
        public void TestNotificationFileOpened()
        {
            helpers.StartTestProvider();

            string fileName = "file.txt";

            // Create the virtual file.
            helpers.CreateVirtualFile(fileName);

            // Open the file to trigger the notification in the provider.
            helpers.ReadFileInVirtRoot(fileName);

            // Wait for the provider to signal that it processed the FileOpened and
            // FileHandleClosedNoModification notifications.
            Assert.That(helpers.NotificationEvents[(int)Helpers.NotifyWaitHandleNames.FileOpened].WaitOne(helpers.WaitTimeoutInMs));
            Assert.That(helpers.NotificationEvents[(int)Helpers.NotifyWaitHandleNames.FileHandleClosedNoModification].WaitOne(helpers.WaitTimeoutInMs));
        }

        [Test]
        public void TestNotificationNewFileCreated()
        {
            helpers.StartTestProvider();

            string fileName = "newfile.txt";

            // Create a new file in the virtualization root.
            helpers.CreateFullFile(fileName);

            // Wait for the provider to signal that it processed the NewFileCreated notification.
            Assert.That(helpers.NotificationEvents[(int)Helpers.NotifyWaitHandleNames.NewFileCreated].WaitOne(helpers.WaitTimeoutInMs));
        }

        [Test]
        public void TestNotificationFileOverwritten()
        {
            helpers.StartTestProvider();

            string fileName = "overwriteme.txt";

            // Create a virtual file.
            helpers.CreateVirtualFile(fileName, "Old content");

            using (FileStream fs = helpers.OpenFileInVirtRoot(fileName, FileMode.Create))
            {

            }

            // Wait for the provider to signal that it processed the FileOverwritten notification.
            Assert.That(helpers.NotificationEvents[(int)Helpers.NotifyWaitHandleNames.FileOverwritten].WaitOne(helpers.WaitTimeoutInMs));
        }

        [Test]
        public void TestNotificationDelete()
        {
            helpers.StartTestProvider();

            string fileName = "deleteme.txt";

            // Create a virtual file.
            string filePath = helpers.CreateVirtualFile(fileName);

            // Delete the file.
            FileInfo fileInfo = new FileInfo(filePath);
            fileInfo.Delete();

            // Wait for the provider to signal that it processed the PreDelete and
            // FileHandleClosedFileModifiedOrDeleted notifications.
            Assert.That(helpers.NotificationEvents[(int)Helpers.NotifyWaitHandleNames.PreDelete].WaitOne(helpers.WaitTimeoutInMs));
            Assert.That(helpers.NotificationEvents[(int)Helpers.NotifyWaitHandleNames.FileHandleClosedFileModifiedOrDeleted].WaitOne(helpers.WaitTimeoutInMs));
        }

        [Test]
        public void TestNotificationRename()
        {
            helpers.StartTestProvider();

            string fileName = "OldName.txt";

            // Create a virtual file.
            string filePath = helpers.CreateVirtualFile(fileName);

            // Rename the file.
            string directory = Path.GetDirectoryName(filePath);
            string newFileName = "NewName.txt";
            string newFilePath = Path.Combine(directory, newFileName);
            File.Move(filePath, newFilePath);

            // Wait for the provider to signal that it processed the PreRename and FileRenamed notifications.
            Assert.That(helpers.NotificationEvents[(int)Helpers.NotifyWaitHandleNames.PreRename].WaitOne(helpers.WaitTimeoutInMs));
            Assert.That(helpers.NotificationEvents[(int)Helpers.NotifyWaitHandleNames.FileRenamed].WaitOne(helpers.WaitTimeoutInMs));
        }

        [DllImport("Kernel32.dll", CharSet = CharSet.Unicode)]
        static extern bool CreateHardLink(
          string lpFileName,
          string lpExistingFileName,
          IntPtr lpSecurityAttributes
          );

        [Test]
        public void TestNotificationHardLink()
        {
            helpers.StartTestProvider();

            string fileName = "linkTarget.txt";

            // Create a virtual file.
            string filePath = helpers.CreateVirtualFile(fileName);

            // Create a hard link to the virtual file.
            string directory = Path.GetDirectoryName(filePath);
            string linkName = "link.txt";
            string linkPath = Path.Combine(directory, linkName);
            Assert.That(CreateHardLink(linkPath, filePath, IntPtr.Zero));

            // Wait for the provider to signal that it processed the PreCreateHardlink and HardlinkCreated
            // notifications.
            Assert.That(helpers.NotificationEvents[(int)Helpers.NotifyWaitHandleNames.PreCreateHardlink].WaitOne(helpers.WaitTimeoutInMs));
            Assert.That(helpers.NotificationEvents[(int)Helpers.NotifyWaitHandleNames.HardlinkCreated].WaitOne(helpers.WaitTimeoutInMs));
        }

        [Test]
        public void TestConvertToFull()
        {
            helpers.StartTestProvider();

            string fileName = "fileToWriteTo.txt";

            // Create a virtual file.
            string filePath = helpers.CreateVirtualFile(fileName, "Original content");

            // Write to the file.
            using (StreamWriter sw = File.AppendText(filePath))
            {
                sw.WriteLine("Some new content!");
            }

            // Wait for the provider to signal that it processed the FilePreConvertToFull notification.
            Assert.That(helpers.NotificationEvents[(int)Helpers.NotifyWaitHandleNames.FilePreConvertToFull].WaitOne(helpers.WaitTimeoutInMs));
        }
    }
}