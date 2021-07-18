using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Onova.Publisher
{
    internal class Publisher
    {
        public string AppName { get; }
        public string AppVersion { get; }
        public string ManifestUrl { get; }
        public string TargetFolder { get; }
        public string OutputFolder { get; }

        public string ReleaseFolder { get; }
        public string ReleaseFileName { get; }
        public string ReleaseFilePath { get; }

        public Publisher(string appName, string appVersion, string manifestUrl, string targetFolder, string outputFolder)
        {
            AppName = appName;
            AppVersion = appVersion;
            ManifestUrl = manifestUrl;
            TargetFolder = targetFolder;
            OutputFolder = outputFolder;

            ReleaseFolder = Path.Combine(OutputFolder, PublisherConstant.OutputFolder);
            ReleaseFileName = $"{AppName.Replace(' ', '-')}-{AppVersion}.zip";
            ReleaseFilePath = Path.Combine(ReleaseFolder, ReleaseFileName);
        }

        public bool CreateZip()
        {
            // check output directory
            if (!Directory.Exists(ReleaseFolder))
                Directory.CreateDirectory(ReleaseFolder);

            // alert user on overwrite
            if (File.Exists(ReleaseFilePath))
            {
                Console.WriteLine($"WARNING: File {ReleaseFileName} will be overwritten.");
                Console.WriteLine($"Press ENTER to continue or ESC to cancel.");

                ConsoleKeyInfo keyinfo;
                do
                {
                    keyinfo = Console.ReadKey();

                    if (keyinfo.Key == ConsoleKey.Enter)
                        break;
                    else if (keyinfo.Key == ConsoleKey.Escape)
                    {
                        Console.WriteLine($"AAborting."); // the first A gets flushed
                        return false;
                    }
                } while (true);

                File.Delete(ReleaseFilePath);

                // TODO: rebuild MANIFEST file on overwrite
            }

            // create zip
            try
            {
                ZipFile.CreateFromDirectory(TargetFolder, ReleaseFilePath);
            }
            catch (Exception ex)
            {
                Console.WriteLine("Cannot create zip file.");
                Console.WriteLine(ex);
                return false;
            }

            return true;
        }

        public bool AppendManifest()
        {
            var manifestFile = Path.Combine(ReleaseFolder, PublisherConstant.ManifestFile);
            var manifestLine = $"{AppVersion} {ReleaseFileName}\n";

            try
            {
                File.AppendAllText(manifestFile, manifestLine);
            }
            catch (Exception ex)
            {
                Console.WriteLine("Cannot write to manifest.");
                Console.WriteLine(ex);
                return false;
            }

            return true;
        }

        public bool CreateInstaller()
        {
            if (!File.Exists(InstallerConstant.InstallerName))
            {
                Console.WriteLine("Publisher cannot find the template installer.");
                return false;
            }

            var installerPath = Path.Combine(ReleaseFolder, PublisherConstant.InstallerName);

            try
            {
                File.Copy(InstallerConstant.InstallerName, installerPath, true);
            }
            catch (Exception ex)
            {
                Console.WriteLine("Cannot copy the installer.");
                Console.WriteLine(ex);
                return false;
            }

            var packedAppName = new byte[InstallerConstant.AppNameLength];
            var packedManifestUrl = new byte[InstallerConstant.ManifestUrlLength];
            var packedReserved = new byte[InstallerConstant.ReservedLength];

            Encoding.ASCII.GetBytes(AppName).CopyTo(packedAppName, 0);
            Encoding.ASCII.GetBytes(ManifestUrl).CopyTo(packedManifestUrl, 0);

            using (var fileStream = new FileStream(installerPath, FileMode.Append, FileAccess.Write, FileShare.None))
            using (var bw = new BinaryWriter(fileStream))
            {
                bw.Write(packedAppName);
                bw.Write(packedManifestUrl);
                bw.Write(packedReserved);
            }

            return true;
        }
    }
}
