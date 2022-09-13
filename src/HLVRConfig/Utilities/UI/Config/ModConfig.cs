using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using HLVRConfig.Utilities.Settings;
using Microsoft.Collections.Extensions;
using static HLVRConfig.Utilities.Settings.Setting;

namespace HLVRConfig.Utilities.UI.Config
{
    public class ModConfig : IConfig
    {
        protected override I18N.I18NString DefaultLabel { get; } = new I18N.I18NString("ModDefault", "Mod Default");

        public static void Initialize(StackPanel panel, OrderedDictionary<SettingCategory, OrderedDictionary<string, Setting>> settingcategories)
        {
            new ModConfig().InternalInitialize(panel, settingcategories);
        }

        private void InternalInitialize(StackPanel panel, OrderedDictionary<SettingCategory, OrderedDictionary<string, Setting>> settingcategories)
        {
            panel.Children.Clear();
            foreach (var category in settingcategories)
            {
                AddCategory(settingcategories, panel, category.Key, category.Value);
            }
        }

    }
}
