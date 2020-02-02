using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using Microsoft.Collections.Extensions;
using Newtonsoft.Json;

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
        private static StoreLoadTask currentStoreLoadTask = StoreLoadTask.NONE;
        private static EventWaitHandle storeLoadTaskWaitHandle;
        private static readonly object storeLoadTaskLock = new object();
        private static Stopwatch storeLoadTaskStopWatch = new Stopwatch();
        private static int storeLoadTaskTimeToWait = 1000;

        private static readonly object settingsFileLock = new object();

        public static HLVRSettings Settings { get; private set; } = new HLVRSettings();

        private static FileSystemWatcher FileSystemWatcher;

        public static bool IsInitialized { get; private set; } = false;

        public static void InitSettings()
        {
            if (IsInitialized)
            {
                // TODO: Check if paths have changed
                return;
            }

            if (!HLVRPaths.CheckHLDirectory() || !HLVRPaths.CheckModDirectory())
            {
                IsInitialized = false;
                return;
            }

            if (File.Exists(HLVRPaths.VRSettingsFile))
            {
                if (!TryLoadSettings())
                {
                    var result = MessageBox.Show("Couldn't load settings file. If you chose OK, HLVRConfig will replace settings with default values. If you chose Cancel, config tabs will not be available.", "Error", MessageBoxButton.OKCancel, MessageBoxImage.Warning);
                    if (result != MessageBoxResult.OK)
                    {
                        IsInitialized = false;
                        return;
                    }
                }
            }

            if (!TryStoreSettings())
            {
                MessageBox.Show("Couldn't synchronize settings. Config tabs will not be available.", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                IsInitialized = false;
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
                NotifyFilter = NotifyFilters.LastWrite,
                //Filter = HLVRPaths.VRSettingsFileName
            };
            FileSystemWatcher.Changed += FileSystemWatcher_Changed;

            IsInitialized = true;
        }

        private static void FileSystemWatcher_Changed(object sender, FileSystemEventArgs e)
        {
            if (HLVRPaths.VRSettingsFileName.Equals(e.Name))
            {
                DelayedLoadSettings();
            }
        }

        public static void SetLauncherSetting(string category, string name, bool value)
        {
            Settings.LauncherSettings[category][name].Value = value ? "1" : "0";
            DelayedStoreSettings();
        }

        public static void SetLauncherSetting(string category, string name, string value)
        {
            Settings.LauncherSettings[category][name].Value = value;
            DelayedStoreSettings();
        }

        internal static void SetModSetting(OrderedDictionary<string, OrderedDictionary<string, Setting>> settings, string category, string name, bool value)
        {
            settings[category][name].Value = value ? "1" : "0";
            DelayedStoreSettings();
        }

        internal static void SetModSetting(OrderedDictionary<string, OrderedDictionary<string, Setting>> settings, string category, string name, string value)
        {
            if (settings[category][name].AllowedValues.Count > 0)
            {
                foreach (var allowedValue in settings[category][name].AllowedValues)
                {
                    if (allowedValue.Value.Equals(value))
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
            DelayedStoreSettings();
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
            Settings.InputSettings = defaultSettings.InputSettings;
            Settings.GraphicsSettings = defaultSettings.GraphicsSettings;
            Settings.AudioSettings = defaultSettings.AudioSettings;
            Settings.OtherSettings = defaultSettings.OtherSettings;
            DelayedStoreSettings();
        }

        private static bool TryStoreSettings()
        {
            lock (settingsFileLock)
            {
                try
                {
                    File.WriteAllText(HLVRPaths.VRSettingsFile, JsonConvert.SerializeObject(Settings));
                    return true;
                }
                catch (Exception)
                {
                    return false;
                }
            }
        }

        private static void CopyNewSettings(OrderedDictionary<string, OrderedDictionary<string, Setting>> settings, OrderedDictionary<string, OrderedDictionary<string, Setting>> newSettings)
        {
            foreach (var settingCategory in settings)
            {
                foreach (var settingName in settingCategory.Value.Keys)
                {
                    try
                    {
                        settingCategory.Value[settingName].Value = newSettings[settingCategory.Key][settingName].Value;
                    }
                    catch (KeyNotFoundException) { }
                }
            }
        }

        private static bool TryLoadSettings()
        {
            lock (settingsFileLock)
            {
                try
                {
                    var newSettings = JsonConvert.DeserializeObject<HLVRSettings>(File.ReadAllText(HLVRPaths.VRSettingsFile));
                    CopyNewSettings(Settings.LauncherSettings, newSettings.LauncherSettings);
                    CopyNewSettings(Settings.InputSettings, newSettings.AudioSettings);
                    CopyNewSettings(Settings.GraphicsSettings, newSettings.GraphicsSettings);
                    CopyNewSettings(Settings.AudioSettings, newSettings.AudioSettings);
                    CopyNewSettings(Settings.OtherSettings, newSettings.OtherSettings);
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

        private static void DelayedStoreSettings()
        {
            lock (storeLoadTaskLock)
            {
                // wait a full second before storing (that way changing multiple settings won't result in lots of stores, every change will push the time further, the file is stored 1 second after the last change)
                storeLoadTaskTimeToWait = 1000;
                storeLoadTaskStopWatch = Stopwatch.StartNew();
                currentStoreLoadTask = StoreLoadTask.STORE;
            }
            storeLoadTaskWaitHandle.Set();
        }

        private static void DelayedLoadSettings()
        {
            lock (storeLoadTaskLock)
            {
                // we only wait 100ms when loading
                storeLoadTaskTimeToWait = 100;
                storeLoadTaskStopWatch = Stopwatch.StartNew();
                currentStoreLoadTask = StoreLoadTask.LOAD;
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
                        if (currentStoreLoadTask == StoreLoadTask.LOAD)
                        {
                            if (TryLoadSettings())
                            {
                                currentStoreLoadTask = StoreLoadTask.NONE;
                                System.Windows.Application.Current.Dispatcher.BeginInvoke((Action)(() => ((MainWindow)System.Windows.Application.Current.MainWindow)?.RefreshConfigTabs()));
                            }
                        }
                        else if (currentStoreLoadTask == StoreLoadTask.STORE)
                        {
                            if (TryStoreSettings())
                            {
                                currentStoreLoadTask = StoreLoadTask.NONE;
                            }
                        }
                    }
                }
            }
        }
    }
}
