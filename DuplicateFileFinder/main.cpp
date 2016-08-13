
#include <Windows.h>
#include <WindowsX.h>
#include <Commdlg.h>
#include <CommCtrl.h>
#include "FileGroup.h"
#include "functions.h"
#include "resource.h"

#pragma comment (lib, "Comctl32.lib")

#define ID_STATUSBAR 1032
#define ID_HEADER    1030
#define ID_LISTVIEW  1031
#define MAX_PATH1	 1024

const wchar_t szAppName[] = L"DuplicateFileFinder";
const POINT ClientWndSize = {1024, 640};
const int StatusBarHigh = 10;

HMENU		g_hMenu;							 // ȫ�ֱ���
HWND		g_hList, g_hStatus, g_hDlgFilter;
HINSTANCE	g_hInstance;
HANDLE      g_ThreadSignal, g_hThread;

FileGroup   g_FileGroup, *g_pFileGroup;
FileGroupS  g_FileGroupS;
FileGroupN  g_FileGroupN;
FileGroupSN g_FileGroupSN;


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

    // ѡ��ͬ���ʱ�ƺ���Ӱ��listview�ʹ��ڵ���Դ�С
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

    // ��ʼ��ͨ�ÿؼ�
    icce.dwSize	= sizeof(INITCOMMONCONTROLSEX);
    icce.dwICC  = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icce);
    
    // �߳��¼�
    g_ThreadSignal = CreateEvent(0, FALSE, FALSE, 0);

    g_hThread = CreateThread(0, 0, Thread, 0, 0, 0);

    // ���ݳ�ʼ������ָ��ָ�����FileGroup
    g_pFileGroup = &g_FileGroup;


    MyRegisterClass(hInstance);

    MSG msg;
    // ִ��Ӧ�ó����ʼ��
    if (InitInstance (hInstance, iCmdShow))
    {
        // �����Ҽ��˵�
        g_hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_MENU1));
        g_hMenu = GetSubMenu(g_hMenu, 0);
    
        // ����Ϣѭ��: 
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

        // ��ȡȫ·��
        ListView_GetItemText(g_hList, iClickItem, 1, szBuffer, _countof(szBuffer));
        StringCbCat(szBuffer, _countof(szBuffer), TEXT("\\"));
        ListView_GetItemText(g_hList, iClickItem, 0, szBuffer1, _countof(szBuffer1));
        StringCbCat(szBuffer, _countof(szBuffer), szBuffer1);

        if (id == IDM_OPENFILE)
            ShellExecute(0, 0, szBuffer, NULL, NULL, SW_SHOW);
        else
        {
            StringCbCopy(szBuffer1, _countof(szBuffer1), TEXT("ȷ��ɾ��"));
            StringCbCat(szBuffer1, _countof(szBuffer1), szBuffer);
            if (IDNO == MessageBox(hDlg, szBuffer1, TEXT("��ʾ"),
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
                MessageBox(hDlg, TEXT("�޷�ɾ���ļ������ֶ�ɾ��"), TEXT("��ʾ"), MB_ICONEXCLAMATION);
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
        if (IDNO == MessageBox(hDlg, TEXT("ȷ��ɾ��ȫ��ѡ���ļ���"), TEXT("��ʾ"),
            MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING))
            return TRUE;

        int delCnt = 0;
        for (int i = 0; i < g_DataBase.size(); ++i)
            if (g_DataBase[i].checkstate == CHECKBOX_SECLECTED)
            {
                std::wstring p = g_pFileGroup->MakePath(g_DataBase[i].fi->PathCrc);
                p += L'\\';
                p += g_pFileGroup->GetFileNameByCrc(g_DataBase[i].fi->NameCrc);

                DeleteFile(p.c_str());
                if (!GetLastError())
                {
                    g_DataBase.erase(g_DataBase.begin() + i);
                    ++delCnt;
                    // i���ֲ���
                    --i;
                }
                else
                {
                    StringCbPrintf(szBuffer1, _countof(szBuffer1),
                        TEXT("�޷�ɾ���ļ�%s�����ֶ�ɾ��"), p.c_str());
                    UpdateStatusBar(0, szBuffer1);
                }
            }

        // ����item����
        ListView_SetItemCount(g_hList, g_DataBase.size());
        UpdateStatusBar(1, nullptr);
        InvalidateRect(hDlg, NULL, TRUE);

        StringCbPrintf(szBuffer1, _countof(szBuffer1),
            TEXT("%d���ļ���ɾ��"), delCnt);
        MessageBox(hDlg, szBuffer1, L"ɾ���ļ�", MB_ICONINFORMATION);
        return TRUE;
    }

    case IDM_HASHALL:
        
        return TRUE;

    case IDM_SELALL:
    case IDM_USELALL:
        for (int i = 0; i < g_DataBase.size(); ++i)
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
            SetDlgItemText(hDlg, id, TEXT("����(&R)"));
            SendMessage(g_hStatus, SB_GETTEXT, 0, (LPARAM)staticBuf);
            SendMessage(g_hStatus, SB_SETTEXT, 0, (LPARAM)TEXT("��ͣ"));
        }
        else
        {
            ResumeThread(g_hThread);
            SetDlgItemText(hDlg, id, TEXT("��ͣ(&P)"));
            SendMessage(g_hStatus, SB_SETTEXT, 0, (LPARAM)staticBuf);
        }
        bPause ^= 1;
        return TRUE;

    case IDB_STOP:
    case ID__40028:
        bTerminateThread = true;
        if (bPause)
            ResumeThread(g_hThread);
        SendMessage(g_hStatus, SB_SETTEXT, 0, (LPARAM)TEXT("�û���ֹ"));
        return TRUE;
    }

    return FALSE;
}


