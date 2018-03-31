#include "ApiHook.h"

static const size_t jmpCmdSize = sizeof(void*) + 1;
static const size_t hookFunctionEntrySize = jmpCmdSize << 1;

template<typename _Ty>
static void writeMemoryOffset(void* pData, size_t offset, const _Ty& val)
{
	*(_Ty*)((char*)pData + offset) = val;
}

MemoryPointer::MemoryPointer()
	:offset(0)
{

}

MemoryPointer::MemoryPointer(size_t offset)
	:offset(offset)
{

}

MemoryPointer::MemoryPointer(const void* pData)
	:offset((size_t)pData)
{

}

size_t calJmpOffset(MemoryPointer src, MemoryPointer dest, size_t skip /*= 0*/)
{
	return dest.offset - src.offset - skip;
}

bool writeMemoryForce(const char* buf, size_t size, void* pDest)
{
	HANDLE hProcess = GetCurrentProcess();
	DWORD oldAccess = NULL;
	BOOL result = VirtualProtectEx(hProcess, pDest, size, PAGE_READWRITE, &oldAccess);
	if (result == FALSE)
		return false;
	WriteProcessMemory(hProcess, (void*)buf, pDest, size, NULL);
	result = VirtualProtectEx(hProcess, pDest, size, oldAccess, &oldAccess);
	return result != FALSE;
}

bool readMemoryForce(char* buf, size_t size, void* pReadbuf)
{
	HANDLE hProcess = GetCurrentProcess();
	DWORD oldAccess = NULL;
	BOOL result = VirtualProtectEx(hProcess, pReadbuf, size, PAGE_EXECUTE_READWRITE, &oldAccess);
	if (result == FALSE)
		return false;
	ReadProcessMemory(hProcess, pReadbuf, buf, size, NULL);
	result = VirtualProtectEx(hProcess, pReadbuf, size, oldAccess, &oldAccess);
	return result != FALSE;
}

bool setExecuteMemory(void* pData, size_t size)
{
	DWORD oldAccess = NULL;
	return VirtualProtectEx(GetCurrentProcess(), pData, size, PAGE_EXECUTE_READ, &oldAccess) != FALSE;
}

void* allocFunctionEntry()
{
	char* ptr = new char[hookFunctionEntrySize];
	memset(ptr, 0, hookFunctionEntrySize);

	DWORD oldAccess;
	VirtualProtect(ptr, hookFunctionEntrySize, PAGE_EXECUTE_READWRITE, &oldAccess);
	return ptr;
}

void freeFunctionEntry(void* pEntry)
{
	if (pEntry == NULL)
		return;
	delete[] (char*)pEntry;
}

ApiHook setApiHook(void* pOldApi, void* pNewApi)
{
	if (pOldApi == NULL || pNewApi == NULL)
	{
		_set_errno(EINVAL);
		return NULL;
	}
	char* pEntry = (char*)allocFunctionEntry();
	//hook���ִ���д���µ�ַ
	memcpy_s(pEntry, jmpCmdSize, pOldApi, jmpCmdSize);
	//�µ�ַ��벿�ָ�Ϊjmp��ԭ��ַ+5
	pEntry[jmpCmdSize]  = 0xE9;
	size_t originAddr = calJmpOffset(&pEntry[jmpCmdSize], (size_t)pOldApi + jmpCmdSize, jmpCmdSize);

	writeMemoryOffset(pEntry, jmpCmdSize + 1, originAddr);
	HANDLE hProcess = GetCurrentProcess();
	DWORD oldAccess;
	VirtualProtectEx(hProcess, pOldApi, jmpCmdSize, PAGE_READWRITE, &oldAccess);
	//����ԭʼapi��ת����api
	size_t newAddr = calJmpOffset(pOldApi, pNewApi, jmpCmdSize);
	((char*)pOldApi)[0] = 0xE9;
	writeMemoryOffset(pOldApi, 1, newAddr);
	VirtualProtectEx(hProcess, pOldApi, jmpCmdSize, oldAccess, &oldAccess);

	ApiHook hook = {};
	hook.pEntry = pEntry;
	//�����ض�����ڵ�ַ
	return hook;
}

ApiHook setDllApiHook(const char* pOldApi, void* pNewApi, HMODULE hModule)
{
	void* pFunc = ::GetProcAddress(hModule, pOldApi);
	return setApiHook(pFunc, pNewApi);
}

void unhookApiHook(ApiHook pEntry, void* pOldApi)
{
	HANDLE hProcess = GetCurrentProcess();
	DWORD oldAccess;
	VirtualProtectEx(hProcess, pOldApi, jmpCmdSize, PAGE_READWRITE, &oldAccess);
	WriteProcessMemory(hProcess, pEntry.pEntry, pOldApi, jmpCmdSize, NULL);
	VirtualProtectEx(hProcess, pOldApi, jmpCmdSize, oldAccess, &oldAccess);
	freeFunctionEntry(pEntry.pEntry);
	pEntry.pEntry = NULL;
}

ApiHook::ApiHook()
	:pEntry(NULL)
{

}

ApiHook::ApiHook(std::nullptr_t)
	:pEntry(NULL)
{

}

bool apiHookInit()
{
	HANDLE hToken;   //�������ƾ��
	TOKEN_PRIVILEGES tp;  //TOKEN_PRIVILEGES�ṹ�壬���а���һ��������+��������Ȩ������
	LUID luid;       //�����ṹ���е�����ֵ
					 //�򿪽������ƻ�
					 //GetCurrentProcess()��ȡ��ǰ���̵�α�����ֻ��ָ��ǰ���̻����߳̾������ʱ�仯
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return false;

	//��ñ��ؽ���name�������Ȩ�����͵ľֲ�ΨһID
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
		return false;

	tp.PrivilegeCount = 1;    //Ȩ��������ֻ��һ����Ԫ�ء�
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;  //Ȩ�޲���
	tp.Privileges[0].Luid = luid;   //Ȩ������

									//��������Ȩ��
	if (!AdjustTokenPrivileges(hToken, 0, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
		return false;
	return true;
}