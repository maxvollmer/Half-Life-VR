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
                hlvrPatcher.Initialize();
                IsValidProcess = true;
                InitializeComponent();

                CheckBox cb = new CheckBox();
                cb.Name = "TestName";
                cb.Content = "TestContent";
                cb.Margin = new Thickness(10);
                Configuration.Children.Add(cb);

                UpdateState();
            }
            catch (CancelAndTerminateAppException)
            {
                System.Windows.Application.Current.Shutdown();
            }
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
                if (hlvrPatcher.IsGameRunning())
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
                DebugMonitor.Stop();
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
            hlvrPatcher.LaunchMod();
            UpdateState();
        }

        private void ClearConsole_Click(object sender, RoutedEventArgs e)
        {
            ConsoleOutput.Inlines.Clear();
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
