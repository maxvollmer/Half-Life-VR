using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using HLVRConfig.Utilities.Settings;
using Microsoft.Collections.Extensions;

namespace HLVRConfig.Utilities
{
    public class HLVRSettingsManager
    {
        private static bool isDisposed = false;

        private enum StoreLoadTask
        {
            NONE,
            STORE,
            LOAD
        }

        private static Thread storeLoadThread;
        private static StoreLoadTask currentModStoreLoadTask = StoreLoadTask.NONE;
        private static StoreLoadTask currentLauncherStoreLoadTask = StoreLoadTask.NONE;
        private static EventWaitHandle storeLoadTaskWaitHandle;
        private static readonly object storeLoadTaskLock = new object();
        private static Stopwatch storeLoadTaskStopWatch = new Stopwatch();
        private static int storeLoadTaskTimeToWait = 1000;

        private static readonly object settingsFileLock = new object();

        public static HLVRModSettings ModSettings { get; private set; } = new HLVRModSettings();
        public static HLVRLauncherSettings LauncherSettings { get; private set; } = new HLVRLauncherSettings();

        private static FileSystemWatcher FileSystemWatcher;

        public static bool AreModSettingsInitialized { get; private set; } = false;
        public static bool AreLauncherSettingsInitialized { get; private set; } = false;

        private static string ModPath = null;
        private static string VRPath = null;

        private static void InitLauncherSettings()
        {
            if (AreLauncherSettingsInitialized)
                return;

            LauncherSettings = new HLVRLauncherSettings();

            if (File.Exists(HLVRPaths.VRLauncherSettingsFile))
            {
                AreLauncherSettingsInitialized =
                    TryLoadSettings(HLVRPaths.VRLauncherSettingsFile)
                    && TryStoreSettings(LauncherSettings, HLVRPaths.VRLauncherSettingsFile);
            }
            else
            {
                AreLauncherSettingsInitialized = TryStoreSettings(LauncherSettings, HLVRPaths.VRLauncherSettingsFile);
            }

            if (string.IsNullOrEmpty(HLVRPaths.HLDirectory))
            {
                HLVRPaths.RestoreLastHLDirectory();
            }
        }

        private static void InitModSettings()
        {
            // If paths have changed, we need to reload mod settings
            if (ModPath == null || VRPath == null || ModPath != HLVRPaths.HLDirectory || VRPath != HLVRPaths.VRDirectory)
            {
                AreModSettingsInitialized = false;
            }

            if (AreModSettingsInitialized)
            {
                return;
            }

            ModSettings = new HLVRModSettings();

            if (!HLVRPaths.CheckHLDirectory() || !HLVRPaths.CheckModDirectory())
            {
                AreModSettingsInitialized = false;
                return;
            }

            if (File.Exists(HLVRPaths.VRModSettingsFile))
            {
                if (!TryLoadSettings(HLVRPaths.VRModSettingsFile))
                {
                    var errorMsg = new I18N.I18NString("ErrorMsgCouldNotLoadModSettings", "Couldn't load mod settings file. If you chose OK, HLVRConfig will replace settings with default values. If you chose Cancel, config tabs will not be available.");
                    var errorTitle = new I18N.I18NString("Error", "Error");
                    var result = MessageBox.Show(I18N.Get(errorMsg), I18N.Get(errorTitle), MessageBoxButton.OKCancel, MessageBoxImage.Warning);
                    if (result != MessageBoxResult.OK)
                    {
                        AreModSettingsInitialized = false;
                        return;
                    }
                }
            }

            if (!TryStoreSettings(ModSettings, HLVRPaths.VRModSettingsFile))
            {
                var errorMsg = new I18N.I18NString("ErrorMsgCouldNotSynchronizeModSettings", "Couldn't synchronize mod settings. Config tabs are not available.");
                var errorTitle = new I18N.I18NString("Error", "Error");
                MessageBox.Show(I18N.Get(errorMsg), I18N.Get(errorTitle), MessageBoxButton.OK, MessageBoxImage.Warning);
                AreModSettingsInitialized = false;
                return;
            }

            storeLoadTaskWaitHandle = new EventWaitHandle(false, EventResetMode.AutoReset);

            storeLoadThread = new Thread(StoreLoadWorkerLoop)
            {
                IsBackground = true
            };
            storeLoadThread.Start();

            FileSystemWatcher = new FileSystemWatcher
            {
                Path = HLVRPaths.VRDirectory,
                EnableRaisingEvents = true,
                IncludeSubdirectories = false,
                NotifyFilter = NotifyFilters.LastWrite
            };
            FileSystemWatcher.Changed += FileSystemWatcher_Changed;

            ModPath = HLVRPaths.HLDirectory;
            VRPath = HLVRPaths.VRDirectory;
            AreModSettingsInitialized = true;
        }

