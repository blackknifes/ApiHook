#include <conio.h>
#include <unordered_map>
#include "ApiHook.h"

ApiHook hook;
HMODULE hModule = NULL;
FILE* f = NULL;


int __stdcall hook_recv(SOCKET s, char * buf, int len, int flags)
{
	int result = hook.callCallback<int>(s, buf, len, flags);

	if (result > 19 && buf && len > 0)
	{
		printf("hook tcp recv\n");
		printf("result:%d\n", result);
		char header[3] = { 0x3F, 0x3F, 0x3F };
		char tag[16] = {0x40, 0x02, 0x77, 0x78, 0x40, 0x01, 0x73, 0x01, 0x40, 0x04, 0x3F, 0x20, 0x20, 0x01, 0x20, 0x01};
// 		if (ComparePiece(buf, header, sizeof(header)) && ComparePiece(&buf[6], tag, sizeof(tag)))
// 		{
			printf("接收到聊天消息:\n");
			_lock_file(f);
			_fwrite_nolock("tcp: ", 1, 5, f);
			_fwrite_nolock(buf, 1, result, f);
			_fwrite_nolock("\r\n\r\n", 1, 4, f);
			_unlock_file(f);
			printf("char:%s\n", buf);
			wprintf(L"wchar: %s\n**********************************\n\n", buf);
/*		}*/
	}

	return result;
}

static DWORD CALLBACK ThreadProc(void*)
{
	while (true)
	{
		int ch = _getch();
		switch (ch)
		{
		case VK_ESCAPE:
			return 0;
		case 'r':
			system("cls");
			break;
		default:
			break;
		}
	}
	return 0;
}

BOOL WINAPI DllMain(
	_In_ HINSTANCE hinstDLL, // 指向自身的句柄
	_In_ DWORD fdwReason, // 调用原因
	_In_ LPVOID lpvReserved // 隐式加载和显式加载
)
{
	AllocConsole();
	freopen("CONOUT$", "w+t", stdout);
	freopen("CONOUT$", "w+t", stderr);
	freopen("CONIN$", "r+t", stdin);
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		bool result = apiHookInit();
		if (fopen_s(&f, "D:/api2.txt", "wb+") != 0)
			printf("log文件创建失败\n");

		printf("注入成功\n");
		::SetDllDirectoryA(R"(C:\Program Files (x86)\AliWorkbench\6.06.02N)");
		hModule = ::LoadLibraryA("ws2_32.dll");
		hook = setDllApiHook("recv", hook_recv, hModule);
		CreateThread(NULL, NULL, ThreadProc, NULL, NULL, NULL);
	}
	break;
	case DLL_PROCESS_DETACH:
		printf("卸载hookDll\n");
		fclose(f);
		::FreeLibrary(hModule);
		::FreeConsole();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	default:
		break;
	}
	return TRUE;
}