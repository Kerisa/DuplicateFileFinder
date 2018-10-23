
#include <iterator>
#include <vector>
#include <string>
#include <Windows.h>
#include <WindowsX.h>
#include <Commdlg.h>
#include <CommCtrl.h>
#include "FileGroup.h"
#include "functions.h"
#include "resource.h"
#include "utls/utility.h"
#include <Shlwapi.h>

#pragma comment (lib, "Comctl32.lib")

using namespace std;

#define ID_STATUSBAR 1032
#define ID_HEADER    1030
#define ID_LISTVIEW  1031
#define MAX_PATH1	 1024

const wchar_t szAppName[] = L"DuplicateFileFinder";
const POINT ClientWndSize = {1024, 640};
const int StatusBarHigh = 10;

HMENU		g_hMenu;							 // 全局变量
HWND		g_hList, g_hStatus, g_hDlgFilter;
HINSTANCE	g_hInstance;
HANDLE      g_ThreadSignal, g_hThread;

Filter		g_Filter;

static int iClickItem;
static int StatusBarSetPart[2];

volatile bool bTerminateThread;

DWORD WINAPI    Thread          (PVOID pvoid);
DWORD WINAPI    ThreadHashOnly  (PVOID pvoid);
LRESULT CALLBACK  MainWndProc     (HWND, UINT, WPARAM, LPARAM);
BOOL  CALLBACK  FilterDialogProc(HWND, UINT, WPARAM, LPARAM);


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
    HWND hWnd = CreateWindow(szAppName, szAppName, WS_CAPTION | WS_MINIMIZEBOX | WS_POPUPWINDOW,
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


static bool Cls_OnCommand_main(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
    static bool bPause = false;
    static TCHAR staticBuf[MAX_PATH];
    wchar_t szBuffer[MAX_PATH1] = { 0 }, szBuffer1[MAX_PATH1] = { 0 };

    switch (id)
    {
    case IDM_OPENFILE:
    case IDM_DELETE:
        if (iClickItem >= g_DataBase.size() || iClickItem < 0)
            return TRUE;

        // 获取全路径
        ListView_GetItemText(g_hList, iClickItem, 1, szBuffer, _countof(szBuffer));
        StringCbCat(szBuffer, _countof(szBuffer), TEXT("\\"));
        ListView_GetItemText(g_hList, iClickItem, 0, szBuffer1, _countof(szBuffer1));
        StringCbCat(szBuffer, _countof(szBuffer), szBuffer1);

        if (id == IDM_OPENFILE)
            ShellExecute(0, 0, szBuffer, NULL, NULL, SW_SHOW);
        else
        {
            StringCbCopy(szBuffer1, _countof(szBuffer1), TEXT("确认删除"));
            StringCbCat(szBuffer1, _countof(szBuffer1), szBuffer);
            if (IDNO == MessageBox(hDlg, szBuffer1, TEXT("提示"),
                MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING))
                return TRUE;
            DeleteFile(szBuffer);
            if (!GetLastError())
            {
                ListView_DeleteItem(g_hList, iClickItem);
                g_DataBase.erase(g_DataBase.begin() + iClickItem);
                UpdateStatusBar(1, 0);
            }
            else
                MessageBox(hDlg, TEXT("无法删除文件，请手动删除"), TEXT("提示"), MB_ICONEXCLAMATION);
        }
        return TRUE;

    case IDM_EXPLORER:
        if (iClickItem < g_DataBase.size() && iClickItem >= 0)
        {
            ListView_GetItemText(g_hList, iClickItem, 1, szBuffer, _countof(szBuffer));
            ShellExecute(0, 0, szBuffer, 0, 0, SW_SHOW);
        }
        return TRUE;

    case IDM_DELETEALL:
    {
        if (IDNO == MessageBox(hDlg, TEXT("确认删除全部选中文件？"), TEXT("提示"),
            MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING))
            return TRUE;

        int delCnt = 0;
        for (size_t i = 0; i < g_DataBase.size(); ++i)
            if (g_DataBase[i].checkstate == CHECKBOX_SECLECTED)
            {
                DeleteFile(g_DataBase[i].fi->mPath.c_str());
                if (!GetLastError())
                {
                    g_DataBase.erase(g_DataBase.begin() + i);
                    ++delCnt;
                    // i保持不变
                    --i;
                }
                else
                {
                    StringCbPrintf(szBuffer1, _countof(szBuffer1),
                        TEXT("无法删除文件%s，请手动删除"), g_DataBase[i].fi->mPath.c_str());
                    UpdateStatusBar(0, szBuffer1);
                }
            }

        // 更新item数量
        ListView_SetItemCount(g_hList, g_DataBase.size());
        UpdateStatusBar(1, nullptr);
        InvalidateRect(hDlg, NULL, TRUE);

        StringCbPrintf(szBuffer1, _countof(szBuffer1),
            TEXT("%d个文件已删除"), delCnt);
        MessageBox(hDlg, szBuffer1, L"删除文件", MB_ICONINFORMATION);
        return TRUE;
    }

    case IDM_HASHALL:
        
        return TRUE;

    case IDM_SELALL:
    case IDM_USELALL:
        for (size_t i = 0; i < g_DataBase.size(); ++i)
            g_DataBase[i].checkstate = 
                id == IDM_SELALL ? CHECKBOX_SECLECTED : CHECKBOX_UNSECLECTED;
        UpdateStatusBar(1, 0);
        InvalidateRect(g_hList, NULL, TRUE);
        return TRUE;

    //----------------------------------------------------------------------------------------------------

    case IDB_SETFILTER:
    case ID__40025:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_FILTER), hDlg, FilterDialogProc);
        return TRUE;


    case IDB_START:
    case ID__40026:
        ListView_SetItemCount(g_hList, 0);

        bPause = false;
        bTerminateThread = false;
        SetEvent(g_ThreadSignal);
        
        return TRUE;

    case IDB_PAUSE:
    case ID__40027:
        if (!bPause)
        {
            SuspendThread(g_hThread);
            SetDlgItemText(hDlg, id, TEXT("继续(&R)"));
            SendMessage(g_hStatus, SB_GETTEXT, 0, (LPARAM)staticBuf);
            SendMessage(g_hStatus, SB_SETTEXT, 0, (LPARAM)TEXT("暂停"));
        }
        else
        {
            ResumeThread(g_hThread);
            SetDlgItemText(hDlg, id, TEXT("暂停(&P)"));
            SendMessage(g_hStatus, SB_SETTEXT, 0, (LPARAM)staticBuf);
        }
        bPause ^= 1;
        return TRUE;

    case IDB_STOP:
    case ID__40028:
        bTerminateThread = true;
        if (bPause)
            ResumeThread(g_hThread);
        SendMessage(g_hStatus, SB_SETTEXT, 0, (LPARAM)TEXT("用户终止"));
        return TRUE;
    }

    return FALSE;
}


