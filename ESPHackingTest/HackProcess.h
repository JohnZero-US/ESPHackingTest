#pragma once
#include <Windows.h>
#include <TlHelp32.h>
//THIS FILE SIMPLY DOES MOST OF THE BACKEND WORK FOR US, FROM FINDING THE PROCESS TO SETTING UP CORRECT MEMORY ACCESS


//黑客程序配置器
class CHackProcess
{
public:
	//目标程序对象
	PROCESSENTRY32 __gameProcess;
	//代理器
	HANDLE __HandleProcess;
	HWND __HWNDCss;
	//Client.dll
	DWORD __dwordClient;
	//Engine.dll
	DWORD __dwordEngine;
	DWORD __dwordOverlay;
	DWORD __dwordVGui;
	DWORD __dwordLibCef;
	DWORD __dwordSteam;

	//通过程序名称找到该程序对象
	DWORD FindProcessName(const char *__ProcessName, PROCESSENTRY32 *pEntry)
	{
		PROCESSENTRY32 __ProcessEntry;
		__ProcessEntry.dwSize = sizeof(PROCESSENTRY32);
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot == INVALID_HANDLE_VALUE) return 0;        if (!Process32First(hSnapshot, &__ProcessEntry))
		{
			CloseHandle(hSnapshot);
			return 0;
		}
		do {
			if (!_strcmpi(__ProcessEntry.szExeFile, __ProcessName))
			{
				memcpy((void *)pEntry, (void *)&__ProcessEntry, sizeof(PROCESSENTRY32));
				CloseHandle(hSnapshot);
				return __ProcessEntry.th32ProcessID;
			}
		} while (Process32Next(hSnapshot, &__ProcessEntry));
		CloseHandle(hSnapshot);
		return 0;
	}

	//通过该程序的id获取线程
	DWORD getThreadByProcess(DWORD __DwordProcess)
	{
		THREADENTRY32 __ThreadEntry;
		__ThreadEntry.dwSize = sizeof(THREADENTRY32);
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
		if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

		if (!Thread32First(hSnapshot, &__ThreadEntry)) { CloseHandle(hSnapshot); return 0; }

		do {
			if (__ThreadEntry.th32OwnerProcessID == __DwordProcess)
			{
				CloseHandle(hSnapshot);
				return __ThreadEntry.th32ThreadID;
			}
		} while (Thread32Next(hSnapshot, &__ThreadEntry));
		CloseHandle(hSnapshot);
		return 0;
	}

	DWORD GetModuleNamePointer(LPSTR LPSTRModuleName, DWORD __DwordProcessId)
	{
		MODULEENTRY32 lpModuleEntry = { 0 };
		HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, __DwordProcessId);
		if (!hSnapShot)
			return NULL;
		lpModuleEntry.dwSize = sizeof(lpModuleEntry);
		BOOL __RunModule = Module32First(hSnapShot, &lpModuleEntry);
		while (__RunModule)
		{
			if (!strcmp(lpModuleEntry.szModule, LPSTRModuleName))
			{
				CloseHandle(hSnapShot);
				return (DWORD)lpModuleEntry.modBaseAddr;
			}
			__RunModule = Module32Next(hSnapShot, &lpModuleEntry);
		}
		CloseHandle(hSnapShot);
		return NULL;
	}

	void runSetDebugPrivs()
	{
		HANDLE __HandleProcess = GetCurrentProcess(), __HandleToken;
		TOKEN_PRIVILEGES priv;
		LUID __LUID;
		OpenProcessToken(__HandleProcess, TOKEN_ADJUST_PRIVILEGES, &__HandleToken);
		LookupPrivilegeValue(0, "seDebugPrivilege", &__LUID);
		priv.PrivilegeCount = 1;
		priv.Privileges[0].Luid = __LUID;
		priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		AdjustTokenPrivileges(__HandleToken, false, &priv, 0, 0, 0);
		CloseHandle(__HandleToken);
		CloseHandle(__HandleProcess);
	}

	void RunProcess()
	{
		//commented lines are for non steam versions of the game
		runSetDebugPrivs();
		while (!FindProcessName("hl2.exe", &__gameProcess)) Sleep(12);
		while (!(getThreadByProcess(__gameProcess.th32ProcessID))) Sleep(12);
		__HandleProcess = OpenProcess(PROCESS_ALL_ACCESS, false, __gameProcess.th32ProcessID);
		while (__dwordClient == 0x0) __dwordClient = GetModuleNamePointer("client.dll", __gameProcess.th32ProcessID);
		while (__dwordEngine == 0x0) __dwordEngine = GetModuleNamePointer("engine.dll", __gameProcess.th32ProcessID);
		//while(__dwordOverlay == 0x0) __dwordOverlay = GetModuleNamePointer("gameoverlayrenderer.dll", __gameProcess.th32ProcessID);
		while (__dwordVGui == 0x0) __dwordVGui = GetModuleNamePointer("vguimatsurface.dll", __gameProcess.th32ProcessID);
		//while(__dwordLibCef == 0x0) __dwordLibCef = GetModuleNamePointer("libcef.dll", __gameProcess.th32ProcessID);
	//	while(__dwordSteam == 0x0) __dwordSteam = GetModuleNamePointer("steam.dll", __gameProcess.th32ProcessID);
		__HWNDCss = FindWindow(NULL, "Counter-Strike Source");
	}
};

extern CHackProcess fProcess;