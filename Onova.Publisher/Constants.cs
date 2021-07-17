namespace Onova.Publisher
{
    static class InstallerConstant
    {
        public const int AppNameLength = 64;
        public const int ManifestUrlLength = 1024;
        public const int ReservedLength = 1024;

        public const string InstallerName = "Onova.Installer.exe";

        public static int TotalLength => AppNameLength + ManifestUrlLength + ReservedLength;
    }

    static class PublisherConstant
    {
        public const string ManifestFile = "MANIFEST";
        public const string OutputFolder = "Releases";
        public const string InstallerName = "Setup.exe";
    }
}
