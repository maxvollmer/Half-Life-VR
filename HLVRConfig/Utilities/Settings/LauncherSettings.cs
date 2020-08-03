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

        public static readonly SettingCategory CategoryModLocation = new SettingCategory(new I18N.I18NString("LauncherSettings.ModLocation", "Mod Directory"));
        public static readonly SettingCategory CategoryLanguage = new SettingCategory(new I18N.I18NString("LauncherSettings.Language", "Language"));
        public static readonly SettingCategory CategoryLauncher = new SettingCategory(new I18N.I18NString("LauncherSettings.Launcher", "Launcher"));
        public static readonly SettingCategory CategoryLog = new SettingCategory(new I18N.I18NString("LauncherSettings.Log", "Log Settings"));
        public static readonly SettingCategory CategoryDebug = new SettingCategory(new I18N.I18NString("LauncherSettings.Debug", "Debug Settings"));

        public static readonly string MinimizeToTray = "MinimizeToTray";
        public static readonly string StartMinimized = "StartMinimized";
        public static readonly string AutoRunMod = "AutoRunMod";
        public static readonly string AutoCloseLauncher = "AutoCloseLauncher";
        public static readonly string AutoCloseGame = "AutoCloseGame";
        public static readonly string HLDirectory = "HLDirectory";
        public static readonly string Language = "Language";
        public static readonly string NumberOfDisplayedLogLines = "NumberOfDisplayedLogLines";
        public static readonly string EnableMiniDumpButton = "EnableMiniDumpButton";
        public static readonly string EnableFullMemoryMiniDump = "EnableFullMemoryMiniDump";


        public OrderedDictionary<SettingCategory, OrderedDictionary<string, Setting>> GeneralSettings = new OrderedDictionary<SettingCategory, OrderedDictionary<string, Setting>>()
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

            { CategoryDebug, new OrderedDictionary<string, Setting>() {
                { EnableMiniDumpButton, Setting.Create( new I18N.I18NString(EnableMiniDumpButton, "Enable Mini Dump Button"), false ) },
                { EnableFullMemoryMiniDump, Setting.Create( new I18N.I18NString(EnableFullMemoryMiniDump, "Enable Full Memory Mini Dump (Creates huge files! Only enable this if directed or if you know what you are doing!)"), false, new SettingDependency(EnableMiniDumpButton, "1") ) },
            } },
        };

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
                        setting.SetValue(value);
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