static bool Cls_OnCreate(HWND hDlg, LPCREATESTRUCT lpCreateStruct)
{
    // ���ò˵�
    SetMenu(hDlg, LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_MENU2)));

    // ��ʼ��List View    ��С������΢������
    g_hList = CreateWindowEx(0, WC_LISTVIEW, NULL,
                    LVS_REPORT | WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_ALIGNTOP | LVS_OWNERDATA,
                    0, 0, ClientWndSize.x-6, ClientWndSize.y-StatusBarHigh-61,
                    hDlg, (HMENU)ID_LISTVIEW, (HINSTANCE)GetWindowLong(hDlg, GWL_HINSTANCE), NULL);
    ListView_EnableGroupView(g_hList, TRUE);
    ListView_SetExtendedListViewStyleEx(g_hList, 0, LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
    if (!InitListViewColumns(g_hList))
        assert(0);
        

    // ��ʼ�� Status Bar
    g_hStatus = CreateStatusWindow(WS_CHILD | WS_VISIBLE, NULL, hDlg, ID_STATUSBAR);
    StatusBarSetPart[0] = ClientWndSize.x * 4 / 5;
    StatusBarSetPart[1] = -1;
    SendMessage(g_hStatus, SB_SETPARTS, 2, (LPARAM)StatusBarSetPart);

    
    // ���ý���
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

                // ʹ�ÿո���л�checkbox
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


                // g_DataBase �̲߳���ȫ (??��?��)

                int itemid = pItem->iItem;
                if (itemid >= g_DataBase.size())
                    break;

                pdi->item.state = INDEXTOSTATEIMAGEMASK(g_DataBase[itemid].checkstate);
                pdi->item.stateMask = LVIS_STATEIMAGEMASK;

                FileGroup::pFileInfo pfi = g_DataBase[itemid].fi.get();
                if (pItem->mask & LVIF_TEXT)
                {
                    SYSTEMTIME st;
                    FILETIME ft;
                    TCHAR Buffer[128] = { 0 };

                    switch (pItem->iSubItem)
                    {
                    case 0: // name
                        wcscpy_s(pItem->pszText,
                            pItem->cchTextMax,
                            g_pFileGroup->GetFileNameByCrc(pfi->NameCrc).c_str()
                            );
                        break;

                    case 1: // path
                        wcscpy_s(pItem->pszText,
                            pItem->cchTextMax,
                            g_pFileGroup->MakePath(pfi->PathCrc).c_str()
                            );
                        break;

                    case 2: // size
                        if (pfi->Size > 966367641)		// ԼΪ0.9GB
                            StringCbPrintf(Buffer, 120, TEXT("%.2lf GB"), pfi->Size / (double)0x40000000);
                        else if (pfi->Size > 943718)	// 0.9MB
                            StringCbPrintf(Buffer, 120, TEXT("%.2lf MB"), pfi->Size / (double)0x100000);
                        else if (pfi->Size > 921)        // 0.9KB
                            StringCbPrintf(Buffer, 120, TEXT("%.2lf KB"), pfi->Size / (double)0x400);
                        else
                            StringCbPrintf(Buffer, 120, TEXT("%d B"), pfi->Size);

                        wcscpy_s(pItem->pszText, pItem->cchTextMax, Buffer);
                        break;

                    case 3: // create time
                        FileTimeToLocalFileTime(&pfi->CreationTime, &ft);
                        FileTimeToSystemTime(&ft, &st);
                        StringCbPrintf(Buffer, 120, TEXT("%04d/%02d/%02d %02d:%02d:%02d"),
                            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

                        wcscpy_s(pItem->pszText, pItem->cchTextMax, Buffer);
                        break;

                    case 4: // last write time
                        FileTimeToLocalFileTime(&pfi->LastWriteTime, &ft);
                        FileTimeToSystemTime(&ft, &st);
                        StringCbPrintf(Buffer, 120, TEXT("%04d/%02d/%02d %02d:%02d:%02d"),
                            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

                        wcscpy_s(pItem->pszText, pItem->cchTextMax, Buffer);
                        break;

                    case 5: // hash
                        wchar_t res[42];
                        static_cast<SHA_1*>(0)->ToHexString(
                            g_DataBase[itemid].hr.b, res, 42 * sizeof(wchar_t));

                        wcscpy_s(pItem->pszText,
                            pItem->cchTextMax,
                            res
                            );
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
    wchar_t Buffer[MAX_PATH];

    switch (id)
    {
    case IDOK:
        DetermineStructType(hDlg);
        UpdateFilterConfig(hDlg, *g_pFileGroup);

    case IDCANCEL:
        EndDialog(hDlg, 0);
        return TRUE;

    case IDC_BUTTON_ADDDIR:
        OpenFolder(hDlg, Buffer);
        if (Buffer[0] &&
            LB_ERR == SendDlgItemMessage(hDlg, IDC_LIST_DIR, LB_FINDSTRINGEXACT, -1, (LPARAM)Buffer))
        {
            if (!HasParentDirectoryInList(GetDlgItem(hDlg, IDC_LIST_DIR), Buffer))
            {
                int ans = 0;
                wchar_t detectChild[MAX_PATH] = { 0 };
                wcscpy_s(detectChild, MAX_PATH - 2, Buffer);
                detectChild[min(MAX_PATH-2, wcslen(Buffer))] = L'\\';
                if (LB_ERR != SendDlgItemMessage(hDlg, IDC_LIST_DIR, LB_FINDSTRING, -1, (LPARAM)detectChild))
                    ans = MessageBox(hDlg, L"�б��а��������ļ��н����޳�", L"��ʾ", MB_YESNO);
                if (ans == IDYES)
                {
                    int i;
                    do
                    {
                        i = SendDlgItemMessage(hDlg, IDC_LIST_DIR, LB_FINDSTRING, -1, (LPARAM)detectChild);
                        SendDlgItemMessage(hDlg, IDC_LIST_DIR, LB_DELETESTRING, i, 0);
                    }
                    while (i != LB_ERR);
                }
            
                if (ans != IDNO)
                    SendDlgItemMessage(hDlg, IDC_LIST_DIR, LB_ADDSTRING, 0, (LPARAM)Buffer);
            }
            else
                MessageBox(hDlg, L"�б����Ѵ�����Ӧ�ĸ�Ŀ¼", L"��ʾ", MB_OK | MB_ICONINFORMATION);
        }
        return TRUE;

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
        EnableControls(hDlg, id, (bool)IsDlgButtonChecked(hDlg, id));
        return TRUE;

    // ��������ѡ��ť��֪��ôû���ֵ�ͬһ��
    case IDC_RADIO_FULLNAME:
        CheckDlgButton(hDlg, IDC_RADIO_PARTNAME, false);
        CheckDlgButton(hDlg, IDC_RADIO_INCNAME, false);
        break;

    case IDC_RADIO_PARTNAME:
        CheckDlgButton(hDlg, IDC_RADIO_INCNAME, false);
        CheckDlgButton(hDlg, IDC_RADIO_FULLNAME, false);
        return TRUE;

    case IDC_RADIO_INCNAME:
        CheckDlgButton(hDlg, IDC_RADIO_FULLNAME, false);
        CheckDlgButton(hDlg, IDC_RADIO_PARTNAME, false);
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
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"��ǰ");
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"���");

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
    //SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"�������ļ���");
    //SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"��ǰ�ļ���");
    //for (int i=1; i<100; ++i)
    //{
    //    StringCchPrintf(Buffer, MAX_PATH, L"%d", i);
    //    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)Buffer);
    //}
    //SetDlgItemText(hDlg, IDC_COMBO_SUBDIR, L"�������ļ���");

    hCombo = GetDlgItem(hDlg, IDC_COMBO_TYPEINC);
    hCombo1 = GetDlgItem(hDlg, IDC_COMBO_TYPEIGN);
    for (UINT i=IDS_STRING103; i<=IDS_STRING107; ++i)
    {
        LoadString(g_hInstance, i, Buffer, MAX_PATH);
        SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)Buffer);
        SendMessage(hCombo1, CB_ADDSTRING, 0, (LPARAM)Buffer);
    }

    SendMessage(hDlg, WM_COMMAND, IDC_CHECK_NAME, 0);
    SendMessage(hDlg, WM_COMMAND, IDC_CHECK_SIZE, 0);
    SendMessage(hDlg, WM_COMMAND, IDC_CHECK_DATE, 0);
    SendMessage(hDlg, WM_COMMAND, IDC_CHECK_DATA, 0);
    LoadFilterConfigToDialog(hDlg, *g_pFileGroup);
    
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


DWORD WINAPI Thread(PVOID pvoid)
{
    while (1)
    {
        WaitForSingleObject(g_ThreadSignal, INFINITE);
        
        // if (...) ExitThread
        g_pFileGroup->bTerminate = &bTerminateThread;

        g_pFileGroup->StartSearchFiles();
    }

    return 0;
}


DWORD WINAPI ThreadHashOnly(PVOID pvoid)
{
    return 0;
}