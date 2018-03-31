#ifndef __APIHOOK_H__
#define __APIHOOK_H__
#include <Windows.h>

struct InjectProcess
{
	InjectProcess();
	InjectProcess(const char* name);
	InjectProcess(const wchar_t* name);
	InjectProcess(DWORD pid);
	HANDLE hProcess;
};

struct MemoryPointer
{
	union
	{
		size_t offset;
		const void* pData;
	};

	MemoryPointer();
	MemoryPointer(size_t offset);
	MemoryPointer(const void* pData);
};

struct ApiHook
{
	ApiHook();
	ApiHook(std::nullptr_t);
	void* pEntry;	//�������

	template<typename _retTy, typename... _argsTy>
	_retTy call(_argsTy... args)
	{
		typedef _retTy(*api_type)(_argsTy...);
		api_type api = (api_type)pEntry;
		return api(std::move(args)...);
	}

	template<typename... _argsTy>
	void callVoid(_argsTy... args)
	{
		typedef _retTy(*api_type)(_argsTy...);
		api_type api = (api_type)pEntry;
		api(std::move(args)...);
	}

	template<typename _retTy, typename... _argsTy>
	_retTy callCallback(_argsTy... args)
	{
		typedef _retTy(__stdcall* api_type)(_argsTy...);
		api_type api = (api_type)pEntry;
		return api(std::move(args)...);
	}

	template<typename... _argsTy>
	void callVoidCallback(_argsTy... args)
	{
		typedef _retTy(__stdcall* api_type)(_argsTy...);
		api_type api = (api_type)pEntry;
		api(std::move(args)...);
	}

	template<typename _retTy, typename _ClassTy, typename... _argsTy>
	_retTy callObject(_ClassTy* pThis, _argsTy... args)
	{
		typedef _retTy(_ClassTy::*api_type)(_argsTy...);
		api_type api = (api_type)pEntry;
		return (pThis->*api)(std::move(args)...);
	}

	template<typename _ClassTy, typename... _argsTy>
	void callObjectVoid(_argsTy... args)
	{
		typedef _retTy(_ClassTy::*api_type)(_argsTy...);
		api_type api = (api_type)pEntry;
		(pThis->*api)(std::move(args)...);
	}
};


inline bool operator==(const ApiHook& hook, std::nullptr_t)
{
	return hook.pEntry == 0;
}

inline bool operator==(const ApiHook& left, const ApiHook& right)
{
	return left.pEntry == right.pEntry;
}

//����jmpƫ�ƣ�ԭ��ַ��Ŀ���ַ����Ҫ�������ֽ�����
extern size_t calJmpOffset(MemoryPointer src, MemoryPointer dest, size_t skip = 0);

//ǿ��д���ڴ棨Ҫд������ݣ����ݳ��ȣ�Ŀ���ַ��
extern bool writeMemoryForce(const char* buf, size_t size, void* pDest);

//ǿ�ƶ�ȡ�ڴ棨Ҫд������ݣ����ݳ��ȣ�Ŀ���ַ��
extern bool readMemoryForce(char* buf, size_t size, void* pReadbuf);

//���ڴ���Ϊ�ɶ���ִ�У�Ҫ���ĵĵ�ַ���ڴ�ߴ磩
extern bool setExecuteMemory(void* pData, size_t size);

//����һ��������ڳ����ڴ�
extern void* allocFunctionEntry();

//�ͷ�һ���������
extern void freeFunctionEntry(void* pEntry);

//д��hook���
extern ApiHook setApiHook(void* pOldApi, void* pNewApi);

//д��Dll hook���
extern ApiHook setDllApiHook(const char* pOldApi, void* pNewApi, HMODULE hModule);

//ȡ��api hook
extern void unhookApiHook(ApiHook hook, void* pOldApi);

//api hook���̳�ʼ��
extern bool apiHookInit();

//dllע��(ע����̣����������֣������pid��Ҫע���dll�ļ�)
extern int injectDll(InjectProcess process, const char* injectDll);
#endif
