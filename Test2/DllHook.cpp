#include "DllHook.h"
#include <TlHelp32.h>
static int EnableDebugPriv()
{
	HANDLE hToken;   //�������ƾ��
	TOKEN_PRIVILEGES tp;  //TOKEN_PRIVILEGES�ṹ�壬���а���һ��������+��������Ȩ������
	LUID luid;       //�����ṹ���е�����ֵ

					 //�򿪽������ƻ�
					 //GetCurrentProcess()��ȡ��ǰ���̵�α�����ֻ��ָ��ǰ���̻����߳̾������ʱ�仯
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return -8;

	//��ñ��ؽ���name�������Ȩ�����͵ľֲ�ΨһID
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
		return -9;

	tp.PrivilegeCount = 1;    //Ȩ��������ֻ��һ����Ԫ�ء�
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;  //Ȩ�޲���
	tp.Privileges[0].Luid = luid;   //Ȩ������

									//��������Ȩ��
	if (!AdjustTokenPrivileges(hToken, 0, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
		return -10;
	return 0;
}

static void PrintError(const char* text)
{
	MessageBoxA(NULL, text, "����", MB_OK | MB_ICONERROR);
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
	//��õ���Ȩ��
	if (EnableDebugPriv() != 0)
	{
		PrintError("û��debugȨ��");
		return -1;
	}

	HANDLE hRemoteProcess = process.hProcess;
	LPVOID pRemoteDllPath = VirtualAllocEx(hRemoteProcess, NULL, strlen(injectDll) + 1, MEM_COMMIT, PAGE_READWRITE);
	if (pRemoteDllPath == NULL)
	{
		PrintError("Զ���ڴ����ʧ��");
		return -3;
	}

	//Զ��dll·��д��
	DWORD Size;
	if (WriteProcessMemory(hRemoteProcess, pRemoteDllPath, injectDll, strlen(injectDll) + 1, &Size) == NULL)
	{
		PrintError("Զ���ڴ�д��ʧ��");
		return -4;
	}

	//���Զ�̽�����LoadLibrary()�ĵ�ַ
	LPTHREAD_START_ROUTINE pLoadLibrary = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "LoadLibraryA");
	if (pLoadLibrary == NULL)
	{
		PrintError("LoadLibrary��ȡʧ��");
		return -5;
	}

	//����Զ���߳�
	DWORD dwThreadId;
	HANDLE hThread;
	if ((hThread = CreateRemoteThread(hRemoteProcess, NULL, 0, pLoadLibrary, pRemoteDllPath, 0, &dwThreadId)) == NULL)
	{
		PrintError("����Զ������ʧ��");
		return -6;
	}

	//�ͷŷ����ڴ�
	if (VirtualFreeEx(hRemoteProcess, pRemoteDllPath, 0, MEM_RELEASE) == 0)
	{
		PrintError("�ͷ��ڴ����");
		return -8;
	}

	//�ͷž��
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
// 	//��ȡthisָ��
// 	void* pThis;
// 	__asm
// 	{
// 		mov pThis, ecx
// 	}
// 	hook.callObjVoid(pThis);
// 	//��������
// 	__asm
// 	{
// 		ret
// 	}
// }