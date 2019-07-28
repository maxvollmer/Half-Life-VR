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

namespace HLVRLauncher.Utilities
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
        private static object storeLoadTaskLock = new object();
        private static Stopwatch storeLoadTaskStopWatch = new Stopwatch();
        private static int storeLoadTaskTimeToWait = 1000;

        private static object settingsFileLock = new object();

        public static HLVRSettings Settings { get; private set; } = new HLVRSettings();

        private static FileSystemWatcher FileSystemWatcher;

        public static void InitSettings()
        {
            if (File.Exists(HLVRPaths.VRSettingsFile))
            {
                if (!TryLoadSettings())
                {
                    var result = MessageBox.Show("Couldn't load settings file. If you chose OK, HLVRLauncher will replace settings with default values. If you chose Cancel, HLVRLauncher will exit.", "Error", MessageBoxButton.OKCancel, MessageBoxImage.Warning);
                    if (result != MessageBoxResult.OK)
                    {
                        throw new CancelAndTerminateAppException();
                    }
                }
            }

            if (!TryStoreSettings())
            {
                var result = MessageBox.Show("Couldn't synchronize settings. If you chose OK, HLVRLauncher will run, but settings shown here might not reflect actual settings stored in the mod. If you chose Cancel, HLVRLauncher will exit.", "Error", MessageBoxButton.OKCancel, MessageBoxImage.Warning);
                if (result != MessageBoxResult.OK)
                {
                    throw new CancelAndTerminateAppException();
                }
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
        }

        private static void FileSystemWatcher_Changed(object sender, FileSystemEventArgs e)
        {
            if (HLVRPaths.VRSettingsFileName.Equals(e.Name))
            {
                DelayedLoadSettings();
            }
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

        private static bool TryLoadSettings()
        {
            lock (settingsFileLock)
            {
                try
                {
                    var newSettings = JsonConvert.DeserializeObject<HLVRSettings>(File.ReadAllText(HLVRPaths.VRSettingsFile));
                    foreach (var settingName in Settings.LauncherSettings.Keys)
                    {
                        Settings.LauncherSettings[settingName].Value = newSettings.LauncherSettings[settingName].Value;
                    }
                    foreach (var settingCategory in Settings.ModSettings)
                    {
                        foreach (var settingName in settingCategory.Value.Keys)
                        {
                            settingCategory.Value[settingName].Value = newSettings.ModSettings[settingCategory.Key][settingName].Value;
                        }
                    }
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