        public static void InitSettings()
        {
            InitModSettings();
            InitLauncherSettings();
        }

        private static void FileSystemWatcher_Changed(object sender, FileSystemEventArgs e)
        {
            if (HLVRPaths.VRModSettingsFileName.Equals(e.Name))
            {
                DelayedLoadModSettings();
            }
            else if (HLVRPaths.VRLauncherSettingsFileName.Equals(e.Name))
            {
                DelayedLoadLauncherSettings();
            }
        }

        public static void SetLauncherSetting(I18N.I18NString category, string name, bool value)
        {
            LauncherSettings.LauncherSettings[category][name].Value = value ? "1" : "0";
            DelayedStoreLauncherSettings();
        }

        public static void SetLauncherSetting(I18N.I18NString category, string name, string value)
        {
            if (LauncherSettings.LauncherSettings[category][name].AllowedValues.Count > 0)
            {
                foreach (var allowedValue in LauncherSettings.LauncherSettings[category][name].AllowedValues)
                {
                    if (allowedValue.Value.Key.Equals(value))
                    {
                        LauncherSettings.LauncherSettings[category][name].Value = allowedValue.Key;
                        break;
                    }
                }
            }
            else
            {
                LauncherSettings.LauncherSettings[category][name].Value = value;
            }
            DelayedStoreLauncherSettings();
        }

        internal static void SetModSetting(OrderedDictionary<I18N.I18NString, OrderedDictionary<string, Setting>> settings, I18N.I18NString category, string name, bool value)
        {
            settings[category][name].Value = value ? "1" : "0";
            DelayedStoreModSettings();
        }

        internal static void SetModSetting(OrderedDictionary<I18N.I18NString, OrderedDictionary<string, Setting>> settings, I18N.I18NString category, string name, string value)
        {
            if (settings[category][name].AllowedValues.Count > 0)
            {
                foreach (var allowedValue in settings[category][name].AllowedValues)
                {
                    if (allowedValue.Value.Key.Equals(value))
                    {
                        settings[category][name].Value = allowedValue.Key;
                        break;
                    }
                }
            }
            else
            {
                settings[category][name].Value = value;
            }
            DelayedStoreModSettings();
        }

        internal static void RestoreLauncherSettings()
        {
            HLVRLauncherSettings defaultSettings = new HLVRLauncherSettings();
            LauncherSettings.LauncherSettings = defaultSettings.LauncherSettings;
            DelayedStoreLauncherSettings();
        }

        internal static void RestoreModSettings()
        {
            HLVRModSettings defaultSettings = new HLVRModSettings();
            ModSettings.InputSettings = defaultSettings.InputSettings;
            ModSettings.GraphicsSettings = defaultSettings.GraphicsSettings;
            ModSettings.AudioSettings = defaultSettings.AudioSettings;
            ModSettings.OtherSettings = defaultSettings.OtherSettings;
            DelayedStoreModSettings();
        }

        private static bool TryStoreSettings(ISettingsContainer settings, string file)
        {
            lock (settingsFileLock)
            {
                try
                {
                    File.WriteAllText(file, SettingsToText(settings));
                    return true;
                }
                catch (Exception)
                {
                    return false;
                }
            }
        }

        private static bool TryLoadSettings(string file)
        {
            lock (settingsFileLock)
            {
                try
                {
                    string text = File.ReadAllText(file);
                    TextToSettings(text, LauncherSettings);
                    TextToSettings(text, ModSettings);
                    return true;
                }
                catch (Exception)
                {
                    return false;
                }
            }
        }

        public static void Dispose()
        {
            lock (storeLoadTaskLock)
            {
                isDisposed = true;
            }
            storeLoadTaskWaitHandle.Set();
        }

