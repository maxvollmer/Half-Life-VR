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
        public static string LastHLDirectory { get; private set; } = "";

        public static string HLDirectory
        {
            get
            {
                return HLVRSettingsManager.LauncherSettings.GeneralSettings[LauncherSettings.CategoryModLocation][LauncherSettings.HLDirectory].Value;
            }

            private set
            {
                HLVRSettingsManager.LauncherSettings.GeneralSettings[LauncherSettings.CategoryModLocation][LauncherSettings.HLDirectory].Value = value;
                LastHLDirectory = value;
            }
        }

        public static string VRDirectory { get; private set; }


        public static string VROpenvr_apidll { get { return Path.Combine(HLVRPaths.VRDirectory, "opnvrpi.dll"); } }
        public static string VREasyHook32dll { get { return Path.Combine(HLVRPaths.VRDirectory, "ezhok32.dll"); } }
        public static string VRServerdll { get { return Path.Combine(VRDirectory, "dlls", "vr.dll"); } }
        public static string VRClientdll { get { return Path.Combine(VRDirectory, "cl_dlls", "client.dll"); } }
        public static string VRLiblistgam { get { return Path.Combine(HLVRPaths.VRDirectory, "liblist.gam"); } }


        public static string HLExecutable { get { return Path.Combine(HLVRPaths.HLDirectory, "hl.exe"); } }


        public static string VRModSettingsFileName { get { return "hlvr.cfg"; } }
        public static string VRModSettingsFile { get { return Path.Combine(HLVRPaths.VRDirectory, VRModSettingsFileName); } }
        public static string VRReadme { get { return Path.Combine(HLVRPaths.VRDirectory, "README.txt"); } }
        public static string VRLogFile { get { return Path.Combine(HLVRPaths.VRDirectory, "hlvr_log.txt"); } }

        public static string VRActionsManifest { get { return Path.Combine(HLVRPaths.VRDirectory, "actions", "actions.manifest"); } }
        public static string VRCustomActionsFile { get { return Path.Combine(HLVRPaths.VRDirectory, "actions", "customactions.cfg"); } }

        public static string VRLauncherSettingsFileName { get { return "hlvrlauncher.cfg"; } }
        public static string VRLauncherSettingsFile { get { return Path.Combine(AppDomain.CurrentDomain.BaseDirectory, VRLauncherSettingsFileName); } }

        public static string VRI18NDirectory { get { return Path.Combine(HLVRPaths.VRDirectory, "i18n"); } }

        public static string VRMiniDumpDirectory { get { return Path.Combine(HLVRPaths.VRDirectory, "dumps"); } }


        public static void Initialize()
        {
            // first try current location (we might be in the mod folder)
            if (!InitializeFromLocation())
            {
                // if that fails use steam to find Half-Life
                InitializeFromSteam();
            }
        }

        public static void RestoreLastHLDirectory()
        {
            HLDirectory = LastHLDirectory;
        }

        private static bool InitializeFromLocation()
        {
            string currentLocation = Path.GetFullPath(Path.GetDirectoryName(System.Reflection.Assembly.GetEntryAssembly().Location));
            if (IsModDirectory(currentLocation))
            {
                string hlDirectory = Path.GetFullPath(Path.Combine(currentLocation, "..\\"));
                if (IsHalflifeDirectory(hlDirectory))
                {
                    HLDirectory = hlDirectory;
                    VRDirectory = currentLocation;
                    return true;
                }
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

        private static string GetHalfLifeDirectoryFromManifest(string libraryfolder, string manifest)
        {
            string foldername = "Half-Life";
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

            return Path.Combine(Path.Combine(Path.Combine(libraryfolder, "steamapps"), "common"), "Half-Life");
        }

        private static string FindHalflifeDirectoryFromSteam()
        {
            string steamDirectory = GetSteamDirectory();
            if (steamDirectory == null || steamDirectory.Length == 0)
                return null;

            string hlmanifest = Path.Combine(Path.Combine(steamDirectory, "steamapps"), "appmanifest_70.acf");
            if (File.Exists(hlmanifest))
            {
                string hlDirectory = GetHalfLifeDirectoryFromManifest(steamDirectory, hlmanifest);
                if (Directory.Exists(hlDirectory))
                {
                    return hlDirectory;
                }
            }

            string libraryfolders = Path.Combine(Path.Combine(steamDirectory, "steamapps"), "libraryfolders.vdf");
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
                        hlmanifest = Path.Combine(Path.Combine(libraryfolder, "steamapps"), "appmanifest_70.acf");
                        if (File.Exists(hlmanifest))
                        {
                            string hlDirectory = GetHalfLifeDirectoryFromManifest(libraryfolder, hlmanifest);
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

        private static void InitializeFromSteam()
        {
            HLDirectory = FindHalflifeDirectoryFromSteam();
            if (CheckHLDirectory())
            {
                VRDirectory = Path.Combine(HLDirectory, "vr");
            }
        }

        public static bool CheckHLDirectory()
        {
            return IsHalflifeDirectory(HLDirectory);
        }

        public static bool CheckModDirectory()
        {
            if (!CheckHLDirectory())
                return false;

            return IsModDirectory(VRDirectory);
        }

        private static bool IsHalflifeDirectory(string directory)
        {
            if (directory == null || directory.Length == 0)
                return false;

            if (!Directory.Exists(directory))
                return false;

            if (!File.Exists(Path.Combine(directory, "hl.exe"))
                || !Directory.Exists(Path.Combine(directory, "valve")))
                return false;

            return true;
        }

        private static bool IsModDirectory(string directory)
        {
            string fmod_dll = Path.Combine(directory, "f.dll");
            string openvr_apidll = Path.Combine(directory, "opnvrpi.dll");
            string EasyHook32dll = Path.Combine(directory, "ezhok32.dll");
            string vrdll = Path.Combine(Path.Combine(directory, "dlls"), "vr.dll");
            string clientdll = Path.Combine(Path.Combine(directory, "cl_dlls"), "client.dll");
            string liblistgam = Path.Combine(directory, "liblist.gam");
            return File.Exists(fmod_dll)
                && File.Exists(openvr_apidll)
                && File.Exists(EasyHook32dll)
                && File.Exists(vrdll)
                && File.Exists(clientdll)
                && File.Exists(liblistgam);
        }
    }
}
