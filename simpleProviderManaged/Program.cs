// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

using CommandLine;
using Serilog;
using System;

namespace SimpleProviderManaged
{
    public class Program
    {
        public static void Main(string[] args)
        {
            // We want verbose logging so we can see all our callback invocations.
            Log.Logger = new LoggerConfiguration()
                .WriteTo.Console()
                .WriteTo.File("SimpleProviderManaged-.log", rollingInterval: RollingInterval.Day)
                .CreateLogger();

            Log.Information("Start");

            // Parse the command-line options and then start the SimpleProvider.
            var parserResult = Parser.Default
                .ParseArguments<ProviderOptions>(args)
                .WithParsed((ProviderOptions options) => { Run(options); });

            Log.Information("Exit");
        }

        private static void Run(ProviderOptions options)
        {
            SimpleProvider provider;
            try
            {
                provider = new SimpleProvider(options);
            }
            catch (Exception ex)
            {
                Log.Fatal(ex, "Failed to create SimpleProvider.");
                throw;
            }

            Log.Information("Starting provider");

            if (!provider.StartVirtualization())
            {
                Log.Error("Could not start provider.");
                Environment.Exit(1);
            }

            Console.WriteLine("Provider is running.  Press Enter to exit.");
            Console.ReadLine();
        }
    }
}