        private static void DelayedStoreModSettings()
        {
            lock (storeLoadTaskLock)
            {
                // wait a full second before storing (that way changing multiple settings won't result in lots of stores, every change will push the time further, the file is stored 1 second after the last change)
                storeLoadTaskTimeToWait = 1000;
                storeLoadTaskStopWatch = Stopwatch.StartNew();
                currentModStoreLoadTask = StoreLoadTask.STORE;
            }
            storeLoadTaskWaitHandle.Set();
        }

        private static void DelayedLoadModSettings()
        {
            lock (storeLoadTaskLock)
            {
                // we only wait 100ms when loading
                storeLoadTaskTimeToWait = 100;
                storeLoadTaskStopWatch = Stopwatch.StartNew();
                currentModStoreLoadTask = StoreLoadTask.LOAD;
            }
            storeLoadTaskWaitHandle.Set();
        }

        private static void DelayedStoreLauncherSettings()
        {
            lock (storeLoadTaskLock)
            {
                // wait a full second before storing (that way changing multiple settings won't result in lots of stores, every change will push the time further, the file is stored 1 second after the last change)
                storeLoadTaskTimeToWait = 1000;
                storeLoadTaskStopWatch = Stopwatch.StartNew();
                currentLauncherStoreLoadTask = StoreLoadTask.STORE;
            }
            storeLoadTaskWaitHandle.Set();
        }

        private static void DelayedLoadLauncherSettings()
        {
            lock (storeLoadTaskLock)
            {
                // we only wait 100ms when loading
                storeLoadTaskTimeToWait = 100;
                storeLoadTaskStopWatch = Stopwatch.StartNew();
                currentLauncherStoreLoadTask = StoreLoadTask.LOAD;
            }
            storeLoadTaskWaitHandle.Set();
        }

        private static void StoreLoadWorkerLoop()
        {
            while (true)
            {
                storeLoadTaskWaitHandle.WaitOne(100);

                lock (storeLoadTaskLock)
                {
                    if (isDisposed)
                        return;

                    if (storeLoadTaskStopWatch.Elapsed.TotalMilliseconds > storeLoadTaskTimeToWait)
                    {
                        if (currentModStoreLoadTask == StoreLoadTask.LOAD)
                        {
                            if (TryLoadSettings(HLVRPaths.VRModSettingsFile))
                            {
                                currentModStoreLoadTask = StoreLoadTask.NONE;
                                System.Windows.Application.Current.Dispatcher.BeginInvoke((Action)(() => ((MainWindow)System.Windows.Application.Current?.MainWindow)?.RefreshConfigTabs(true, false)));
                            }
                        }
                        else if (currentModStoreLoadTask == StoreLoadTask.STORE)
                        {
                            if (TryStoreSettings(ModSettings, HLVRPaths.VRModSettingsFile))
                            {
                                currentModStoreLoadTask = StoreLoadTask.NONE;
                            }
                        }

                        if (currentLauncherStoreLoadTask == StoreLoadTask.LOAD)
                        {
                            if (TryLoadSettings(HLVRPaths.VRLauncherSettingsFile))
                            {
                                currentLauncherStoreLoadTask = StoreLoadTask.NONE;
                                System.Windows.Application.Current.Dispatcher.BeginInvoke((Action)(() => ((MainWindow)System.Windows.Application.Current?.MainWindow)?.RefreshConfigTabs(false, true)));
                            }
                        }
                        else if (currentLauncherStoreLoadTask == StoreLoadTask.STORE)
                        {
                            if (TryStoreSettings(LauncherSettings, HLVRPaths.VRLauncherSettingsFile))
                            {
                                currentLauncherStoreLoadTask = StoreLoadTask.NONE;
                                System.Windows.Application.Current.Dispatcher.BeginInvoke((Action)(() => ((MainWindow)System.Windows.Application.Current?.MainWindow)?.RefreshConfigTabs(true, true)));
                            }
                        }
                    }
                }
            }
        }

        private static string SettingsToText(ISettingsContainer settings)
        {
            return string.Join("\n", settings.KeyValuePairs.ToList().ConvertAll(pair => pair.Key + "=" + pair.Value.Value));
        }

        private static void TextToSettings(string text, ISettingsContainer settings)
        {
            using (StringReader reader = new StringReader(text))
            {
                string line;
                while ((line = reader.ReadLine()) != null)
                {
                    var pair = line.Split(new char[] { '=' }, 2);
                    if (pair.Length == 2)
                    {
                        settings.SetValue(pair[0], pair[1]);
                    }
                }
            }
        }
    }
}
