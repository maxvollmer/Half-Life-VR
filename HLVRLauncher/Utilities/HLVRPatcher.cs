using DbMon.NET;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;

namespace HLVRLauncher.Utilities
{
    public class HLVRPatcher
    {
        private readonly object patchLock = new object();
        private readonly object gameLock = new object();

        public bool IsPatching { get; private set; } = false;

        internal bool IsUnpatching { get; private set; } = false;

        private FileStream opengl32LockStream = null;
        private Process hlProcess = null;

        public void Initialize()
        {
            try
            {
                if (File.Exists(HLVRPaths.HLOpengl32dll) && FilesAreEqual(HLVRPaths.VROpengl32dll, HLVRPaths.HLOpengl32dll)
                    && File.Exists(HLVRPaths.HLOpenvr_apidll) && FilesAreEqual(HLVRPaths.VROpenvr_apidll, HLVRPaths.HLOpenvr_apidll)
                    && File.Exists(HLVRPaths.HLEasyHook32dll) && FilesAreEqual(HLVRPaths.VREasyHook32dll, HLVRPaths.HLEasyHook32dll))
                {
                    opengl32LockStream = new FileStream(HLVRPaths.HLOpengl32dll, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
                }
            }
            catch (Exception)
            {
                opengl32LockStream = null;
                DeleteDLL(HLVRPaths.HLOpengl32dll, false);
                DeleteDLL(HLVRPaths.HLOpenvr_apidll, true);
                DeleteDLL(HLVRPaths.HLEasyHook32dll, true);
            }
        }

        public void PatchGame()
        {
            lock (patchLock)
            {
                if (IsPatching || IsUnpatching || IsGamePatched())
                    return;

                IsPatching = true;
                System.Windows.Application.Current.Dispatcher.BeginInvoke((Action)(() => ((MainWindow)System.Windows.Application.Current.MainWindow)?.UpdateState()));

                var thread = new Thread(() =>
                {
                    try
                    {
                        CopyDLL(HLVRPaths.VROpengl32dll, HLVRPaths.HLOpengl32dll, false);
                        CopyDLL(HLVRPaths.VROpenvr_apidll, HLVRPaths.HLOpenvr_apidll, true);
                        CopyDLL(HLVRPaths.VREasyHook32dll, HLVRPaths.HLEasyHook32dll, true);
                        opengl32LockStream = new FileStream(HLVRPaths.HLOpengl32dll, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
                    }
                    catch (Exception e)
                    {
                        MessageBox.Show($"Unable to patch Half-Life:\n {e}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                        if (opengl32LockStream != null)
                        {
                            opengl32LockStream.Close();
                        }
                        opengl32LockStream = null;
                        DeleteDLL(HLVRPaths.HLOpengl32dll, false);
                        DeleteDLL(HLVRPaths.HLOpenvr_apidll, true);
                        DeleteDLL(HLVRPaths.HLEasyHook32dll, true);
                    }
                    finally
                    {
                        IsPatching = false;
                        System.Windows.Application.Current.Dispatcher.BeginInvoke((Action)(() => ((MainWindow)System.Windows.Application.Current.MainWindow)?.UpdateState()));
                    }
                })
                {
                    IsBackground = true
                };
                thread.Start();
            }
        }

        public void UnpatchGame()
        {
            lock (patchLock)
            {
                if (IsPatching || IsUnpatching || !IsGamePatched())
                    return;

                IsUnpatching = true;
                System.Windows.Application.Current.Dispatcher.BeginInvoke((Action)(() => ((MainWindow)System.Windows.Application.Current.MainWindow)?.UpdateState()));

                var thread = new Thread(() =>
                {
                    try
                    {
                        if (opengl32LockStream != null)
                        {
                            opengl32LockStream.Close();
                        }
                        opengl32LockStream = null;
                        DeleteDLL(HLVRPaths.HLOpengl32dll, false);
                        DeleteDLL(HLVRPaths.HLOpenvr_apidll, true);
                        DeleteDLL(HLVRPaths.HLEasyHook32dll, true);
                    }
                    catch (Exception e)
                    {
                        MessageBox.Show($"Unable to unpatch Half-Life:\n {e}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                    }
                    finally
                    {
                        IsUnpatching = false;
                        System.Windows.Application.Current.Dispatcher.BeginInvoke((Action)(() => ((MainWindow)System.Windows.Application.Current.MainWindow)?.UpdateState()));
                    }
                })
                {
                    IsBackground = true
                };
                thread.Start();
            }
        }

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
                                else if(text.ToLower().Contains("warning"))
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
                System.Windows.Application.Current.Dispatcher.BeginInvoke((Action)(() => ((MainWindow)System.Windows.Application.Current.MainWindow)?.UpdateState()));
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

        public bool IsGamePatched()
        {
            lock (patchLock)
            {
                return File.Exists(HLVRPaths.HLOpengl32dll)
                && File.Exists(HLVRPaths.HLOpenvr_apidll)
                && File.Exists(HLVRPaths.HLEasyHook32dll)
                && opengl32LockStream != null;
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

        private void CopyDLL(string vrDLLPath, string hlDLLPath, bool createBackup)
        {
            try
            {
                if (File.Exists(hlDLLPath))
                {
                    if (createBackup && !File.Exists(hlDLLPath + ".bak"))
                    {
                        File.Move(hlDLLPath, hlDLLPath + ".bak");
                    }
                    else
                    {
                        File.Delete(hlDLLPath);
                    }
                    }
                }
            catch (Exception) { }    // ignore exceptions here, UpdateState will make sure GUI represents patch state of game
            try
            {
                File.Copy(vrDLLPath, hlDLLPath);
            }
            catch (Exception) { }    // ignore exceptions here, UpdateState will make sure GUI represents patch state of game
        }

        private void DeleteDLL(string hlDLLPath, bool restoreBackup)
        {
            try
            {
                if (File.Exists(hlDLLPath))
                {
                    File.Delete(hlDLLPath);
                }
            }
            catch (Exception) {}    // ignore exceptions here, UpdateState will make sure GUI represents patch state of game
            try
            {
                if (restoreBackup && File.Exists(hlDLLPath + ".bak"))
                {
                    File.Move(hlDLLPath + ".bak", hlDLLPath);
                }
            }
            catch (Exception) { }    // ignore exceptions here, UpdateState will make sure GUI represents patch state of game
        }

        private bool FilesAreEqual(string file1, string file2)
        {
            try
            {
                return new FileInfo(file1).Length == new FileInfo(file2).Length
                    && File.ReadAllBytes(file1).SequenceEqual(File.ReadAllBytes(file2));
            }
            catch (Exception)
            {
                return false;
            }
        }

    }

}
