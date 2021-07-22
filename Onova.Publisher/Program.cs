using System;
using System.IO;
using System.CommandLine;
using System.CommandLine.Invocation;

namespace Onova.Publisher
{
    class Program
    {

        static int Main(string[] args)
        {
            var rootCommand = new RootCommand
            {
                new Option<string>(
                    new [] { "--name", "-n" },
                    "Your application's name (name of executable without extension). Maximum 64 characters.")
                { IsRequired = false },

                new Option<string>(
                    new [] { "--version", "-v" },
                    "Version in format major.minor[.build[.revision]].")
                { IsRequired = false },

                new Option<string>(
                    new [] { "--url", "-u" },
                    "URL to web where the manifest resides. Maximum 1024 characters.")
                { IsRequired = false },

                new Option<string>(
                    new [] { "--target", "--in", "-i" },
                    "Folder which will get packed into a zip.")
                { IsRequired = false },

                new Option<string>(
                    new [] {"--output", "--out", "-o" },
                    () => ".",
                    "Output folder which will contain the publish folder. Publish folder will contain the updated manifest file, zip and installer.")
                { IsRequired = false },

                new Option<bool>(
                    new [] {"--no-releasenotes", "--no-rn"},
                    "Disables generation of an empty release note (.rn) file.")
                { IsRequired = false }
            };

            rootCommand.Description = "Publishes your application for Onova.";
            rootCommand.Handler = CommandHandler.Create<string, string, string, string, string, bool>(CommandLineHandler);

            return rootCommand.InvokeAsync(args).Result;
        }

        static int CommandLineHandler(string name, string version, string url, string target, string output, bool noReleaseNotes)
        {
            if (string.IsNullOrWhiteSpace(name) || string.IsNullOrWhiteSpace(version) ||
                string.IsNullOrWhiteSpace(url) || string.IsNullOrWhiteSpace(target) ||
                string.IsNullOrWhiteSpace(output))
            {
                Console.Error.WriteLine("Missing a required option. Try running Onova.Publisher -h to get more information.");
                return 1;
            }

            if (name.Length > InstallerConstant.AppNameLength)
            {
                Console.Error.WriteLine("App name is invalid.");
                return 1;
            }

            if (!Version.TryParse(version, out _))
            {
                Console.Error.WriteLine("Version is invalid.");
                return 1;
            }

            if (url.Length > InstallerConstant.ManifestUrlLength)
            {
                
                Console.Error.WriteLine("Manifest URL is invalid.");
                return 1;
            }

            if (!Directory.Exists(target))
            {
                Console.Error.WriteLine("Target folder is invalid.");
                return 1;
            }

            if (!Directory.Exists(output))
            {
                Console.Error.WriteLine("Output folder is invalid.");
                return 1;
            }

            if (!File.Exists(Path.Combine(target, name + ".exe")))
            {
                Console.Error.WriteLine("Target folder does not contain the executable. Check that the target name is the actual name of the executable.");
                return 1;
            }

            var publisher = new Publisher(name, version, url, target, output);

            if (noReleaseNotes)
                publisher.DisableReleaseNotes();

            try
            {
                if (publisher.CheckVersionPublished())
                    return 1;

                publisher.CreateZip();
                publisher.RebuildManifest();
                publisher.CreateInstaller();
                publisher.CreateEmptyReleaseNote();
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
