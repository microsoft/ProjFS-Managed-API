using NUnit.Framework;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Threading;

namespace ProjectedFSLib.Managed.Test
{
    class Helpers
    {
        private Process m_providerProcess;
        public Process ProviderProcess { get => m_providerProcess; set => m_providerProcess = value; }

        private int m_waitTimeoutInMs;
        public int WaitTimeoutInMs { get => m_waitTimeoutInMs; set => m_waitTimeoutInMs = value; }

        internal enum NotifyWaitHandleNames
        {
            FileOpened,
            NewFileCreated,
            FileOverwritten,
            PreDelete,
            PreRename,
            PreCreateHardlink,
            FileRenamed,
            HardlinkCreated,
            FileHandleClosedNoModification,
            FileHandleClosedFileModifiedOrDeleted,
            FilePreConvertToFull,
        }

        private List<EventWaitHandle> notificationEvents;
        public List<EventWaitHandle> NotificationEvents { get => notificationEvents; }
        
        public Helpers(
            int waitTimeoutInMs
        )
        {
            m_waitTimeoutInMs = waitTimeoutInMs;

            // Create the events that the notifications tests use.
            notificationEvents = new List<EventWaitHandle>();
            foreach (string eventName in Enum.GetNames(typeof(NotifyWaitHandleNames)))
            {
                notificationEvents.Add(new EventWaitHandle(false, EventResetMode.AutoReset, eventName));
            }
        }

        public void StartTestProvider(string sourceRoot, string virtRoot)
        {
            // Get the provider name from the command line.
            var providerExe = TestContext.Parameters.Get("ProviderExe");

            // Create an event for the provider to signal once it is up and running.
            EventWaitHandle waitHandle = new EventWaitHandle(false, EventResetMode.AutoReset, "ProviderTestProceed");

            // Set up the provider process and start it.
            ProviderProcess = new Process();
            ProviderProcess.StartInfo.FileName = providerExe;

            string sourceArg = " --sourceroot " + sourceRoot;
            string virtRootArg = " --virtroot " + virtRoot;

            // Add the source and virtRoot arguments, as well as the "test mode" argument.
            ProviderProcess.StartInfo.Arguments = sourceArg + virtRootArg + " -t";
            ProviderProcess.StartInfo.UseShellExecute = true;

            ProviderProcess.Start();

            // Wait for the provider to signal the event.
            if (!waitHandle.WaitOne(WaitTimeoutInMs))
            {
                throw new Exception("SimpleProviderManaged did not signal the ProviderTestProceed event in a timely manner.");
            }
        }

        public void StopTestProvider()
        {
            ProviderProcess.CloseMainWindow();
        }

        // Makes name strings for the source and virtualization roots for a test, using the NUnit
        // WorkDirectory as the base.
        // 
        // Example: Given a test method called "TestStuff", if the test's WorkDirectory is C:\MyTestDir,
        // the output is:
        //      sourceName:     C:\MyTestDir\TestStuff_source
        //      virtRootName:   C:\MyTestDir\TestStuff_virtRoot
        //      
        // This method expects to be invoked while a test is running, either from the test case
        // itself or in a setup/teardown fixture for a test case.
        public void GetRootNamesForTest(out string sourceName, out string virtRootName)
        {
            string baseName = Path.Combine(
                TestContext.CurrentContext.WorkDirectory,
                TestContext.CurrentContext.Test.MethodName);

            sourceName = baseName + "_source";
            virtRootName = baseName + "_virtRoot";
        }

        public void CreateRootsForTest(out string sourceName, out string virtRootName)
        {
            GetRootNamesForTest(out sourceName, out virtRootName);

            DirectoryInfo sourceInfo = new DirectoryInfo(sourceName);
            if (!sourceInfo.Exists)
            {
                sourceInfo.Create();
            }

            // The provider create the virtualization root.
        }

        // Creates a file in the source so that it is projected into the virtualization root.
        // Returns the full path to the virtual file.
        public string CreateVirtualFile(string fileName, string fileContent)
        {
            GetRootNamesForTest(out string sourceRoot, out string virtRoot);

            string sourceFileName = Path.Combine(sourceRoot, fileName);
            FileInfo sourceFile = new FileInfo(sourceFileName);

            if (!sourceFile.Exists)
            {
                DirectoryInfo ancestorPath = new DirectoryInfo(Path.GetDirectoryName(sourceFile.FullName));
                if (!ancestorPath.Exists)
                {
                    ancestorPath.Create();
                }

                using (StreamWriter sw = sourceFile.CreateText())
                {
                    sw.Write(fileContent);
                }
            }

            // Tell our caller what the path to the virtual file is.
            return Path.Combine(virtRoot, fileName);
        }

        // Creates a file in the source so that it is projected into the virtualization root.
        // Returns the full path to the virtual file.
        public string CreateVirtualFile(string fileName)
        {
            return CreateVirtualFile(fileName, "Virtual");
        }

        // Create a file in the virtualization root (i.e. a non-projected or "full" file).
        // Returns the full path to the full file.
        public string CreateFullFile(string fileName, string fileContent)
        {
            GetRootNamesForTest(out string sourceRoot, out string virtRoot);

            string fullFileName = Path.Combine(virtRoot, fileName);
            FileInfo fullFile = new FileInfo(fullFileName);

            if (!fullFile.Exists)
            {
                DirectoryInfo ancestorPath = new DirectoryInfo(Path.GetDirectoryName(fullFile.FullName));
                if (!ancestorPath.Exists)
                {
                    ancestorPath.Create();
                }

                using (StreamWriter sw = fullFile.CreateText())
                {
                    sw.Write(fileContent);
                }
            }

            return fullFileName;
        }

        // Create a file in the virtualization root (i.e. a non-projected or "full" file).
        // Returns the full path to the full file.
        public string CreateFullFile(string fileName)
        {
            return CreateFullFile(fileName, "Full");
        }

        public string ReadFileInVirtRoot(string fileName)
        {
            GetRootNamesForTest(out string sourceRoot, out string virtRoot);

            string destFileName = Path.Combine(virtRoot, fileName);
            FileInfo destFile = new FileInfo(destFileName);
            string fileContent;
            using (StreamReader sr = destFile.OpenText())
            {
                fileContent = sr.ReadToEnd();
            }

            return fileContent;
        }

        public FileStream OpenFileInVirtRoot(string fileName, FileMode mode)
        {
            GetRootNamesForTest(out string sourceRoot, out string virtRoot);

            string destFileName = Path.Combine(virtRoot, fileName);
            FileStream stream = File.Open(destFileName, mode);

            return stream;
        }

        private Random random = new Random();
        public string RandomString(int length)
        {
            const string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
            return new string(Enumerable.Repeat(chars, length)
              .Select(s => s[random.Next(s.Length)]).ToArray());
        }
    }
}
