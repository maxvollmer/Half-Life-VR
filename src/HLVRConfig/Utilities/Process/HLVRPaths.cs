using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Interop;
using HLVRConfig.Utilities.Settings;
using Microsoft.Win32;

namespace HLVRConfig.Utilities.Process
{
    public class HLVRPaths
    {
        public static string LastHLVRDirectory { get; private set; } = "";
        public static string LastSteamExeDirectory { get; private set; } = "";

        public static string HLVRDirectory
        {
            get
            {
                return HLVRSettingsManager.LauncherSettings.GeneralSettings[LauncherSettings.CategoryModSpecifics][LauncherSettings.HLVRDirectory].Value;
            }

            private set
            {
                HLVRSettingsManager.LauncherSettings.GeneralSettings[LauncherSettings.CategoryModSpecifics][LauncherSettings.HLVRDirectory].Value = value;
                LastHLVRDirectory = value;
            }
        }

        public static string SteamExeDirectory
        {
            get
            {
                return HLVRSettingsManager.LauncherSettings.GeneralSettings[LauncherSettings.CategoryModSpecifics][LauncherSettings.SteamExeDirectory].Value;
            }

            private set
            {
                HLVRSettingsManager.LauncherSettings.GeneralSettings[LauncherSettings.CategoryModSpecifics][LauncherSettings.SteamExeDirectory].Value = value;
                LastSteamExeDirectory = value;
            }
        }

        public static string ValveDirectory
        {
            get
            {
                return Path.Combine(HLVRDirectory, "valve");
            }
        }

        public static string VROpenvr_apidll { get { return Path.Combine(HLVRDirectory, "openvr_api.dll"); } }
        public static string VREasyHook32dll { get { return Path.Combine(HLVRDirectory, "EasyHook32.dll"); } }
        public static string VRServerdll { get { return Path.Combine(ValveDirectory, "dlls", "hl.dll"); } }
        public static string VRClientdll { get { return Path.Combine(ValveDirectory, "cl_dlls", "client.dll"); } }
        public static string VRLiblistgam { get { return Path.Combine(ValveDirectory, "liblist.gam"); } }


        public static string HLVRExecutable { get { return Path.Combine(HLVRDirectory, "hl.exe"); } }
        public static string SteamExecutable { get { return Path.Combine(SteamExeDirectory, "Steam.exe"); } }


        public static string VRModSettingsFileName { get { return "hlvr.cfg"; } }
        public static string VRModSettingsFile { get { return Path.Combine(HLVRPaths.HLVRDirectory, VRModSettingsFileName); } }
        public static string VRReadme { get { return Path.Combine(HLVRPaths.HLVRDirectory, "README.md"); } }
        public static string VRLogFile { get { return Path.Combine(HLVRPaths.HLVRDirectory, "hlvr_log.txt"); } }

        public static string VRActionsManifest { get { return Path.Combine(HLVRPaths.ValveDirectory, "actions", "actions.manifest"); } }
        public static string VRCustomActionsFile { get { return Path.Combine(HLVRPaths.ValveDirectory, "actions", "customactions.cfg"); } }

        public static string VRLauncherSettingsFileName { get { return "hlvrlauncher.cfg"; } }
        public static string VRLauncherSettingsFile { get { return Path.Combine(AppDomain.CurrentDomain.BaseDirectory, VRLauncherSettingsFileName); } }

        public static string VRI18NDirectory { get { return Path.Combine(HLVRPaths.HLVRDirectory, "i18n"); } }

        public static string VRMiniDumpDirectory { get { return Path.Combine(HLVRPaths.HLVRDirectory, "dumps"); } }


        public static void Initialize()
        {
            // first find steam
            InitializeSteamDirectory();

            // try current location (we might be in the mod folder)
            if (!InitializeFromLocation())
            {
                // if that fails use steam to find Half-Life
                InitializeFromSteam();
            }
        }

        public static void RestoreLastHLDirectory()
        {
            HLVRDirectory = LastHLVRDirectory;
        }

        public static void RestoreLastSteamDirectory()
        {
            SteamExeDirectory = LastSteamExeDirectory;
        }

        private static bool InitializeFromLocation()
        {
            string currentLocation = Path.GetFullPath(Path.GetDirectoryName(System.Reflection.Assembly.GetEntryAssembly().Location));
            if (IsHLVRDirectory(currentLocation))
            {
                HLVRDirectory = currentLocation;
                return true;
            }
            return false;
        }

        private static string GetSteamDirectory()
        {
            string steamDirectory = null;

            try
            {
                using (RegistryKey key = Registry.LocalMachine.OpenSubKey("Software\\Valve\\Steam"))
                {
                    if (key != null)
                    {
                        steamDirectory = key.GetValue("InstallPath")?.ToString() ?? "";
                    }
                }
            }
            catch (Exception)
            {
                steamDirectory = null;
            }

            if (steamDirectory == null || steamDirectory.Length == 0 || !Directory.Exists(steamDirectory))
            {
                return null;
            }

            return steamDirectory;
        }