static bool Cls_OnCreate(HWND hDlg, LPCREATESTRUCT lpCreateStruct)
{
    // 设置菜单
    SetMenu(hDlg, LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_MENU2)));

    // 初始化List View    大小在这里微调……
    g_hList = CreateWindowEx(0, WC_LISTVIEW, NULL,
                    LVS_REPORT | WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_ALIGNTOP | LVS_OWNERDATA,
                    0, 0, ClientWndSize.x-6, ClientWndSize.y-StatusBarHigh-61,
                    hDlg, (HMENU)ID_LISTVIEW, (HINSTANCE)GetWindowLong(hDlg, GWL_HINSTANCE), NULL);
    ListView_EnableGroupView(g_hList, TRUE);
    ListView_SetExtendedListViewStyleEx(g_hList, 0, LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
    if (!InitListViewColumns(g_hList))
        assert(0);
        

    // 初始化 Status Bar
    g_hStatus = CreateStatusWindow(WS_CHILD | WS_VISIBLE, NULL, hDlg, ID_STATUSBAR);
    StatusBarSetPart[0] = ClientWndSize.x * 4 / 5;
    StatusBarSetPart[1] = -1;
    SendMessage(g_hStatus, SB_SETPARTS, 2, (LPARAM)StatusBarSetPart);

    
    // 设置焦点
    SetFocus(GetDlgItem(hDlg, IDB_BROWSE));
    return false;
}


