using Microsoft.Collections.Extensions;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HLVRConfig.Utilities
{
    public class I18N
    {
        public struct I18NString
        {
            public string Key;
            public string DefaultValue;

            public I18NString(string key, string defaultValue)
            {
                Key = key;
                DefaultValue = defaultValue;
            }

            public I18NString(string keyIsValue)
            {
                Key = keyIsValue;
                DefaultValue = keyIsValue;
            }

            public override string ToString()
            {
                return I18N.Get(this);
            }
        }

        private static string CurrentLanguage { get; set; } = "en";

        private static string I18NFile { get { return Path.Combine(HLVRPaths.VRI18NDirectory, CurrentLanguage + ".lang"); } }

        private static readonly Dictionary<string, string> texts = new Dictionary<string, string>();

        public static void Init()
        {
            CurrentLanguage = HLVRSettingsManager.LauncherSettings.LauncherSettings[HLVRLauncherSettings.CategoryLanguage][HLVRLauncherSettings.Language].Value;
            if (CurrentLanguage == null || CurrentLanguage.Length == 0)
            {
                CurrentLanguage = "en";
            }

            if (HLVRPaths.CheckModDirectory())
            {
                if (!Directory.Exists(HLVRPaths.VRI18NDirectory))
                {
                    Directory.CreateDirectory(HLVRPaths.VRI18NDirectory);
                }
                if (File.Exists(I18NFile))
                {
                    texts.Clear();

                    using (StringReader reader = new StringReader(File.ReadAllText(I18NFile)))
                    {
                        string line;
                        while ((line = reader.ReadLine()) != null)
                        {
                            var pair = line.Split(new char[] { '=' }, 2);
                            if (pair.Length == 2)
                            {
                                texts[pair[0]] = pair[1];
                            }
                        }
                    }
                }
            }

            UpdateLanguages();
        }

        public static void SaveI18NFile()
        {
            if (HLVRPaths.CheckModDirectory())
            {
                if (!Directory.Exists(HLVRPaths.VRI18NDirectory))
                {
                    Directory.CreateDirectory(HLVRPaths.VRI18NDirectory);
                }
                var lines = texts.ToList().ConvertAll(pair => pair.Key + "=" + pair.Value);
                lines.Sort();
                File.WriteAllText(I18NFile, string.Join("\n", lines));
            }
        }

        public static string Get(I18NString i18nString)
        {
            if (texts.TryGetValue(i18nString.Key, out string value)) 
            {
                return value;
            }
            else
            {
                texts[i18nString.Key] = i18nString.DefaultValue;
                return i18nString.DefaultValue;
            }
        }

        private static void UpdateLanguages()
        {
            HLVRSettingsManager.LauncherSettings.Languages.Clear();
            HLVRSettingsManager.LauncherSettings.Languages["en"] = new I18NString("Language.en", "English");

            if (HLVRPaths.CheckModDirectory())
            {
                foreach (var file in Directory.GetFiles(HLVRPaths.VRI18NDirectory, "*.lang", SearchOption.AllDirectories))
                {
                    var language = Path.GetFileNameWithoutExtension(file);
                    if (language.Length > 0 && language != "en")
                    {
                        HLVRSettingsManager.LauncherSettings.Languages[language] = new I18NString("Language." + language, language);
                    }
                }
            }

            System.Windows.Application.Current.Dispatcher.BeginInvoke((Action)(() => ((MainWindow)System.Windows.Application.Current?.MainWindow)?.UpdateGUITexts()));
        }
    }
}
