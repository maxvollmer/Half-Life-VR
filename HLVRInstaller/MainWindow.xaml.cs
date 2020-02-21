using System;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;

namespace HLVRInstaller
{
    public partial class MainWindow : Window
    {
        private string InstallError { get; set; }
        private volatile bool doCancel = false;

        public MainWindow()
        {
            InitializeComponent();
            HalflifeDirectoryFinder.InitializeFromSteam();
            HalfLifeDirectoryInput.Text = HalflifeDirectoryFinder.HLDirectory;
            HalfLifeDirectoryInput.TextChanged += (object sender, TextChangedEventArgs e) => 
            { 
                HalflifeDirectoryFinder.HLDirectory = HalfLifeDirectoryInput.Text;
                UninstallButton.Visibility = HalflifeDirectoryFinder.CheckModDirectory() ? Visibility.Visible : Visibility.Collapsed;
            };
            using (var reader = new StreamReader(Assembly.GetExecutingAssembly().GetManifestResourceStream("HLVRInstaller.README.txt"), Encoding.UTF8))
            {
                ReadmeContent.Inlines.Add(new Run(reader.ReadToEnd()));
            }
            UninstallButton.Visibility = HalflifeDirectoryFinder.CheckModDirectory() ? Visibility.Visible : Visibility.Collapsed;
        }

        private void BrowseHalfLifeDirectory(object sender, RoutedEventArgs args)
        {
            using (var folderBrowserDialog = new System.Windows.Forms.FolderBrowserDialog() { SelectedPath = HalflifeDirectoryFinder.HLDirectory })
            {
                if (System.Windows.Forms.DialogResult.OK == folderBrowserDialog.ShowDialog())
                {
                    HalfLifeDirectoryInput.Text = folderBrowserDialog.SelectedPath;
                    HalflifeDirectoryFinder.HLDirectory = folderBrowserDialog.SelectedPath;
                }
            }
        }

        private void Quit(object sender, RoutedEventArgs args)
        {
            Application.Current.Shutdown();
        }

        private void CancelInstallation(object sender, RoutedEventArgs args)
        {
            var result = MessageBox.Show(this, "Are you sure you want to cancel installation? Files might be in a weird state if you do. (This installer is way too dumb to be able to undo a half-completed installation.)", "Warning", MessageBoxButton.YesNo, MessageBoxImage.Exclamation);
            if (result == MessageBoxResult.Yes)
            {
                doCancel = true;
            }
        }

