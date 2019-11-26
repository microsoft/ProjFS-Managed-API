// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

using CommandLine;
using Serilog;
using System;
using System.Threading;

namespace SimpleProviderManaged
{
    public class Program
    {
        private enum ReturnCode
        {
            Success = 0,
            InvalidArguments = 1,
            GeneralException = 2,
        }
        
        public static int Main(string[] args)
        {
            try
            {
                // We want verbose logging so we can see all our callback invocations.
                Log.Logger = new LoggerConfiguration()
                    .WriteTo.Console()
                    .WriteTo.File("SimpleProviderManaged-.log", rollingInterval: RollingInterval.Day)
                    .CreateLogger();

                Log.Information("Start");

                var parserResult = Parser.Default
                    .ParseArguments<ProviderOptions>(args)
                    .WithParsed((ProviderOptions options) => Run(options));

                Log.Information("Exit successfully");
                return (int) ReturnCode.Success;
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine($"Unexpected exception: {ex}");
                Thread.Sleep(5 * 1000);
                return (int)ReturnCode.GeneralException;
            }
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
