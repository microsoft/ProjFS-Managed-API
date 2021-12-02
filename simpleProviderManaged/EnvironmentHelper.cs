using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SimpleProviderManaged
{
    public static class EnvironmentHelper
    {
        public static bool IsFullSymlinkSupportAvailable()
        {
            if (System.Runtime.InteropServices.RuntimeInformation.FrameworkDescription.StartsWith(".NET Core"))
            {
                // Using registry instead of OSVersion due to https://docs.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getversionexa?redirectedfrom=MSDN.
                // This code can be replaced with Environment.OSVersion on .NET Core 5 and higher.
                Microsoft.Win32.RegistryKey registryKey
                    = Microsoft.Win32.Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Microsoft\Windows NT\CurrentVersion");
                int build = Convert.ToInt32(registryKey.GetValue("CurrentBuild"));

                if (build >= 19041)
                {
                    return true;
                }
            }

            return false;
        }
    }
}
