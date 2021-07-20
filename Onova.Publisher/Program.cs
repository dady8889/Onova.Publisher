using System;
using System.IO;
using System.IO.Compression;

namespace Onova.Publisher
{
    class Program
    {
        /// <summary>
        /// Publishes your application for Onova.
        /// </summary>
        /// <param name="name">Your application's name (name of executable without extension). Maximum 64 characters.</param>
        /// <param name="version">Version in format major.minor[.build[.revision]].</param>
        /// <param name="url">URL to web where the manifest resides. Maximum 1024 characters.</param>
        /// <param name="target">Folder which will get packed into a zip.</param>
        /// <param name="output">Output folder which will contain the Releases folder. This folder will contain the updated manifest file, zip and installer.</param>
        static int Main(string name, string version, string url, string target, string output)
        {
            if (string.IsNullOrWhiteSpace(name) || string.IsNullOrWhiteSpace(version) || 
                string.IsNullOrWhiteSpace(url) || string.IsNullOrWhiteSpace(target) || 
                string.IsNullOrWhiteSpace(output))
            {
                Console.WriteLine("Required parameter is missing.");
                return 1;
            }

            if (name.Length > InstallerConstant.AppNameLength)
            {
                Console.WriteLine("App name is invalid.");
                return 1;
            }

            if (!Version.TryParse(version, out _))
            {
                Console.WriteLine("Version is invalid.");
                return 1;
            }

            if (url.Length > InstallerConstant.ManifestUrlLength)
            {
                Console.WriteLine("Manifest URL is invalid.");
                return 1;
            }

            if (!Directory.Exists(target))
            {
                Console.WriteLine("Target folder is invalid.");
                return 1;
            }

            if (!Directory.Exists(output))
            {
                Console.WriteLine("Output folder is invalid.");
                return 1;
            }

            if (!File.Exists(Path.Combine(target, name + ".exe")))
            {
                Console.WriteLine("Target folder does not contain the executable. Check that the target name is the actual name of the executable.");
                return 1;
            }

            var publisher = new Publisher(name, version, url, target, output);

            if (!publisher.CreateZip())
                return 1;

            if (!publisher.RebuildManifest())
                return 1;

            if (!publisher.CreateInstaller())
                return 1;

            Console.WriteLine($"Successfully published {name} version {version} into {publisher.ReleaseFileName}.");

            return 0;
        }
    }
}
