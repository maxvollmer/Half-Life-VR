using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using HLVRConfig.Utilities.Settings;
using Microsoft.Collections.Extensions;
using static HLVRConfig.Utilities.Settings.Setting;

namespace HLVRConfig.Utilities.UI.Config
{
    public abstract class IConfig
    {
        private static readonly float CM_TO_UNIT = 0.375f;
        private static readonly float UNIT_TO_CM = 1.0f / CM_TO_UNIT;

        protected abstract I18N.I18NString DefaultLabel { get; }

        protected void AddCategory(OrderedDictionary<SettingCategory, OrderedDictionary<string, Setting>> settingcategories, StackPanel panel, SettingCategory category, OrderedDictionary<string, Setting> settings)
        {
            if (category.Dependency != null && !category.Dependency.IsSatisfied())
                return;

            StackPanel categoryPanel = new StackPanel()
            {
                Orientation = Orientation.Vertical,
                Margin = new Thickness(20),
            };

            AddTitle(categoryPanel, category.Title);
            AddDescription(categoryPanel, category.Description);

            int numOfSettings = 0;
            foreach (var setting in settings)
            {
                if (setting.Value.Dependency != null && !setting.Value.Dependency.IsSatisfied())
                    continue;

                switch (setting.Value.Type)
                {
                    case SettingType.BOOLEAN:
                        AddCheckBox(settingcategories, categoryPanel, category, setting.Key, setting.Value.Description, setting.Value.IsTrue(), !setting.Value.DefaultValue.Equals("0"));
                        break;
                    default:
                        AddInput(settingcategories, categoryPanel, category, setting.Key, setting.Value.Description, setting.Value);
                        break;
                }

                numOfSettings++;
            }

            if (numOfSettings > 0)
                panel.Children.Add(categoryPanel);
        }

        private void AddInput(OrderedDictionary<SettingCategory, OrderedDictionary<string, Setting>> settingcategories, StackPanel panel, SettingCategory category, string name, I18N.I18NString label, Setting value)
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
                var textbox = MakeTextBox(value);
                textbox.TextChanged += (object sender, TextChangedEventArgs e) =>
                {
                    HLVRSettingsManager.TrySetSetting(settingcategories, category, name, textbox.Text);
                };
                inputPanel.Children.Add(textbox);

                if (value.Type == SettingType.SPEED || value.Type == SettingType.DISTANCE)
                {
                    var meterTextbox = MakeTextBox(value);
                    meterTextbox.Text = UnitToMeter(textbox.Text);
                    bool preventinfiniteloop = false;
                    textbox.TextChanged += (object sender, TextChangedEventArgs e) =>
                    {
                        if (preventinfiniteloop) return;
                        preventinfiniteloop = true;
                        meterTextbox.Text = UnitToMeter(textbox.Text);
                        preventinfiniteloop = false;
                    };
                    meterTextbox.TextChanged += (object sender, TextChangedEventArgs e) =>
                    {
                        if (preventinfiniteloop) return;
                        preventinfiniteloop = true;
                        textbox.Text = MeterToUnit(meterTextbox.Text);
                        preventinfiniteloop = false;
                    };
                    inputPanel.Children.Add(CreateMiniText(value.Type == SettingType.SPEED ? "units/s" : "units"));
                    inputPanel.Children.Add(meterTextbox);
                    inputPanel.Children.Add(CreateMiniText(value.Type == SettingType.SPEED ? "cm/s" : "cm"));

                    inputPanel.Children.Add(CreateDefaultLabel(value.DefaultValue + "/" + UnitToMeter(value.DefaultValue), value.IsFolder));
                }
                else
                {
                    inputPanel.Children.Add(CreateDefaultLabel(value.DefaultValue, value.IsFolder));
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
                I18N.I18NString defaultValue = null;
                foreach (var allowedValue in value.AllowedValues)
                {
                    var comboboxitem = new ComboBoxItem() { };
                    comboboxitem.Content = allowedValue.Value;
                    combobox.Items.Add(comboboxitem);
                    if (allowedValue.Key.Equals(value.Value))
                    {
                        selectedIndex = index;
                    }
                    if (allowedValue.Key.Equals(value.DefaultValue))
                    {
                        defaultValue = allowedValue.Value;
                    }
                    index++;
                }
                combobox.SelectedIndex = selectedIndex;
                combobox.SelectionChanged += (object sender, SelectionChangedEventArgs e) =>
                {
                    HLVRSettingsManager.TrySetSetting(settingcategories, category, name, ((I18N.I18NString)(combobox.SelectedValue as ComboBoxItem).Content).Key);
                };
                inputPanel.Children.Add(combobox);

                if (defaultValue != null)
                    inputPanel.Children.Add(CreateDefaultLabel(I18N.Get(defaultValue), value.IsFolder));
            }

