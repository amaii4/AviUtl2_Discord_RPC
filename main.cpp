#include <windows.h>
#include <vfw.h>
#include <process.h>
#include <string>
#include <shlwapi.h>
#include "discord_rpc.h"
#include "input2.h"
#pragma comment(lib, "vfw32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "discord-rpc.lib")

#define MENU_ID 1000 
#define MENU_ID_ENABLE_RPC 1001
#define MENU_ID_SHOW_PFNAME 1002

WNDPROC g_pfnOriginalWndProc = NULL;
HWND    g_hExEdit2 = NULL;
HMENU   g_hViewMenu = NULL;
HMENU   hRpcMenu = NULL;
bool    discordInitialized = false;
wchar_t pfName[256];
LRESULT CALLBACK RPCSetting(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void InitDiscord();
void Update();
bool isrpcenable = false;
bool ispfname = false;
HINSTANCE hInstance;

std::string GetConfigIniPath()
{
	char path[MAX_PATH];
	GetModuleFileNameA(hInstance, path, MAX_PATH);
	PathRemoveFileSpecA(path);

	strcat_s(path, "\\discordrpc.ini");
	return path;
}

void SaveSettings()
{
	std::string iniPath = GetConfigIniPath();
	WritePrivateProfileStringA("Settings", "isrpcenable", isrpcenable ? "1" : "0", iniPath.c_str());
	WritePrivateProfileStringA("Settings", "ispfname", ispfname ? "1" : "0", iniPath.c_str());
}

void LoadSettings()
{
	std::string iniPath = GetConfigIniPath();
	char value[8];
	GetPrivateProfileStringA("Settings", "isrpcenable", "1", value, sizeof(value), iniPath.c_str());
	isrpcenable = (strcmp(value, "1") == 0);

	GetPrivateProfileStringA("Settings", "ispfname", "1", value, sizeof(value), iniPath.c_str());
	ispfname = (strcmp(value, "1") == 0);

}

std::string WcharToChar(const wchar_t* wide_str) {
	if (wide_str == nullptr) return "";
	int required_size = WideCharToMultiByte(CP_UTF8, 0, wide_str, -1, NULL, 0, NULL, NULL);
	if (required_size == 0) return "";
	std::string result(required_size, 0);
	int converted_size = WideCharToMultiByte(CP_UTF8, 0, wide_str, -1, &result[0], required_size, NULL, NULL);
	result.resize(converted_size - 1);
	return result;
}

unsigned __stdcall MenuSetupThread(void* pArguments) {

	g_hExEdit2 = FindWindowW(L"aviutl2Manager", NULL); 
	if (!g_hExEdit2) {
		return 1;
	}

	HMENU hMenuBar = GetMenu(g_hExEdit2);
	g_hViewMenu = GetSubMenu(hMenuBar, 3); 
	if (!g_hViewMenu) {
		return 1;
	}
	hRpcMenu = CreatePopupMenu();

	UINT rpcFlag = isrpcenable ? MF_CHECKED : MF_UNCHECKED;
	AppendMenuW(hRpcMenu, MF_STRING | rpcFlag, MENU_ID_ENABLE_RPC, L"RPCを有効化");

	UINT pfnameFlag = ispfname ? MF_CHECKED : MF_UNCHECKED;
	AppendMenuW(hRpcMenu, MF_STRING | pfnameFlag, MENU_ID_SHOW_PFNAME, L"プロジェクト名を表示");
	AppendMenuW(g_hViewMenu, MF_POPUP, (UINT_PTR)hRpcMenu, L"DiscordRPC");
	g_pfnOriginalWndProc = (WNDPROC)SetWindowLongPtrW(g_hExEdit2, GWLP_WNDPROC, (LONG_PTR)RPCSetting);
	InitDiscord();
	Update();
	return 0;
}

INPUT_PLUGIN_TABLE input_plugin_table = {
	INPUT_PLUGIN_TABLE::FLAG_VIDEO | INPUT_PLUGIN_TABLE::FLAG_AUDIO,
	L"DiscordRPC",
	L"",
	L"DiscordRPC",
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};
void InitDiscord()
{
	if (discordInitialized) {
		return;
	}
	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));
	Discord_Initialize("1278325804794642494", &handlers, 1, NULL);
	discordInitialized = true;
}

void UpdateDiscordPresence() {
	
	if (isrpcenable == true)
	{
		if(ispfname == false){
			InitDiscord();
			DiscordRichPresence presence;
			memset(&presence, 0, sizeof(presence));
			presence.state = u8"編集中";
			presence.details = u8"";
			presence.largeImageKey = "aviutl2";
			presence.largeImageText = "AviUtl2";
			Discord_UpdatePresence(&presence);
		}
		else {
			InitDiscord();
			GetWindowTextW(g_hExEdit2, pfName, 256);
			std::string pfNameS = WcharToChar(pfName);
			DiscordRichPresence presence;
			memset(&presence, 0, sizeof(presence));
			presence.state = u8"編集中";
			presence.details = pfNameS.c_str();
			presence.largeImageKey = "aviutl2";
			presence.largeImageText = "AviUtl2";
			Discord_UpdatePresence(&presence);
		}
	}
}
void Update() {
	while (1) {
		UpdateDiscordPresence();
		Sleep(2000);
	}
}
void func_init(void) {
	_beginthreadex(NULL, 0, &MenuSetupThread, NULL, 0, NULL);
	LoadSettings();
}

void func_exit(void) {
	if (g_pfnOriginalWndProc && g_hExEdit2) {
		SetWindowLongPtrW(g_hExEdit2, GWLP_WNDPROC, (LONG_PTR)g_pfnOriginalWndProc);
	}
	if (g_hViewMenu) {
		RemoveMenu(g_hViewMenu, MENU_ID, MF_BYCOMMAND);
		RemoveMenu(g_hViewMenu, GetMenuItemCount(g_hViewMenu) - 1, MF_BYPOSITION);
	}
}

EXTERN_C __declspec(dllexport) INPUT_PLUGIN_TABLE* GetInputPluginTable(void)
{
	return &input_plugin_table;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		hInstance = hinstDLL;
		func_init();
		break;
	case DLL_PROCESS_DETACH:
		if (lpvReserved != nullptr) break;
		Discord_Shutdown();
		func_exit();
		break;
	}
	return TRUE;
}

LRESULT CALLBACK RPCSetting(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (uMsg == WM_COMMAND) {
		switch (LOWORD(wParam)) {
		case MENU_ID_ENABLE_RPC:
		{
			isrpcenable = !isrpcenable;
			if (isrpcenable == false) {
				Discord_Shutdown();
				discordInitialized = false;
			}
			else {
				UpdateDiscordPresence();
			}
			UINT rpcFlag = isrpcenable ? MF_CHECKED : MF_UNCHECKED;
			CheckMenuItem(hRpcMenu, MENU_ID_ENABLE_RPC, MF_BYCOMMAND | rpcFlag);

			SaveSettings();
			return 0;
		}
		case MENU_ID_SHOW_PFNAME:
		{
			ispfname = !ispfname;
			UpdateDiscordPresence();
			UINT pfnameFlag = ispfname ? MF_CHECKED : MF_UNCHECKED;
			CheckMenuItem(hRpcMenu, MENU_ID_SHOW_PFNAME, MF_BYCOMMAND | pfnameFlag);

			SaveSettings();
			return 0;
		}
		}
	}
	return CallWindowProc(g_pfnOriginalWndProc, hWnd, uMsg, wParam, lParam);
}
