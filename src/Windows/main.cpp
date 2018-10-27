
#include <iterator>
#include <vector>
#include <string>
#include <Windows.h>
#include <WindowsX.h>
#include <Commdlg.h>
#include <CommCtrl.h>
#include <sstream>
#include "FileGroup.h"
#include "functions.h"
#include "resource.h"
#include "utls/utility.h"
#include <Shlwapi.h>
#include "UI.h"

#pragma comment (lib, "Comctl32.lib")

using namespace std;


const POINT ClientWndSize = {1024, 640};



DWORD WINAPI    Thread          (PVOID pvoid);
DWORD WINAPI    ThreadHashOnly  (PVOID pvoid);
LRESULT CALLBACK  MainWndProc     (HWND, UINT, WPARAM, LPARAM);


ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = MainWndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON128));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCE(IDR_MENU2);
    wcex.lpszClassName  = szAppName;
    wcex.hIconSm        = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON16));

    return RegisterClassEx(&wcex);
}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    g_hInstance = hInstance;

    // 选择不同风格时似乎会影响listview和窗口的相对大小
    HWND hWnd = CreateWindow(szAppName, szAppName, WS_OVERLAPPEDWINDOW,
        200, 100, ClientWndSize.x, ClientWndSize.y, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, PSTR pCmdLine, int iCmdShow)
{
    //TCHAR	szBuffer[32];
    INITCOMMONCONTROLSEX	icce;

    // 初始化通用控件
    icce.dwSize	= sizeof(INITCOMMONCONTROLSEX);
    icce.dwICC  = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icce);
    
    // 线程事件
    g_ThreadSignal = CreateEvent(0, FALSE, FALSE, 0);

    g_hThread = CreateThread(0, 0, Thread, 0, 0, 0);
	

    MyRegisterClass(hInstance);

    MSG msg;
    // 执行应用程序初始化
    if (InitInstance (hInstance, iCmdShow))
    {
        // 加载右键菜单
        g_hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_MENU1));
        g_hMenu = GetSubMenu(g_hMenu, 0);
    
        // 主消息循环: 
        while (GetMessage(&msg, nullptr, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }    

    CloseHandle(g_hThread);
    CloseHandle(g_ThreadSignal);
    return (int) msg.wParam;
}


LRESULT CALLBACK MainWndProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_COMMAND:
        Cls_OnCommand_main(hDlg, (int)LOWORD(wParam), (HWND)lParam, (UINT)HIWORD(wParam));
        break;

    case WM_CLOSE:
        PostQuitMessage(0);
        break;

    case WM_SYSCOMMAND:
        Cls_OnSysCommand0(hDlg, (UINT)wParam, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));
        break;

	case WM_SIZE:
		Cls_OnSize(hDlg, (UINT)wParam, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));
		break;
        
    case WM_CREATE:
        Cls_OnCreate(hDlg, (LPCREATESTRUCT)lParam);
        break;
        
    case WM_NOTIFY: {
        BOOL bHandled = FALSE;
        LRESULT ret = Cls_OnListViewNotify(hDlg, wParam, lParam, bHandled);
        if (bHandled)
            return ret;
        break;
    }

    default:
        break;
    }

    return DefWindowProc(hDlg, Msg, wParam, lParam);
}


BOOL CALLBACK FilterDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        HANDLE_MSG(hDlg, WM_COMMAND,	 Cls_OnCommand_filter);
        HANDLE_MSG(hDlg, WM_CLOSE,		 Cls_OnClose);
        HANDLE_MSG(hDlg, WM_SYSCOMMAND,	 Cls_OnSysCommand);
        HANDLE_MSG(hDlg, WM_INITDIALOG,  Cls_OnInitDialog_filter);
        return TRUE;
    }
    return FALSE;
}




