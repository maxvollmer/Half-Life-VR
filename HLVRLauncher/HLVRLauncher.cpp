
#include "stdafx.h"
#include "HLVRLauncher.h"
#include <windows.h>
#include <stdio.h>
#include <accctrl.h>
#include <Aclapi.h>
#include <string>



#include <chrono>
#include <thread>

void RunCommandAndWait(std::wstring command, const wchar_t* directory = NULL)
{
	TCHAR* szCommand = new TCHAR[command.size() + 1]();
	command.copy(szCommand, command.size());

	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));

	if (CreateProcess(NULL, szCommand, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, directory, &si, &pi))
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	delete[] szCommand;
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	TCHAR szFileName[MAX_PATH + 1];
	GetModuleFileName(NULL, szFileName, MAX_PATH + 1);

	std::wstring filename = szFileName;
	std::wstring vrDirectory = filename.substr(0, filename.rfind('\\'));
	std::wstring hlDirectory = vrDirectory.substr(0, vrDirectory.rfind('\\'));

	std::wstring icaclsSetInheritanceCommandLine = TEXT("icacls ") + hlDirectory + TEXT(" /inheritance:d");
	std::wstring icaclsDisableDeletionOnFolderCommandLine = TEXT("icacls ") + hlDirectory + TEXT(" /deny Everyone:(DE,DC)");
	std::wstring icaclsDisableDeletionOnFileCommandLine = TEXT("icacls ") + hlDirectory + TEXT("\\opengl32.dll /deny Everyone:(DE,DC)");
	std::wstring icaclsReenableDeletionOnFolderCommandLine = TEXT("icacls ") + hlDirectory + TEXT(" /remove:d Everyone");
	std::wstring icaclsReenableDeletionOnFileCommandLine = TEXT("icacls ") + hlDirectory + TEXT("\\opengl32.dll /remove:d Everyone");
	
	std::wstring attribDisableOnFileCommandLine = TEXT("attrib -R ") + hlDirectory + TEXT("\\opengl32.dll");
	std::wstring attribRenableOnFileCommandLine = TEXT("attrib +R ") + hlDirectory + TEXT("\\opengl32.dll");

	std::wstring hlExeCommandLine = hlDirectory + TEXT("\\hl.exe -game vr -dev -env -console -insecure -nomouse -nojoy +sv_lan 1");

	RunCommandAndWait(attribRenableOnFileCommandLine);
	RunCommandAndWait(icaclsSetInheritanceCommandLine);
	RunCommandAndWait(icaclsReenableDeletionOnFolderCommandLine);
	RunCommandAndWait(icaclsReenableDeletionOnFileCommandLine);


	RunCommandAndWait(attribDisableOnFileCommandLine);	

	DeleteFile((hlDirectory + TEXT("\\opengl32.dll")).c_str());
	CopyFile((vrDirectory + TEXT("\\opengl32.dll")).c_str(), (hlDirectory + TEXT("\\opengl32.dll")).c_str(), FALSE);
	RunCommandAndWait(attribRenableOnFileCommandLine);

	RunCommandAndWait(icaclsDisableDeletionOnFolderCommandLine);
	RunCommandAndWait(icaclsDisableDeletionOnFileCommandLine);
	RunCommandAndWait(hlExeCommandLine, hlDirectory.c_str());
	
	RunCommandAndWait(icaclsReenableDeletionOnFolderCommandLine);
	RunCommandAndWait(icaclsReenableDeletionOnFileCommandLine);

	

	RunCommandAndWait(attribDisableOnFileCommandLine);
	
	DeleteFile((hlDirectory + TEXT("\\opengl32.dll")).c_str());

    return 0;
}
