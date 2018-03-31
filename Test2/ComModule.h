#ifndef __COMMODULE_H__
#define __COMMODULE_H__
#include <Windows.h>
#include <unordered_map>


template<>
struct std::hash<GUID>
{
	size_t operator()(const GUID& guid)const
	{
		char buf[17] = {};
		memcpy_s(buf, sizeof(buf), &guid, sizeof(guid));
		return std::hash<const char*>()(buf);
	}
};

typedef decltype(DllGetClassObject)* PFN_DllGetClassObject;
class ComModule
{
public:
	typedef std::unordered_map<IID, IClassFactory*> factory_map;

	static void Startup();
	static void Shuwdown();
	ComModule();
	ComModule(const wchar_t* path);
	~ComModule();
	bool load(const wchar_t* path);
	HRESULT DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID FAR* ppv);
	HRESULT CreateInstance(
		_In_ REFCLSID rclsid,
		_In_ REFIID riid,
		_COM_Outptr_ _At_(*ppv, _Post_readable_size_(_Inexpressible_(varies))) LPVOID FAR * ppv);
private:
	HMODULE hModule;
	PFN_DllGetClassObject pDllGetClassObject;
	factory_map factoryMap;
};
#endif
