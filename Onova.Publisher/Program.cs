using System;
using System.IO;
using System.CommandLine;
using System.CommandLine.Invocation;
using System.Text;
using System.Runtime.InteropServices;

namespace Onova.Publisher
{
    class Program
    {
        static int Main(string[] args)
        {
            Console.OutputEncoding = Encoding.Default;

            var rootCommand = new RootCommand
            {
                new Option<string>(
                    new [] { "--name", "-n" },
                    "Your application's name (name of executable without extension). Maximum 31 characters.")
                { IsRequired = false },

                new Option<string>(
                    new [] { "--version", "-v" },
                    "Version in format major.minor[.build[.revision]].")
                { IsRequired = false },

                new Option<string>(
                    new [] { "--url", "-u" },
                    "URL to web where the manifest resides. Maximum 1023 characters.")
                { IsRequired = false },

                new Option<string>(
                    new [] { "--target", "--in", "-i" },
                    "Folder which will get packed into a zip.")
                { IsRequired = false },

                new Option<string>(
                    new [] {"--output", "--out", "-o" },
                    () => ".\\" + PublisherConstant.OutputFolder,
                    "Folder which will contain the updated manifest file, zip and installer.")
                { IsRequired = false },

                new Option<bool>(
                    new [] {"--no-releasenotes", "--no-rn"},
                    "Disables generation of an empty release note (.rn) file.")
                { IsRequired = false },

                new Option<string>(
                    new [] {"--sign", "-s"},
                    "Sign AppName.exe/.dll files and the installer. This field accepts SignTool parameters.")
                { IsRequired = false }
            };

            rootCommand.Description = "Publishes your application for Onova.";
            rootCommand.Handler = CommandHandler.Create<string, string, string, string, string, bool, string>(CommandLineHandler);

            return rootCommand.InvokeAsync(args).Result;
        }

        static int CommandLineHandler(string name, string version, string url, string target, string output, bool noReleaseNotes, string sign)
        {
            if (string.IsNullOrWhiteSpace(name) || string.IsNullOrWhiteSpace(version) ||
                string.IsNullOrWhiteSpace(url) || string.IsNullOrWhiteSpace(target) ||
                string.IsNullOrWhiteSpace(output))
            {
                Console.Error.WriteLine("Missing a required option. Try running Onova.Publisher -h to get more information.");
                return 1;
            }

            if (Encoding.Unicode.GetBytes(name).Length >= InstallerConstant.AppNameLength)
            {
                Console.Error.WriteLine("App name is too long.");
                return 1;
            }

            if (!Version.TryParse(version, out _))
            {
                Console.Error.WriteLine("Version is invalid.");
                return 1;
            }

            if (url.Length >= InstallerConstant.ManifestUrlLength)
            {
                Console.Error.WriteLine("Manifest URL is too long.");
                return 1;
            }

            if (!Directory.Exists(target))
            {
                Console.Error.WriteLine("Target folder not found.");
                return 1;
            }

            if (!File.Exists(Path.Combine(target, name + ".exe")))
            {
                Console.Error.WriteLine("Target folder does not contain the executable. Check that the target name is the actual name of the executable.");
                return 1;
            }

            Console.WriteLine($"Publishing application {name}...");

            var publisher = new Publisher(name, version, url, target, output);

            if (noReleaseNotes)
                publisher.DisableReleaseNotes();

            try
            {
                if (publisher.CheckVersionPublished())
                    return 1;

                if (!string.IsNullOrEmpty(sign))
                    publisher.SignExecutables(sign);

                publisher.CreateZip();
                publisher.RebuildManifest();
                publisher.CreateInstaller();
                publisher.CreateEmptyReleaseNote();

                if (!string.IsNullOrEmpty(sign))
                    publisher.SignInstaller(sign);
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine(ex.Message);
                return 1;
            }

            Console.WriteLine($"Successfully published {name} version {version} into {publisher.ReleaseFileName}.");

            return 0;
        }
    }
}
