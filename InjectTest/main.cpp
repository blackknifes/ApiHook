#include "DllHook.h"
#include <conio.h>
#include <tlhelp32.h>

#define INJECT_DLL "AliWorkbench.exe"

int main()
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	si.dwFlags = STARTF_USESHOWWINDOW;  // ָ��wShowWindow��Ա��Ч
	si.wShowWindow = TRUE;          // �˳�Ա��ΪTRUE�Ļ�����ʾ�½����̵������ڣ�
	char cmd[] = R"("C:\Program Files (x86)\AliWorkbench\AliWorkbench.exe" /run:desktop)";
	CreateProcessA(
		NULL,
		cmd,
		NULL,
		NULL,
		FALSE,
		NORMAL_PRIORITY_CLASS,
		NULL,
		NULL,
		&si,
		&pi
	);
	int result = DllHook::InjectDll(INJECT_DLL, "WndTest.dll");
	for (size_t i = 0; i < 10000 && result != 0; ++i)
	{
		result = DllHook::InjectDll(INJECT_DLL, "WndTest.dll");
	}
	if (result != 0)
	{
		printf("dllע����: %d\n", result);
		_getch();
	}
	return 0;
}