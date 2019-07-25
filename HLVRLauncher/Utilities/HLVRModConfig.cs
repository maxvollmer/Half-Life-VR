using System;
using System.Windows;
using System.Windows.Controls;

namespace HLVRLauncher.Utilities
{
    public class HLVRModConfig
    {
        public static void Initialize(StackPanel panel)
        {
            CheckBox cb = new CheckBox();
            cb.Name = "TestName";
            cb.Content = "TestContent";
            cb.Margin = new Thickness(10);
            panel.Children.Add(cb);
        }
    }
}