static void Cls_OnClose(HWND hwnd)
{
    EndDialog(hwnd, 0);
}


static void Cls_OnSysCommand0(HWND hwnd, UINT cmd, int x, int y)
{
    if (cmd == SC_CLOSE)
    {
        SendMessage(g_hDlgFilter, WM_SYSCOMMAND, SC_CLOSE, 0);
        PostQuitMessage(0);
    }
}


LRESULT CALLBACK MainWndProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    static int CurCtlIdx;
    
    LVHITTESTINFO lvh;

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
        
    case WM_CREATE:
        Cls_OnCreate(hDlg, (LPCREATESTRUCT)lParam);
        break;
        
    case WM_NOTIFY:
        if (((NMHDR*)lParam)->idFrom == ID_LISTVIEW)
        {
            switch (((NMHDR*)lParam)->code)
            {
            case LVN_KEYDOWN:
                switch (((LPNMLVKEYDOWN)lParam)->wVKey)
                {
                case VK_RETURN:
                    iClickItem = ListView_GetSelectionMark(g_hList);
                    SendMessage(hDlg, WM_COMMAND, IDM_OPENFILE, 0);
                    return TRUE;

                case VK_DELETE:
                    iClickItem = ListView_GetSelectionMark(g_hList);
                    SendMessage(hDlg, WM_COMMAND, IDM_DELETE, 0);
                    return TRUE;

                // 使用空格键切换checkbox
                case VK_SPACE:
                    iClickItem = ListView_GetSelectionMark(g_hList);
                    g_DataBase[iClickItem].checkstate ^= 0x3;
                    SendMessage(g_hList, LVM_REDRAWITEMS, iClickItem, iClickItem);
                    UpdateStatusBar(1, nullptr);
                    break;
                }
                break;

            case NM_CLICK:
                UpdateStatusBar(1, nullptr);
                break;

            case NM_RCLICK:
            case NM_DBLCLK:
                GetCursorPos(&lvh.pt);
                ScreenToClient(hDlg, &lvh.pt);
                iClickItem =  SendMessage(g_hList, LVM_SUBITEMHITTEST, 0, (LPARAM)&lvh);
                if (((NMHDR*)lParam)->code == NM_DBLCLK)
                {
                    SendMessage(hDlg, WM_COMMAND, IDM_OPENFILE, 0);
                }
                else
                {
                    GetCursorPos(&lvh.pt);
                    TrackPopupMenu(g_hMenu, TPM_RIGHTBUTTON, lvh.pt.x, lvh.pt.y, 0, hDlg, NULL);
                }
                break;
            case LVN_GETDISPINFO:
            {
                NMLVDISPINFO *pdi = reinterpret_cast<NMLVDISPINFO*>(lParam);
                LVITEMW *pItem = &pdi->item;

                int itemid = pItem->iItem;
                if (itemid >= g_DataBase.size())
                    break;

                pdi->item.state = INDEXTOSTATEIMAGEMASK(g_DataBase[itemid].checkstate);
                pdi->item.stateMask = LVIS_STATEIMAGEMASK;

                FileRecord* pfi = g_DataBase[itemid].fi;
                if (pItem->mask & LVIF_TEXT)
                {
                    TCHAR Buffer[128] = { 0 };

                    switch (pItem->iSubItem)
                    {
                    case 0: // name
                        wcscpy_s(pItem->pszText, pItem->cchTextMax, g_DataBase[itemid].fi->mPath.c_str());
                        break;

                    case 1: // path
                        wcscpy_s(pItem->pszText, pItem->cchTextMax, g_DataBase[itemid].fi->mPath.c_str());
                        break;

                    case 2: // size
                        if (g_DataBase[itemid].fi->mFileSize > 966367641ull)		// 约为0.9GB
                            StringCbPrintf(Buffer, 120, TEXT("%.2lf GB"), g_DataBase[itemid].fi->mFileSize / (double)0x40000000);
                        else if (g_DataBase[itemid].fi->mFileSize > 943718)	// 0.9MB
                            StringCbPrintf(Buffer, 120, TEXT("%.2lf MB"), g_DataBase[itemid].fi->mFileSize / (double)0x100000);
                        else if (g_DataBase[itemid].fi->mFileSize > 921)        // 0.9KB
                            StringCbPrintf(Buffer, 120, TEXT("%.2lf KB"), g_DataBase[itemid].fi->mFileSize / (double)0x400);
                        else
                            StringCbPrintf(Buffer, 120, TEXT("%d B"), g_DataBase[itemid].fi->mFileSize);

                        wcscpy_s(pItem->pszText, pItem->cchTextMax, Buffer);
                        break;

					case 3: {// create time
						SYSTEMTIME st;
						FILETIME tmp, ft;
						tmp.dwHighDateTime = (DWORD)(pfi->mLastWriteTime >> 32);
						tmp.dwLowDateTime = (DWORD)pfi->mLastWriteTime;
						FileTimeToLocalFileTime(&tmp, &ft);
						FileTimeToSystemTime(&ft, &st);
						StringCbPrintf(Buffer, 120, TEXT("%04d/%02d/%02d %02d:%02d:%02d"),
							st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

						wcscpy_s(pItem->pszText, pItem->cchTextMax, Buffer);
						break;
					}
					case 4: {// last write time
						SYSTEMTIME st;
						FILETIME tmp, ft;
						tmp.dwHighDateTime = (DWORD)(pfi->mLastWriteTime >> 32);
						tmp.dwLowDateTime = (DWORD)pfi->mLastWriteTime;
						FileTimeToLocalFileTime(&tmp, &ft);
						FileTimeToSystemTime(&ft, &st);
						StringCbPrintf(Buffer, 120, TEXT("%04d/%02d/%02d %02d:%02d:%02d"),
							st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

						wcscpy_s(pItem->pszText, pItem->cchTextMax, Buffer);
						break;
					}
                    case 5: // hash
                        wchar_t res[42];
                        static_cast<SHA_1*>(0)->ToHexString(
                            g_DataBase[itemid].hr.b, res, 42 * sizeof(wchar_t));

                        wcscpy_s(pItem->pszText, pItem->cchTextMax, res);
                        break;

                    default: break;
                    }
                }
            }
            }
        }
        break;

    default:
        break;
    }

    return DefWindowProc(hDlg, Msg, wParam, lParam);
}


