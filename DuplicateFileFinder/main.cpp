
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
    wchar_t szBuffer[MAX_PATH1], szBuffer1[MAX_PATH1];

    switch (id)
    {
    case IDM_OPENFILE:
    case IDM_DELETE:	
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
            if (! GetLastError())
            {
                ListView_DeleteItem(g_hList, iClickItem);
                UpdateStatusBar(1, 0);
            }
            else
                MessageBox(hDlg, TEXT("�޷�ɾ���ļ������ֶ�ɾ��"), TEXT("��ʾ"), MB_ICONEXCLAMATION);
        }
        return TRUE;

    case IDM_EXPLORER:
        ListView_GetItemText(g_hList, iClickItem, 1, szBuffer, _countof(szBuffer));
        ShellExecute(0, 0, szBuffer, 0, 0, SW_SHOW);
        return TRUE;

    case IDM_DELETEALL:
        if (IDNO == MessageBox(hDlg, TEXT("ȷ��ɾ��ȫ��ѡ���ļ���"), TEXT("��ʾ"),
                MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING))
                return TRUE;

        for (int i=ListView_GetItemCount(g_hList)-1; i>=0; --i)
            if (ListView_GetCheckState(g_hList, i) == TRUE)
            {
                ListView_GetItemText(g_hList, i, 1, szBuffer, _countof(szBuffer));
                StringCbCat(szBuffer, _countof(szBuffer), TEXT("\\"));
                ListView_GetItemText(g_hList, i, 0, szBuffer1, _countof(szBuffer1));
                StringCbCat(szBuffer, _countof(szBuffer), szBuffer1);
                DeleteFile(szBuffer);
                if (!GetLastError())
                    ListView_DeleteItem(g_hList, i);
                else
                {
                    StringCbPrintf(szBuffer1, _countof(szBuffer1), TEXT("�޷�ɾ���ļ�%s�����ֶ�ɾ��"), szBuffer);
                    SendMessage(g_hStatus, SB_SETTEXT, 0, (LPARAM)szBuffer1);
                }
            }
        return TRUE;

    case IDM_REMOVE:
        StringCbPrintf(szBuffer, _countof(szBuffer), TEXT("���Ƴ�%d��"), DelDifferentHash(g_hList));
        SendMessage(g_hStatus, SB_SETTEXT, 0, (LPARAM)szBuffer);
        UpdateStatusBar(1, 0);
        return TRUE;

    case IDM_HASHALL:
        //params.bTerminate = FALSE;
        //CreateThread(NULL, 0, ThreadHashOnly, &params.bTerminate, 0, NULL);
        return TRUE;

    case IDM_SELALL:
        for (int i=ListView_GetItemCount(g_hList)-1; i>=0; --i)
            ListView_SetCheckState(g_hList, i, TRUE);
        UpdateStatusBar(1, 0);
        return TRUE;

    case IDM_USELALL:
        for (int i=ListView_GetItemCount(g_hList); i>=0; --i)
            ListView_SetCheckState(g_hList, i, FALSE);
        UpdateStatusBar(1, 0);
        return TRUE;

    case IDM_SELSAMEHASH:
        SelectSameHash(g_hList);
        UpdateStatusBar(1, 0);
        return TRUE;
    //----------------------------------------------------------------------------------------------------
    case IDC_NEEDHASH:
        EnableWindow(GetDlgItem(hDlg, IDC_FILESIZE),
            IsDlgButtonChecked(hDlg, IDC_NEEDHASH));
        return TRUE;
    //----------------------------------------------------------------------------------------------------
    case IDB_SETFILTER:
    case ID__40025:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_FILTER), hDlg, FilterDialogProc);
        return TRUE;


    case IDB_START:
    case ID__40026:
        ListView_DeleteAllItems(g_hList);
        ListView_RemoveAllGroups(g_hList);

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
                case VK_CONTROL:			// �޷�����VK_RETURN
                    iClickItem = ListView_GetSelectionMark(g_hList);
                    SendMessage(hDlg, WM_COMMAND, IDM_OPENFILE, 0);
                    return TRUE;

                case VK_DELETE:
                    iClickItem = ListView_GetSelectionMark(g_hList);
                    SendMessage(hDlg, WM_COMMAND, IDM_DELETE, 0);
                    return TRUE;
                }
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
                FileGroup::pFileInfo pfi = g_DataBase[itemid].first.get();
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
                            pfi->Name.c_str()
                            );
                        break;

                    case 1: // path
                        wcscpy_s(pItem->pszText,
                            pItem->cchTextMax,
                            pfi->Path.c_str()
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
                        wcscpy_s(pItem->pszText,
                            pItem->cchTextMax,
                            g_DataBase[itemid].second.c_str()
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
    TCHAR Buffer[MAX_PATH];

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
                if (LB_ERR != SendDlgItemMessage(hDlg, IDC_LIST_DIR, LB_FINDSTRING, -1, (LPARAM)Buffer))
                    ans = MessageBox(hDlg, L"�б��а��������ļ��н����޳�", L"��ʾ", MB_YESNO);
                if (ans == IDYES)
                {
                    int i;
                    do
                    {
                        i = SendDlgItemMessage(hDlg, IDC_LIST_DIR, LB_FINDSTRING, -1, (LPARAM)Buffer);
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


//////////////////////////////////////////////////////////////////////////////////////////////
// ����  -	�Ҽ��˵��й�ϣѡ���ļ�ʱ����
// pviod -	ָ��pparams->bOptions��ָ�룬������ֹ��ϣ
//////////////////////////////////////////////////////////////////////////////////////////////
DWORD WINAPI ThreadHashOnly(PVOID pvoid)
{
    //BOOL  * pf = (BOOL *)pvoid;
    //int		i;
    //TCHAR	szBuffer[MAX_PATH1], szBuffer1[MAX_PATH];
    //wchar_t Result[48];
    //SHA_1   Hash;

    //EnableWindow(GetDlgItem(GetParent(g_hList), IDB_BROWSE), FALSE);
    //EnableWindow(GetDlgItem(GetParent(g_hList), IDB_SEARCH), FALSE);
    //EnableWindow(GetDlgItem(GetParent(g_hList), IDB_PAUSE),  FALSE);
    //
    //for (i=ListView_GetItemCount(g_hList)-1; i>=0; --i)
    //	if (ListView_GetCheckState(g_hList, i) == TRUE)			// ��ȡѡ�е��в���ϣ
    //	{
    //		ListView_GetItemText(g_hList, i, 1, szBuffer, MAX_PATH1);
    //		StringCbCat(szBuffer, MAX_PATH1, TEXT("\\"));
    //		ListView_GetItemText(g_hList, i, 0, szBuffer1, MAX_PATH1);
    //		StringCbCat(szBuffer, MAX_PATH1, szBuffer1);
    //		ListView_SetItemText(g_hList, i, 5, TEXT("��ϣ��..."));
    //		Hash.CalculateSha1(szBuffer, Result, MAX_PATH, !(*pf));
    //		ListView_SetItemText(g_hList, i, 5, Result);
    //	}
    //EnableWindow(GetDlgItem(GetParent(g_hList), IDB_BROWSE), TRUE);
    //EnableWindow(GetDlgItem(GetParent(g_hList), IDB_SEARCH), TRUE);
    //EnableWindow(GetDlgItem(GetParent(g_hList), IDB_PAUSE),  TRUE);
    return 0;
}