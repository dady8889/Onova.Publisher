namespace Onova.Publisher
{
    static class InstallerConstant
    {
        public const int AppNameLength = 32 * 2; // utf-16 is 2 bytes long
        public const int ManifestUrlLength = 1024;

        public const string InstallerName = "Onova.Installer.exe";

        public static int TotalLength => AppNameLength + ManifestUrlLength;
    }

    static class PublisherConstant
    {
        public const string ManifestFile = "MANIFEST";
        public const string OutputFolder = "OnovaPublish";
        public const string InstallerName = "websetup.exe";
    }
}
