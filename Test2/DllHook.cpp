#include "DllHook.h"
#include <TlHelp32.h>
static int EnableDebugPriv()
{
	HANDLE hToken;   //进程令牌句柄
	TOKEN_PRIVILEGES tp;  //TOKEN_PRIVILEGES结构体，其中包含一个【类型+操作】的权限数组
	LUID luid;       //上述结构体中的类型值

					 //打开进程令牌环
					 //GetCurrentProcess()获取当前进程的伪句柄，只会指向当前进程或者线程句柄，随时变化
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return -8;

	//获得本地进程name所代表的权限类型的局部唯一ID
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
		return -9;

	tp.PrivilegeCount = 1;    //权限数组中只有一个“元素”
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;  //权限操作
	tp.Privileges[0].Luid = luid;   //权限类型

									//调整进程权限
	if (!AdjustTokenPrivileges(hToken, 0, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
		return -10;
	return 0;
}

static void PrintError(const char* text)
{
	MessageBoxA(NULL, text, "错误", MB_OK | MB_ICONERROR);
}

static HANDLE FindProcessA(const char* str)
{
	if (str == NULL)
		return GetCurrentProcess();
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	HANDLE hResult = NULL;
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		printf("CreateToolHelp32Snapshot error!\n");
		return NULL;
	}

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);
	char buff[1024] = { 0 };

	BOOL bProcess = Process32First(hProcessSnap, &pe32);
	while (bProcess)
	{
		if (strcmp(str, pe32.szExeFile) == 0)
		{
			hResult = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
			break;
		}

		bProcess = Process32Next(hProcessSnap, &pe32);
	}

	CloseHandle(hProcessSnap);
	return hResult;
}

static HANDLE FindProcessW(const wchar_t* str)
{
	if (str == NULL)
		return GetCurrentProcess();
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	HANDLE hResult = NULL;
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		printf("CreateToolHelp32Snapshot error!\n");
		return NULL;
	}

	PROCESSENTRY32W pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32W);
	char buff[1024] = { 0 };

	BOOL bProcess = Process32FirstW(hProcessSnap, &pe32);
	while (bProcess)
	{
		if (wcscmp(str, pe32.szExeFile) == 0)
		{
			hResult = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
			break;
		}

		bProcess = Process32NextW(hProcessSnap, &pe32);
	}

	CloseHandle(hProcessSnap);
	return hResult;
}

DllHook::DllHook()
	:m_hProcess(NULL), m_pOldApi(NULL), m_pOldCode(), m_pNewCode()
{
	memset(m_pOldCode, 0, sizeof(m_pOldCode));
	memset(m_pNewCode, 0, sizeof(m_pNewCode));
}

DllHook::~DllHook()
{
	if (m_hProcess)
		::CloseHandle(m_hProcess);
}

void DllHook::init(const char* apiName, void* pNewApi, HMODULE hInjectModule, InjectProcess process)
{
	this->m_hProcess = process.hProcess;
	this->m_pOldApi = GetProcAddress(hInjectModule, apiName);

	memcpy_s(m_pOldCode, 5, m_pOldApi, 5);
	m_pNewCode[0] = (char)0xe9;
	size_t addr = ((size_t)pNewApi - (size_t)m_pOldApi - 5);
	memcpy_s(&m_pNewCode[1], 4, &addr, sizeof(addr));
}

void DllHook::enableHook(bool flag)
{
	DWORD dwTemp = 0;
	DWORD dwOldProtect;
	VirtualProtectEx(m_hProcess, m_pOldApi, 5, PAGE_READWRITE, &dwOldProtect);
	if (flag)
		WriteProcessMemory(m_hProcess, m_pOldApi, m_pNewCode, 5, 0);
	else
		WriteProcessMemory(m_hProcess, m_pOldApi, m_pOldCode, 5, 0);
	VirtualProtectEx(m_hProcess, m_pOldApi, 5, dwOldProtect, &dwTemp);
}

void DllHook::disableHook()
{
	enableHook(false);
}

void* DllHook::getOldApi() const
{
	return m_pOldApi;
}

int DllHook::InjectDll(InjectProcess process, const char* injectDll)
{
	//获得调试权限
	if (EnableDebugPriv() != 0)
	{
		PrintError("没有debug权限");
		return -1;
	}

	HANDLE hRemoteProcess = process.hProcess;
	LPVOID pRemoteDllPath = VirtualAllocEx(hRemoteProcess, NULL, strlen(injectDll) + 1, MEM_COMMIT, PAGE_READWRITE);
	if (pRemoteDllPath == NULL)
	{
		PrintError("远程内存分配失败");
		return -3;
	}

	//远程dll路径写入
	DWORD Size;
	if (WriteProcessMemory(hRemoteProcess, pRemoteDllPath, injectDll, strlen(injectDll) + 1, &Size) == NULL)
	{
		PrintError("远程内存写入失败");
		return -4;
	}

	//获得远程进程中LoadLibrary()的地址
	LPTHREAD_START_ROUTINE pLoadLibrary = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "LoadLibraryA");
	if (pLoadLibrary == NULL)
	{
		PrintError("LoadLibrary获取失败");
		return -5;
	}

	//启动远程线程
	DWORD dwThreadId;
	HANDLE hThread;
	if ((hThread = CreateRemoteThread(hRemoteProcess, NULL, 0, pLoadLibrary, pRemoteDllPath, 0, &dwThreadId)) == NULL)
	{
		PrintError("启动远程例程失败");
		return -6;
	}

	//释放分配内存
	if (VirtualFreeEx(hRemoteProcess, pRemoteDllPath, 0, MEM_RELEASE) == 0)
	{
		PrintError("释放内存出错");
		return -8;
	}

	//释放句柄
	if (hThread != NULL) CloseHandle(hThread);
	if (hRemoteProcess != NULL) CloseHandle(hRemoteProcess);

	return 0;
}

InjectProcess::InjectProcess(const char* name)
	:hProcess(FindProcessA(name))
{
}

InjectProcess::InjectProcess(const wchar_t* name)
	: hProcess(FindProcessW(name))
{

}

InjectProcess::InjectProcess(DWORD pid)
	: hProcess(pid == 0 ? GetCurrentProcess() : OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid))
{

}

InjectProcess::InjectProcess(HANDLE hProcess)
	: hProcess(hProcess == NULL ? GetCurrentProcess() : hProcess)
{

}

InjectProcess::InjectProcess()
	: hProcess(GetCurrentProcess())
{

}



// void CallFunction(DllHook& hook)
// {
// 	//获取this指针
// 	void* pThis;
// 	__asm
// 	{
// 		mov pThis, ecx
// 	}
// 	hook.callObjVoid(pThis);
// 	//跳出函数
// 	__asm
// 	{
// 		ret
// 	}
// }