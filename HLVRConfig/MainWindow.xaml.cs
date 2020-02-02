using DbMon.NET;
using HLVRConfig.Utilities;
using System;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Windows;
using System.Windows.Documents;
using System.Windows.Interop;
using System.Windows.Media;

namespace HLVRConfig
{
    public partial class MainWindow : Window
    {
        private System.Windows.Forms.NotifyIcon notifyIcon;

        private readonly SingleProcessEnforcer singleProcessEnforcer = new SingleProcessEnforcer();
        private readonly HLVRModLauncher hlvrModLauncher = new HLVRModLauncher();

        public bool IsValidProcess
        {
            get;
            private set;
        } = false;

        public MainWindow()
        {
            try
            {
                singleProcessEnforcer.ForceSingleProcess();
                HLVRPaths.Initialize();

                HLVRSettingsManager.InitSettings();
                RefreshConfigTabs();

                InitializeNotifyIcon();
                UpdateState();
                HandleInitialSettings();
                LoadReadme();

                InitializeComponent();
                IsValidProcess = true;
            }
            catch (CancelAndTerminateAppException)
            {
                System.Windows.Application.Current.Shutdown();
            }
        }

        private void LoadReadme()
        {
            AboutText.Inlines.Clear();
            try
            {
                AboutText.Inlines.Add(File.ReadAllText(HLVRPaths.VRReadme, Encoding.UTF8));
            }
            catch (IOException e)
            {
                AboutText.Inlines.Clear();
                AboutText.Inlines.Add("Couldn't load README.txt: " + e.Message);
            }
        }

        private void HandleInitialSettings()
        {
            if (HLVRSettingsManager.Settings.LauncherSettings[HLVRLauncherConfig.CategoryLauncher][HLVRLauncherConfig.StartMinimized].IsTrue())
            {
                WindowState = WindowState.Minimized;
                OnWindowStateChanged();
            }
            if (HLVRSettingsManager.Settings.LauncherSettings[HLVRLauncherConfig.CategoryLauncher][HLVRLauncherConfig.AutoRunMod].IsTrue())
            {
                hlvrModLauncher.LaunchMod(false);
            }
        }

        protected override void OnSourceInitialized(EventArgs e)
        {
            base.OnSourceInitialized(e);
            HwndSource source = HwndSource.FromHwnd(new WindowInteropHelper(this).Handle);
            source.AddHook(new HwndSourceHook(WndProc));
        }

        private void InitializeNotifyIcon()
        {
            notifyIcon = new System.Windows.Forms.NotifyIcon();
            notifyIcon.Icon = System.Drawing.Icon.ExtractAssociatedIcon(System.Reflection.Assembly.GetEntryAssembly().ManifestModule.Name);
            notifyIcon.Click += TrayIconClicked;
            notifyIcon.DoubleClick += TrayIconClicked;
            notifyIcon.MouseClick += TrayIconMouseClicked;
            notifyIcon.MouseDoubleClick += TrayIconMouseClicked;
        }

        private void TrayIconClicked(object sender, EventArgs e)
        {
            BringToForeground();
        }

        private void TrayIconMouseClicked(object sender, System.Windows.Forms.MouseEventArgs e)
        {
            BringToForeground();
        }

        public void BringToForeground()
        {
            if (WindowState == WindowState.Minimized || Visibility == Visibility.Hidden)
            {
                Show();
                WindowState = WindowState.Normal;
            }
            Activate();
            var topmost = Topmost;
            Topmost = true;
            Topmost = false;
            Topmost = topmost;
            Focus();
            UpdateState();
        }

        public void UpdateState()
        {
            if (!HLVRPaths.CheckHLDirectory() || !HLVRPaths.CheckModDirectory())
            {
                ModNotFoundPanel.Visibility = Visibility.Visible;
                NotRunningGamePanel.Visibility = Visibility.Collapsed;
                RunningGamePannel.Visibility = Visibility.Collapsed;
            }
            else
            {
                ModNotFoundPanel.Visibility = Visibility.Collapsed;
                NotRunningGamePanel.Visibility = hlvrModLauncher.IsGameRunning() ? Visibility.Collapsed : Visibility.Visible;
                RunningGamePannel.Visibility = hlvrModLauncher.IsGameRunning() ? Visibility.Visible : Visibility.Collapsed;
                HLVRSettingsManager.InitSettings();
            }
        }

