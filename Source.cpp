#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "rpcrt4")
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "wininet")
#pragma comment(lib, "comctl32")

#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <wininet.h>
#include <vector>
#include <string>

#define IDC_EDIT		1000
#define IDC_PROGRESS	1001

HWND hMainWnd;
BOOL bAbort;

void DropData(HWND hWnd, IDataObject *pDataObject)
{
	FORMATETC fmtetc = { CF_UNICODETEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stgmed;
	if (pDataObject->QueryGetData(&fmtetc) == S_OK) {
		if (pDataObject->GetData(&fmtetc, &stgmed) == S_OK) {
			PVOID data = GlobalLock(stgmed.hGlobal);
			SetDlgItemTextW(hWnd, IDC_EDIT, (LPWSTR)data);
			SendDlgItemMessage(hWnd, IDC_EDIT, EM_SETSEL, 0, -1);
			GlobalUnlock(stgmed.hGlobal);
			ReleaseStgMedium(&stgmed);
		}
	}
}

class CDropTarget : public IDropTarget
{
public:
	HRESULT __stdcall QueryInterface(REFIID iid, void ** ppvObject) {
		if (iid == IID_IDropTarget || iid == IID_IUnknown) {
			AddRef();
			*ppvObject = this;
			return S_OK;
		}
		else {
			*ppvObject = 0;
			return E_NOINTERFACE;
		}
	}
	ULONG	__stdcall AddRef(void) {
		return InterlockedIncrement(&m_lRefCount);
	}
	ULONG	__stdcall Release(void) {
		LONG count = InterlockedDecrement(&m_lRefCount);
		if (count == 0) {
			delete this;
			return 0;
		}
		else {
			return count;
		}
	}
	HRESULT __stdcall DragEnter(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect) {
		m_fAllowDrop = QueryDataObject(pDataObject);
		if (m_fAllowDrop) {
			*pdwEffect = DropEffect(grfKeyState, pt, *pdwEffect);
			SetFocus(m_hWnd);
		}
		else {
			*pdwEffect = DROPEFFECT_NONE;
		}
		return S_OK;
	}
	HRESULT __stdcall DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect) {
		if (m_fAllowDrop) {
			*pdwEffect = DropEffect(grfKeyState, pt, *pdwEffect);
		}
		else {
			*pdwEffect = DROPEFFECT_NONE;
		}
		return S_OK;
	}
	HRESULT __stdcall DragLeave(void) {
		return S_OK;
	}
	HRESULT __stdcall Drop(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect) {
		if (m_fAllowDrop) {
			DropData(m_hWnd, pDataObject);
			*pdwEffect = DropEffect(grfKeyState, pt, *pdwEffect);
		}
		else {
			*pdwEffect = DROPEFFECT_NONE;
		}
		return S_OK;
	}
	CDropTarget(HWND hwnd) {
		m_lRefCount = 1;
		m_hWnd = hwnd;
		m_fAllowDrop = false;
	}
	~CDropTarget() {}
private:
	DWORD DropEffect(DWORD grfKeyState, POINTL pt, DWORD dwAllowed) {
		DWORD dwEffect = 0;
		if (grfKeyState & MK_CONTROL) {
			dwEffect = dwAllowed & DROPEFFECT_COPY;
		}
		else if (grfKeyState & MK_SHIFT) {
			dwEffect = dwAllowed & DROPEFFECT_MOVE;
		}
		if (dwEffect == 0) {
			if (dwAllowed & DROPEFFECT_COPY) dwEffect = DROPEFFECT_COPY;
			if (dwAllowed & DROPEFFECT_MOVE) dwEffect = DROPEFFECT_MOVE;
		}
		return dwEffect;
	}
	BOOL QueryDataObject(IDataObject *pDataObject) {
		FORMATETC fmtetc = { CF_UNICODETEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		return pDataObject->QueryGetData(&fmtetc) == S_OK;
	}
	LONG	m_lRefCount;
	HWND	m_hWnd;
	BOOL    m_fAllowDrop;
	IDataObject *m_pDataObject;
};

LPBYTE DownloadToMemory(IN LPCWSTR lpszURL)
{
	LPBYTE lpszReturn = 0;
	DWORD dwSize = 0;
	const HINTERNET hSession = InternetOpenW(L"Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/38.0.2125.111 Safari/537.36", INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, INTERNET_FLAG_NO_COOKIES);
	if (hSession)
	{
		URL_COMPONENTSW uc = { 0 };
		WCHAR HostName[MAX_PATH];
		WCHAR UrlPath[MAX_PATH];
		uc.dwStructSize = sizeof(uc);
		uc.lpszHostName = HostName;
		uc.lpszUrlPath = UrlPath;
		uc.dwHostNameLength = MAX_PATH;
		uc.dwUrlPathLength = MAX_PATH;
		InternetCrackUrlW(lpszURL, 0, 0, &uc);
		const HINTERNET hConnection = InternetConnectW(hSession, HostName, INTERNET_DEFAULT_HTTP_PORT, 0, 0, INTERNET_SERVICE_HTTP, 0, 0);
		if (hConnection)
		{
			const HINTERNET hRequest = HttpOpenRequestW(hConnection, L"GET", UrlPath, 0, 0, 0, 0, 0);
			if (hRequest)
			{
				ZeroMemory(&uc, sizeof(URL_COMPONENTS));
				WCHAR Scheme[16];
				uc.dwStructSize = sizeof(uc);
				uc.lpszScheme = Scheme;
				uc.lpszHostName = HostName;
				uc.dwSchemeLength = 16;
				uc.dwHostNameLength = MAX_PATH;
				InternetCrackUrlW(lpszURL, 0, 0, &uc);
				WCHAR szReferer[1024];
				lstrcpyW(szReferer, L"Referer: ");
				lstrcatW(szReferer, Scheme);
				lstrcatW(szReferer, L"://");
				lstrcatW(szReferer, HostName);
				HttpAddRequestHeadersW(hRequest, szReferer, lstrlenW(szReferer), HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD);
				HttpSendRequestW(hRequest, 0, 0, 0, 0);
				lpszReturn = (LPBYTE)GlobalAlloc(GMEM_FIXED, 1);
				DWORD dwRead;
				BYTE szBuf[1024 * 16];
				LPBYTE lpTmp;
				for (;;)
				{
					if (!InternetReadFile(hRequest, szBuf, (DWORD)sizeof(szBuf), &dwRead) || !dwRead) break;
					lpTmp = (LPBYTE)GlobalReAlloc(lpszReturn, (SIZE_T)(dwSize + dwRead), GMEM_MOVEABLE);
					if (lpTmp == NULL) break;
					lpszReturn = lpTmp;
					CopyMemory(lpszReturn + dwSize, szBuf, dwRead);
					dwSize += dwRead;
				}
				InternetCloseHandle(hRequest);
			}
			InternetCloseHandle(hConnection);
		}
		InternetCloseHandle(hSession);
	}
	return lpszReturn;
}

BOOL DownloadToFile(IN LPCWSTR lpszURL, IN LPCWSTR lpszFilePath)
{
	BOOL bReturn = FALSE;
	LPBYTE lpByte = DownloadToMemory(lpszURL);
	if (lpByte) {
		const DWORD nSize = (DWORD)GlobalSize(lpByte);
		const HANDLE hFile = CreateFileW(lpszFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE) {
			DWORD dwWritten;
			WriteFile(hFile, lpByte, nSize, &dwWritten, NULL);
			CloseHandle(hFile);
			bReturn = TRUE;
		}
		GlobalFree(lpByte);
	}
	return bReturn;
}

BOOL CreateTempDirectory(LPWSTR pszDir)
{
	DWORD dwSize = GetTempPath(0, 0);
	if (dwSize == 0 || dwSize > MAX_PATH - 14) { goto END0; }
	LPWSTR pTmpPath = (LPWSTR)GlobalAlloc(GPTR, sizeof(WCHAR)*(dwSize + 1));
	GetTempPathW(dwSize + 1, pTmpPath);
	dwSize = GetTempFileNameW(pTmpPath, L"", 0, pszDir);
	GlobalFree(pTmpPath);
	if (dwSize == 0) { goto END0; }
	DeleteFileW(pszDir);
	if (CreateDirectoryW(pszDir, 0) == 0) { goto END0; }
	return TRUE;
END0:
	return FALSE;
}

VOID MyCreateFileFromResource(WCHAR *szResourceName, WCHAR *szResourceType, WCHAR *szResFileName)
{
	DWORD dwWritten;
	HRSRC hRs = FindResourceW(0, szResourceName, szResourceType);
	DWORD dwResSize = SizeofResource(0, hRs);
	HGLOBAL hMem = LoadResource(0, hRs);
	LPBYTE lpByte = (BYTE *)LockResource(hMem);
	HANDLE hFile = CreateFileW(szResFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	WriteFile(hFile, lpByte, dwResSize, &dwWritten, NULL);
	CloseHandle(hFile);
}

std::wstring Replace(std::wstring String1, std::wstring String2, std::wstring String3)
{
	std::wstring::size_type Pos(String1.find(String2));
	while (Pos != std::wstring::npos) {
		String1.replace(Pos, String2.length(), String3);
		Pos = String1.find(String2, Pos + String3.length());
	}
	return String1;
}

LPWSTR Download2WChar(LPCWSTR lpszURL)
{
	LPWSTR lpszReturn = 0;
	LPBYTE lpByte = DownloadToMemory(lpszURL);
	if (lpByte) {
		const DWORD len = MultiByteToWideChar(CP_UTF8, 0, (LPCCH)lpByte, -1, 0, 0);
		lpszReturn = (LPWSTR)GlobalAlloc(0, (len + 1) * sizeof(WCHAR));
		if (lpszReturn) {
			MultiByteToWideChar(CP_UTF8, 0, (LPCCH)lpByte, -1, lpszReturn, len);
		}
		GlobalFree(lpByte);
	}
	return lpszReturn;
}

BOOL DownloadTumblrVideo(LPCWSTR lpszURL)
{
	// 入力される URL 例1: https://www.tumblr.com/video/kenjinote/165813628414/700/
	// 例2: http://kenjinote.tumblr.com/post/165813628414/

	// URL から UserName, TweetID を取得し、ダウンロードファイル名を決定する
	WCHAR szUserName[256] = { 0 };
	WCHAR szTweetID[256] = { 0 };
	WCHAR szTargetFilePath[MAX_PATH] = { 0 };
	std::wstring url(lpszURL);
	{
		if (url.find(L"/post/") != std::wstring::npos)
		{
			size_t posStart = url.find(L"://");
			if (posStart == std::wstring::npos) return FALSE;
			posStart += 3;
			size_t posEnd = url.find(L"/post/", posStart);
			if (posEnd == std::wstring::npos) return FALSE;
			std::wstring strUserName(url, posStart, posEnd - posStart);
			lstrcpyW(szUserName, strUserName.c_str());
			posStart = posEnd + 6;
			lstrcpyW(szTweetID, std::to_wstring(std::stoull(std::wstring(url, posStart))).c_str());
		} else if (url.find(L"://www.tumblr.com/video/")) {
			size_t posStart = url.find(L"/video/");
			if (posStart == std::wstring::npos) return FALSE;
			posStart += 7;
			size_t posEnd = url.find(L"/", posStart);
			if (posEnd == std::wstring::npos) return FALSE;
			std::wstring strUserName(url, posStart, posEnd - posStart);
			lstrcpyW(szUserName, strUserName.c_str());
			posStart = posEnd + 1;
			lstrcpyW(szTweetID, std::to_wstring(std::stoull(std::wstring(url, posStart))).c_str());
		}
		if (szUserName[0] == 0 || szTweetID[0] == 0) {
			return FALSE;
		}
		lstrcatW(szTargetFilePath, szUserName);
		lstrcatW(szTargetFilePath, L"_");
		lstrcatW(szTargetFilePath, szTweetID);
		lstrcatW(szTargetFilePath, L".mp4");
	}
	// 既にダウンロード済なら抜ける
	if (PathFileExistsW(szTargetFilePath))
	{
		return FALSE;
	}
	// HTML から動画ファイルを取得
	LPWSTR lpszWeb;
	std::wstring srcW;
	size_t posStart, posEnd;
	if (url.find(L"://www.tumblr.com/video/") == std::wstring::npos)
	{
		lpszWeb = Download2WChar(lpszURL);
		if (!lpszWeb)
		{
			return 0;
		}
		srcW = std::wstring(lpszWeb);
		GlobalFree(lpszWeb);
		posStart = srcW.find(L"<iframe src='https://www.tumblr.com/video/");
		if (posStart == std::wstring::npos)
		{
			return FALSE;
		}
		posStart += 13;
		posEnd = srcW.find(L'\'', posStart);
		url = std::wstring(srcW, posStart, posEnd - posStart);
	}
	if (url.find(L"://www.tumblr.com/video/") == std::wstring::npos)
	{
		return FALSE;
	}
	lpszWeb = Download2WChar(url.c_str());
	if (!lpszWeb) return 0;
	srcW = std::wstring(lpszWeb);
	GlobalFree(lpszWeb);
	//<source src="https://kenjinote.tumblr.com/video_file/t:xxxxxxx-xx_xxxxxxxxxxx/000000000000/tumblr_nsnva7Qh5b1swsrod" type="video/mp4"> を探す
	posStart = srcW.find(L"<source src=\"");
	if (posStart == std::wstring::npos)
	{
		return FALSE;
	}
	posStart += 13;
	posEnd = srcW.find(L'\"', posStart);
	if (posEnd == std::wstring::npos)
	{
		return FALSE;
	}
	return DownloadToFile(std::wstring(srcW, posStart, posEnd - posStart).c_str(), szTargetFilePath);
}

BOOL GetVideoUrlListFromHtmlSource(std::vector<std::wstring> * urllist, LPCWSTR lpszWeb)
{
	if (!lpszWeb) return 0;
	std::wstring srcW(lpszWeb);
	size_t posStart = 0, posEnd = 0;
	for (;;) {
		posStart = srcW.find(L"<iframe src='https://www.tumblr.com/video/", posStart);
		if (posStart == std::wstring::npos) break;
		posStart += 13;
		posEnd = srcW.find(L'\'', posStart);
		if (posEnd == std::wstring::npos) break;
		urllist->push_back(std::wstring(srcW, posStart, posEnd - posStart).c_str());
		posStart = posEnd;
	}
	return TRUE;
}

BOOL GetVideoUrlListFromFile(std::vector<std::wstring> * urllist, LPCWSTR lpszFilePath)
{
	if (!lpszFilePath) return 0;
	const HANDLE hFile = CreateFileW(lpszFilePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) return 0;
	DWORD nSize = GetFileSize(hFile, 0);
	if (!nSize) {
		CloseHandle(hFile);
		return 0;
	}
	LPBYTE lpByte = (LPBYTE)GlobalAlloc(0, nSize + 1);
	if (!lpByte) {
		CloseHandle(hFile);
		return 0;
	}
	DWORD dwRead = 0;
	ReadFile(hFile, lpByte, nSize, &dwRead, NULL);
	CloseHandle(hFile);
	lpByte[nSize] = 0;
	const DWORD len = MultiByteToWideChar(CP_UTF8, 0, (LPCCH)lpByte, -1, 0, 0);
	LPWSTR lpszWeb = (LPWSTR)GlobalAlloc(0, (len + 1) * sizeof(WCHAR));
	if (!lpszWeb) {
		GlobalFree(lpByte);
		return 0;
	}
	MultiByteToWideChar(CP_UTF8, 0, (LPCCH)lpByte, -1, lpszWeb, len);
	GlobalFree(lpByte);
	BOOL bRet = GetVideoUrlListFromHtmlSource(urllist, lpszWeb);
	GlobalFree(lpszWeb);
	return bRet;
}

BOOL GetVideoUrlListFromID(std::vector<std::wstring> * urllist, LPCWSTR lpszUserID)
{
	WCHAR szUrl[1024] = { 0 };
	if (StrStrW(lpszUserID, L".tumblr.com") == NULL) {
		lstrcatW(szUrl, L"https://");
		lstrcatW(szUrl, lpszUserID);
		{
			LPWSTR lpszWeb = Download2WChar(szUrl);
			if (!lpszWeb) return 0;
			BOOL bRet = GetVideoUrlListFromHtmlSource(urllist, lpszWeb);
			GlobalFree(lpszWeb);
		}
		lstrcatW(szUrl, L".tumblr.com/");
		{
			LPWSTR lpszWeb = Download2WChar(szUrl);
			if (!lpszWeb) return 0;
			BOOL bRet = GetVideoUrlListFromHtmlSource(urllist, lpszWeb);
			GlobalFree(lpszWeb);
		}
	}
	return TRUE;
}

VOID GetVideoUrlList(std::vector<std::wstring> * urllist, LPCWSTR lpszInput)
{
	if (PathFileExistsW(lpszInput)) {
		GetVideoUrlListFromFile(urllist, lpszInput);
	}
	else if (StrStrW(lpszInput, L"/post/")) {
		urllist->push_back(lpszInput);
	}
	else {
		GetVideoUrlListFromID(urllist, lpszInput);
	}
}

DWORD WINAPI ThreadFunc(LPVOID lpV)
{
	std::vector<std::wstring> * urllist = (std::vector<std::wstring> *)lpV;
	HWND hProgress = GetDlgItem(hMainWnd, IDC_PROGRESS);
	const int nSize = (int)urllist->size();
	SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, nSize));
	SendMessage(hProgress, PBM_SETSTEP, 1, 0);
	SendMessage(hProgress, PBM_SETPOS, 0, 0);
	for (int i = 0; i < nSize && bAbort == FALSE; ++i) {
		DownloadTumblrVideo(urllist->at(i).c_str());
		SendMessage(hProgress, PBM_STEPIT, 0, 0);
	}
	PostMessage(hMainWnd, WM_APP, 0, 0);
	return 0;
}

