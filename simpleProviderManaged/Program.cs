using CommandLine;
using Serilog;
using System;

namespace SimpleProviderManaged
{
    public class Program
    {
        public static void Main(string[] args)
        {
            Log.Logger = new LoggerConfiguration()
                .WriteTo.Console()
                .WriteTo.File("SimpleProviderManaged-.txt", rollingInterval: RollingInterval.Day)
                .CreateLogger();

            Log.Information("Start");

            var parserResult = Parser.Default
                .ParseArguments<ProviderOptions>(args)
                .WithParsed((ProviderOptions options) => { Run(options); });

            Log.Information("Exit");
        }

        private static void Run(ProviderOptions options)
        {
            SimpleProvider provider = new SimpleProvider(options);

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
