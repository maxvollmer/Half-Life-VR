using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;

namespace HLVRLauncher.Utilities
{
    public class HLVRLauncherConfig
    {
        public static readonly string MinimizeToTray = "MinimizeToTray";
        public static readonly string StartMinimized = "StartMinimized";
        public static readonly string AutoPatchAndRunMod = "AutoPatchAndRunMod";
        public static readonly string AutoUnpatchAndCloseLauncher = "AutoUnpatchAndCloseLauncher";
        public static readonly string AutoUnpatchAndCloseGame = "AutoUnpatchAndCloseGame";

        public static void Initialize(StackPanel panel)
        {
            foreach (var setting in HLVRSettingsManager.Settings.LauncherSettings)
            {
                AddCheckBox(panel, setting.Key, setting.Value.Description, setting.Value.Value.Equals("1"));
            }
        }

        private static void AddCheckBox(StackPanel panel, string name, string label, bool isChecked)
        {
            CheckBox cb = new CheckBox
            {
                Name = name,
                Content = label,
                Margin = new Thickness(10),
                IsChecked = isChecked
            };
            cb.Checked += (object sender, RoutedEventArgs e) =>
            {
                HLVRSettingsManager.SetLauncherSetting(name, true);
            };
            cb.Unchecked += (object sender, RoutedEventArgs e) =>
            {
                HLVRSettingsManager.SetLauncherSetting(name, false);
            };
            cb.Indeterminate += (object sender, RoutedEventArgs e) =>
            {
                HLVRSettingsManager.SetLauncherSetting(name, false);
            };
            panel.Children.Add(cb);
        }
    }
}
