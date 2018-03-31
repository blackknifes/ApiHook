#ifndef __DLLHOOK_H__
#define __DLLHOOK_H__
#include <string>
#include <Windows.h>

#define HOOK_WRAPPER(call) \
call; \
__asm { ret };

#define HOOK_OBJECT_WRAPPER(call) \
void* pThis; \
__asm \
{ \
	mov pThis, ecx \
} \
call; \
__asm { ret };

struct InjectProcess
{
	InjectProcess();
	InjectProcess(const char* name);
	InjectProcess(const wchar_t* name);
	InjectProcess(DWORD pid);
	InjectProcess(HANDLE hProcess);
	HANDLE hProcess;
};

class DllHook
{
public:
	DllHook();
	~DllHook();
	void init(const char* apiName, void* pNewApi, HMODULE hInjectModule, InjectProcess process);
	void enableHook(bool flag = true);
	void disableHook();

	void* getOldApi() const;

	template<typename _retTy, typename... _argsTy>
	_retTy call(_argsTy... args)
	{
		typedef _retTy(__stdcall*function_type)(_argsTy...);
		disableHook();
		_retTy ret = ((function_type)getOldApi())(std::move(args)...);
		enableHook();
		return ret;
	}

	template<typename... _argsTy>
	void callVoid(_argsTy... args)
	{
		typedef void(*function_type)(_argsTy...);
		disableHook();
		((function_type)getOldApi())(std::move(args)...);
		enableHook();
	}

	template<typename _retTy, typename... _argsTy>
	_retTy callObj(void* pThis, _argsTy... args)
	{
		typedef _retTy(*function_type)(_argsTy...);
		disableHook();
		void* pThiz = this;
		auto pFunc = ((function_type)getOldApi());
		__asm
		{
			mov ecx, pThis
		}

		_retTy ret = pFunc(std::move(args)...);
		__asm
		{
			mov ecx, pThiz;
		}
		enableHook();
		return ret;
	}

	template<typename... _argsTy>
	void callObjVoid(void* pThis, _argsTy... args)
	{
		typedef void(*function_type)(_argsTy...);
		disableHook();
		void* pThiz = this;
		auto pFunc = ((function_type)getOldApi());
		__asm
		{
			mov ecx, pThis
		}
		pFunc(std::move(args)...);
		__asm
		{
			mov ecx, pThiz;
		}
		enableHook();
	}

	static int InjectDll(InjectProcess process, const char* injectDll);
private:
	HANDLE m_hProcess;					//进程句柄
	std::string m_strApiName;			//需要hook的api名字
	void* m_pOldApi;					//原API地址
	char m_pOldCode[5];					//原入口代码
	char m_pNewCode[5];					//新入口代码
};
#endif