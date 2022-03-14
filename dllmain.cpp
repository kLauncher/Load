#include "pch.h"
#include <vector>
#include <string>
#include "Memory.h"
#include <ShlObj_core.h>

using namespace std;

typedef BOOL(WINAPI* LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

char LauncherPath[MAX_PATH];                      // 디렉토리 경로
//char myKDocumnet[MAX_PATH];

vector<string> parse(string a, char split) {
	vector<string > p;
	int bef = 0;
	for (unsigned int i = 0; i < a.size(); i++) {
		if (a.at(i) == split) {
			if (bef != i)
				p.push_back(a.substr(bef, i - bef));
			bef = i + 1;
		}
	}
	if (bef != a.size()) {
		p.push_back(a.substr(bef, a.size() - bef));
	}
	return p;
}

char* GetPathLauncher(char* pName) {
	Memory memory;
	int lPid = memory.GetProcessId(pName);
	HANDLE process_handle = OpenProcess(
		PROCESS_QUERY_LIMITED_INFORMATION,
		FALSE,
		lPid
	);
	static char buffer[MAX_PATH] = {};
	DWORD buffer_size = MAX_PATH;
	if (!QueryFullProcessImageName(process_handle, 0, buffer, &buffer_size)) {
		//printf("wLauncher 위치 가져오기 실패");
	}
	CloseHandle(process_handle);
	return buffer;
}

void FindLauncherPath() {
	char drive[MAX_PATH];               // 드라이브 명
	char dir[MAX_PATH];                      // 디렉토리 경로
	char fname[MAX_PATH];           // 파일명
	char ext[MAX_PATH];                    // 확장자 명	

	char* path = GetPathLauncher("kLauncher.exe");
	_splitpath_s(path, drive, dir, fname, ext);
	sprintf_s(LauncherPath, "%s%s", drive, dir);
}

string GetSetting() {
	char return_string[MAX_PATH];
	char dir_set[MAX_PATH];
	char* pszFileName = "Lib\\Load.ini";
	sprintf_s(dir_set, "%s%s", LauncherPath, pszFileName);
	GetPrivateProfileString("Moudle", "List", "", return_string, 256, dir_set);
	return return_string;
}

BOOL IsElevated() {
	BOOL fRet = FALSE;
	HANDLE hToken = NULL;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
		TOKEN_ELEVATION Elevation;
		DWORD cbSize = sizeof(TOKEN_ELEVATION);
		if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
			fRet = Elevation.TokenIsElevated;
		}
	}
	if (hToken) {
		CloseHandle(hToken);
	}
	return fRet;
}

BOOL IsWow64() {
	BOOL bIsWow64 = FALSE;
	LPFN_ISWOW64PROCESS fnIsWow64Process;

	//IsWow64Process is not available on all supported versions of Windows.
	//Use GetModuleHandle to get a handle to the DLL that contains the function
	//and GetProcAddress to get a pointer to the function if available.

	fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
		GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

	if (NULL != fnIsWow64Process) {
		if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64)) {
			//handle error
		}
	}
	return bIsWow64;
}

//void FindMydocumnetPath()
//{
//    SHGetSpecialFolderPath(NULL, myKDocumnet, CSIDL_PERSONAL, FALSE);
//    string pathTmp = myKDocumnet;
//    pathTmp.append("\\kLauncher");
//    strcpy_s(myKDocumnet, pathTmp.c_str());
//}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
) {
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
		{
			//FILE* stream;
			//AllocConsole();
			//freopen_s(&stream, "CONOUT$", "w", stdout); //freopen도 가능           
			DWORD pid = GetCurrentProcessId();
			HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
			bool err = false;
			//if (!IsElevated())
			//{
			//	err = true;
			//	MessageBox(NULL, "런처 실행 실패 (스타크래프트를 관리자 권한으로 실행 해주세요)", "알림", MB_ICONHAND);
			//	break;
			//}

			if (!err) {
				FindLauncherPath();
				//FindMydocumnetPath();
				string LoadingList = GetSetting();
				vector<string> SpiltLoadingList = parse(LoadingList, ',');
				if (!SpiltLoadingList.size())
					MessageBox(NULL, "Moudle 인식 불가 (프로그램을 재설치 해주세요)", "알림", MB_ICONHAND);
				for (unsigned int i = 0; i < SpiltLoadingList.size(); i++) {
					string path = LauncherPath;
					path.append("Lib\\");
					path += SpiltLoadingList[i];
					if (::LoadLibraryA(path.c_str()) == NULL) {
						string err = "LoadLibrary Error";
						err += GetLastError();
					}
				}
				string path = LauncherPath;
				path += "Lib\\kDetector.k";
				::LoadLibraryA(path.c_str());
			}
			break;
		}
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}

