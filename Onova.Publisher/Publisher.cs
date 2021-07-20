using System;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading;

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

        public string PublisherDirectory { get; }
        public string InstallerPath { get; }

        public Publisher(string appName, string appVersion, string manifestUrl, string targetFolder, string outputFolder)
        {
            // append the manifest file to url, if not part of it
            if (!manifestUrl.EndsWith(PublisherConstant.ManifestFile))
            {
                manifestUrl = manifestUrl.TrimEnd('/');
                manifestUrl += "/" + PublisherConstant.ManifestFile;
            }

            AppName = appName;
            AppVersion = appVersion;
            ManifestUrl = manifestUrl;
            TargetFolder = targetFolder;
            OutputFolder = outputFolder;

            ReleaseFolder = Path.Combine(OutputFolder, PublisherConstant.OutputFolder);
            ReleaseFileName = ReleasifyName(AppName, AppVersion);
            ReleaseFilePath = Path.Combine(ReleaseFolder, ReleaseFileName);

            PublisherDirectory = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            InstallerPath = Path.Combine(PublisherDirectory, InstallerConstant.InstallerName);
        }

        public bool CheckVersionPublished()
        {
            // check output directory
            if (!Directory.Exists(ReleaseFolder))
                Directory.CreateDirectory(ReleaseFolder);

            // alert user on overwrite
            if (File.Exists(ReleaseFilePath))
            {
                Console.WriteLine($"WARNING: File {ReleaseFileName} will be overwritten.");
                Console.WriteLine("Resuming in 3 seconds...");
                Thread.Sleep(3000);
                File.Delete(ReleaseFilePath);
            }

            return false;
        }

        public void CreateZip()
        {
            try
            {
                ZipFile.CreateFromDirectory(TargetFolder, ReleaseFilePath);
            }
            catch (Exception ex)
            {
                throw new Exception("Cannot create zip file.", ex);
            }
        }

        public void RebuildManifest()
        {
            var manifestFile = Path.Combine(ReleaseFolder, PublisherConstant.ManifestFile);

            using var writer = new FileStream(manifestFile, FileMode.Create, FileAccess.Write);
            writer.SetLength(0);

            using var textWriter = new StreamWriter(writer);
            
            foreach (var zipPath in Directory.GetFiles(ReleaseFolder, "*.zip"))
            {
                var fileName = Path.GetFileNameWithoutExtension(zipPath);
                var pos = fileName.LastIndexOf('-');

                var name = new string(fileName.Take(pos).ToArray());
                var version = new string(fileName.Skip(pos + 1).ToArray());

                AppendManifest(textWriter, version, ReleasifyName(name, version));
            }
        }

        private void AppendManifest(TextWriter writer, string appVersion, string fileName)
        {
            var manifestLine = $"{appVersion} {fileName}\n";

            try
            {
                writer.Write(manifestLine);
            }
            catch (Exception ex)
            {
                throw new Exception("Cannot write to manifest.", ex);
            }
        }

        private string ReleasifyName(string name, string version)
        {
            return $"{name.Replace(' ', '-')}-{version}.zip";
        }

        public void CreateInstaller()
        {
            if (!File.Exists(InstallerPath))
            {
                throw new Exception("Publisher cannot find the template installer.");
            }

            var installerPath = Path.Combine(ReleaseFolder, PublisherConstant.InstallerName);

            try
            {
                File.Copy(InstallerPath, installerPath, true);
            }
            catch (Exception ex)
            {
                throw new Exception("Cannot copy the installer.", ex);
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
        }
    }
}
