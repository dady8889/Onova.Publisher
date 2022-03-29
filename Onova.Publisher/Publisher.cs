using System;
using System.Collections.Generic;
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
        
        public bool GenerateReleaseNotes { get; private set; }

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

            GenerateReleaseNotes = true;
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

            var files = new List<(string FileName, Version Version)>();

            foreach (var zipPath in Directory.GetFiles(ReleaseFolder, "*.zip"))
            {
                var fileName = Path.GetFileName(zipPath);
                var fileNameWithoutExtension = Path.GetFileNameWithoutExtension(zipPath);
                var pos = fileNameWithoutExtension.LastIndexOf('-');

                // var name = new string(fileName.Take(pos).ToArray());
                var version = new string(fileNameWithoutExtension.Skip(pos + 1).ToArray());

                if (!Version.TryParse(version, out Version parsedVersion))
                    throw new Exception($"Filename {fileName} contains invalid version.");

                files.Add((fileName, parsedVersion));
            }

            foreach (var file in files.OrderBy(x => x.Version))
                AppendManifest(textWriter, file.Version.ToString(), file.FileName);
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

        private string ReleasifyName(string name, string version, bool withExtension = true)
        {
            string relName = $"{name.Replace(' ', '-')}-{version}";

            if (withExtension)
                relName += ".zip";

            return relName;
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

            Encoding.Unicode.GetBytes(AppName).CopyTo(packedAppName, 0);
            Encoding.ASCII.GetBytes(ManifestUrl).CopyTo(packedManifestUrl, 0);

            using (var fileStream = new FileStream(installerPath, FileMode.Append, FileAccess.Write, FileShare.None))
            using (var bw = new BinaryWriter(fileStream))
            {
                bw.Write(packedAppName);
                bw.Write(packedManifestUrl);
            }
        }

        public void DisableReleaseNotes()
        {
            GenerateReleaseNotes = false;
        }

        public void CreateEmptyReleaseNote()
        {
            if (!GenerateReleaseNotes)
                return;

            try
            {
                var rnPath = Path.Combine(ReleaseFolder, ReleasifyName(AppName, AppVersion, false) + ".rn");

                // if the rn file already exists, just skip
                if (File.Exists(rnPath))
                    return;

                File.Create(rnPath);
            }
            catch (Exception ex)
            {
                throw new Exception("Cannot create release note file.", ex);
            }
        }
    }
}
