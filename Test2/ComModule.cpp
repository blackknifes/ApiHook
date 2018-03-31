#include "ComModule.h"



void ComModule::Startup()
{
	CoInitialize(NULL);
}

void ComModule::Shuwdown()
{
	CoUninitialize();
}

ComModule::ComModule()
	:hModule(NULL), pDllGetClassObject(NULL)
{

}

ComModule::ComModule(const wchar_t* path)
	: hModule(NULL), pDllGetClassObject(NULL)
{
	load(path);
}

ComModule::~ComModule()
{
	for (auto itor = factoryMap.begin(); itor != factoryMap.end(); ++itor)
		itor->second->Release();
	if (hModule)
		::CoFreeLibrary(hModule);
}

bool ComModule::load(const wchar_t* path)
{
	if (hModule != NULL)
		return true;
	hModule = CoLoadLibrary((BSTR)path, FALSE);
	pDllGetClassObject = (PFN_DllGetClassObject)GetProcAddress(hModule, "DllGetClassObject");
	return pDllGetClassObject != NULL;
}

HRESULT ComModule::DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID FAR* ppv)
{
	if (pDllGetClassObject == NULL)
		return E_FAIL;
	return pDllGetClassObject(rclsid, riid, ppv);
}

HRESULT ComModule::CreateInstance(
	_In_ REFCLSID rclsid, 
	_In_ REFIID riid,
	_COM_Outptr_ _At_(*ppv, _Post_readable_size_(_Inexpressible_(varies))) LPVOID FAR * ppv)
{
	HRESULT result = NOERROR;
	auto itor = factoryMap.find(riid);
	if (itor == factoryMap.end())
	{
		IClassFactory* pFactory = NULL;
		result = pDllGetClassObject(rclsid, IID_IClassFactory, (void**)&pFactory);
		if (pFactory == NULL)
			return result;
		itor = factoryMap.insert(factory_map::value_type(riid, pFactory)).first;
	}
	*ppv = itor->second;
	return result;
}
