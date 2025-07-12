#include "Console.h"
#include "Game.h"
#include "resource.h"
#include <atomic>
#include <consoleapi.h>
#include <consoleapi2.h>
#include <consoleapi3.h>
#include <cstdio>
#include <cstring>
#include <dwmapi.h>
#include <fileapi.h>
#include <handleapi.h>
#include <intrin.h>
#include <iphlpapi.h>
#include <ipifcons.h>
#include <IPTypes.h>
#include <libloaderapi.h>
#include <lmcons.h>
#include <processenv.h>
#include <processthreadsapi.h>
#include <psapi.h>
#include <string>
#include <sysinfoapi.h>
#include <thread>
#include <Windows.h>
#include <winreg.h>
#if _DEBUG
#include <debugapi.h>
#include "Version.h"
#endif
#include <minwinbase.h>
#include <synchapi.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "wintrust.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using namespace std;

bool CheckCPUIDForVMGuestOnly() {
	int cpuInfo[4] = { 0 };
	__cpuid(cpuInfo, 0x40000000);

	char hyperVendor[13];
	memcpy(hyperVendor, &cpuInfo[1], 4);
	memcpy(hyperVendor + 4, &cpuInfo[2], 4);
	memcpy(hyperVendor + 8, &cpuInfo[3], 4);
	hyperVendor[12] = 0;

	std::string vendor(hyperVendor);

	if (vendor == "Microsoft Hv") return false;

	const char* blockedVendors[] = {
		"VMwareVMware",
		"VBoxVBoxVBox",
		"KVMKVMKVM",
		"XenVMMXenVMM",
		"QEMU"
	};

	for (const char* block : blockedVendors) {
		if (vendor == block) return true;
	}

	return false;
}

bool CheckRegistryForVM() {
	HKEY hKey;
	const char* keys[] = {
		"SOFTWARE\\VMware, Inc.\\VMware Tools",
		"SOFTWARE\\Oracle\\VirtualBox Guest Additions",
		"SYSTEM\\CurrentControlSet\\Services\\VBoxGuest"
	};

	for (const char* key : keys) {
		if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, key, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
			RegCloseKey(hKey);
			return true;
		}
	}
	return false;
}

bool CheckDriversForVM() {
	char systemPath[MAX_PATH];
	GetSystemDirectoryA(systemPath, MAX_PATH);
	string driverPath = string(systemPath) + "\\drivers\\";

	const char* vmDrivers[] = {
		"vmmouse.sys", "vmhgfs.sys", "vm3dgl.dll",
		"VBoxMouse.sys", "VBoxGuest.sys",
		"qemu-ga.exe", "xen.sys"
	};

	for (const char* file : vmDrivers) {
		string full = driverPath + file;
		if (GetFileAttributesA(full.c_str()) != INVALID_FILE_ATTRIBUTES) {
			return true;
		}
	}

	return false;
}

bool CheckMACAddressForVM_Safe() {
	IP_ADAPTER_INFO adapterInfo[16];
	DWORD size = sizeof(adapterInfo);

	if (GetAdaptersInfo(adapterInfo, &size) == NO_ERROR) {
		PIP_ADAPTER_INFO p = adapterInfo;
		while (p) {
			if (p->Type != MIB_IF_TYPE_ETHERNET) {
				p = p->Next;
				continue;
			}

			BYTE* addr = p->Address;
			BYTE vmMACs[][3] = {
				{0x00, 0x05, 0x69},
				{0x00, 0x0C, 0x29},
				{0x00, 0x50, 0x56},
				{0x08, 0x00, 0x27},
				{0x52, 0x54, 0x00}
			};

			for (auto& prefix : vmMACs) {
				if (memcmp(addr, prefix, 3) == 0) {
					if (strstr(p->AdapterName, "VMnet") != nullptr) {
						continue;
					}
					return true;
				}
			}

			p = p->Next;
		}
	}
	return false;
}

bool IsRunningInWindowsSandbox() {
	char username[UNLEN + 1];
	DWORD size = UNLEN + 1;
	if (GetUserNameA(username, &size)) {
		if (strcmp(username, "WDAGUtilityAccount") == 0) {
			return true;
		}
	}

	HMODULE hModule = GetModuleHandleA("SbieDll.dll");
	if (hModule != NULL) return true;

	return false;
}

