#include <Windows.h>

#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

const wchar_t* AppName = L"Browser Selector";
const size_t BUF_SIZE = 32768;

int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPWSTR lpCmdLine, int /*nCmdShow*/)
{
	if (lpCmdLine[0] == L'\0')
	{
		MessageBox(NULL, L"Usage: BrowserSelector.exe <URL>", AppName, MB_ICONINFORMATION | MB_OK);
		return 1;
	}

	wchar_t* buf1 = new wchar_t[BUF_SIZE];
	wchar_t* buf2 = new wchar_t[BUF_SIZE];
	wchar_t* iniFile = new wchar_t[BUF_SIZE];

	// Get path to our own executable
	size_t pathLen = GetModuleFileName(NULL, iniFile, BUF_SIZE);
	if (pathLen == 0)
	{
		MessageBox(NULL, L"Failed to obtain self-exe location!", AppName, MB_ICONERROR | MB_OK);
		return 1;
	}

	// Construct INI file path
	if (_wcsicmp(iniFile + pathLen - 4, L".exe") == 0)
		wcscpy_s(iniFile + pathLen - 3, BUF_SIZE - pathLen + 3, L"ini");
	else
	{
		MessageBox(NULL, L"Invalid self-exe file name!", AppName, MB_ICONERROR | MB_OK);
		return 1;
	}

	// Determine which browser should be started
	// First, extract hostname from the URL
	const wchar_t* hostStart = wcsstr(lpCmdLine, L"://");
	if (hostStart == NULL)
	{
		wsprintf(buf1, L"Invalid URL format:\n%s", lpCmdLine);
		MessageBox(NULL, buf1, AppName, MB_ICONERROR | MB_OK);
		return 1;
	}
	hostStart += 3;
	const wchar_t* hostEnd = wcsstr(hostStart, L"/");
	if (hostEnd == NULL)
		wcscpy_s(buf1, BUF_SIZE, hostStart);
	else
		wcsncpy_s(buf1, BUF_SIZE, hostStart, hostEnd - hostStart);
	// Now search for this hostname in our INI file, section [Hostnames]
	DWORD res = GetPrivateProfileString(L"Hostnames", buf1, NULL, buf2, BUF_SIZE, iniFile);
	if ((res == 0) || (buf2[0] == L'\0'))
	{
		// Not found, taking the defalut browser
		res = GetPrivateProfileString(L"General", L"DefaultBrowser", NULL, buf2, BUF_SIZE, iniFile);
		if ((res == 0) || (buf2[0] == L'\0'))
		{
			MessageBox(NULL, L"Failed to determine the browser!", AppName, MB_ICONERROR | MB_OK);
			return 1;
		}
	}
	// Apply hacks: replacing HTTPS with HTTP for specified sites
	const size_t MAX_PROTO_LEN = 16;
	wchar_t proto[MAX_PROTO_LEN];
	if ((wcsncmp(lpCmdLine, L"\"https", 5) == 0) && (GetPrivateProfileInt(L"AntiHttps", buf1, 0, iniFile) != 0))
		wcscpy_s(proto, L"http");
	else
		wcsncpy_s(proto, lpCmdLine + 1, hostStart - lpCmdLine - 4);
	// Get the executable for the chosen browser
	res = GetPrivateProfileString(buf2, L"Browser", NULL, buf1, BUF_SIZE, iniFile);
	if ((res == 0) || (buf1[0] == L'\0'))
	{
		MessageBox(NULL, L"Failed to determine the browser executable!", AppName, MB_ICONERROR | MB_OK);
		return 1;
	}

	// Run the browser
	PROCESS_INFORMATION pi = { 0 };
	STARTUPINFO si = { 0 };
	si.cb = sizeof(si);
	wsprintf(buf2, L"\"%s\" \"%s://%s", buf1, proto, hostStart);
	if (CreateProcess(NULL, buf2, NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi) == 0)
	{
		wsprintf(buf1, L"Failed to start editor (%d)", GetLastError());
		MessageBox(NULL, buf1, AppName, MB_ICONERROR | MB_OK);
		return 1;
	}
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return 0;
}
