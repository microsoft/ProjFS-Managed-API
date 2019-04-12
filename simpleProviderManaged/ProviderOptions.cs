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

        [Option('n', "notifications", HelpText = "Enable file system operation notifications.")]
        public bool EnableNotifications { get; set; }

        // Used by ProjectedFSLib.Managed.Test.BasicTests.
        [Option('t', "testmode", HelpText = "Use this when running the provider with the test package.", Hidden = true)]
        public bool TestMode { get; set; }

        // Use in conjunction with the EnableNotifications option.  When set the provider will not
        // allow deletes to happen beneath the virtualization root.
        [Option('d', "denyDeletes", HelpText = "Deny deletes.", Hidden = true)]
        public bool DenyDeletes { get; set; }

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
