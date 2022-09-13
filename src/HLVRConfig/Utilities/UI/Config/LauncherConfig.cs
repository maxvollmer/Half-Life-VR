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

namespace HLVRConfig.Utilities.UI.Config
{
    public class LauncherConfig : IConfig
    {
        protected override I18N.I18NString DefaultLabel { get; } = new I18N.I18NString("Default", "Default");

        public static void Initialize(StackPanel panel)
        {
            new LauncherConfig().InternalInitialize(panel);
        }

        private void InternalInitialize(StackPanel panel)
        {
            panel.Children.Clear();
            foreach (var category in HLVRSettingsManager.LauncherSettings.GeneralSettings)
            {
                AddCategory(HLVRSettingsManager.LauncherSettings.GeneralSettings, panel, category.Key, category.Value);
            }
        }

    }
}
