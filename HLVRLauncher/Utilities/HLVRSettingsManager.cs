using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Collections.Extensions;
using Newtonsoft.Json;

namespace HLVRLauncher.Utilities
{
    public class HLVRSettingsManager
    {
        public static HLVRSettings Settings { get; private set; } = new HLVRSettings();

        public static void InitSettings()
        {
            if (File.Exists(HLVRPaths.VRSettingsFile))
            {
                Settings = JsonConvert.DeserializeObject<HLVRSettings>(File.ReadAllText(HLVRPaths.VRSettingsFile));
            }
            StoreSettings();
        }

        public static void SetLauncherSetting(string name, bool value)
        {
            Settings.LauncherSettings[name].Value = value ? "1" : "0";
            DelayedStoreSettings();
        }

        internal static void SetModSetting(string category, string name, bool value)
        {
            Settings.ModSettings[category][name].Value = value ? "1" : "0";
            DelayedStoreSettings();
        }

        internal static void SetModSetting(string category, string name, string value)
        {
            if (Settings.ModSettings[category][name].AllowedValues.Count > 0)
            {
                foreach (var allowedValue in Settings.ModSettings[category][name].AllowedValues)
                {
                    if (allowedValue.Value.Equals(value))
                    {
                        Settings.ModSettings[category][name].Value = allowedValue.Key;
                        break;
                    }
                }
            }
            else
            {
                Settings.ModSettings[category][name].Value = value;
            }
            DelayedStoreSettings();
        }

        private static void DelayedStoreSettings()
        {
            // TODO: Actually make delayed
            StoreSettings();
        }

        private static void StoreSettings()
        {
            File.WriteAllText(HLVRPaths.VRSettingsFile, JsonConvert.SerializeObject(Settings));
        }

        internal static void RestoreLauncherSettings()
        {
            HLVRSettings defaultSettings = new HLVRSettings();
            Settings.LauncherSettings = defaultSettings.LauncherSettings;
            DelayedStoreSettings();
        }

        internal static void RestoreModSettings()
        {
            HLVRSettings defaultSettings = new HLVRSettings();
            Settings.ModSettings = defaultSettings.ModSettings;
            DelayedStoreSettings();
        }
    }
}
