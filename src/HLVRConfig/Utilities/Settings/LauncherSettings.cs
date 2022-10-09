using HLVRConfig.Utilities.Process;
using HLVRConfig.Utilities.Settings;
using HLVRConfig.Utilities.UI;
using Microsoft.Collections.Extensions;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HLVRConfig.Utilities.Settings
{
    public class LauncherSettings : ISettingsContainer
    {
        public OrderedDictionary<string, I18N.I18NString> Languages
        {
            get
            {
                return GeneralSettings[CategoryLanguage][Language].AllowedValues;
            }
        }

        public int MaxLogLines
        {
            get
            {
                string value = GeneralSettings[CategoryLog][NumberOfDisplayedLogLines].Value;
                if (I18N.TryParseInt(value, out int result))
                {
                    return result;
                }
                else
                {
                    return 100;
                }
            }
        }

        public static readonly SettingCategory CategoryModSpecifics = new SettingCategory(new I18N.I18NString("LauncherSettings.ModSpecifics", "Mod Launch Options"));
        public static readonly SettingCategory CategoryLanguage = new SettingCategory(new I18N.I18NString("LauncherSettings.Language", "Language"));
        public static readonly SettingCategory CategoryLauncher = new SettingCategory(new I18N.I18NString("LauncherSettings.Launcher", "Launcher"));
        public static readonly SettingCategory CategoryLog = new SettingCategory(new I18N.I18NString("LauncherSettings.Log", "Log Settings"));
        public static readonly SettingCategory CategoryDebug = new SettingCategory(new I18N.I18NString("LauncherSettings.Debug", "Debug Settings"));

        public static readonly string MinimizeToTray = "MinimizeToTray";
        public static readonly string StartMinimized = "StartMinimized";
        public static readonly string AutoRunMod = "AutoRunMod";
        public static readonly string AutoCloseLauncher = "AutoCloseLauncher";
        public static readonly string AutoCloseGame = "AutoCloseGame";
        public static readonly string HLVRDirectory = "HLVRDirectory";
        public static readonly string SteamExeDirectory = "SteamExeDirectory";
        public static readonly string Language = "Language";
        public static readonly string NumberOfDisplayedLogLines = "NumberOfDisplayedLogLines";
        public static readonly string EnableMiniDumpButton = "EnableMiniDumpButton";
        public static readonly string EnableFullMemoryMiniDump = "EnableFullMemoryMiniDump";
        public static readonly string ModWindowSize = "ModWindowSize";


        public OrderedDictionary<SettingCategory, OrderedDictionary<string, Setting>> GeneralSettings = new OrderedDictionary<SettingCategory, OrderedDictionary<string, Setting>>()
        {
            { CategoryModSpecifics, new OrderedDictionary<string, Setting>() {
                { SteamExeDirectory, Setting.Create( new I18N.I18NString(SteamExeDirectory, "Steam.exe Directory"), HLVRPaths.LastSteamExeDirectory ).MakeFolderSetting() },
                { HLVRDirectory, Setting.Create( new I18N.I18NString(HLVRDirectory, "Half-Life: VR Directory"), HLVRPaths.LastHLVRDirectory ).MakeFolderSetting() },
                { ModWindowSize, Setting.Create( new I18N.I18NString(ModWindowSize, "Mod Window Size"), MakeAllowedWindowSizes(), "-width 1600 -height 1200" ) },
            } },

            { CategoryLanguage, new OrderedDictionary<string, Setting>() {
                { Language, Setting.Create( new I18N.I18NString(Language, "Language"), new OrderedDictionary<string, I18N.I18NString>(), "en" ) },
            } },

            { CategoryLauncher, new OrderedDictionary<string, Setting>() {
                { MinimizeToTray, Setting.Create( new I18N.I18NString(MinimizeToTray, "Minimize to tray"), false ) },
                { StartMinimized, Setting.Create( new I18N.I18NString(StartMinimized, "Start HLVRConfig minimized"), false ) },
                { AutoRunMod, Setting.Create( new I18N.I18NString(AutoRunMod, "Run mod automatically when starting HLVRConfig"), false ) },
                { AutoCloseLauncher, Setting.Create( new I18N.I18NString(AutoCloseLauncher, "Exit HLVRConfig automatically when Half-Life: VR exits"), false ) },
                { AutoCloseGame, Setting.Create( new I18N.I18NString(AutoCloseGame, "Terminate game automatically when HLVRConfig is closed"), false ) },
            } },

            { CategoryLog, new OrderedDictionary<string, Setting>() {
                { NumberOfDisplayedLogLines, Setting.Create( new I18N.I18NString(NumberOfDisplayedLogLines, "Max number of displayed lines"), SettingType.COUNT, "100" ) },
            } },

            { CategoryDebug, new OrderedDictionary<string, Setting>() {
                { EnableMiniDumpButton, Setting.Create( new I18N.I18NString(EnableMiniDumpButton, "Enable Mini Dump Button"), false ) },
                { EnableFullMemoryMiniDump, Setting.Create( new I18N.I18NString(EnableFullMemoryMiniDump, "Enable Full Memory Mini Dump (Creates huge files! Only enable this if directed or if you know what you are doing!)"), false, new SettingDependency(EnableMiniDumpButton, "1") ) },
            } },
        };

        public static void AddAllowedWindowSize(OrderedDictionary<string, I18N.I18NString> allowedWindowSizes, int width, int height)
        {
            allowedWindowSizes.Add($"-width {width} -height {height}", new I18N.I18NString($"{width}x{height}", $"{width}x{height}"));
        }

        private static OrderedDictionary<string, I18N.I18NString> MakeAllowedWindowSizes()
        {
            var allowedWindowSizes = new OrderedDictionary<string, I18N.I18NString>();

            // "normal"
            AddAllowedWindowSize(allowedWindowSizes, 640, 480);
            AddAllowedWindowSize(allowedWindowSizes, 800, 600);
            AddAllowedWindowSize(allowedWindowSizes, 1024, 768);
            AddAllowedWindowSize(allowedWindowSizes, 1152, 864);
            AddAllowedWindowSize(allowedWindowSizes, 1280, 960);
            AddAllowedWindowSize(allowedWindowSizes, 1600, 1200);
            AddAllowedWindowSize(allowedWindowSizes, 1920, 1440);
            AddAllowedWindowSize(allowedWindowSizes, 2048, 1536);

            // widescreen
            AddAllowedWindowSize(allowedWindowSizes, 720, 480);
            AddAllowedWindowSize(allowedWindowSizes, 720, 576);
            AddAllowedWindowSize(allowedWindowSizes, 1176, 664);
            AddAllowedWindowSize(allowedWindowSizes, 1280, 720);
            AddAllowedWindowSize(allowedWindowSizes, 1280, 768);
            AddAllowedWindowSize(allowedWindowSizes, 1280, 800);
            AddAllowedWindowSize(allowedWindowSizes, 1280, 1024);
            AddAllowedWindowSize(allowedWindowSizes, 1360, 768);
            AddAllowedWindowSize(allowedWindowSizes, 1366, 768);
            AddAllowedWindowSize(allowedWindowSizes, 1600, 900);
            AddAllowedWindowSize(allowedWindowSizes, 1600, 1024);
            AddAllowedWindowSize(allowedWindowSizes, 1680, 1050);
            AddAllowedWindowSize(allowedWindowSizes, 1768, 992);
            AddAllowedWindowSize(allowedWindowSizes, 1920, 1080);
            AddAllowedWindowSize(allowedWindowSizes, 1920, 1200);
            AddAllowedWindowSize(allowedWindowSizes, 2048, 1080);
            AddAllowedWindowSize(allowedWindowSizes, 2048, 1280);
            AddAllowedWindowSize(allowedWindowSizes, 2560, 1440);
            AddAllowedWindowSize(allowedWindowSizes, 2560, 1600);
            AddAllowedWindowSize(allowedWindowSizes, 3840, 2160);

            return allowedWindowSizes;
        }

        public IEnumerable<KeyValuePair<string, Setting>> KeyValuePairs
        {
            get
            {
                foreach (var foo in new[] { GeneralSettings })
                {
                    foreach (var bar in foo)
                    {
                        foreach (var boo in bar.Value)
                        {
                            yield return boo;
                        }
                    }
                }
            }
        }

        public bool SetValue(string key, string value)
        {
            foreach (var foo in new[] { GeneralSettings })
            {
                foreach (var bar in foo)
                {
                    if (bar.Value.TryGetValue(key, out Setting setting))
                    {
                        // don't clear out paths, always reset to default value instead
                        if (setting.IsFolder && string.IsNullOrEmpty(value))
                        {
                            setting.SetValue(setting.DefaultValue);
                        }
                        else
                        {
                            setting.SetValue(value);
                        }
                        return true;
                    }
                }
            }
            return false;
        }

        public LauncherSettings()
        {
        }
    }
}
