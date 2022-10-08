using DbMon.NET;
using HLVRConfig.Utilities.Settings;
using Microsoft.Win32.SafeHandles;
using System;
using System.Diagnostics;
using System.IO;
using System.Threading;
using System.Windows.Media;

namespace HLVRConfig.Utilities.Process
{
    public class ModLauncher
    {
        private readonly object patchLock = new object();
        private readonly object gameLock = new object();

        private System.Diagnostics.Process hlProcess = null;

        public void LaunchMod()
        {
            lock (gameLock)
            {
                if (!IsGameRunning())
                {
                    string windowsize = HLVRSettingsManager.LauncherSettings.GeneralSettings[LauncherSettings.CategoryModSpecifics][LauncherSettings.ModWindowSize].Value;

                    // Steam runs game default with the following arguments:
                    // -console -dev 2 -insecure -nomouse -nowinmouse -nojoy -noip -nofbo -window -width 1600 -height 1200 +sv_lan 1 +cl_mousegrab 0 +gl_vsync 0 +fps_max 90 +fps_override 1

                    // windowsize contains custom size set by player or default ("-width 1600 -height 1200")

                    // steam://rungameid/1908720
                    // Steam.exe -applaunch 1908720 -width 1600 -height 1200

                    hlProcess = System.Diagnostics.Process.Start(new ProcessStartInfo
                    {
                        WorkingDirectory = HLVRPaths.SteamExeDirectory,
                        FileName = HLVRPaths.SteamExecutable,
                        Arguments = "-applaunch 1908720 " + windowsize,
                        UseShellExecute = false,
                        RedirectStandardError = false,
                        RedirectStandardOutput = false
                    });

                    // ugly wait loop that checks if hl.exe has been launched by Steam
                    // and then hookes into it
                    // tries for maximum 2 seconds, then gives up
                    int tries = 0;
                    while (!IsGameRunning() && tries < 20)
                    {
                        Thread.Sleep(100);
                        tries++;
                    }
                }

                System.Windows.Application.Current?.Dispatcher?.BeginInvoke((Action)(() =>
                {
                    (System.Windows.Application.Current?.MainWindow as MainWindow)?.UpdateState();
                }));
            }
        }

        private void HookIntoHLProcess()
        {
            lock (gameLock)
            {
                hlProcess.EnableRaisingEvents = true;
                hlProcess.Exited += new EventHandler(GameTerminated);

                try
                {
                    if (!DebugMonitor.IsStarted)
                    {
                        DebugMonitor.Start();
                        DebugMonitor.OnOutputDebugString += (int pid, string text) =>
                        {
                            if (hlProcess != null && pid == hlProcess.Id)
                            {
                                Brush color;
                                if (text.ToLower().Contains("error"))
                                    color = Brushes.Red;
                                else if (text.ToLower().Contains("warning"))
                                    color = Brushes.OrangeRed;
                                else
                                    color = Brushes.Black;
                                System.Windows.Application.Current?.Dispatcher?.BeginInvoke((Action)(() => (System.Windows.Application.Current?.MainWindow as MainWindow)?.ConsoleLog(text, color)));
                            }
                        };
                    }
                }
                catch (Exception e)
                {
                    string error = "ERROR Couldn't connect to Half-Life VR console: " + e.Message + "\n\n" + e.StackTrace;
                    System.Windows.Application.Current?.Dispatcher?.BeginInvoke((Action)(() => (System.Windows.Application.Current?.MainWindow as MainWindow)?.ConsoleLog(error, Brushes.Red)));
                }
            }
        }

        private void GameTerminated(object sender, EventArgs e)
        {
            lock (gameLock)
            {
                hlProcess = null;
                if (HLVRSettingsManager.LauncherSettings.GeneralSettings[LauncherSettings.CategoryLauncher][LauncherSettings.AutoCloseLauncher].IsTrue())
                {
                    System.Windows.Application.Current?.Dispatcher?.BeginInvoke((Action)(() => System.Windows.Application.Current?.Shutdown()));
                }
                else
                {
                    System.Windows.Application.Current?.Dispatcher?.BeginInvoke((Action)(() => (System.Windows.Application.Current?.MainWindow as MainWindow)?.UpdateState()));
                }
            }
        }

        public void TerminateGame()
        {
            lock (gameLock)
            {
                if (hlProcess != null)
                {
                    try { hlProcess.Kill(); hlProcess.WaitForExit(); } catch (Exception) { }
                    hlProcess = null;
                }

                System.Windows.Application.Current?.Dispatcher?.BeginInvoke((Action)(() => (System.Windows.Application.Current?.MainWindow as MainWindow)?.UpdateState()));
            }
        }

        public bool IsGameRunning()
        {
            lock (gameLock)
            {
                if (hlProcess != null && hlProcess.HasExited)
                {
                    hlProcess = null;
                }

                if (hlProcess == null)
                {
                    var potentialHLProcesses = System.Diagnostics.Process.GetProcessesByName("hl");
                    foreach (var potentialHLProcess in potentialHLProcesses)
                    {
                        var path1 = Path.GetFullPath(HLVRPaths.HLVRExecutable);
                        var path2 = Path.GetFullPath(potentialHLProcess.MainModule.FileName);
                        if (string.Equals(path1, path2, StringComparison.OrdinalIgnoreCase))
                        {
                            hlProcess = potentialHLProcess;
                            HookIntoHLProcess();
                            break;
                        }
                    }
                }

                if (hlProcess == null)
                {
                    return false;
                }

                if (hlProcess.HasExited)
                {
                    hlProcess = null;
                    return false;
                }

                try { System.Diagnostics.Process.GetProcessById(hlProcess.Id); }
                catch (InvalidOperationException) { return false; }
                catch (ArgumentException) { return false; }
                return true;
            }
        }

        public SafeProcessHandle GetProcessHandle()
        {
            return hlProcess?.SafeHandle;
        }

        public int? GetProcessID()
        {
            return hlProcess?.Id;
        }
    }

}