        private static string GetHLVRDirectoryFromManifest(string libraryfolder, string manifest)
        {
            string foldername = "Half-Life VR";
            StreamReader file = new StreamReader(manifest);
            string line;
            while ((line = file.ReadLine()) != null)
            {
                Match regMatch = Regex.Match(line, "^[\\s]*\"installdir\"[\\s]*\"([^\"]+)\"[\\s]*$");
                if (regMatch.Success)
                {
                    foldername = Regex.Unescape(regMatch.Groups[1].Value);
                }
            }

            if (Path.IsPathRooted(foldername) && Directory.Exists(foldername))
            {
                return foldername;
            }

            if (!Path.IsPathRooted(foldername))
            {
                string hlDirectory = Path.Combine(Path.Combine(Path.Combine(libraryfolder, "steamapps"), "common"), foldername);
                if (Directory.Exists(hlDirectory))
                {
                    return hlDirectory;
                }
            }

            return Path.Combine(Path.Combine(Path.Combine(libraryfolder, "steamapps"), "common"), "Half-Life VR");
        }

        private static string FindHLVRDirectoryFromSteam()
        {
            InitializeSteamDirectory();
            if (!CheckSteamDirectory())
                return null;

            string hlmanifest = Path.Combine(Path.Combine(SteamExeDirectory, "steamapps"), "appmanifest_1908720.acf");
            if (File.Exists(hlmanifest))
            {
                string hlDirectory = GetHLVRDirectoryFromManifest(SteamExeDirectory, hlmanifest);
                if (Directory.Exists(hlDirectory))
                {
                    return hlDirectory;
                }
            }

            string libraryfolders = Path.Combine(Path.Combine(SteamExeDirectory, "steamapps"), "libraryfolders.vdf");
            if (!File.Exists(libraryfolders))
            {
                return null;
            }

            StreamReader file = new StreamReader(libraryfolders);
            string line;
            while ((line = file.ReadLine()) != null)
            {
                Match regMatch = Regex.Match(line, "^[\\s]*\"[0-9]+\"[\\s]*\"([^\"]+)\"[\\s]*$");
                if (regMatch.Success)
                {
                    string libraryfolder = Regex.Unescape(regMatch.Groups[1].Value);
                    if (Directory.Exists(libraryfolder))
                    {
                        hlmanifest = Path.Combine(Path.Combine(libraryfolder, "steamapps"), "appmanifest_1908720.acf");
                        if (File.Exists(hlmanifest))
                        {
                            string hlDirectory = GetHLVRDirectoryFromManifest(libraryfolder, hlmanifest);
                            if (Directory.Exists(hlDirectory))
                            {
                                return hlDirectory;
                            }
                        }
                    }
                }
            }

            return null;
        }

        private static void InitializeSteamDirectory()
        {
            if (!CheckSteamDirectory())
            {
                var steamDirectory = GetSteamDirectory();
                if (IsSteamDirectory(steamDirectory))
                {
                    SteamExeDirectory = steamDirectory;
                }
            }
        }

        private static void InitializeFromSteam()
        {
            var directory = FindHLVRDirectoryFromSteam();
            if (IsHLVRDirectory(directory))
            {
                HLVRDirectory = directory;
            }
        }

        public static bool CheckHLVRDirectory()
        {
            return IsHLVRDirectory(HLVRDirectory);
        }

        public static bool CheckSteamDirectory()
        {
            return IsSteamDirectory(SteamExeDirectory);
        }

        private static bool IsSteamDirectory(string steamDir)
        {
            return steamDir != null && Directory.Exists(steamDir) && File.Exists(Path.Combine(steamDir, "Steam.exe"));
        }

        private static bool IsHLVRDirectory(string hlvrDir)
        {
            if (hlvrDir == null || hlvrDir.Length == 0)
                return false;

            if (!Directory.Exists(hlvrDir))
                return false;

            if (!File.Exists(Path.Combine(hlvrDir, "hl.exe")))
                return false;

            if (!File.Exists(Path.Combine(hlvrDir, "HLVRConfig.exe")))
                return false;

            if (!File.Exists(Path.Combine(hlvrDir, "fmod.dll")))
                return false;

            if (!File.Exists(Path.Combine(hlvrDir, "openvr_api.dll")))
                return false;

            if (!File.Exists(Path.Combine(hlvrDir, "EasyHook32.dll")))
                return false;

            string valveDir = Path.Combine(hlvrDir, "valve");

            if (!Directory.Exists(valveDir))
                return false;

            if (!File.Exists(Path.Combine(Path.Combine(valveDir, "dlls"), "hl.dll")))
                return false;

            if (!File.Exists(Path.Combine(Path.Combine(valveDir, "cl_dlls"), "client.dll")))
                return false;

            if (!File.Exists(Path.Combine(valveDir, "liblist.gam")))
                return false;

            return true;
        }
    }
}