static bool Cls_OnCommand_filter(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
        UpdateFilterConfig(hDlg, g_Filter);

    case IDCANCEL:
        EndDialog(hDlg, 0);
        return TRUE;

	case IDC_BUTTON_ADDDIR: {
		std::wstring Buffer = OpenFolder(hDlg);
		if (Buffer[0] &&
			LB_ERR == SendDlgItemMessage(hDlg, IDC_LIST_DIR, LB_FINDSTRINGEXACT, -1, (LPARAM)Buffer.c_str()))
		{
			if (!HasParentDirectoryInList(GetDlgItem(hDlg, IDC_LIST_DIR), Buffer))
			{
				int ans = 0;
				std::wstring detectChild(Buffer);
				if (!detectChild.empty() && detectChild.back() != L'\\')
					detectChild += L'\\';
				if (LB_ERR != SendDlgItemMessage(hDlg, IDC_LIST_DIR, LB_FINDSTRING, -1, (LPARAM)detectChild.c_str()))
				{
					ans = MessageBox(hDlg, L"列表中包含的子文件夹将被剔除", L"提示", MB_YESNO);
				}
				if (ans == IDYES)
				{
					int i;
					do
					{
						i = SendDlgItemMessage(hDlg, IDC_LIST_DIR, LB_FINDSTRING, -1, (LPARAM)detectChild.c_str());
						SendDlgItemMessage(hDlg, IDC_LIST_DIR, LB_DELETESTRING, i, 0);
					} while (i != LB_ERR);
				}

				if (ans != IDNO)
				{
					SendDlgItemMessage(hDlg, IDC_LIST_DIR, LB_ADDSTRING, 0, (LPARAM)Buffer.c_str());
				}
			}
			else
			{
				MessageBox(hDlg, L"列表中已存在相应的父目录", L"提示", MB_OK | MB_ICONINFORMATION);
			}
		}
		return TRUE;
	}

    case IDC_BUTTON_REMOVEDIR:
        SendDlgItemMessage(hDlg, IDC_LIST_DIR, LB_DELETESTRING, 
            (WPARAM)SendDlgItemMessage(hDlg, IDC_LIST_DIR, LB_GETCURSEL, 0, 0), 0);
        return TRUE;

    case IDC_CHECK_NAME:
    case IDC_CHECK_SIZE:
    case IDC_CHECK_DATE:
    case IDC_CHECK_DATA:
    case IDC_CHECK_SIZERANGE:
    case IDC_CHECK_ATTRIB:
	case IDC_CHECK_FILE_TYPE:
        EnableControls(hDlg, id, !!IsDlgButtonChecked(hDlg, id));
        return TRUE;

    case IDC_CHECK_ATTR_A:
    case IDC_CHECK_ATTR_R:
    case IDC_CHECK_ATTR_S:
    case IDC_CHECK_ATTR_H:
        if (IsDlgButtonChecked(hDlg, id))
        {
            CheckDlgButton(hDlg, IDC_CHECK_ATTR_N, false);
        }
        break;

    case IDC_CHECK_ATTR_N: 
        if (IsDlgButtonChecked(hDlg, id))
        {
            CheckDlgButton(hDlg, IDC_CHECK_ATTR_A, false);
            CheckDlgButton(hDlg, IDC_CHECK_ATTR_R, false);
            CheckDlgButton(hDlg, IDC_CHECK_ATTR_S, false);
            CheckDlgButton(hDlg, IDC_CHECK_ATTR_H, false);
        }
        break;

    case IDC_RADIO_FULLNAME:
        CheckDlgButton(hDlg, IDC_RADIO_INCNAME, false);
		EnableWindow(GetDlgItem(hDlg, IDC_CHECK_NOSUFFIX), TRUE);
		EnableWindow(GetDlgItem(hDlg, IDC_EDIT_NAMEINC), FALSE);
        break;

    case IDC_RADIO_INCNAME:
		CheckDlgButton(hDlg, IDC_RADIO_FULLNAME, false);
		EnableWindow(GetDlgItem(hDlg, IDC_CHECK_NOSUFFIX), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_EDIT_NAMEINC), TRUE);
        return TRUE;
    }

    return FALSE;
}