DWORD WINAPI Thread(PVOID pvoid)
{
	struct Data
	{
		wstring Cmd;
		wstring TempFile;
		HANDLE  hFinderProcess;
		HANDLE  hTempFile;
	};
	vector<Data> findData;


	vector<TCHAR> szTempPath(MAX_PATH);
	GetTempPath(MAX_PATH, szTempPath.data());

	SECURITY_ATTRIBUTES sa{ sizeof(sa), NULL, TRUE };

    while (1)
    {
		findData.clear();
        WaitForSingleObject(g_ThreadSignal, INFINITE);
        
		auto cmds = g_Filter.MakeSearchCommand();
		if (cmds.size() > 64)
		{
			MessageBox(NULL, L"最多添加 64 个目录", szAppName, MB_ICONWARNING);
			continue;
		}


        ListView_SetItemCount(g_hList, 0);
        g_DataBase.clear();


		findData.resize(cmds.size());
		for (size_t i = 0; i < cmds.size(); ++i)
			findData[i].Cmd = std::move(cmds[i]);

		vector<TCHAR> szTempFileName(MAX_PATH);

		for (size_t i = 0; i < findData.size(); ++i)
		{
			GetTempFileName(szTempPath.data(), szAppName, 0, szTempFileName.data());
			findData[i].TempFile = szTempFileName.data();
		}

		for (size_t i = 0; i < findData.size(); ++i)
		{
			findData[i].hTempFile = CreateFile(findData[i].TempFile.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

			vector<wchar_t> cmdLine;
			copy(findData[i].Cmd.begin(), findData[i].Cmd.end(), back_inserter(cmdLine));
			cmdLine.push_back(L'\0');
			STARTUPINFO si = { sizeof(STARTUPINFO) };
			si.hStdInput = INVALID_HANDLE_VALUE;
			si.hStdOutput = findData[i].hTempFile;
			si.hStdError = INVALID_HANDLE_VALUE;
			si.dwFlags = STARTF_USESTDHANDLES;
			PROCESS_INFORMATION pi = { 0 };
			if (!CreateProcess(NULL, cmdLine.data(), &sa, NULL, TRUE, 0, NULL, NULL, &si, &pi))
			{
				MessageBox(NULL, L"启动搜索失败", szAppName, MB_ICONERROR);
				break;
			}

			CloseHandle(pi.hThread);
			findData[i].hFinderProcess = pi.hProcess;
		}

		vector<HANDLE> hWaitFinder(findData.size());
		for (size_t i = 0; i < hWaitFinder.size(); ++i)
			hWaitFinder[i] = findData[i].hFinderProcess;

		DWORD dwWait = WaitForMultipleObjects(hWaitFinder.size(), hWaitFinder.data(), TRUE, INFINITE);
		if (dwWait != WAIT_OBJECT_0)
		{
			MessageBox(NULL, L"等待失败", szAppName, MB_ICONERROR);
			continue;
		}

		for (auto& d : findData)
		{
			CloseHandle(d.hFinderProcess);
			//CloseHandle(d.hTempFile);
		}


		// 合并
		vector<wchar_t> szMerge(MAX_PATH);
		GetTempFileName(szTempPath.data(), szAppName, 0, szMerge.data());
		HANDLE hMerge = CreateFile(szMerge.data(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		for (size_t i = 0; i < findData.size(); ++i)
		{
			DWORD size = GetFileSize(findData[i].hTempFile, NULL);
			vector<char> data(size);
			DWORD R, W;
			SetFilePointer(findData[i].hTempFile, 0, NULL, FILE_BEGIN);
			ReadFile(findData[i].hTempFile, data.data(), size, &R, NULL);
			WriteFile(hMerge, data.data(), size, &W, NULL);
			CloseHandle(findData[i].hTempFile);
			DeleteFile(findData[i].TempFile.c_str());
		}
		CloseHandle(hMerge);

		
		auto cmd2 = g_Filter.MakeCompareCommand();
		cmd2 += szMerge.data();
		vector<wchar_t> szGroup(MAX_PATH);
		GetTempFileName(szTempPath.data(), szAppName, 0, szGroup.data());
		HANDLE hGroup = CreateFile(szGroup.data(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		STARTUPINFO si = { sizeof(STARTUPINFO) };
		si.hStdInput = INVALID_HANDLE_VALUE;
		si.hStdOutput = hGroup;
		si.hStdError = INVALID_HANDLE_VALUE;
		si.dwFlags = STARTF_USESTDHANDLES;
		PROCESS_INFORMATION pi = { 0 };
		vector<wchar_t> cmdLine;
		copy(cmd2.begin(), cmd2.end(), back_inserter(cmdLine));
		cmdLine.push_back(L'\0');

		if (!CreateProcess(NULL, cmdLine.data(), &sa, NULL, TRUE, 0, NULL, NULL, &si, &pi))
		{
			MessageBox(NULL, L"启动合并失败", szAppName, MB_ICONERROR);
			break;
		}
		if (WAIT_OBJECT_0 != WaitForSingleObject(pi.hProcess, INFINITE))
		{
			MessageBox(NULL, L"等待失败2", szAppName, MB_ICONERROR);
			break;
		}
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		SetFilePointer(hGroup, 0, NULL, FILE_BEGIN);
		DWORD size = GetFileSize(hGroup, NULL);
		vector<char> data(size+1);
		DWORD R;
		ReadFile(hGroup, data.data(), size, &R, NULL);
		CloseHandle(hGroup);
		DeleteFile(szMerge.data());
		DeleteFile(szGroup.data());

		int n = MultiByteToWideChar(CP_UTF8, NULL, data.data(), -1, NULL, NULL);
		vector<wchar_t> wdata(n);
		int n2 = MultiByteToWideChar(CP_UTF8, NULL, data.data(), -1, wdata.data(), wdata.size());
		wdata.push_back(L'\0');

		auto groups = Utility::Splite(wdata.data(), L"*");

        g_DataBase2.clear();
		g_DataBase2.reserve(groups.size());
		for (size_t i = 0; i < groups.size(); ++i)
		{
			auto files = Utility::Splite(groups[i], L"\r\n");
			g_DataBase2.push_back(std::vector<FileRecord>());
			for (size_t k = 0; k < files.size(); ++k)
			{
                g_DataBase2[i].emplace_back();
                if (!FileRecord::FromUTF16(files[k], g_DataBase2[i].back()))
                    g_DataBase2[i].pop_back();
			}
		}

        for (size_t i = 0; i < g_DataBase2.size(); ++i)
        {
            sort(g_DataBase2[i].begin(), g_DataBase2[i].end(), [](const FileRecord& lhs, const FileRecord& rhs) {
                // 将最新的文件放到组内的第一个
                return lhs.mLastWriteTime > rhs.mLastWriteTime;
            });

            for (size_t k = 0; k < g_DataBase2[i].size(); ++k)
            {
                g_DataBase.push_back(DataBaseInfo(&g_DataBase2[i][k], k == 0));
                g_DataBase.back().mTextColor = k == 0 ? RGB(0, 0, 0) : RGB(128, 128, 128);
            }
        }

        MessageBox(NULL, L"查找结束", szAppName, MB_ICONINFORMATION);
		ListView_SetItemCount(g_hList, g_DataBase.size());
    }

    return 0;
}


DWORD WINAPI ThreadHashOnly(PVOID pvoid)
{
    return 0;
}