bool CheckProcessNameForVM() {
	DWORD processes[1024], needed, count;
	if (!K32EnumProcesses(processes, sizeof(processes), &needed)) return false;
	count = needed / sizeof(DWORD);

	char exeName[MAX_PATH];
	for (unsigned int i = 0; i < count; i++) {
		if (processes[i] == 0) continue;
		HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processes[i]);
		if (hProc) {
			if (K32GetModuleBaseNameA(hProc, NULL, exeName, MAX_PATH)) {
				if (strstr(exeName, "VBox") || strstr(exeName, "vmtoolsd") || strstr(exeName, "vboxtray")) {
					CloseHandle(hProc);
					return true;
				}
			}
			CloseHandle(hProc);
		}
	}
	return false;
}

bool IsVirtualOrSandbox() {
	return CheckCPUIDForVMGuestOnly() ||
		CheckRegistryForVM() ||
		CheckDriversForVM() ||
		CheckMACAddressForVM_Safe() ||
		IsRunningInWindowsSandbox() ||
		CheckProcessNameForVM();
}

bool EnforceEnvironmentCheck() {
	if (IsVirtualOrSandbox()) {
		printf("このプログラムは仮想環境やサンドボックスでは実行できません。\n");
		return false;
	}
	return true;
}

typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

bool IsAfterWindows11()
{
	HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
	if (hMod) {
		RtlGetVersionPtr fn = (RtlGetVersionPtr)::GetProcAddress(hMod, "RtlGetVersion");
		if (fn != nullptr) {
			RTL_OSVERSIONINFOW rovi = { 0 };
			rovi.dwOSVersionInfoSize = sizeof(rovi);
			if (fn(&rovi) == 0) {
				return (rovi.dwMajorVersion > 10 || (rovi.dwMajorVersion == 10 && rovi.dwMinorVersion > 0) || (rovi.dwMajorVersion == 10 && rovi.dwMinorVersion == 0 && rovi.dwBuildNumber >= 22000));
			}
		}
	}
	return false;
}

bool IsConsoleWindow() {
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdOut == INVALID_HANDLE_VALUE || hStdOut == NULL) {
		return false;
	}

	DWORD mode;
	BOOL isConsole = GetConsoleMode(hStdOut, &mode);

	if (!isConsole) {
		return false;
	}

	HWND hConsole = GetConsoleWindow();
	if (!hConsole) {
		return false;
	}

	if (!IsWindowVisible(hConsole)) {
		return false;
	}

	return true;
}

DWORD g_dwMode = 0;

bool EnableVirtualTerminalProcessing() {
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE || !hOut) return false;

	if (!GetConsoleMode(hOut, &g_dwMode)) return false;

	DWORD dwMode = g_dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	return SetConsoleMode(hOut, dwMode);
}

void RestoreConsoleMode() {
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE || hOut == NULL) return;

	SetConsoleMode(hOut, g_dwMode);
}

CRITICAL_SECTION g_cs;
HHOOK g_hHook = nullptr;
atomic<bool> g_running = true;

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION) {
		KBDLLHOOKSTRUCT* p = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
		if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
			if (p->vkCode == VK_RETURN && (GetAsyncKeyState(VK_MENU) & 0x8000)) {
				return 1;
			}
		}
	}
	return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

