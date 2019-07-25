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

        class Setting
        {
            public string Label { get; }
            public bool Value { get; set; }

            public Setting(string label, bool defaultValue)
            {
                this.Label = label;
                this.Value = defaultValue;
            }
        }

        private static readonly Dictionary<string, Setting> settings = new Dictionary<string, Setting>()
        {
            { MinimizeToTray, new Setting( "Minimize to tray", true ) },
            { StartMinimized, new Setting( "Start HLVRLauncher minimized", true ) },
            { AutoPatchAndRunMod, new Setting( "Patch game and run mod automatically when starting HLVRLauncher", true ) },
            { AutoUnpatchAndCloseLauncher, new Setting( "Unpatch game and exit HLVRLauncher automatically when Half-Life: VR exits", true ) },
        };

        public static bool GetSetting(string name) 
        {
            return settings[name].Value;
        }

        private static void SetSetting(string name, bool value)
        {
            settings[name].Value = value;
        }

        public static void Initialize(StackPanel panel)
        {
            foreach (var setting in settings)
            {
                AddCheckBox(panel, setting.Key, setting.Value.Label);
            }
        }

        private static void AddCheckBox(StackPanel panel, string name, string label)
        {
            CheckBox cb = new CheckBox();
            cb.Name = name;
            cb.Content = label;
            cb.Margin = new Thickness(10);
            cb.IsChecked = GetSetting(name);
            cb.Checked += (object sender, RoutedEventArgs e) =>
            {
                SetSetting(name, true);
            };
            cb.Unchecked += (object sender, RoutedEventArgs e) =>
            {
                SetSetting(name, false);
            };
            cb.Indeterminate += (object sender, RoutedEventArgs e) =>
            {
                SetSetting(name, false);
            };
            panel.Children.Add(cb);
        }
    }
}
