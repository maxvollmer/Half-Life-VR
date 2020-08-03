using HLVRConfig.Utilities.Settings;
using Microsoft.Win32.SafeHandles;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace HLVRConfig.Utilities.Process
{
    public class MiniDumpCreator
    {
        private static readonly uint GENERIC_READ = 0x80000000;
        private static readonly uint GENERIC_WRITE = 0x40000000;

        private static readonly uint CREATE_NEW = 1;
        private static readonly uint CREATE_ALWAYS = 2;
        private static readonly uint OPEN_EXISTING = 3;
        private static readonly uint OPEN_ALWAYS = 4;
        private static readonly uint TRUNCATE_EXISTING = 5;

        private static readonly uint FILE_ATTRIBUTE_NORMAL = 0x80;

        private static readonly uint MiniDumpNormal = 0x00000000;
        private static readonly uint MiniDumpWithDataSegs = 0x00000001;
        private static readonly uint MiniDumpWithFullMemory = 0x00000002;
        private static readonly uint MiniDumpWithHandleData = 0x00000004;
        private static readonly uint MiniDumpFilterMemory = 0x00000008;
        private static readonly uint MiniDumpScanMemory = 0x00000010;
        private static readonly uint MiniDumpWithUnloadedModules = 0x00000020;
        private static readonly uint MiniDumpWithIndirectlyReferencedMemory = 0x00000040;
        private static readonly uint MiniDumpFilterModulePaths = 0x00000080;
        private static readonly uint MiniDumpWithProcessThreadData = 0x00000100;
        private static readonly uint MiniDumpWithPrivateReadWriteMemory = 0x00000200;
        private static readonly uint MiniDumpWithoutOptionalData = 0x00000400;
        private static readonly uint MiniDumpWithFullMemoryInfo = 0x00000800;
        private static readonly uint MiniDumpWithThreadInfo = 0x00001000;
        private static readonly uint MiniDumpWithCodeSegs = 0x00002000;
        private static readonly uint MiniDumpWithoutAuxiliaryState = 0x00004000;
        private static readonly uint MiniDumpWithFullAuxiliaryState = 0x00008000;
        private static readonly uint MiniDumpWithPrivateWriteCopyMemory = 0x00010000;
        private static readonly uint MiniDumpIgnoreInaccessibleMemory = 0x00020000;
        private static readonly uint MiniDumpWithTokenInformation = 0x00040000;
        private static readonly uint MiniDumpWithModuleHeaders = 0x00080000;
        private static readonly uint MiniDumpFilterTriage = 0x00100000;
        private static readonly uint MiniDumpWithAvxXStateContext = 0x00200000;
        private static readonly uint MiniDumpWithIptTrace = 0x00400000;
        private static readonly uint MiniDumpScanInaccessiblePartialPages = 0x00800000;
        private static readonly uint MiniDumpValidTypeFlags = 0x00ffffff;

        [DllImport("dbghelp.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.StdCall, SetLastError = true)]
        public static extern bool MiniDumpWriteDump(
            SafeProcessHandle hProcess,
            int ProcessId,
            SafeFileHandle hFile,
            int DumpType,
            IntPtr ExceptionParam,
            IntPtr UserStreamParam,
            IntPtr CallStackParam);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.StdCall, SetLastError = true)]
        public static extern SafeFileHandle CreateFile(
            string lpFileName,
            uint dwDesiredAccess,
            uint dwShareMode,
            IntPtr SecurityAttributes,
            uint dwCreationDisposition,
            uint dwFlagsAndAttributes,
            IntPtr hTemplateFile
        );

        private static void ShowError(string error)
        {
            MessageBox.Show(
                error,
                "Can't create mini dump!",
                MessageBoxButton.OK,
                MessageBoxImage.Error);
        }

        public static void CreateMiniDump(ModLauncher hlvrModLauncher)
        {
            if (!HLVRPaths.CheckHLDirectory() || !HLVRPaths.CheckModDirectory())
            {
                ShowError("Can't create mini dump, path to Half-Life and mod aren't set. (How did you press this button, it should be invisible?!)");
                return;
            }

            if (!hlvrModLauncher.IsGameRunning())
            {
                ShowError("Can't create mini dump, Half-Life isn't running. (How did you press this button, it should be invisible?!)");
                return;
            }

            var processHandle = hlvrModLauncher.GetProcessHandle();
            if (processHandle == null || processHandle.IsInvalid)
            {
                ShowError("Can't create mini dump, process handle is invalid.");
                return;
            }

            int? processID = hlvrModLauncher.GetProcessID();
            if (processID == null)
            {
                ShowError("Can't create mini dump, process ID is invalid.");
                return;
            }

            try
            {
                System.IO.Directory.CreateDirectory(HLVRPaths.VRMiniDumpDirectory);
            }
            catch (Exception e)
            {
                ShowError("Can't create mini dump, failed to create directory. Error message: " + e.Message);
                return;
            }

            bool fullMemoryDump = HLVRSettingsManager.LauncherSettings.GeneralSettings[LauncherSettings.CategoryDebug][LauncherSettings.EnableFullMemoryMiniDump].IsTrue();

            string path = Path.Combine(HLVRPaths.VRMiniDumpDirectory, (fullMemoryDump ? "hlvr_fullmem_" : "hlvr_") + DateTime.Now.ToString("yyyy-MM-dd_HH-mm-ss_fff") + ".dmp");

            var fileHandle = CreateFile(path, GENERIC_WRITE, 0, IntPtr.Zero, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, IntPtr.Zero);
            if (fileHandle == null || fileHandle.IsInvalid)
            {
                ShowError("Can't create mini dump, unable to create file: " + path + ". Make sure the folder exists and HLVRConfig has write access. Error code: " + Marshal.GetLastWin32Error());
                return;
            }

            var dumped = MiniDumpWriteDump(
                processHandle,
                processID.Value,
                fileHandle,
                (int)(fullMemoryDump ?
                (MiniDumpWithDataSegs
                | MiniDumpWithFullMemory
                | MiniDumpWithHandleData
                | MiniDumpScanMemory
                | MiniDumpWithIndirectlyReferencedMemory
                | MiniDumpIgnoreInaccessibleMemory)
                : MiniDumpNormal),
                IntPtr.Zero,
                IntPtr.Zero,
                IntPtr.Zero);

            if (!dumped)
            {
                if (fullMemoryDump)
                    ShowError("Can't create mini dump. Try disabling full memory dump. Error code: " + Marshal.GetLastWin32Error());
                else
                    ShowError("Can't create mini dump. Error code: " + Marshal.GetLastWin32Error());
            }

            fileHandle.Close();

            MessageBox.Show(
                "Mini dump successfully created at " + path + ".",
                "Mini dump successfully created!",
                MessageBoxButton.OK,
                MessageBoxImage.Information);
        }
    }
}
