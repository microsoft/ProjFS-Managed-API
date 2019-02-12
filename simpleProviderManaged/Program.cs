using CommandLine;
using System;
using System.Collections.Generic;
using System.Linq;

namespace SimpleProviderManaged
{
    public class Program
    {
        public static void Main(string[] args)
        {
            var parserResult = Parser.Default
                .ParseArguments<ProviderOptions>(args)
                .WithParsed((ProviderOptions options) => { Run(options); });
        }

        private static void Run(ProviderOptions options)
        {
            SimpleProvider provider = new SimpleProvider(options);

            if (!provider.StartVirtualization())
            {
                Console.Error.WriteLine("Could not start provider.");
                Environment.Exit(1);
            }

            Console.WriteLine("Provider is running.  Press Enter to exit.");
            Console.ReadLine();
        }
    }
}
