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
using Microsoft.Win32;

namespace HLVRLauncher.Utilities
{
    public class HLVRPaths
    {
        public static string HLDirectory { get; private set; }
        public static string VRDirectory { get; private set; }


        public static string VROpengl32dll { get { return Path.Combine(HLVRPaths.VRDirectory, "opengl32.dll"); } }
        public static string VROpenvr_apidll { get { return Path.Combine(HLVRPaths.VRDirectory, "openvr_api.dll"); } }
        public static string VREasyHook32dll { get { return Path.Combine(HLVRPaths.VRDirectory, "EasyHook32.dll"); } }


        public static string HLOpenvr_apidll { get { return Path.Combine(HLVRPaths.HLDirectory, "openvr_api.dll"); } }
        public static string HLEasyHook32dll { get { return Path.Combine(HLVRPaths.HLDirectory, "EasyHook32.dll"); } }

        public static string HLExecutable { get { return Path.Combine(HLVRPaths.HLDirectory, "hl.exe"); } }


        public static string VRSettingsFileName { get { return "hlvrsettings.cfg"; } }
        public static string VRSettingsFile { get { return Path.Combine(HLVRPaths.VRDirectory, VRSettingsFileName); } }
        public static string VRReadme { get { return Path.Combine(HLVRPaths.VRDirectory, "README.txt"); } }
        public static string VRLogFile { get { return Path.Combine(HLVRPaths.VRDirectory, "hlvr_log.txt"); } }


        public static string MetaHookExe { get { return Path.Combine(HLVRPaths.HLDirectory, "MetaHook.exe"); } }
        public static string MetaHookAudioDLL { get { return Path.Combine(Path.Combine(Path.Combine(HLVRPaths.VRDirectory, "metahook"), "plugins"), "Audio.dll"); } }
        public static string MetaHookPluginsLST { get { return Path.Combine(Path.Combine(Path.Combine(HLVRPaths.VRDirectory, "metahook"), "configs"), "plugins.lst"); } }


        public static void Initialize()
        {
            // first try current location (we might be in the mod folder)
            if (!InitializeFromLocation())
            {
                // if that fails use steam to find Half-Life
                InitializeFromSteam();
            }
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
                MessageBox.Show("Couldn't verify Steam installation. Please make sure Steam is installed. If the problem persists, try running HLVRLauncher with administrative privileges.", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                throw new CancelAndTerminateAppException();
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
            if (HLDirectory == null || HLDirectory.Length == 0 || !Directory.Exists(HLDirectory))
            {
                MessageBox.Show("Couldn't find Half-Life installation. Please make sure your installation of Half-Life is valid and run the game at least once from the Steam library. If the problem persists, try running HLVRLauncher with administrative privileges.", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                throw new CancelAndTerminateAppException();
            }

            string hlexe = Path.Combine(HLDirectory, "hl.exe");
            string valvefolder = Path.Combine(HLDirectory, "valve");
            if (!File.Exists(hlexe) || !Directory.Exists(valvefolder))
            {
                MessageBox.Show("Couldn't verify Half-Life installation. Please make sure your installation of Half-Life is valid and run the game at least once from the Steam library.", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                throw new CancelAndTerminateAppException();
            }

            CheckModDirectory();
        }

        private static void CheckModDirectory()
        {
            VRDirectory = Path.Combine(HLDirectory, "vr");
            if (!Directory.Exists(VRDirectory))
            {
                MessageBox.Show("Couldn't find Half-Life: VR. Please reinstall the mod.", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                throw new CancelAndTerminateAppException();
            }

            string opengl32dll = Path.Combine(VRDirectory, "opengl32.dll");
            string openvr_apidll = Path.Combine(VRDirectory, "openvr_api.dll");
            string EasyHook32dll = Path.Combine(VRDirectory, "EasyHook32.dll");
            string vrdll = Path.Combine(Path.Combine(VRDirectory, "dlls"), "vr.dll");
            string clientdll = Path.Combine(Path.Combine(VRDirectory, "cl_dlls"), "client.dll");
            string liblistgam = Path.Combine(VRDirectory, "liblist.gam");
            if (!File.Exists(opengl32dll)
                || !File.Exists(openvr_apidll)
                || !File.Exists(EasyHook32dll)
                || !File.Exists(vrdll)
                || !File.Exists(clientdll)
                || !File.Exists(liblistgam))
            {
                MessageBox.Show("Couldn't find necessary mod files. Please reinstall the mod.", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                throw new CancelAndTerminateAppException();
            }
        }

        private static bool IsHalflifeDirectory(string directory)
        {
            if (!Directory.Exists(directory))
                return false;

            if (!File.Exists(Path.Combine(directory, "hl.exe")))
                return false;

            return true;
        }

        private static bool IsModDirectory(string directory)
        {
            string opengl32dll = Path.Combine(directory, "opengl32.dll");
            string openvr_apidll = Path.Combine(directory, "openvr_api.dll");
            string EasyHook32dll = Path.Combine(directory, "EasyHook32.dll");
            string vrdll = Path.Combine(Path.Combine(directory, "dlls"), "vr.dll");
            string clientdll = Path.Combine(Path.Combine(directory, "cl_dlls"), "client.dll");
            string liblistgam = Path.Combine(directory, "liblist.gam");
            return File.Exists(opengl32dll)
                && File.Exists(openvr_apidll)
                && File.Exists(EasyHook32dll)
                && File.Exists(vrdll)
                && File.Exists(clientdll)
                && File.Exists(liblistgam);
        }
    }
}
