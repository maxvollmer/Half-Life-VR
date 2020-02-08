using HLVRConfig.Utilities.Settings;
using Microsoft.Collections.Extensions;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using static HLVRConfig.Utilities.Settings.Setting;

namespace HLVRConfig.Utilities
{
    public class HLVRLauncherConfig
    {
        public static void Initialize(StackPanel panel)
        {
            panel.Children.Clear();
            foreach (var category in HLVRSettingsManager.LauncherSettings.LauncherSettings)
            {
                AddCategory(panel, category.Key, category.Value);
            }
        }

        private static void AddCategory(StackPanel panel, I18N.I18NString category, OrderedDictionary<string, Setting> settings)
        {
            StackPanel categoryPanel = new StackPanel()
            {
                Orientation = Orientation.Vertical,
                Margin = new Thickness(20),
            };

            AddTitle(categoryPanel, category);

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

            panel.Children.Add(categoryPanel);
        }

        private static void AddTitle(StackPanel panel, I18N.I18NString title)
        {
            TextBlock textBlock = new TextBlock()
            {
                TextWrapping = TextWrapping.WrapWithOverflow,
                Padding = new Thickness(5),
                Margin = new Thickness(5),
                Focusable = true
            };
            textBlock.Inlines.Add(new Bold(new Run(I18N.Get(title))));
            panel.Children.Add(textBlock);
        }

        private static void AddCheckBox(StackPanel panel, I18N.I18NString category, string name, I18N.I18NString label, bool isChecked, bool isDisabled = false)
        {
            CheckBox cb = new CheckBox
            {
                Name = name,
                Content = new Run(I18N.Get(label)),
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

        private static void AddInput(StackPanel panel, I18N.I18NString category, string name, I18N.I18NString label, Setting value)
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
                Text = I18N.Get(label)
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
                    if (value.IsFolder)
                    {
                        System.Windows.Application.Current.Dispatcher.BeginInvoke((Action)(() => ((MainWindow)System.Windows.Application.Current?.MainWindow)?.UpdateState()));
                        System.Windows.Application.Current.Dispatcher.BeginInvoke((Action)(() => ((MainWindow)System.Windows.Application.Current?.MainWindow)?.RefreshConfigTabs(true, false)));
                    }
                };
                inputPanel.Children.Add(textbox);

                if (value.IsFolder)
                {
                    var folderSelectButton = new System.Windows.Controls.Button()
                    {
                        MinWidth = 100,
                        Content = new Run(I18N.Get(new I18N.I18NString("BrowseForFolder", "Browse For Folder")))
                    };

                    folderSelectButton.Click += (object sender, RoutedEventArgs e) =>
                    {
                        using (var folderBrowserDialog = new System.Windows.Forms.FolderBrowserDialog())
                        {
                            if (System.Windows.Forms.DialogResult.OK == folderBrowserDialog.ShowDialog())
                            {
                                textbox.Text = folderBrowserDialog.SelectedPath;
                            }
                        }
                    };

                    inputPanel.Children.Add(folderSelectButton);
                }
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
                    HLVRSettingsManager.SetLauncherSetting(category, name, ((I18N.I18NString)(combobox.SelectedValue as ComboBoxItem).Content).Key);
                };
                inputPanel.Children.Add(combobox);
            }

            panel.Children.Add(inputPanel);
        }
    }
}
