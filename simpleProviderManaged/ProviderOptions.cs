// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

using CommandLine;
using CommandLine.Text;
using System.Collections.Generic;

namespace SimpleProviderManaged
{
    public class ProviderOptions
    {
        [Option(Required = true, HelpText = "Path to the files and directories to project.")]
        public string SourceRoot { get; set; }

        [Option(Required = true, HelpText = "Path to the virtualization root.")]
        public string VirtRoot { get; set; }

        [Option('t', "testmode", HelpText = "Use this when running the provider with the test package.", Hidden = true)]
        public bool TestMode { get; set; }

        [Option('n', "notifications", HelpText = "Enable file system operation notifications.")]
        public bool EnableNotifications { get; set; }

        [Usage(ApplicationAlias = "SimpleProviderManaged")]
        public static IEnumerable<Example> Examples
        {
            get
            {
                return new List<Example>()
                {
                    new Example(
                        "Start provider, projecting files and directories from 'c:\\source' into 'c:\\virtRoot'",
                        new ProviderOptions { SourceRoot = "c:\\source", VirtRoot = "c:\\virtRoot" })
                };
            }
        }
    }
}
