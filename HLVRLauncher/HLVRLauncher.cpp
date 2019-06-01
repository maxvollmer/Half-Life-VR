
#include "stdafx.h"
#include "HLVRLauncher.h"
#include <windows.h>
#include <stdio.h>
#include <accctrl.h>
#include <Aclapi.h>
#include <string>
#include <iostream>
#include "ConsoleColor.h"

#pragma warning( disable : 4996 ) 

bool g_hadError = false;

void RunCommandAndWait(std::string description, std::wstring command, const wchar_t* directory = nullptr)
{
	std::cout << description << std::flush;

	SetLastError(0);

	try
	{
		STARTUPINFO si{ 0 };
		si.cb = sizeof(si);

		PROCESS_INFORMATION pi{ 0 };

		if (CreateProcessW(NULL, const_cast<LPWSTR>(command.data()), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, directory, &si, &pi))
		{
			WaitForSingleObject(pi.hProcess, INFINITE);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			std::cout << " done." << std::endl;
		}
		else
		{
			throw 0;
		}
	}
	catch (...)
	{
		std::cerr << std::endl << red << "Error: Command failed with error code " << GetLastError() << "." << white << std::endl;
		g_hadError = true;
	}
}

// FileExists from https://stackoverflow.com/a/6218957/9199167
BOOL FileExistsW(LPCWSTR szPath)
{
	DWORD dwAttrib = GetFileAttributesW(szPath);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES) && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}

void DeleteDLL(const std::wstring& hlDirectory, const std::wstring& dll, bool createBackup)
{
	std::cout << "Deleting " << std::string{ dll.begin(), dll.end() } << ".dll." << std::endl;

	SetLastError(0);

	try
	{
		std::wstring pathDLL = (hlDirectory + L"\\" + dll + L".dll");
		if (createBackup)
		{
			std::wstring pathBAK = (hlDirectory + L"\\" + dll + L".dll.bak");
			if (FileExistsW(pathBAK.data()) && !DeleteFileW(pathBAK.data()))
			{
				throw 0;
			}
			if (FileExistsW(pathDLL.data()) && !MoveFileW(pathDLL.data(), pathBAK.data()))
			{
				throw 0;
			}
		}
		else
		{
			if (FileExistsW(pathDLL.data()) && !DeleteFileW(pathDLL.data()))
			{
				throw 0;
			}
		}
	}
	catch (...)
	{
		std::cerr << yellow << "Warning: Failed to delete " << std::string{ dll.begin(), dll.end() } << ".dll. Error: " << GetLastError() << "." << white << std::endl;
		g_hadError = true;
	}
}

void CopyDLL(const std::wstring& hlDirectory, const std::wstring& vrDirectory, const std::wstring& dll)
{
	std::cout << "Copying " << std::string{ dll.begin(), dll.end() } << ".dll." << std::endl;

	SetLastError(0);

	try
	{
		std::wstring from = (vrDirectory + L"\\" + dll + L".dll");
		std::wstring to = (hlDirectory + L"\\" + dll + L".dll");
		if (!CopyFileW(from.data(), to.data(), FALSE))
		{
			throw 0;
		}
	}
	catch (...)
	{
		std::cerr << yellow << "Warning: Couldn't copy " << std::string{ dll.begin(), dll.end() } << ".dll. Error: " << GetLastError() << ". If the game doesn't run, you need to copy manually." << white << std::endl;
		g_hadError = true;
	}
}

void RestoreDLL(const std::wstring& hlDirectory, const std::wstring& dll)
{
	std::cout << "Restoring " << std::string{ dll.begin(), dll.end() } << ".dll." << std::endl;

	SetLastError(0);

	try
	{
		std::wstring pathBAK = (hlDirectory + L"\\" + dll + L".dll.bak");
		std::wstring pathDLL = (hlDirectory + L"\\" + dll + L".dll");
		if (FileExistsW(pathBAK.data()) && !MoveFileW(pathBAK.data(), pathDLL.data()))
		{
			throw 0;
		}
	}
	catch (...)
	{
		std::cerr << yellow << "Warning: Failed to restore " << std::string{ dll.begin(), dll.end() } << ".dll. Error: " << GetLastError() << "." << white << std::endl;
		g_hadError = true;
	}
}

void ForceSingleProcess()
{
	SetLastError(0);
	HANDLE mutex = CreateMutexW(NULL, FALSE, L"HalfLifeVirtualRealityLauncherMutexLaliludgnskdagjfgbs");
	if (mutex == NULL)
	{
		std::cerr << red << "Error: Not enough rights to run HLVRLauncher. Try running as administrator. Shutting down." << white << std::endl;
		system("pause");
		std::exit(-1);
	}
	else if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		std::cerr << red << "Error: Another instance of HLVRLauncher is already running. Shutting down." << white << std::endl;
		system("pause");
		std::exit(-1);
	}
}

void CheckHalfLifeDirectory(const std::wstring& directory)
{
	std::wstring hlexe = (directory + L"\\hl.exe");
	DWORD dwAttrib = GetFileAttributesW(hlexe.data());
	if ((dwAttrib == INVALID_FILE_ATTRIBUTES || (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)))
	{
		std::cerr << red << "Error: Couldn't find hl.exe. Make sure you run HLVRLauncher from the HLVR mod directory in your Half-Life folder." << white << std::endl;
		system("pause");
		std::exit(-1);
	}
}

