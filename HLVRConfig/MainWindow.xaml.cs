using DbMon.NET;
using HLVRConfig.Utilities;
using HLVRConfig.Utilities.UI.Controls;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Interop;
using System.Windows.Media;
using HLVRConfig.Utilities.UI;
using HLVRConfig.Utilities.Settings;
using HLVRConfig.Utilities.UI.Config;
using HLVRConfig.Utilities.Process;

namespace HLVRConfig
{
    public partial class MainWindow : Window
    {
        private System.Windows.Forms.NotifyIcon notifyIcon;

        private readonly SingleProcessEnforcer singleProcessEnforcer = new SingleProcessEnforcer();
        private readonly ModLauncher hlvrModLauncher = new ModLauncher();

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
                UpdateSettingsAndLanguage();

                InitializeComponent();

                UpdateState();
                RefreshConfigTabs();

                LoadReadme();
                InitializeNotifyIcon();

                HandleInitialSettings();

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
                var errorMsg = new I18N.I18NString("ErrorMsgCouldNotLoadReadme", "Couldn't load README.txt: %s");
                AboutText.Inlines.Add(new Regex(Regex.Escape("%s")).Replace(I18N.Get(errorMsg), e.Message, 1));
            }
        }

        private void HandleInitialSettings()
        {
            if (HLVRSettingsManager.LauncherSettings.GeneralSettings[LauncherSettings.CategoryLauncher][LauncherSettings.StartMinimized].IsTrue())
            {
                WindowState = WindowState.Minimized;
                OnWindowStateChanged();
            }
            if (HLVRSettingsManager.LauncherSettings.GeneralSettings[LauncherSettings.CategoryLauncher][LauncherSettings.AutoRunMod].IsTrue()
                && HLVRPaths.CheckHLDirectory()
                && HLVRPaths.CheckModDirectory())
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

        private void UpdateSettingsAndLanguage()
        {
            I18N.Init();
            HLVRSettingsManager.InitSettings();
            I18N.Init();
        }

        public void UpdateState()
        {
            UpdateSettingsAndLanguage();
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
            }
        }

        protected override void OnClosing(CancelEventArgs e)
        {
            I18N.SaveI18NFile();

            if (IsValidProcess 
                && hlvrModLauncher.IsGameRunning()
                && HLVRSettingsManager.LauncherSettings.GeneralSettings[LauncherSettings.CategoryLauncher][LauncherSettings.AutoCloseGame].IsTrue())
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
                && HLVRSettingsManager.LauncherSettings.GeneralSettings[LauncherSettings.CategoryLauncher][LauncherSettings.MinimizeToTray].IsTrue())
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

        public void RefreshConfigTabs(bool mod = true, bool launcher = true)
        {
            UpdateSettingsAndLanguage();

            if (mod)
            {
                if (HLVRSettingsManager.AreModSettingsInitialized)
                {
                    ModConfig.Initialize(InputConfig, HLVRSettingsManager.ModSettings.InputSettings);
                    ModConfig.Initialize(ImmersionConfig, HLVRSettingsManager.ModSettings.ImmersionSettings);
                    ModConfig.Initialize(PerformanceConfig, HLVRSettingsManager.ModSettings.PerformanceSettings);
                    ModConfig.Initialize(VisualsConfig, HLVRSettingsManager.ModSettings.VisualsSettings);
                    ModConfig.Initialize(AudioConfig, HLVRSettingsManager.ModSettings.AudioSettings);
                    ModConfig.Initialize(OtherConfig, HLVRSettingsManager.ModSettings.OtherSettings);
                }
                else
                {
                    InputConfig.Children.Clear();
                    ImmersionConfig.Children.Clear();
                    PerformanceConfig.Children.Clear();
                    VisualsConfig.Children.Clear();
                    AudioConfig.Children.Clear();
                    OtherConfig.Children.Clear();
                    var errorMsg = I18N.Get(new I18N.I18NString("ErrorMsgCouldNotSynchronizeModSettings", "Couldn't synchronize mod settings. Config tabs are not available."));
                    AddNopeText(InputConfig, errorMsg);
                    AddNopeText(ImmersionConfig, errorMsg);
                    AddNopeText(PerformanceConfig, errorMsg);
                    AddNopeText(VisualsConfig, errorMsg);
                    AddNopeText(AudioConfig, errorMsg);
                    AddNopeText(OtherConfig, errorMsg);
                }
            }

            if (launcher)
            {
                Utilities.UI.Config.LauncherConfig.Initialize(LauncherConfig);
                if (!HLVRSettingsManager.AreLauncherSettingsInitialized)
                {
                    var errorMsg = I18N.Get(new I18N.I18NString("ErrorMsgCouldNotSynchronizeLauncherSettings", "Couldn't synchronize launcher settings. You can modify these settings, but they might be lost after closing HLVRConfig."));
                    AddNopeText(LauncherConfig, errorMsg);
                }
            }

            SkipTooManyLines();
        }

        private void AddNopeText(StackPanel panel, string text)
        {
            panel.Children.Insert(0, new TextBlock(new Run(text))
            {
                TextWrapping = TextWrapping.WrapWithOverflow,
                Padding = new Thickness(5),
                Margin = new Thickness(5),
                Focusable = true
            });
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

        private void SkipTooManyLines()
        {
            int maxloglines = HLVRSettingsManager.LauncherSettings.MaxLogLines;
            if (maxloglines > 0 && ConsoleOutput.Inlines.Count > maxloglines)
            {
                var lines = new List<Inline>(ConsoleOutput.Inlines.Skip(ConsoleOutput.Inlines.Count - maxloglines));
                ConsoleOutput.Inlines.Clear();
                ConsoleOutput.Inlines.AddRange(lines);
            }
        }

        string lastmsg = null;
        string lastfullmsg = null;
        Run lastrun = null;
        int lastmsgcount = 1;

        public void ConsoleLog(string msg, Brush color)
        {
            string timestamp = DateTime.Now.ToString("HH:mm:ss.ffff");
            if (lastmsg != null && lastrun != null && msg.Equals(lastmsg))
            {
                lastmsgcount++;
                if (msg.Trim().Length > 0)
                {
                    if (lastmsgcount > 2)
                    {
                        lastrun.Text = lastfullmsg.Trim() + " [Repeats " + lastmsgcount + " times, last at " + timestamp + "]\n";
                    }
                }
                return;
            }

            lastmsgcount = 1;
            lastmsg = msg;
            if (msg.Trim().Length > 0)
            {
                msg = "[" + timestamp + "] " + msg;
            }
            lastfullmsg = msg;

            Run run = new Run(msg) { Foreground = color };
            lastrun = run;

            SkipTooManyLines();
            ConsoleOutput.Inlines.Add(run);
            try
            {
                File.AppendAllText(HLVRPaths.VRLogFile, msg);
            }
            catch (IOException) { } // TODO: Somehow notify user of exception
        }

        private IEnumerable<II18NControl> Find18NControls(DependencyObject control)
        {
            foreach (var child in LogicalTreeHelper.GetChildren(control))
            {
                if (child is Button button)
                    if (button.Content is II18NControl)
                        yield return button.Content as II18NControl;

                if (child is TextBlock textBlock)
                    foreach (var inline in textBlock.Inlines)
                        if (inline is II18NControl)
                            yield return inline as II18NControl;

                if (child is II18NControl)
                    yield return child as II18NControl;

                if (child is DependencyObject dependencyObject)
                    foreach (II18NControl childOfChild in Find18NControls(dependencyObject))
                        yield return childOfChild;
            }
        }

        public void UpdateGUITexts()
        {
            foreach (var i18nControl in Find18NControls(this).ToList())
            {
                i18nControl.Update();
            }
        }
    }
}
