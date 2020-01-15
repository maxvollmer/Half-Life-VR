using DbMon.NET;
using System;
using System.Diagnostics;
using System.IO;
using System.Windows.Media;

namespace HLVRLauncher.Utilities
{
    public class HLVRModLauncher
    {
        private readonly object patchLock = new object();
        private readonly object gameLock = new object();

        private Process hlProcess = null;

        public void LaunchMod(bool terminateIfAlreadyRunning)
        {
            lock (gameLock)
            {
                if (IsGameRunning())
                {
                    if (terminateIfAlreadyRunning)
                    {
                        TerminateGame();
                    }
                    else
                    {
                        return;
                    }
                }

                hlProcess = Process.Start(new ProcessStartInfo
                {
                    WorkingDirectory = HLVRPaths.HLDirectory,
                    FileName = HLVRPaths.HLExecutable,
                    Arguments = "-game vr -console -dev 2 -insecure -nomouse -nowinmouse -nojoy -noip -nofbo -window -width 1600 -height 1200 +sv_lan 1 +cl_mousegrab 0 +gl_vsync 0 +fps_max 90 +fps_override 1",
                    UseShellExecute = false,
                    RedirectStandardError = true,
                    RedirectStandardOutput = true
                });

                HookIntoHLProcess();

                System.Windows.Application.Current.Dispatcher.BeginInvoke((Action)(() => ((MainWindow)System.Windows.Application.Current.MainWindow)?.UpdateState()));
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
                                System.Windows.Application.Current.Dispatcher.BeginInvoke((Action)(() => ((MainWindow)System.Windows.Application.Current.MainWindow)?.ConsoleLog(text, color)));
                            }
                        };
                    }
                }
                catch (Exception) { }
            }
        }

        private void GameTerminated(object sender, EventArgs e)
        {
            lock (gameLock)
            {
                hlProcess = null;
                if (HLVRSettingsManager.Settings.LauncherSettings[HLVRLauncherConfig.CategoryLauncher][HLVRLauncherConfig.AutoCloseLauncher].IsTrue())
                {
                    System.Windows.Application.Current.Dispatcher.BeginInvoke((Action)(() => System.Windows.Application.Current.Shutdown()));
                }
                else
                {
                    System.Windows.Application.Current.Dispatcher.BeginInvoke((Action)(() => ((MainWindow)System.Windows.Application.Current.MainWindow)?.UpdateState()));
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

                System.Windows.Application.Current.Dispatcher.BeginInvoke((Action)(() => ((MainWindow)System.Windows.Application.Current.MainWindow)?.UpdateState()));
            }
        }

        public bool IsGameRunning()
        {
            lock (gameLock)
            {
                if (hlProcess == null)
                {
                    var potentialHLProcesses = Process.GetProcessesByName("hl");
                    foreach (Process potentialHLProcess in potentialHLProcesses)
                    {
                        var path1 = Path.GetFullPath(HLVRPaths.HLExecutable);
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
                    return false;

                if (hlProcess.HasExited)
                    return false;

                try { Process.GetProcessById(hlProcess.Id); }
                catch (InvalidOperationException) { return false; }
                catch (ArgumentException) { return false; }
                return true;
            }
        }

    }

}