int main(int argc, char *argv[])
{
	ForceSingleProcess();

	std::cout << "HLVRLauncher initializing directories and commands..." << std::endl;

	TCHAR szFileName[MAX_PATH + 1];
	GetModuleFileNameW(NULL, szFileName, MAX_PATH + 1);

	std::wstring filename = szFileName;
	std::wstring vrDirectory = filename.substr(0, filename.rfind('\\'));
	std::wstring hlDirectory = vrDirectory.substr(0, vrDirectory.rfind('\\'));

	CheckHalfLifeDirectory(hlDirectory);

	std::wstring icaclsSetInheritanceOnFolderCommandLine = L"icacls \"" + hlDirectory + L"\" /inheritance:d";
	std::wstring icaclsSetInheritanceOnFileCommandLine = L"icacls \"" + hlDirectory + L"\\opengl32.dll\" /inheritance:d";
	std::wstring icaclsDisableDeletionOnFolderCommandLine = L"icacls \"" + hlDirectory + L"\" /deny Everyone:(DE,DC)";
	std::wstring icaclsDisableDeletionOnFileCommandLine = L"icacls \"" + hlDirectory + L"\\opengl32.dll\" /deny Everyone:(DE,DC)";
	std::wstring icaclsEnableReadingOnFileCommandLine = L"icacls \"" + hlDirectory + L"\\opengl32.dll\" /grant Everyone:(R)";
	std::wstring icaclsReenableDeletionOnFolderCommandLine = L"icacls \"" + hlDirectory + L"\" /remove:d Everyone";
	std::wstring icaclsReenableDeletionOnFileCommandLine = L"icacls \"" + hlDirectory + L"\\opengl32.dll\" /remove:d Everyone";

	std::wstring hlExeCommandLine = L"\"" + hlDirectory + L"\\hl.exe\" -game vr -console -insecure -nomouse -nowinmouse -nojoy -noip -nofbo -window -width 1600 -height 1200 +sv_lan 1 +cl_mousegrab 0";

	std::cout << "Finished initializing!" << std::endl;

	std::cout << std::endl;

	std::cout
		<< "HLVRLauncher is now going to run a set of commands to patch OpenGL32.dll."
		<< std::endl
		<< "It will then launch the game."
		<< std::endl
		<< "Once the game ends, HLVRLauncher will clean up afterwards, restoring the original OpenGL32.dll."
		<< std::endl
		<< std::endl
		<< yellow << "SOME OF THESE COMMANDS MAY TAKE A WHILE TO COMPLETE. PLEASE BE PATIENT!" << white
		<< std::endl
		<< std::endl
		<< red << "NO WARRANTIES WHATSOEVER. USE AT YOUR OWN RISK. THIS MIGHT GET YOU VAC BANNED." << white
		<< std::endl;

	std::cout << std::endl;

	RunCommandAndWait("Disabling Access Control List inheritance on the Half-Life folder...", icaclsSetInheritanceOnFolderCommandLine);
	RunCommandAndWait("Enabling deletion of files in the Half-Life folder...", icaclsReenableDeletionOnFolderCommandLine);
	RunCommandAndWait("Enabling deletion of OpenGL32.dll in the Half-Life folder...", icaclsReenableDeletionOnFileCommandLine);

	std::cout << std::endl;

	DeleteDLL(hlDirectory, L"OpenGL32", false);
	CopyDLL(hlDirectory, vrDirectory, L"OpenGL32");

	std::cout << std::endl;

	std::cout << red << "Attention: Your OpenGL32.dll is now patched. From this point on you are at risk of getting VAC banned if you play Half-Life and any of its mods in multiplayer." << white << std::endl;

	std::cout << std::endl;

	RunCommandAndWait("Disabling Access Control List inheritance on OpenGL32.dll in the Half-Life folder...", icaclsSetInheritanceOnFileCommandLine);
	RunCommandAndWait("Disabling deletion of OpenGL32.dll in the Half-Life folder...", icaclsDisableDeletionOnFileCommandLine);
	RunCommandAndWait("Enable read-only access to OpenGL32.dll in the Half-Life folder...", icaclsEnableReadingOnFileCommandLine);

	std::cout << std::endl;

	DeleteDLL(hlDirectory, L"openvr_api", true);
	CopyDLL(hlDirectory, vrDirectory, L"openvr_api");

	std::cout << std::endl;

	RunCommandAndWait("Disabling deletion of files in the Half-Life folder...", icaclsDisableDeletionOnFolderCommandLine);

	std::cout << std::endl;

	RunCommandAndWait("Launching the game...", hlExeCommandLine, hlDirectory.data());

	std::cout << std::endl;

	std::cout << "Game shut down, cleaning up." << std::endl;

	std::cout << std::endl;

	RunCommandAndWait("Enabling deletion of files in the Half-Life folder...", icaclsReenableDeletionOnFolderCommandLine);
	RunCommandAndWait("Enabling deletion of OpenGL32.dll in the Half-Life folder...", icaclsReenableDeletionOnFileCommandLine);

	std::cout << std::endl;

	DeleteDLL(hlDirectory, L"OpenGL32", false);
	DeleteDLL(hlDirectory, L"openvr_api", false);
	RestoreDLL(hlDirectory, L"openvr_api");

	std::cout << std::endl;

	std::cout << yellow << "The patched OpenGL32.dll should have been removed now. You should be able to safely play Half-Life and any of its mods in multiplayer.";
	std::cout << red << " NO WARRANTIES WHATSOEVER! PLEASE CHECK YOUR HALF-LIFE FOLDER TO MAKE SURE THE PATCH HAS BEEN REMOVED." << white << std::endl;

	std::cout << std::endl;

	if (g_hadError)
	{
		std::cerr << red << "HLVRLauncher encountered errors, please check the messages above." << white << std::endl << std::endl;
		system("pause");
		return -1;
	}
	else
	{
		std::cout << white << "HLVRLauncher finished cleaning up, exiting." << white << std::endl;
		return 0;
	}

}