void DisableConsoleExtra() {
	InitializeCriticalSection(&g_cs);
	EnterCriticalSection(&g_cs);
	g_hHook = SetWindowsHookExA(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandleA(NULL), 0);
	MSG msg;
	while (g_running && GetMessageA(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}
	UnhookWindowsHookEx(g_hHook);
	g_hHook = nullptr;
	LeaveCriticalSection(&g_cs);
}

int main() {
#ifndef WIN32
	printf("Windows上で実行してください。\n");
	return 1;
#endif
	if (GetSystemMetrics(SM_CLEANBOOT) != 0) {
		printf("セーフモードでの実行はできません。\n");
		return 1;
	}
	if (!EnforceEnvironmentCheck()) {
		return 1;
	}
	if (!IsAfterWindows11()) {
		printf("このアプリケーションはWindows 11以降のOSで対応しています。\n");
		return 1;
	}
	if (!IsConsoleWindow()) {
		printf("出力がリダイレクトされているため、使用できません。\n");
		return 1;
	}
	if (!EnableVirtualTerminalProcessing()) {
		printf("出力の拡張機能の有効化に失敗しました。\n");
		return 1;
	}
	thread hookWatcher(DisableConsoleExtra);

	HWND csHandle = GetConsoleWindow();
	HICON AppIco = LoadIconA(GetModuleHandleA(NULL), (LPCSTR)IDI_ICON1);
	HICON OldBig = (HICON)SendMessageA(csHandle, WM_SETICON, ICON_BIG, (LPARAM)AppIco);
	HICON OldSmall = (HICON)SendMessageA(csHandle, WM_SETICON, ICON_SMALL, (LPARAM)AppIco);
	DWORD dwSize = sizeof(DWM_WINDOW_CORNER_PREFERENCE);
	DWMWINDOWATTRIBUTE attr = DWMWA_WINDOW_CORNER_PREFERENCE;
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	UINT dwCol_c = 0xff8800, dwCol_b = 0xff8800;
	DwmSetWindowAttribute(csHandle, DWMWA_CAPTION_COLOR, &dwCol_c, sizeof(UINT));
	DwmSetWindowAttribute(csHandle, DWMWA_BORDER_COLOR, &dwCol_b, sizeof(UINT));
	DWM_WINDOW_CORNER_PREFERENCE dwcp = DWMWCP_DONOTROUND;
	DwmSetWindowAttribute(csHandle, attr, &dwcp, dwSize);
	SetWindowLongA(csHandle, GWL_STYLE, GetWindowLongA(csHandle, GWL_STYLE) & ~WS_SYSMENU & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME);

	Console::Instance()->GetOriginalWindowAndBufferSize();
	Console::Instance()->SetWindowAndBufferSize(STAGE_W, STAGE_H);
	Console::Instance()->SetCursorInvisible();

#ifdef _DEBUG
	string s = string("Console Tetris ");
	s += CTEX_VER;
	BOOL b;
	CheckRemoteDebuggerPresent(GetCurrentProcess(), &b);
	if (b) {
		s += " (Debug)";
	}
	SetConsoleTitleA(s.c_str());
#else
	SetConsoleTitleA("Console Tetris Ex");
#endif

	Game game;
	if (game.LoadFile()) {
		MessageBoxA(csHandle, "ファイルを読み込めなかったため、デフォルトの最高スコアが使われます。", "読み込みエラー", MB_ICONEXCLAMATION);
	}
	while (Console::Instance()->ProcessLoop()) {
		Console::Instance()->SetWindowAndBufferSize(STAGE_W, STAGE_H);
		Console::Instance()->SetCursorInvisible();
		game.Update();
		game.Draw();
	}
	if (game.SaveFile()) {
		MessageBoxA(csHandle, "ファイルを書き込めなかったため、最高スコアの保存に失敗しました。", "書き込みエラー", MB_ICONEXCLAMATION);
	}

	Console::Instance()->SetCursorVisible(TRUE);
	dwcp = DWMWCP_DEFAULT;
	DwmSetWindowAttribute(csHandle, attr, &dwcp, dwSize);
	dwCol_b = dwCol_c = 0xffffffff;
	DwmSetWindowAttribute(csHandle, DWMWA_CAPTION_COLOR, &dwCol_c, sizeof(UINT));
	DwmSetWindowAttribute(csHandle, DWMWA_BORDER_COLOR, &dwCol_b, sizeof(UINT));
	SetWindowLongA(csHandle, GWL_STYLE, GetWindowLongA(csHandle, GWL_STYLE) | WS_SYSMENU | WS_MAXIMIZEBOX | WS_THICKFRAME);
	Console::Instance()->RestoreToDefaultWindow();
	SendMessageA(csHandle, WM_SETICON, ICON_BIG, (LPARAM)OldBig);
	SendMessageA(csHandle, WM_SETICON, ICON_SMALL, (LPARAM)OldSmall);
	RestoreConsoleMode();

	g_running = false;
	PostThreadMessageA(GetThreadId(hookWatcher.native_handle()), WM_QUIT, 0, 0);
	hookWatcher.join();

	printf("\x1b[2J");
	printf("\x1b[3J");

	return 0;
}
