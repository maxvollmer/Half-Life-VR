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

namespace HLVRInstaller
{
    public class HalflifeDirectoryFinder
    {
        public static string HLDirectory { get; set; } = "";
        public static string VRDirectory { get { return Path.Combine(HLDirectory, "vr"); } }
        public static string VRConfigExecutable { get { return Path.Combine(VRDirectory, "HLVRConfig.exe"); } }


        public static void InitializeFromSteam()
        {
            HLDirectory = FindHalflifeDirectoryFromSteam();
        }

        public static bool CheckHLDirectory()
        {
            return IsHalflifeDirectory(HLDirectory);
        }

        public static bool CheckModDirectory()
        {
            return IsModDirectory(VRDirectory);
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

        private static bool IsHalflifeDirectory(string directory)
        {
            if (directory == null || directory.Length == 0)
                return false;

            if (!Directory.Exists(directory))
                return false;

            if (!File.Exists(Path.Combine(directory, "hl.exe")) || !Directory.Exists(Path.Combine(directory, "valve")))
                return false;

            return true;
        }

        private static bool IsModDirectory(string directory)
        {
            if (directory == null || directory.Length == 0)
                return false;

            if (!Directory.Exists(directory))
                return false;

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
                && File.Exists(liblistgam)
                && File.Exists(VRConfigExecutable);
        }
    }
}