            panel.Children.Add(inputPanel);
        }

        private TextBlock CreateDefaultLabel(string defaultValue, bool isFolder)
        {
            if (!isFolder && defaultValue.Contains("("))
            {
                defaultValue = defaultValue.Substring(0, defaultValue.IndexOf("(")).Trim();
            }

            return CreateMiniText("(" + I18N.Get(DefaultLabel) + ": " + defaultValue + ")", 20);
        }

        private TextBlock CreateMiniText(string text, double leftmargin = 0)
        {
            return new TextBlock(new Run(text))
            {
                TextWrapping = TextWrapping.NoWrap,
                Padding = new Thickness(0, 10, 10, 0),
                Margin = new Thickness(leftmargin, 0, 0, 0),
                Focusable = false
            };
        }

        protected TextBlock MakeTitle(I18N.I18NString title)
        {
            TextBlock textBlock = new TextBlock()
            {
                TextWrapping = TextWrapping.WrapWithOverflow,
                Padding = new Thickness(5),
                Margin = new Thickness(5),
                Focusable = true
            };
            textBlock.Inlines.Add(new Bold(new Run(I18N.Get(title))));
            return textBlock;
        }

        protected void AddTitle(StackPanel panel, I18N.I18NString title)
        {
            panel.Children.Add(MakeTitle(title));
        }

        private void AddDescription(StackPanel panel, I18N.I18NString description)
        {
            if (description == null)
                return;

            TextBlock textBlock = new TextBlock()
            {
                TextWrapping = TextWrapping.WrapWithOverflow,
                Padding = new Thickness(5, 0, 5, 5),
                Margin = new Thickness(5, 0, 5, 5),
                Focusable = true
            };
            textBlock.FontSize *= 0.8;
            textBlock.Inlines.Add(new Run(I18N.Get(description)));
            panel.Children.Add(textBlock);
        }

        private void AddCheckBox(OrderedDictionary<SettingCategory, OrderedDictionary<string, Setting>> settingcategories, StackPanel panel, SettingCategory category, string name, I18N.I18NString label, bool isChecked, bool isDefaultChecked)
        {
            StackPanel inputPanel = new StackPanel()
            {
                Orientation = Orientation.Horizontal,
            };

            CheckBox cb = new CheckBox
            {
                Name = name,
                Content = new Run(I18N.Get(label)),
                Margin = new Thickness(10),
                IsChecked = isChecked
            };
            cb.Checked += (object sender, RoutedEventArgs e) =>
            {
                HLVRSettingsManager.TrySetSetting(settingcategories, category, name, true);
            };
            cb.Unchecked += (object sender, RoutedEventArgs e) =>
            {
                HLVRSettingsManager.TrySetSetting(settingcategories, category, name, false);
            };
            cb.Indeterminate += (object sender, RoutedEventArgs e) =>
            {
                HLVRSettingsManager.TrySetSetting(settingcategories, category, name, false);
            };
            inputPanel.Children.Add(cb);

            panel.Children.Add(inputPanel);
        }

        protected TextBox MakeTextBox(Setting value)
        {
            return MakeTextBox(value.Type, value.Value);
        }

        protected TextBox MakeTextBox(SettingType type, string text)
        {
            var textbox = new TextBox()
            {
                TextWrapping = TextWrapping.NoWrap,
                Padding = new Thickness(5),
                Margin = new Thickness(5),
                Focusable = true,
                MaxLines = 1,
                MinLines = 1,
                MinWidth = GetTextBoxWidth(type),
                Text = text
            };
            return textbox;
        }

        private double GetTextBoxWidth(SettingType type)
        {
            switch (type)
            {
                case SettingType.STRING:
                    return 300;
                case SettingType.SPEED:
                    return 75;
                case SettingType.DISTANCE:
                    return 75;
                case SettingType.FACTOR:
                    return 75;
                case SettingType.COUNT:
                    return 75;
                default:
                    return 100;
            }
        }

        private string UnitToMeter(string text)
        {
            try
            {
                float worldScale = HLVRSettingsManager.ModSettings.GetWorldScale();
                return I18N.IntToString((int)(I18N.ParseFloat(text) * UNIT_TO_CM * worldScale));
            }
            catch (Exception)
            {
                return "";
            }
        }

        private string MeterToUnit(string text)
        {
            try
            {
                float worldScale = HLVRSettingsManager.ModSettings.GetWorldScale();
                float invWorldScale = worldScale != 0 ? 1 / worldScale : 0;
                return I18N.IntToString((int)(I18N.ParseFloat(text) * CM_TO_UNIT * invWorldScale));
            }
            catch (Exception)
            {
                return "";
            }
        }
    }
}
