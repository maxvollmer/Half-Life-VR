using DbMon.NET;
using HLVRLauncher.Utilities;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace HLVRLauncher
{
    public partial class MainWindow : Window
    {
        private System.Windows.Forms.NotifyIcon notifyIcon;

        private readonly SingleProcessEnforcer singleProcessEnforcer = new SingleProcessEnforcer();
        private readonly HLVRPatcher hlvrPatcher = new HLVRPatcher();

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
                hlvrPatcher.Initialize();
                IsValidProcess = true;
                InitializeComponent();
                HLVRLauncherConfig.Initialize(LauncherConfig);
                HLVRModConfig.Initialize(ModConfig);
                InitializeNotifyIcon();
                UpdateState();
                HandleInitialSettings();
            }
            catch (CancelAndTerminateAppException)
            {
                System.Windows.Application.Current.Shutdown();
            }
        }

        private void HandleInitialSettings()
        {
            if (HLVRSettingsManager.Settings.LauncherSettings[HLVRLauncherConfig.StartMinimized].IsTrue())
            {
                WindowState = WindowState.Minimized;
                OnWindowStateChanged();
            }
            if (HLVRSettingsManager.Settings.LauncherSettings[HLVRLauncherConfig.AutoPatchAndRunMod].IsTrue())
            {
                hlvrPatcher.PatchGame();
                hlvrPatcher.LaunchMod(false);
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
            PatchedNotRunningGamePanel.Visibility = (!hlvrPatcher.IsPatching && !hlvrPatcher.IsUnpatching && hlvrPatcher.IsGamePatched() && !hlvrPatcher.IsGameRunning()) ? Visibility.Visible : Visibility.Collapsed;
            PatchedRunningGamePannel.Visibility = (!hlvrPatcher.IsPatching && !hlvrPatcher.IsUnpatching && hlvrPatcher.IsGamePatched() && hlvrPatcher.IsGameRunning()) ? Visibility.Visible : Visibility.Collapsed;
            UnpatchedNotRunningGamePanel.Visibility = (!hlvrPatcher.IsPatching && !hlvrPatcher.IsUnpatching && !hlvrPatcher.IsGamePatched() && !hlvrPatcher.IsGameRunning()) ? Visibility.Visible : Visibility.Collapsed;
            UnpatchedRunningGamePannel.Visibility = (!hlvrPatcher.IsPatching && !hlvrPatcher.IsUnpatching && !hlvrPatcher.IsGamePatched() && hlvrPatcher.IsGameRunning()) ? Visibility.Visible : Visibility.Collapsed;
            PatchingGamePanel.Visibility = (hlvrPatcher.IsPatching) ? Visibility.Visible : Visibility.Collapsed;
            UnpatchingGamePanel.Visibility = (hlvrPatcher.IsUnpatching) ? Visibility.Visible : Visibility.Collapsed;
        }

        protected override void OnClosing(CancelEventArgs e)
        {
            Boolean cancel = false;

            if (IsValidProcess && hlvrPatcher.IsGamePatched())
            {
                MessageBoxResult result;
                if (HLVRSettingsManager.Settings.LauncherSettings[HLVRLauncherConfig.AutoUnpatchAndCloseGame].IsTrue())
                {
                    result = MessageBoxResult.Yes;
                }
                else if (hlvrPatcher.IsGameRunning())
                {
                    result = MessageBox.Show("Half-Life: VR is still running and Half-Life is still patched. Do you want to quit Half-Life and unpatch the game before exiting? If you chose no, Half-Life will remain patched after Half-Life: VR quit.", "Half-Life: VR is running!", MessageBoxButton.YesNoCancel, MessageBoxImage.Warning);
                }
                else
                {
                    result = MessageBox.Show("Half-Life is still patched. Do you want to unpatch the game before exiting? If you chose no, Half-Life will remain patched.", "Half-Life is still patched!", MessageBoxButton.YesNoCancel, MessageBoxImage.Warning);
                }

                switch (result)
                {
                    case MessageBoxResult.Yes:
                    case MessageBoxResult.OK:
                        hlvrPatcher.TerminateGame();
                        hlvrPatcher.UnpatchGame();
                        break;
                    case MessageBoxResult.No:
                        break;
                    case MessageBoxResult.Cancel:
                    case MessageBoxResult.None:
                    default:
                        cancel = true;
                        break;
                }
            }

            e.Cancel = cancel;

            if (!cancel)
            {
                singleProcessEnforcer.Dispose();
                try { if (DebugMonitor.IsStarted) DebugMonitor.Stop(); } catch(Exception) { }
            }
        }

        private static IntPtr WndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled)
        {
            System.Windows.Application.Current.Dispatcher.BeginInvoke((Action)(() => ((MainWindow)System.Windows.Application.Current.MainWindow)?.OnWindowStateChanged()));
            return IntPtr.Zero;
        }

        private void OnWindowStateChanged()
        {
            if (WindowState == WindowState.Minimized
                && HLVRSettingsManager.Settings.LauncherSettings[HLVRLauncherConfig.MinimizeToTray].IsTrue())
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

        private void PatchGame_Click(object sender, RoutedEventArgs e)
        {
            hlvrPatcher.PatchGame();
            UpdateState();
        }

        private void UnpatchGame_Click(object sender, RoutedEventArgs e)
        {
            hlvrPatcher.UnpatchGame();
            UpdateState();
        }

        private void LaunchMod_Click(object sender, RoutedEventArgs e)
        {
            hlvrPatcher.LaunchMod(true);
            UpdateState();
        }

        private void ClearConsole_Click(object sender, RoutedEventArgs e)
        {
            ConsoleOutput.Inlines.Clear();
        }

        public void RefreshConfigTabs()
        {
            LauncherConfig.Children.Clear();
            HLVRLauncherConfig.Initialize(LauncherConfig);
            ModConfig.Children.Clear();
            HLVRModConfig.Initialize(ModConfig);
        }

        private void RestoreDefaultLauncherConfig_Click(object sender, RoutedEventArgs e)
        {
            HLVRSettingsManager.RestoreLauncherSettings();
            RefreshConfigTabs();
        }

        private void RestoreDefaultModConfig_Click(object sender, RoutedEventArgs e)
        {
            HLVRSettingsManager.RestoreModSettings();
            RefreshConfigTabs();
        }

        public void ConsoleLog(string msg, Brush color)
        {
            ConsoleOutput.Inlines.Add(new Run(msg) { Foreground = color });
        }

        public void ConsoleLogLn(string msg, Brush color)
        {
            ConsoleLog(msg, color);
            ConsoleOutput.Inlines.Add(new LineBreak());
        }
    }
}
