
#include "stdafx.h"
#include "HLVRLauncher.h"
#include <windows.h>
#include <stdio.h>
#include <accctrl.h>
#include <Aclapi.h>
#include <string>
#include <iostream>

#pragma warning( disable : 4996 ) 

void RunCommandAndWait(std::string description, std::wstring command, const wchar_t* directory = nullptr)
{
	std::cout << "Running command: " << description << std::endl;

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
		}
		else
		{
			throw 0;
		}
	}
	catch (...)
	{
		std::cerr << "Error: Command failed with error: " << GetLastError() << ". Shutting down." << std::endl;
		std::exit(-1);
	}
}

// FileExists from https://stackoverflow.com/a/6218957/9199167
BOOL FileExistsW(LPCWSTR szPath)
{
	DWORD dwAttrib = GetFileAttributesW(szPath);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

void DeleteOpenGL(const std::wstring& hlDirectory)
{
	std::cout << "Deleting OpenGL32.dll." << std::endl;

	SetLastError(0);

	try
	{
		auto path = (hlDirectory + L"\\opengl32.dll").data();
		if (FileExistsW(path) && !DeleteFileW(path))
		{
			throw 0;
		}
	}
	catch (...)
	{
		std::cerr << "Warning: Failed to delete OpenGL32.dll. Error: " << GetLastError() << "." << std::endl;
	}
}

void PatchOpenGL(const std::wstring& hlDirectory, const std::wstring& vrDirectory)
{
	std::cout << "Patching OpenGL32.dll." << std::endl;

	SetLastError(0);

	try
	{
		if (!CopyFileW((vrDirectory + L"\\opengl32.dll").data(), (hlDirectory + L"\\opengl32.dll").data(), FALSE))
		{
			throw 0;
		}
	}
	catch (...)
	{
		std::cerr << "Warning: Couldn't patch OpenGL32.dll. Error: " << GetLastError() << ". If the game doesn't run, you need to patch manually." << std::endl;
	}
}

void ForceSingleProcess()
{
	SetLastError(0);
	HANDLE mutex = CreateMutexW(0, FALSE, L"HalfLifeVirtualRealityLauncherMutexLaliludgnskdagjfgbs");
	if (mutex == NULL)
	{
		std::cerr << "Error: Not enough rights to run HLVRLauncher. Try running as administrator. Shutting down." << std::endl;
		std::exit(-1);
	}
	else if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		std::cerr << "Error: Another instance of HLVRLauncher is already running. Shutting down." << std::endl;
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

	std::wstring icaclsSetInheritanceCommandLine = L"icacls " + hlDirectory + L" /inheritance:d";
	std::wstring icaclsDisableDeletionOnFolderCommandLine = L"icacls " + hlDirectory + L" /deny Everyone:(DE,DC)";
	std::wstring icaclsDisableDeletionOnFileCommandLine = L"icacls " + hlDirectory + L"\\opengl32.dll /deny Everyone:(DE,DC)";
	std::wstring icaclsReenableDeletionOnFolderCommandLine = L"icacls " + hlDirectory + L" /remove:d Everyone";
	std::wstring icaclsReenableDeletionOnFileCommandLine = L"icacls " + hlDirectory + L"\\opengl32.dll /remove:d Everyone";

	std::wstring hlExeCommandLine = hlDirectory + L"\\hl.exe -game vr -dev 2 -console -insecure -nomouse -nowinmouse -nojoy -noip -nofbo -window -width 800 -height 600 +sv_lan 1 +cl_mousegrab 0";

	std::cout << "Finished initializing!" << std::endl;

	std::cout << std::endl;

	std::cout
		<< "HLVRLauncher is now going to run a set of commands to patch OpenGL32.dll."
		<< std::endl
		<< "It will then launch the game."
		<< std::endl
		<< "Once the game ends, HLVRLauncher will clean up afterwards, restoring the original OpenGL32.dll."
		<< std::endl
		<< "SOME OF THESE COMMANDS MAY TAKE A WHILE TO COMPLETE. PLEASE BE PATIENT!"
		<< std::endl;

	std::cout << std::endl;

	RunCommandAndWait("Disable Access Control List inheritance on the Half-Life folder.", icaclsSetInheritanceCommandLine);
	RunCommandAndWait("Enable deletion of files in the Half-Life folder.", icaclsReenableDeletionOnFolderCommandLine);
	RunCommandAndWait("Enable deletion of OpenGL32.dll in the Half-Life folder.", icaclsReenableDeletionOnFileCommandLine);

	std::cout << std::endl;

	DeleteOpenGL(hlDirectory);
	PatchOpenGL(hlDirectory, vrDirectory);

	std::cout << std::endl;

	RunCommandAndWait("Disable deletion of files in the Half-Life folder.", icaclsDisableDeletionOnFolderCommandLine);
	RunCommandAndWait("Disable deletion of OpenGL32.dll in the Half-Life folder.", icaclsDisableDeletionOnFileCommandLine);

	std::cout << std::endl;

	RunCommandAndWait("Launching the game.", hlExeCommandLine, hlDirectory.data());

	std::cout << std::endl;

	std::cout << "Game shut down, cleaning up..." << std::endl;

	std::cout << std::endl;

	RunCommandAndWait("Enable deletion of files in the Half-Life folder.", icaclsReenableDeletionOnFolderCommandLine);
	RunCommandAndWait("Enable deletion of OpenGL32.dll in the Half-Life folder.", icaclsReenableDeletionOnFileCommandLine);

	std::cout << std::endl;

	DeleteOpenGL(hlDirectory);

	std::cout << std::endl;

	std::cout << "HLVRLauncher finished cleaning up, exiting.";

    return 0;
}