static bool Cls_OnInitDialog_filter(HWND hDlg, HWND hwndFocus, LPARAM lParam)
{
    HWND hCombo, hCombo1;
    TCHAR Buffer[MAX_PATH];

    g_hDlgFilter = hDlg;

    hCombo = GetDlgItem(hDlg, IDC_COMBO_DATAFRONT);
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"最前");
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"最后");

    //hCombo = GetDlgItem(hDlg, IDC_COMBO_DATAUNIT);
    //SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"B");
    //SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"KB");
    //SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"MB");
    //SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"GB");
    //SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"TB");
    //SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"PB");
    //SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"EB");

    //hCombo = GetDlgItem(hDlg, IDC_COMBO_SIZEUNIT);
    //SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"B");
    //SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"KB");
    //SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"MB");
    //SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"GB");
    //SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"TB");
    //SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"PB");
    //SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"EB");

    //hCombo = GetDlgItem(hDlg, IDC_COMBO_SUBDIR);
    //SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"所有子文件夹");
    //SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"当前文件夹");
    //for (int i=1; i<100; ++i)
    //{
    //    StringCchPrintf(Buffer, MAX_PATH, L"%d", i);
    //    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)Buffer);
    //}
    //SetDlgItemText(hDlg, IDC_COMBO_SUBDIR, L"所有子文件夹");

    hCombo = GetDlgItem(hDlg, IDC_COMBO_TYPEINC);
    hCombo1 = GetDlgItem(hDlg, IDC_COMBO_TYPEIGN);
    for (UINT i=IDS_STRING103; i<=IDS_STRING107; ++i)
    {
        LoadString(g_hInstance, i, Buffer, MAX_PATH);
        SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)Buffer);
        SendMessage(hCombo1, CB_ADDSTRING, 0, (LPARAM)Buffer);
    }

    hCombo = GetDlgItem(hDlg, IDC_COMBO_SEARCH_SIZE_UNIT1);
    hCombo1 = GetDlgItem(hDlg, IDC_COMBO_SEARCH_SIZE_UNIT2);
    for (UINT i = 0; i < _countof(szUnit); ++i)
    {
        SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)szUnit[i].first);
        SendMessage(hCombo1, CB_ADDSTRING, 0, (LPARAM)szUnit[i].first);
    }
    SendMessage(hCombo, CB_SETCURSEL, 0, 0);
    SendMessage(hCombo1, CB_SETCURSEL, 0, 0);

    SendMessage(hDlg, WM_COMMAND, IDC_CHECK_NAME, 0);
    SendMessage(hDlg, WM_COMMAND, IDC_CHECK_SIZE, 0);
    SendMessage(hDlg, WM_COMMAND, IDC_CHECK_DATE, 0);
    SendMessage(hDlg, WM_COMMAND, IDC_CHECK_DATA, 0);
    
	LoadFilterConfigToDialog(hDlg, g_Filter);
	
	CheckDlgButton(hDlg, IDC_RADIO_FULLNAME,  TRUE);
	CheckDlgButton(hDlg, IDC_RADIO_SIZEEQUAL, TRUE);
	CheckDlgButton(hDlg, IDC_RADIO_DATEEQUAL, TRUE);
	CheckDlgButton(hDlg, IDC_RADIO_DATAEQUAL, TRUE);
    
    return true;
}