        protected override void OnClosing(CancelEventArgs e)
        {
            if (IsValidProcess 
                && hlvrModLauncher.IsGameRunning()
                && HLVRSettingsManager.Settings.LauncherSettings[HLVRLauncherConfig.CategoryLauncher][HLVRLauncherConfig.AutoCloseGame].IsTrue())
            {
                hlvrModLauncher.TerminateGame();
            }

            singleProcessEnforcer.Dispose();
            try { if (DebugMonitor.IsStarted) DebugMonitor.Stop(); } catch (Exception) { }
        }

        private static IntPtr WndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled)
        {
            System.Windows.Application.Current.Dispatcher.BeginInvoke((Action)(() => ((MainWindow)System.Windows.Application.Current.MainWindow)?.OnWindowStateChanged()));
            return IntPtr.Zero;
        }

        private void OnWindowStateChanged()
        {
            if (WindowState == WindowState.Minimized
                && HLVRSettingsManager.Settings.LauncherSettings[HLVRLauncherConfig.CategoryLauncher][HLVRLauncherConfig.MinimizeToTray].IsTrue())
            {
                ShowInTaskbar = false;
                notifyIcon.Visible = true;
            }
            else
            {
                notifyIcon.Visible = false;
                ShowInTaskbar = true;
            }
        }

        private void LaunchMod_Click(object sender, RoutedEventArgs e)
        {
            hlvrModLauncher.LaunchMod(true);
            UpdateState();
        }

        private void ClearConsole_Click(object sender, RoutedEventArgs e)
        {
            ConsoleOutput.Inlines.Clear();
        }

        public void RefreshConfigTabs()
        {
            HLVRSettingsManager.InitSettings();
            if (HLVRSettingsManager.IsInitialized)
            {
                HLVRLauncherConfig.Initialize(LauncherConfig);
                HLVRModConfig.Initialize(InputConfig, HLVRSettingsManager.Settings.InputSettings);
                HLVRModConfig.Initialize(GraphicsConfig, HLVRSettingsManager.Settings.GraphicsSettings);
                HLVRModConfig.Initialize(AudioConfig, HLVRSettingsManager.Settings.AudioSettings);
                HLVRModConfig.Initialize(OtherConfig, HLVRSettingsManager.Settings.OtherSettings);
            }
            else
            {
                LauncherConfig.Children.Clear();
                InputConfig.Children.Clear();
                GraphicsConfig.Children.Clear();
                AudioConfig.Children.Clear();
                OtherConfig.Children.Clear();
            }
        }

        private void RestoreDefaultLauncherConfig_Click(object sender, RoutedEventArgs e)
        {
            if (HLVRSettingsManager.IsInitialized)
            {
                HLVRSettingsManager.RestoreLauncherSettings();
                RefreshConfigTabs();
            }
        }

        private void RestoreDefaultModConfig_Click(object sender, RoutedEventArgs e)
        {
            if (HLVRSettingsManager.IsInitialized)
            {
                HLVRSettingsManager.RestoreModSettings();
                RefreshConfigTabs();
            }
        }

        public void ConsoleLog(string msg, Brush color)
        {
            ConsoleOutput.Inlines.Add(new Run(msg) { Foreground = color });
            var lines = ConsoleOutput.Inlines.Skip(ConsoleOutput.Inlines.Count() - 100);
            ConsoleOutput.Inlines.Clear();
            ConsoleOutput.Inlines.AddRange(lines);
            try
            {
                File.AppendAllText(HLVRPaths.VRLogFile, msg);
            }
            catch (IOException) { } // TODO: Somehow notify user of exception
        }
    }
}