        private void UninstallMod(object sender, RoutedEventArgs args)
        {
            if (!ReadmeCheckbox.IsChecked ?? false)
            {
                MessageBox.Show(this, "Please confirm that you read and understood the text.", "Confirmation required", MessageBoxButton.OK, MessageBoxImage.Exclamation);
                return;
            }

            if (!HalflifeDirectoryFinder.CheckModDirectory())
            {
                UninstallButton.Visibility = Visibility.Collapsed;
                return;
            }

            var result = MessageBox.Show(this, "Are you sure you want to uninstall Half-Life: VR? All files, including savegames and your settings, will be removed.", "Warning", MessageBoxButton.YesNo, MessageBoxImage.Exclamation);
            if (result == MessageBoxResult.Yes)
            {
                try
                {
                    Directory.Delete(HalflifeDirectoryFinder.VRDirectory, true);

                    MessageBox.Show(this, "Successfully uninstalled Half-Life: VR.", "Info", MessageBoxButton.OK, MessageBoxImage.Information);
                }
                catch (Exception e)
                {
                    MessageBox.Show(this, "Error while trying to uninstall Half-Life: VR: " + e.Message, "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                }
            }

            UninstallButton.Visibility = HalflifeDirectoryFinder.CheckModDirectory() ? Visibility.Visible : Visibility.Collapsed;
        }

        private async void InstallMod(object sender, RoutedEventArgs args)
        {
            if (!ReadmeCheckbox.IsChecked ?? false)
            {
                MessageBox.Show(this, "Please confirm that you read and understood the text.", "Confirmation required", MessageBoxButton.OK, MessageBoxImage.Exclamation);
                return;
            }

            if (!HalflifeDirectoryFinder.CheckHLDirectory())
            {
                var result = MessageBox.Show(this, "\"" + HalflifeDirectoryFinder.HLDirectory + "\" doesn't seem to be a valid Half-Life installation. Install anyways?", "Warning", MessageBoxButton.OKCancel, MessageBoxImage.Exclamation);
                if (result == MessageBoxResult.Cancel)
                    return;
            }

            if (HalflifeDirectoryFinder.CheckModDirectory())
            {
                var result = MessageBox.Show(this, "Half-Life: VR is already installed. Install anyways? (All files will be overwritten.)", "Warning", MessageBoxButton.YesNo, MessageBoxImage.Exclamation);
                if (result != MessageBoxResult.Yes)
                    return;
            }

            try
            {
                PreInstall.Visibility = Visibility.Collapsed;
                DuringInstall.Visibility = Visibility.Visible;

                InstallError = "";
                doCancel = false;

                await Task.Run(() =>
                {
                    Install();
                });

                if (doCancel)
                {
                    doCancel = false;
                    return;
                }

                if (!string.IsNullOrEmpty(InstallError))
                {
                    MessageBox.Show(this, InstallError, "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                    return;
                }

                if (HalflifeDirectoryFinder.CheckModDirectory())
                {
                    var result = MessageBox.Show(this, "Installation succeeded! Installer will quit. Do you want to launch Half-Life: VR now?", "Yay", MessageBoxButton.YesNo, MessageBoxImage.Information);
                    if (result == MessageBoxResult.Yes)
                    {
                        Process.Start(new ProcessStartInfo
                        {
                            WorkingDirectory = HalflifeDirectoryFinder.VRDirectory,
                            FileName = HalflifeDirectoryFinder.VRConfigExecutable,
                            UseShellExecute = false
                        });
                    }
                    Application.Current.Shutdown();
                }
                else
                {
                    MessageBox.Show(this, "Installation failed. Not really sure why :/", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                }
            }
            finally
            {
                PreInstall.Visibility = Visibility.Visible;
                DuringInstall.Visibility = Visibility.Collapsed;
                UninstallButton.Visibility = HalflifeDirectoryFinder.CheckModDirectory() ? Visibility.Visible : Visibility.Collapsed;
            }
        }

        public void Install()
        {
            try
            {
                using (ZipArchive archive = new ZipArchive(Assembly.GetExecutingAssembly().GetManifestResourceStream("HLVRInstaller.latest.zip"), ZipArchiveMode.Read))
                {
                    ExtractToDirectory(archive, HalflifeDirectoryFinder.HLDirectory);
                }
            }
            catch (Exception e)
            {
                InstallError = "Error while trying to install Half-Life: VR:\n\n" + e.Message;
            }
        }

        private void Invoke(Action action)
        {
            Application.Current.Dispatcher.BeginInvoke(action);
        }

        private void ExtractToDirectory(ZipArchive archive, string halflifeDirectory)
        {
            long total = archive.Entries.Count;
            long count = 0;

            foreach (ZipArchiveEntry file in archive.Entries)
            {
                if (doCancel)
                    return;

                string fullFilePath = Path.GetFullPath(Path.Combine(halflifeDirectory, file.FullName));
                if (!fullFilePath.StartsWith(halflifeDirectory, StringComparison.OrdinalIgnoreCase))
                {
                    throw new IOException("Invalid path: " + halflifeDirectory);
                }

                if (string.IsNullOrEmpty(file.Name))
                {
                    Directory.CreateDirectory(Path.GetDirectoryName(fullFilePath));
                }
                else
                {
                    file.ExtractToFile(fullFilePath, true);
                }

                count++;
                Invoke(() => { InstallProgressBar.Value = count / (double)total; });
            }

            Invoke(() => { InstallProgressBar.Value = 1; });
        }
    }
}