static void Cls_OnSysCommand(HWND hwnd, UINT cmd, int x, int y)
{
    if (cmd == SC_CLOSE)
        EndDialog(hwnd, 0);
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


std::vector<std::wstring> MakeSearchCommand(Filter& filter)
{
	if (filter.mSearchDirectory.empty())
		return std::vector<std::wstring>();

	std::wstring cmd;
	if (filter.Switch[Filter::Search_Suffix] != Filter::Type_Off && !filter.mSuffix.empty())
	{
		if (filter.Switch[Filter::Search_Suffix] == Filter::Type_Include)
			cmd += L"-type +(";
		else
			cmd += L"-type -(";
		for (auto& s : filter.mSuffix)
		{
			cmd += s;
			cmd += L",";
		}
		cmd.pop_back();
		cmd += L") ";
	}

	if (filter.Switch[Filter::Search_FileSize] != Filter::Type_Off)
	{
		TCHAR buf[128];
		swprintf_s(buf, _countof(buf), L"-size (%lld:%lld) ", filter.FileSizeRangeLow, filter.FileSizeRangeHigh);
		cmd += buf;
	}

	if (filter.Switch[Filter::Search_FileAttribute] != Filter::Type_Off && filter.SelectedAttributes != 0)
	{
		std::wstring tmp;
		if (filter.SelectedAttributes & Filter::Attrib_Archive)
			tmp += L'a';
		if (filter.SelectedAttributes & Filter::Attrib_ReadOnly)
			tmp += L'r';
		if (filter.SelectedAttributes & Filter::Attrib_Hide)
			tmp += L'h';
		if (filter.SelectedAttributes & Filter::Attrib_System)
			tmp += L's';
        if (filter.SelectedAttributes & Filter::Attrib_Normal)
            tmp += L'n';

		if (filter.Switch[Filter::Search_FileAttribute] == Filter::Type_Include)
			cmd += L"-attr +(";
		else
			cmd += L"-attr -(";
		cmd += tmp;
		cmd += L") ";
	}
	
	std::vector<std::wstring> cmds;
	for (auto& it : filter.mSearchDirectory)
	{
		cmds.push_back(wstring(L"Finder.exe ") + it.first + L" " + cmd);
	}
	return cmds;
}


std::wstring MakeCompareCommand(Filter& filter)
{
	wstring cmd(L"DoubleChecker.exe ");
	if (filter.Switch[Filter::Compare_FileName] != Filter::Type_Off)
	{
		if (filter.Switch[Filter::Compare_FileName] == Filter::Type_Whole)
			cmd += L"-n ";
		else
			cmd += L"-ns " + filter.mKeyWordOfFileName;
	}

	if (filter.Switch[Filter::Compare_FileSize] != Filter::Type_Off)
		cmd += L"-s ";

	if (filter.Switch[Filter::Compare_FileDate] != Filter::Type_Off)
		cmd += L"-t ";

	if (filter.Switch[Filter::Compare_FileHash] != Filter::Type_Off)
		cmd += L"-c ";

	return cmd;
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
        
		auto cmds = MakeSearchCommand(g_Filter);
		if (cmds.size() > 64)
		{
			MessageBox(NULL, L"最多添加 64 个目录", szAppName, MB_ICONWARNING);
			continue;
		}

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

		
		auto cmd2 = MakeCompareCommand(g_Filter);
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
        g_DataBase.clear();
		g_DataBase2.reserve(groups.size());
		for (size_t i = 0; i < groups.size(); ++i)
		{
			auto files = Utility::Splite(groups[i], L"\r\n");
			g_DataBase2.push_back(std::vector<FileRecord>());
			for (size_t k = 0; k < files.size(); ++k)
			{
				g_DataBase2[i].emplace_back();
				FileRecord& fr = g_DataBase2[i].back();

				auto parts = Utility::Splite(files[k], L"|");
				fr.mPath = parts[0];
				fr.mSuffixOffset = PathFindExtension(parts[0].c_str()) - parts[0].c_str();
				if (fr.mSuffixOffset < parts[0].size())
					++fr.mSuffixOffset;
				fr.mNameOffset = PathFindFileName(parts[0].c_str()) - parts[0].c_str();

				fr.mFileSize = _wtoll(parts[1].c_str());
				fr.mLastWriteTime = _wtoll(parts[2].c_str());
			}
		}

        for (size_t i = 0; i < g_DataBase2.size(); ++i)
        {
            for (size_t k = 0; k < g_DataBase2[i].size(); ++k)
            {
                g_DataBase.push_back(DataBaseInfo(&g_DataBase2[i][k], SHA_1::__HashResult()));
            }
        }

        ListView_SetItemCount(g_hList, g_DataBase.size());
    }

    return 0;
}


DWORD WINAPI ThreadHashOnly(PVOID pvoid)
{
    return 0;
}