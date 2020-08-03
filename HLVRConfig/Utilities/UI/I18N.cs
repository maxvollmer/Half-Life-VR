using HLVRConfig.Utilities.Process;
using HLVRConfig.Utilities.Settings;
using Microsoft.Collections.Extensions;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace HLVRConfig.Utilities.UI
{
    public class I18N
    {
        private static readonly System.UInt32 MUI_LANGUAGE_ID = 0x4;      // Use traditional language ID convention
        // private static readonly System.UInt32 MUI_LANGUAGE_NAME = 0x8;    // Use ISO language (culture) name convention

        [DllImport("Kernel32.dll", CharSet = CharSet.Auto)]
        static extern System.Boolean GetUserPreferredUILanguages(
            System.UInt32 dwFlags,
            ref System.UInt32 pulNumLanguages,
            System.IntPtr pwszLanguagesBuffer,
            ref System.UInt32 pcchLanguagesBuffer
            );


        public class I18NString
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
            string previouslanguage = CurrentLanguage;

            CurrentLanguage = HLVRSettingsManager.LauncherSettings.GeneralSettings[LauncherSettings.CategoryLanguage][LauncherSettings.Language].Value;
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

                    using (StringReader reader = new StringReader(File.ReadAllText(I18NFile, Encoding.UTF8)))
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

            if (previouslanguage != CurrentLanguage)
            {
                System.Windows.Application.Current?.Dispatcher?.BeginInvoke((Action)(() => (System.Windows.Application.Current?.MainWindow as MainWindow)?.RefreshConfigTabs(true, true)));
            }
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
                File.WriteAllText(I18NFile, string.Join("\n", lines), Encoding.UTF8);
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

            System.Windows.Application.Current?.Dispatcher?.BeginInvoke((Action)(() => (System.Windows.Application.Current?.MainWindow as MainWindow)?.UpdateGUITexts()));

            UpdateSpeechLanguages();
        }

        private static void UpdateSpeechLanguages()
        {
            HLVRSettingsManager.ModSettings.SpeechRecognitionLanguages.Clear();

            uint pulNumLanguages = 0;
            uint pcchLanguagesBuffer = 0;
            if (GetUserPreferredUILanguages(MUI_LANGUAGE_ID, ref pulNumLanguages, IntPtr.Zero, ref pcchLanguagesBuffer)
                && pcchLanguagesBuffer > 0)
            {
                // size is given in wchar characters, so we need to allocate size*2 bytes
                IntPtr pwszLanguagesBuffer = Marshal.AllocHGlobal((int)pcchLanguagesBuffer * 2);

                if (GetUserPreferredUILanguages(MUI_LANGUAGE_ID, ref pulNumLanguages, pwszLanguagesBuffer, ref pcchLanguagesBuffer))
                {
                    if (pulNumLanguages > 0)
                    {
                        // make sure sizes are correct:
                        // each language is 4 characters + 0 delimiter and the entire list ends with an additional 0
                        if (pulNumLanguages * 5 + 1 == pcchLanguagesBuffer)
                        {
                            for (int i = 0; i < pulNumLanguages; i++)
                            {
                                // copy language (4 characters excluding 0) into char array
                                // all characters are 0-F, so we can safely cast them to char and create a string to parse the value from
                                char[] language = new char[4];
                                Marshal.Copy(IntPtr.Add(pwszLanguagesBuffer, i * 5), language, 0, 4);
                                string hexCultureId = new string(language);
                                CultureInfo cultureInfo;
                                try
                                {
                                    int cultureId = I18N.ParseInt(hexCultureId, NumberStyles.HexNumber);
                                    cultureInfo = new CultureInfo(cultureId, true);
                                }
                                catch (Exception)
                                {
                                    continue;
                                }
                                HLVRSettingsManager.ModSettings.SpeechRecognitionLanguages[hexCultureId] = new I18NString("vr_speech_language_id." + hexCultureId, cultureInfo.DisplayName);
                            }
                        }
                    }
                }

                Marshal.FreeHGlobal(pwszLanguagesBuffer);
            }

            if (HLVRSettingsManager.ModSettings.SpeechRecognitionLanguages.Count == 0)
            {
                HLVRSettingsManager.ModSettings.SpeechRecognitionLanguages["1400"] = new I18NString("vr_speech_language_id.1400", "System Default");
            }

            // Make sure selected language exists in current list - if not, select first available language
            if (!HLVRSettingsManager.ModSettings.SpeechRecognitionLanguages.ContainsKey(HLVRSettingsManager.ModSettings.AudioSettings[ModSettings.CategorySpeechRecognition][ModSettings.SpeechRecognitionLanguage].Value))
            {
                HLVRSettingsManager.TrySetSetting(HLVRSettingsManager.ModSettings.AudioSettings, ModSettings.CategorySpeechRecognition, ModSettings.SpeechRecognitionLanguage, "vr_speech_language_id." + HLVRSettingsManager.ModSettings.SpeechRecognitionLanguages.Keys.First());
            }
        }

        public static int ParseInt(string value, NumberStyles style = NumberStyles.Integer)
        {
            return int.Parse(value, style, CultureInfo.InvariantCulture.NumberFormat);
        }

        public static bool TryParseInt(string value, out int result)
        {
            return int.TryParse(value, NumberStyles.Integer, CultureInfo.InvariantCulture.NumberFormat, out result);
        }

        public static string IntToString(int value)
        {
            return value.ToString(CultureInfo.InvariantCulture.NumberFormat);
        }

        public static float ParseFloat(string value)
        {
            return float.Parse(value, NumberStyles.Float, CultureInfo.InvariantCulture.NumberFormat);
        }

        public static bool TryParseFloat(string value, out float result)
        {
            return float.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture.NumberFormat, out result);
        }

        public static string FloatToString(float value)
        {
            return value.ToString(CultureInfo.InvariantCulture.NumberFormat);
        }
    }
}
