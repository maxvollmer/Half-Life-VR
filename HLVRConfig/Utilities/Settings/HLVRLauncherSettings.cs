using HLVRConfig.Utilities.Settings;
using Microsoft.Collections.Extensions;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HLVRConfig.Utilities
{
    public class HLVRLauncherSettings : ISettingsContainer
    {
        public OrderedDictionary<string, I18N.I18NString> Languages
        {
            get
            {
                return LauncherSettings[CategoryLanguage][Language].AllowedValues;
            }
        }

        public int MaxLogLines
        {
            get
            {
                string value = LauncherSettings[HLVRLauncherSettings.CategoryLog][HLVRLauncherSettings.NumberOfDisplayedLogLines].Value;
                if (int.TryParse(value, out int result))
                {
                    return result;
                }
                else
                {
                    return 100;
                }
            }
        }

        public static readonly SettingCategory CategoryModLocation = new SettingCategory(new I18N.I18NString("LauncherSettings.ModLocation", "Mod Directory"));
        public static readonly SettingCategory CategoryLanguage = new SettingCategory(new I18N.I18NString("LauncherSettings.Language", "Language"));
        public static readonly SettingCategory CategoryLauncher = new SettingCategory(new I18N.I18NString("LauncherSettings.Launcher", "Launcher"));
        public static readonly SettingCategory CategoryLog = new SettingCategory(new I18N.I18NString("LauncherSettings.Log", "Log Settings"));

        public static readonly string MinimizeToTray = "MinimizeToTray";
        public static readonly string StartMinimized = "StartMinimized";
        public static readonly string AutoRunMod = "AutoRunMod";
        public static readonly string AutoCloseLauncher = "AutoCloseLauncher";
        public static readonly string AutoCloseGame = "AutoCloseGame";
        public static readonly string HLDirectory = "HLDirectory";
        public static readonly string Language = "Language";
        public static readonly string NumberOfDisplayedLogLines = "Language";


        public OrderedDictionary<SettingCategory, OrderedDictionary<string, Setting>> LauncherSettings = new OrderedDictionary<SettingCategory, OrderedDictionary<string, Setting>>()
        {
            { CategoryModLocation, new OrderedDictionary<string, Setting>() {
                { HLDirectory, Setting.Create( new I18N.I18NString(HLDirectory, "Half-Life directory"), HLVRPaths.LastHLDirectory ).MakeFolderSetting() },
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
        };

        public IEnumerable<KeyValuePair<string, Setting>> KeyValuePairs
        {
            get
            {
                foreach (var foo in new[] { LauncherSettings })
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
            foreach (var foo in new[] { LauncherSettings })
            {
                foreach (var bar in foo)
                {
                    if (bar.Value.TryGetValue(key, out Setting setting))
                    {
                        setting.SetValue(value);
                        return true;
                    }
                }
            }
            return false;
        }

        public HLVRLauncherSettings()
        {
        }
    }
}
