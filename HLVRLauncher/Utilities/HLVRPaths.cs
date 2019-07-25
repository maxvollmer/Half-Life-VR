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
        public static string SteamDirectory { get; private set; }
        public static string HLDirectory { get; private set; }
        public static string VRDirectory { get; private set; }


        public static string VROpengl32dll { get { return Path.Combine(HLVRPaths.VRDirectory, "opengl32.dll"); } }
        public static string VROpenvr_apidll { get { return Path.Combine(HLVRPaths.VRDirectory, "openvr_api.dll"); } }
        public static string VREasyHook32dll { get { return Path.Combine(HLVRPaths.VRDirectory, "EasyHook32.dll"); } }

        public static string HLOpengl32dll { get { return Path.Combine(HLVRPaths.HLDirectory, "opengl32.dll"); } }
        public static string HLOpenvr_apidll { get { return Path.Combine(HLVRPaths.HLDirectory, "openvr_api.dll"); } }
        public static string HLEasyHook32dll { get { return Path.Combine(HLVRPaths.HLDirectory, "EasyHook32.dll"); } }

        public static string HLExecutable { get { return Path.Combine(HLVRPaths.HLDirectory, "hl.exe"); } }

        public static void Initialize()
        {
            InitializeSteamDirectory();
            InitializeHalfLifeDirectory();
            CheckModDirectory();
        }

        private static void InitializeSteamDirectory()
        {
            try
            {
                using (RegistryKey key = Registry.LocalMachine.OpenSubKey("Software\\Valve\\Steam"))
                {
                    if (key != null)
                    {
                        SteamDirectory = key.GetValue("InstallPath")?.ToString()??"";
                    }
                }
            }
            catch (Exception)
            {
                SteamDirectory = null;
            }

            if (SteamDirectory == null || SteamDirectory.Length == 0 || !Directory.Exists(SteamDirectory))
            {
                MessageBox.Show("Couldn't verify Steam installation. Please make sure Steam is installed. If the problem persists, try running HLVRLauncher with administrative privileges.", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                throw new CancelAndTerminateAppException();
            }
        }

        private static string FindHalflifeDirectory()
        {
            string hlmanifest = Path.Combine(Path.Combine(SteamDirectory, "steamapps"), "appmanifest_70.acf");
            if (File.Exists(hlmanifest))
            {
                return Path.Combine(Path.Combine(Path.Combine(SteamDirectory, "steamapps"), "common"), "Half-Life");
            }

            string libraryfolders = Path.Combine(Path.Combine(SteamDirectory, "steamapps"), "libraryfolders.vdf");
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
                            return Path.Combine(Path.Combine(Path.Combine(libraryfolder, "steamapps"), "common"), "Half-Life");
                        }
                    }
                }
            }

            return null;
        }

        private static void InitializeHalfLifeDirectory()
        {
            HLDirectory = FindHalflifeDirectory();
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
    }
}
