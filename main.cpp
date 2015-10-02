#include <Windows.h>

const wchar_t* AppName = L"Browser Selector";
const size_t MAX_PATH_EX = 32768;

int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPWSTR lpCmdLine, int /*nCmdShow*/)
{
	if (lpCmdLine[0] == L'\0')
	{
		MessageBox(NULL, L"Usage: BrowserSelector.exe <URL>", AppName, MB_ICONINFORMATION | MB_OK);
		return 1;
	}

	wchar_t* buf1 = new wchar_t[MAX_PATH_EX];
	wchar_t* buf2 = new wchar_t[MAX_PATH_EX];
	wchar_t* iniFile = new wchar_t[MAX_PATH_EX];

	// Get path to our own executable
	size_t pathLen = GetModuleFileName(NULL, iniFile, MAX_PATH_EX);
	if (pathLen == 0)
	{
		MessageBox(NULL, L"Failed to obtain self-exe location!", AppName, MB_ICONERROR | MB_OK);
		return 1;
	}

	// Construct INI file path
	if (wcsicmp(iniFile + pathLen - 4, L".exe") == 0)
		wcscpy(iniFile + pathLen - 3, L"ini");
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
		wcscpy(buf1, hostStart);
	else
		wcsncpy(buf1, hostStart, hostEnd - hostStart);
	// Now search for this hostname in our INI file, section [Hostnames]
	DWORD res = GetPrivateProfileString(L"Hostnames", buf1, NULL, buf2, MAX_PATH_EX, iniFile);
	if ((res == 0) || (buf2[0] == L'\0'))
	{
		// Not found, taking the defalut browser
		res = GetPrivateProfileString(L"General", L"DefaultBrowser", NULL, buf2, MAX_PATH_EX, iniFile);
		if ((res == 0) || (buf2[0] == L'\0'))
		{
			MessageBox(NULL, L"Failed to determine the browser!", AppName, MB_ICONERROR | MB_OK);
			return 1;
		}
	}
	// Get the executable for the chosen browser
	res = GetPrivateProfileString(buf2, L"Browser", NULL, buf1, MAX_PATH_EX, iniFile);
	if ((res == 0) || (buf1[0] == L'\0'))
	{
		MessageBox(NULL, L"Failed to determine the browser executable!", AppName, MB_ICONERROR | MB_OK);
		return 1;
	}

	// Run the browser
	PROCESS_INFORMATION pi = { 0 };
	STARTUPINFO si = { 0 };
	si.cb = sizeof(si);
	wsprintf(buf2, L"\"%s\" %s", buf1, lpCmdLine);
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
