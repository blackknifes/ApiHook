#include <Windows.h>
#include <conio.h>
#include <atlbase.h>
#include <atlconv.h>
#include <Mshtml.h>
#include "ComModule.h"
#include "DllHook.h"

#import "C:/Program Files (x86)/AliWorkbench/6.06.02N/IMChatMessageView.dll" no_namespace no_auto_exclude raw_interfaces_only
#import "C:/Program Files (x86)/AliWorkbench/6.06.02N/WWApplication.dll" no_namespace
#import "C:/Program Files (x86)/AliWorkbench/6.06.02N/wwutils.dll"
#import "C:/Program Files (x86)/AliWorkbench/6.06.02N/imbiz.dll"
#import "C:/Program Files (x86)/AliWorkbench/6.06.02N/IMBizLoader.dll"

#import "libid:258EFC18-DE50-44D2-94E8-4DE23882DF82" version("1.0") raw_interfaces_only no_namespace

typedef decltype(DllGetClassObject)* PFN_DllGetClassObject;

ComModule wwModule;
ComModule imCharModule;
ComModule wwXModule;

typedef void(*PFNS)();

// "??0CId2WangHaoMapCache@@QAE@ABV0@@Z",
// "??4?$singleton_base@VCId2WangHaoMapCache@@@@QAEAAV0@ABV0@@Z",
// "??4CId2WangHaoMapCache@@QAEAAV0@ABV0@@Z",
// "??_7CId2WangHaoMapCache@@6B@",
// "?finalize@?$singleton_base@VCId2WangHaoMapCache@@@@AAEXXZ",
// "?instance@?$singleton_base@VCId2WangHaoMapCache@@@@SAAAVCId2WangHaoMapCache@@XZ",
// "DllCanUnloadNow",
// "DllGetClassObject"

//#pragma comment(linker, "/EXPORT:??0CId2WangHaoMapCache@@QAE@ABV0@@Z=aa")


class AA
{
public:
	void aa()
	{
		int a = 0;
	};

	void bb()
	{
		__asm
		{
			lea ecx,[this]
			call AA::aa
		}
	}
};

typedef long (*PFN_IMCM_GetChated)(wchar_t**);

DllHook getCurrentUserId;
HMODULE hModule = NULL;
int main()
{
	hModule = LoadLibraryA("wwutils.dll");
	{
		wchar_t exePath[1024] = {};
		::GetModuleFileNameW(GetModuleHandle(NULL), exePath, 1024);
		*wcsrchr(exePath, '\\') = 0;
		::SetCurrentDirectoryW(exePath);
/*		::AddDllDirectory(exePath);*/
		::SetDllDirectoryW(LR"(C:\Program Files (x86)\AliWorkbench\6.06.02N\)");
	}
	wwXModule.load(L"wwutils.dll");
	PFN_IMCM_GetChated pIMCM_GetChated = (PFN_IMCM_GetChated)GetProcAddress(hModule, "?IMCM_GetChated@@YAHV_bstr_t@@@Z");
	wchar_t* ptr;
	pIMCM_GetChated(&ptr);
	
	_getch();
	return 0;
}