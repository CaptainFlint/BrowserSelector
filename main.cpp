#include <Windows.h>
#include <Winternl.h>

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

#pragma comment(lib, "ntdll.lib")

const wchar_t* AppName = L"Browser Selector";
const size_t BUF_SIZE = 32768;

extern "C" BOOLEAN WINAPI RtlIsNameInExpression(
	PUNICODE_STRING Expression,
	PUNICODE_STRING Name,
	BOOLEAN         IgnoreCase,
	PWCH            UpcaseTable
);
extern "C" NTSTATUS WINAPI RtlUpcaseUnicodeString(
	PUNICODE_STRING DestinationString,
	PCUNICODE_STRING SourceString,
	BOOLEAN AllocateDestinationString
);


bool Match(const wchar_t* HostName, const wchar_t* Mask)
{
	UNICODE_STRING uHostName, uMask, uMaskUC;
	RtlInitUnicodeString(&uHostName, HostName);
	RtlInitUnicodeString(&uMask, Mask);
	RtlUpcaseUnicodeString(&uMaskUC, &uMask, TRUE);
	BOOLEAN res = RtlIsNameInExpression(&uMaskUC, &uHostName, TRUE, NULL);
	RtlFreeUnicodeString(&uMaskUC);
	return (res != FALSE);
}

int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPWSTR lpCmdLine, int /*nCmdShow*/)
{
#if 0
	bool r;
	r = Match(L".tinkoff.ru", L"*.tinkoff.ru");
	MessageBox(NULL, r ? L"+" : L"-", L"", MB_OK);
	return 0;
#endif
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
	size_t hostLen;
	if (hostEnd == NULL)
		hostLen = wcslen(hostStart) - 1;
	else
		hostLen = hostEnd - hostStart;
	wcsncpy_s(buf1, BUF_SIZE, hostStart, hostLen);
	// Now search for this hostname in our INI file, section [Hostnames]
	wchar_t* allHostnames = new wchar_t[32767];
	if (GetPrivateProfileSection(L"Hostnames", allHostnames, 32767, iniFile) > 0)
	{
		wchar_t* hostmaskLine = allHostnames;
		bool found = false;
		// INI section lines are 0-byte-delimited, and the whole section ends with two 0-bytes
		while (*hostmaskLine != L'\0')
		{
			size_t len = wcslen(hostmaskLine);  // Length of the current line
			wchar_t* pos = wcschr(hostmaskLine, L'=');
			if (pos != NULL)
			{
				*pos = L'\0';
				if (Match(buf1, hostmaskLine))
				{
					// Found match
					found = true;
					wcscpy_s(buf2, BUF_SIZE, pos + 1);
					break;
				}
			}
			// Move to the next line
			hostmaskLine += len + 1;
		}
		if (!found)
		{
			// Not found, taking the defalut browser
			DWORD res = GetPrivateProfileString(L"General", L"DefaultBrowser", NULL, buf2, BUF_SIZE, iniFile);
			if ((res == 0) || (buf2[0] == L'\0'))
			{
				MessageBox(NULL, L"Failed to determine the browser!", AppName, MB_ICONERROR | MB_OK);
				return 1;
			}
		}
	}
	delete[] allHostnames;
	// Apply hacks: replacing HTTPS with HTTP for specified sites
	const size_t MAX_PROTO_LEN = 16;
	wchar_t proto[MAX_PROTO_LEN];
	if ((wcsncmp(lpCmdLine, L"\"https", 5) == 0) && (GetPrivateProfileInt(L"AntiHttps", buf1, 0, iniFile) != 0))
		wcscpy_s(proto, L"http");
	else
		wcsncpy_s(proto, lpCmdLine + 1, hostStart - lpCmdLine - 4);
	// Get the executable for the chosen browser
	DWORD res = GetPrivateProfileString(buf2, L"Browser", NULL, buf1, BUF_SIZE, iniFile);
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
