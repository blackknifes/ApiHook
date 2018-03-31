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
	void* pEntry;	//函数入口

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

//计算jmp偏移（原地址，目标地址，需要跳过的字节数）
extern size_t calJmpOffset(MemoryPointer src, MemoryPointer dest, size_t skip = 0);

//强制写入内存（要写入的数据，数据长度，目标地址）
extern bool writeMemoryForce(const char* buf, size_t size, void* pDest);

//强制读取内存（要写入的数据，数据长度，目标地址）
extern bool readMemoryForce(char* buf, size_t size, void* pReadbuf);

//将内存设为可读可执行（要更改的地址，内存尺寸）
extern bool setExecuteMemory(void* pData, size_t size);

//分配一个函数入口长度内存
extern void* allocFunctionEntry();

//释放一个函数入口
extern void freeFunctionEntry(void* pEntry);

//写入hook入口
extern ApiHook setApiHook(void* pOldApi, void* pNewApi);

//写入Dll hook入口
extern ApiHook setDllApiHook(const char* pOldApi, void* pNewApi, HMODULE hModule);

//取消api hook
extern void unhookApiHook(ApiHook hook, void* pOldApi);

//api hook进程初始化
extern bool apiHookInit();

//dll注入(注入进程（可输入名字，句柄，pid，要注入的dll文件)
extern int injectDll(InjectProcess process, const char* injectDll);
#endif