void RegisterDropWindow(HWND hwnd, IDropTarget **ppDropTarget)
{
	CDropTarget *pDropTarget = new CDropTarget(hwnd);
	CoLockObjectExternal(pDropTarget, TRUE, FALSE);
	RegisterDragDrop(hwnd, pDropTarget);
	*ppDropTarget = pDropTarget;
}

void UnregisterDropWindow(HWND hwnd, IDropTarget *pDropTarget)
{
	RevokeDragDrop(hwnd);
	CoLockObjectExternal(pDropTarget, FALSE, TRUE);
	pDropTarget->Release();
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static IDropTarget *pDropTarget;
	static HWND hEdit, hButton1, hButton2, hProgress;
	static HANDLE hThread;
	static std::vector<std::wstring> urllist;
	switch (msg) {
	case WM_CREATE:
		InitCommonControls();
		hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, (HMENU)IDC_EDIT, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessage(hEdit, EM_SETCUEBANNER, TRUE, (LPARAM)TEXT("http://kenjinote.tumblr.com/post/165816695691"));
		hButton1 = CreateWindow(TEXT("BUTTON"), TEXT("ダウンロード開始"), WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDOK, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		hButton2 = CreateWindow(TEXT("BUTTON"), TEXT("停止"), WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_DISABLED, 0, 0, 0, 0, hWnd, (HMENU)IDCANCEL, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		hProgress = CreateWindow(TEXT("msctls_progress32"), 0, WS_VISIBLE | WS_CHILD | PBS_SMOOTH, 0, 0, 0, 0, hWnd, (HMENU)IDC_PROGRESS, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		RegisterDropWindow(hWnd, &pDropTarget);
		DragAcceptFiles(hWnd, TRUE);
		break;
	case WM_SIZE:
		MoveWindow(hEdit, 10, 10, LOWORD(lParam) - 20, 32, TRUE);
		MoveWindow(hButton1, 10, 50, 256, 32, TRUE);
		MoveWindow(hButton2, 276, 50, 256, 32, TRUE);
		MoveWindow(hProgress, 10, 90, LOWORD(lParam) - 20, 32, TRUE);
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK) {
			if (GetWindowTextLength(hEdit) == 0) return 0;
			EnableWindow(hEdit, FALSE);
			EnableWindow(hButton1, FALSE);
			urllist.clear();
			WCHAR szInput[MAX_PATH];
			GetWindowTextW(hEdit, szInput, _countof(szInput));
			PathUnquoteSpacesW(szInput);
			GetVideoUrlList(&urllist, szInput);
			bAbort = FALSE;
			hThread = CreateThread(0, 0, ThreadFunc, (LPVOID)&urllist, 0, 0);
			EnableWindow(hButton2, TRUE);
			SetFocus(hButton2);
		}
		else if (LOWORD(wParam) == IDCANCEL) {
			EnableWindow(hButton2, FALSE);
			bAbort = TRUE;
		}
		break;
	case WM_APP:
		EnableWindow(hButton2, FALSE);
		WaitForSingleObject(hThread, INFINITE);
		CloseHandle(hThread);
		hThread = 0;
		urllist.clear();
		SendMessage(hProgress, PBM_SETPOS, 0, 0);
		EnableWindow(hEdit, TRUE);
		EnableWindow(hButton1, TRUE);
		SetFocus(hEdit);
		break;
	case WM_DROPFILES:
		{
			UINT uFileNum = DragQueryFileW((HDROP)wParam, -1, NULL, 0);
			if (uFileNum == 1)
			{
				WCHAR szFilePath[MAX_PATH];
				DragQueryFileW((HDROP)wParam, 0, szFilePath, MAX_PATH);
				SetWindowText(hEdit, szFilePath);
			}
			DragFinish((HDROP)wParam);
		}
		break;
	case WM_CLOSE:
		UnregisterDropWindow(hWnd, pDropTarget);
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefDlgProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	int n;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &n);
	if (n == 1) {
		OleInitialize(0);
		TCHAR szClassName[] = TEXT("Window");
		MSG msg;
		WNDCLASS wndclass = {
			CS_HREDRAW | CS_VREDRAW,
			WndProc,
			0,
			DLGWINDOWEXTRA,
			hInstance,
			0,
			LoadCursor(0,IDC_ARROW),
			0,
			0,
			szClassName
		};
		RegisterClass(&wndclass);
		hMainWnd = CreateWindow(
			szClassName,
			TEXT("Tumblrのツイートに含まれる動画をMP4形式でダウンロードする"),
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			0,
			CW_USEDEFAULT,
			0,
			0,
			0,
			hInstance,
			0
		);
		ShowWindow(hMainWnd, SW_SHOWDEFAULT);
		UpdateWindow(hMainWnd);
		while (GetMessage(&msg, 0, 0, 0)) {
			if (!IsDialogMessage(hMainWnd, &msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		OleUninitialize();
	}
	else if (n == 2) {
		std::vector<std::wstring> urllist;
		GetVideoUrlList(&urllist, argv[1]);
		HANDLE hThread = CreateThread(0, 0, ThreadFunc, (LPVOID)&urllist, 0, 0);
		WaitForSingleObject(hThread, INFINITE);
		CloseHandle(hThread);
		hThread = 0;
	}
	LocalFree(argv);
	return 0;
}
