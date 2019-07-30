using Microsoft.Collections.Extensions;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;

namespace HLVRLauncher.Utilities
{
    public class HLVRLauncherConfig
    {
        private static readonly string MetaHookReleasesURL = "https://github.com/LAGonauta/metahook/releases";

        public static readonly string CategoryLauncher = "Launcher";
        public static readonly string CategoryMetaHook = "MetaHook (Credits: LAGonauta, nagist, hzqst)";

        public static readonly string MinimizeToTray = "MinimizeToTray";
        public static readonly string StartMinimized = "StartMinimized";
        public static readonly string AutoPatchAndRunMod = "AutoPatchAndRunMod";
        public static readonly string AutoUnpatchAndCloseLauncher = "AutoUnpatchAndCloseLauncher";
        public static readonly string AutoUnpatchAndCloseGame = "AutoUnpatchAndCloseGame";

        public static readonly string UseMetaHook = "UseMetaHook";
        public static readonly string MetaHookDoppler = "MetaHookDoppler";

        public static void Initialize(StackPanel panel)
        {
            foreach (var category in HLVRSettingsManager.Settings.LauncherSettings)
            {
                AddCategory(panel, category.Key, category.Value);
            }
        }

        private static void AddCategory(StackPanel panel, string category, OrderedDictionary<string, Setting> settings)
        {
            StackPanel categoryPanel = new StackPanel()
            {
                Orientation = Orientation.Vertical,
                Margin = new Thickness(20),
            };

            AddTitle(categoryPanel, category);

            if (category.Equals(CategoryMetaHook) && !(File.Exists(HLVRPaths.MetaHookExe) && File.Exists(HLVRPaths.MetaHookAudioDLL) && File.Exists(HLVRPaths.MetaHookPluginsLST)))
            {
                AddCheckBox(categoryPanel, category, UseMetaHook, HLVRSettingsManager.Settings.LauncherSettings[CategoryMetaHook][UseMetaHook].Description, false, true);
                AddMetaHookLink(categoryPanel);
            }
            else
            {
                foreach (var setting in settings)
                {
                    switch (setting.Value.Type)
                    {
                        case SettingValueType.BOOLEAN:
                            AddCheckBox(categoryPanel, category, setting.Key, setting.Value.Description, setting.Value.IsTrue());
                            break;
                        default:
                            AddInput(categoryPanel, category, setting.Key, setting.Value.Description, setting.Value);
                            break;
                    }
                }
            }

            panel.Children.Add(categoryPanel);
        }

        private static void AddTitle(StackPanel panel, string title)
        {
            TextBlock textBlock = new TextBlock()
            {
                TextWrapping = TextWrapping.WrapWithOverflow,
                Padding = new Thickness(5),
                Margin = new Thickness(5),
                Focusable = true
            };
            textBlock.Inlines.Add(new Bold(new Run(title)));
            panel.Children.Add(textBlock);
        }

        private static void AddCheckBox(StackPanel panel, string category, string name, string label, bool isChecked, bool isDisabled = false)
        {
            CheckBox cb = new CheckBox
            {
                Name = name,
                Content = label,
                Margin = new Thickness(10),
                IsChecked = !isDisabled && isChecked,
                IsEnabled = !isDisabled
            };
            if (!isDisabled)
            {
                cb.Checked += (object sender, RoutedEventArgs e) =>
                {
                    HLVRSettingsManager.SetLauncherSetting(category, name, true);
                };
                cb.Unchecked += (object sender, RoutedEventArgs e) =>
                {
                    HLVRSettingsManager.SetLauncherSetting(category, name, false);
                };
                cb.Indeterminate += (object sender, RoutedEventArgs e) =>
                {
                    HLVRSettingsManager.SetLauncherSetting(category, name, false);
                };
            }
            panel.Children.Add(cb);
        }

        private static void AddInput(StackPanel panel, string category, string name, string label, Setting value)
        {
            StackPanel inputPanel = new StackPanel()
            {
                Orientation = Orientation.Horizontal,
                Margin = new Thickness(5),
            };

            inputPanel.Children.Add(new TextBlock()
            {
                TextWrapping = TextWrapping.NoWrap,
                Padding = new Thickness(5),
                Margin = new Thickness(5),
                Focusable = true,
                MinWidth = 150,
                Text = label
            });

            if (value.AllowedValues.Count == 0)
            {
                var textbox = new TextBox()
                {
                    TextWrapping = TextWrapping.NoWrap,
                    Padding = new Thickness(5),
                    Margin = new Thickness(5),
                    Focusable = true,
                    MaxLines = 1,
                    MinLines = 1,
                    MinWidth = (value.Type == SettingValueType.STRING) ? 300 : 100,
                    Text = value.Value
                };
                textbox.TextChanged += (object sender, TextChangedEventArgs e) =>
                {
                    HLVRSettingsManager.SetLauncherSetting(category, name, textbox.Text);
                };
                inputPanel.Children.Add(textbox);
            }
            else
            {
                var combobox = new ComboBox()
                {
                    MinWidth = 200,
                };
                int index = 0;
                int selectedIndex = 0;
                foreach (var allowedValue in value.AllowedValues)
                {
                    var comboboxitem = new ComboBoxItem() { };
                    comboboxitem.Content = allowedValue.Value;
                    combobox.Items.Add(comboboxitem);
                    if (allowedValue.Key.Equals(value.Value))
                    {
                        selectedIndex = index;
                    }
                    index++;
                }
                combobox.SelectedIndex = selectedIndex;
                combobox.SelectionChanged += (object sender, SelectionChangedEventArgs e) =>
                {
                    HLVRSettingsManager.SetLauncherSetting(category, name, (combobox.SelectedValue as ComboBoxItem).Content as string);
                };
                inputPanel.Children.Add(combobox);
            }

            panel.Children.Add(inputPanel);
        }

        private static void AddMetaHookLink(StackPanel panel)
        {
            StackPanel inputPanel = new StackPanel()
            {
                Orientation = Orientation.Horizontal,
                Margin = new Thickness(5),
            };

            inputPanel.Children.Add(new TextBlock()
            {
                TextWrapping = TextWrapping.NoWrap,
                Padding = new Thickness(5),
                Margin = new Thickness(5),
                Focusable = true,
                MinWidth = 150,
                Text = "MetaHook is not installed. Get it from here: "
            });

            Button downloadbutton = new Button()
            {
                Padding = new Thickness(5),
                Margin = new Thickness(5),
                Content = "Download MetaHook"
            };
            downloadbutton.Click += DownloadButton_Click;
            inputPanel.Children.Add(downloadbutton);

            panel.Children.Add(inputPanel);
        }

        private static void DownloadButton_Click(object sender, RoutedEventArgs e)
        {
            Process.Start(new ProcessStartInfo(MetaHookReleasesURL));
            e.Handled = true;
        }
    }
}
