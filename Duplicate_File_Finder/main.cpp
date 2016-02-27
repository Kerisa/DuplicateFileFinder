#include "FileGroup.h"

#include <Windows.h>
#include <WindowsX.h>

HMENU		hMenu;							 // 全局变量
HWND		hList, hCombo, hStatus, g_hDlgFilter;
HINSTANCE	g_hInstance;
HANDLE      g_ThreadSignal, g_hThread;
FileGroup   g_FileGroup;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInstance,
					  PSTR pCmdLine, int iCmdShow)
{
	TCHAR	szBuffer[32];
	INITCOMMONCONTROLSEX	icce;

	g_hInstance = hInstance;
	icce.dwSize	= sizeof(INITCOMMONCONTROLSEX);
	icce.dwICC  = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icce);	// 初始化通用控件
    
    // 线程事件
    g_ThreadSignal = CreateEvent(0, FALSE, FALSE, 0);

    g_hThread = CreateThread(0, 0, Thread, 0, 0, 0);

	if (-1 == DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_DFF), NULL, MainDlgProc, 0))
	{
		StringCbPrintf(szBuffer, 30, TEXT("初始化失败!错误码(0x%08X)"), GetLastError());
		MessageBox(NULL, szBuffer, TEXT("Duplicate_File_Finder"), MB_ICONERROR);
	}

    CloseHandle(g_hThread);
    CloseHandle(g_ThreadSignal);
	return 0;
}

int my_ListView_GetCheckedCount(HWND hList)
{
	int i, ret = 0;
	for (i=ListView_GetItemCount(hList)-1; i>=0; --i)
	{
		if (ListView_GetCheckState(hList, i))
			++ret;
	}
	return ret;
}

#define UpdateStatusPart1() { \
	StringCbPrintf(szBuffer, MAX_PATH1, TEXT("选中文件:%d  /  文件总数:%d"),\
	    my_ListView_GetCheckedCount(hList), ListView_GetItemCount(hList)); \
	SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)szBuffer); }


int iClickItem;

static bool Cls_OnCommand_main(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
    TCHAR szBuffer[MAX_PATH], szBuffer1[MAX_PATH];

    switch (id)
    {
    case IDM_OPENFILE:
    case IDM_DELETE:	
	    // 获取全路径
	    ListView_GetItemText(hList, iClickItem, 1, szBuffer, MAX_PATH1);
	    StringCbCat(szBuffer, MAX_PATH1, TEXT("\\"));
	    ListView_GetItemText(hList, iClickItem, 0, szBuffer1, MAX_PATH1);
	    StringCbCat(szBuffer, MAX_PATH1, szBuffer1);

	    if (id == IDM_OPENFILE)
		    ShellExecute(0, 0, szBuffer, NULL, NULL, SW_SHOW);
	    else
	    {
		    StringCbCopy(szBuffer1, MAX_PATH1, TEXT("确认删除"));
		    StringCbCat(szBuffer1, MAX_PATH1, szBuffer);
		    if (IDNO == MessageBox(hDlg, szBuffer1, TEXT("提示"),
			    MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING))
			    return TRUE;
		    DeleteFile(szBuffer);
		    if (! GetLastError())
		    {
			    ListView_DeleteItem(hList, iClickItem);
			    UpdateStatusPart1();
		    }
		    else
                MessageBox(hDlg, TEXT("无法删除文件，请手动删除"), TEXT("提示"), MB_ICONEXCLAMATION);
	    }
	    return TRUE;

    case IDM_EXPLORER:
	    ListView_GetItemText(hList, iClickItem, 1, szBuffer, MAX_PATH1);
	    ShellExecute(0, 0, szBuffer, 0, 0, SW_SHOW);
	    return TRUE;

    case IDM_DELETEALL:
	    if (IDNO == MessageBox(hDlg, TEXT("确认删除全部选中文件？"), TEXT("提示"),
			    MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING))
			    return TRUE;

	    for (int i=ListView_GetItemCount(hList)-1; i>=0; --i)
		    if (ListView_GetCheckState(hList, i) == TRUE)
		    {
			    ListView_GetItemText(hList, i, 1, szBuffer, MAX_PATH1);
			    StringCbCat(szBuffer, MAX_PATH1, TEXT("\\"));
			    ListView_GetItemText(hList, i, 0, szBuffer1, MAX_PATH1);
			    StringCbCat(szBuffer, MAX_PATH1, szBuffer1);
			    DeleteFile(szBuffer);
			    if (!GetLastError())
				    ListView_DeleteItem(hList, i);
			    else
			    {
				    StringCbPrintf(szBuffer1, MAX_PATH1, TEXT("无法删除文件%s，请手动删除"), szBuffer);
				    SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)szBuffer1);
			    }
		    }
	    return TRUE;

    case IDM_REMOVE:
	    StringCbPrintf(szBuffer, MAX_PATH1, TEXT("已移除%d项"), DelDifferentHash(hList));
	    SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)szBuffer);
	    UpdateStatusPart1();
	    return TRUE;

    case IDM_HASHALL:
	    //params.bTerminate = FALSE;
	    //CreateThread(NULL, 0, ThreadHashOnly, &params.bTerminate, 0, NULL);
	    return TRUE;

    case IDM_SELALL:
	    for (int i=ListView_GetItemCount(hList)-1; i>=0; --i)
		    ListView_SetCheckState(hList, i, TRUE);
	    UpdateStatusPart1();
	    return TRUE;

    case IDM_USELALL:
	    for (int i=ListView_GetItemCount(hList); i>=0; --i)
		    ListView_SetCheckState(hList, i, FALSE);
	    UpdateStatusPart1();
	    return TRUE;

    case IDM_SELSAMEHASH:
	    SelectSameHash(hList);
	    UpdateStatusPart1();
	    return TRUE;
    //----------------------------------------------------------------------------------------------------
    case IDC_NEEDHASH:
	    EnableWindow(GetDlgItem(hDlg, IDC_FILESIZE),
            IsDlgButtonChecked(hDlg, IDC_NEEDHASH));
	    return TRUE;
    //----------------------------------------------------------------------------------------------------
    case IDB_SETFILTER:
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_FILTER), hDlg, FilterDialogProc);
        return TRUE;

    case IDB_SEARCH:
	    //if (params.szFolderName[0] != '\0')
	    //{
		   // SetNewParam(hwnd, &params);
				
		   // hThread = CreateThread(NULL, 0, Thread, &params, 0, NULL);
		   // if (hThread == INVALID_HANDLE_VALUE)
			  //  MessageBox(hwnd, TEXT("线程创建失败！"), 0, MB_ICONERROR);
	    //}
	    //else
		   // SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)TEXT("请选择搜索目录"));
	    return TRUE;

    case IDB_START:
        ListView_DeleteAllItems(hList);
        SetEvent(g_ThreadSignal);
        return TRUE;

    case IDB_PAUSE:
	    //if (!bPaused)
	    //{
		   // SuspendThread(hThread);
		   // SetDlgItemText(hwnd, IDB_PAUSE, TEXT("继续(&R)"));
		   // SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)TEXT("暂停"));
	    //}
	    //else
	    //{
		   // ResumeThread(hThread);
		   // SetDlgItemText(hwnd, IDB_PAUSE, TEXT("暂停(&P)"));
	    //}
	    //bPaused ^= 1;
	    return TRUE;

    case IDB_STOP:
	    //params.bTerminate = TRUE;
	    //if (bPaused)
		   // ResumeThread(hThread);
	    //SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)TEXT("用户终止"));
	    return TRUE;
    }

    return FALSE;
}

static void Cls_OnClose(HWND hDlg)
{
    EndDialog(hDlg, 0);
}

static bool Cls_OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam)
{
    static const int cListViewPos[4] = {7, 64, 1020, 450};

    int setpart[2];

    //g_hDlgFilter = CreateDialog(g_hInstance, 
    //    MAKEINTRESOURCE(IDD_FILTER), hDlg, FilterDialogProc);

    // 加载图标
    SendMessage(hDlg, WM_SETICON, ICON_BIG,
		    (LPARAM)LoadIcon((HINSTANCE)GetWindowLong(hDlg, GWL_HINSTANCE), MAKEINTRESOURCE(IDI_ICON64)));

    // 初始化List View
    hList = CreateWindowEx(0, WC_LISTVIEW, NULL,
				    LVS_REPORT | WS_CHILD | WS_VISIBLE | WS_BORDER,
				    cListViewPos[0], cListViewPos[1], cListViewPos[2], cListViewPos[3],
				    hDlg, (HMENU)ID_LISTVIEW, (HINSTANCE)GetWindowLong(hDlg, GWL_HINSTANCE), NULL);
    ListView_SetExtendedListViewStyleEx(hList, 0, LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
    if (!InitListViewColumns(hList))
	    EndDialog(hDlg, 0);
		
    // 初始化 Status Bar
    hStatus = CreateStatusWindow(WS_CHILD | WS_VISIBLE, NULL, hDlg, ID_STATUSBAR);
    setpart[0] = 800;  setpart[1] = -1;
    SendMessage(hStatus, SB_SETPARTS, 2, (LPARAM)setpart);

    // 初始化线程函数参数
    //RtlZeroMemory(params.szFolderName, MAX_PATH1);
    //params.bOptions			= 0;
    //params.bTerminate		= FALSE;

    // 初始化按钮状态
    CheckDlgButton(hDlg, IDC_NEEDHASH, BST_CHECKED);
    CheckDlgButton(hDlg, IDC_SAMENAME, BST_CHECKED);
    SetDlgItemInt (hDlg, IDC_FILESIZE, 50, FALSE);

    // 加载右键菜单
    hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_MENU1));
    hMenu = GetSubMenu(hMenu, 0);
    
    // 设置焦点
    SetFocus(GetDlgItem(hDlg, IDB_BROWSE));
    return FALSE;
}

void Cls_OnSysCommand0(HWND hwnd, UINT cmd, int x, int y)
{
    if (cmd == SC_CLOSE)
    {
        SendMessage(g_hDlgFilter, WM_SYSCOMMAND, SC_CLOSE, 0);
        EndDialog(hwnd, 0);
    }
}

BOOL CALLBACK MainDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    static const int cListViewPos[4] = {7, 64, 1020, 450};
    static int CurCtlIdx;

    LVHITTESTINFO    lvh;

    switch (Msg)
    {
        HANDLE_MSG(hDlg, WM_COMMAND,	 Cls_OnCommand_main);
        HANDLE_MSG(hDlg, WM_CLOSE,		 Cls_OnClose);
        HANDLE_MSG(hDlg, WM_SYSCOMMAND,	 Cls_OnSysCommand0);
        HANDLE_MSG(hDlg, WM_INITDIALOG,  Cls_OnInitDialog);
        case WM_NOTIFY:			// 7, 64, 1020, 390
		if (((NMHDR*)lParam)->idFrom == ID_LISTVIEW)
		{
			switch (((NMHDR*)lParam)->code)
			{
			case LVN_KEYDOWN:
//			case NM_RETURN:
				switch (((LPNMLVKEYDOWN)lParam)->wVKey)
				{
				case VK_CONTROL:			// 无法接收VK_RETURN
					iClickItem = ListView_GetSelectionMark(hList);
					SendMessage(hDlg, WM_COMMAND, IDM_OPENFILE, 0);
					return TRUE;

				case VK_DELETE:
					iClickItem = ListView_GetSelectionMark(hList);
					SendMessage(hDlg, WM_COMMAND, IDM_DELETE, 0);
					return TRUE;
				}
				break;

			case NM_RCLICK:
			case NM_DBLCLK:
				GetCursorPos(&lvh.pt);
				ScreenToClient(hDlg, &lvh.pt);
				lvh.pt.x -= cListViewPos[0];
				lvh.pt.y -= cListViewPos[1];
				iClickItem =  SendMessage(hList, LVM_SUBITEMHITTEST, 0, (LPARAM)&lvh);
				if (((NMHDR*)lParam)->code == NM_DBLCLK)
				{
					SendMessage(hDlg, WM_COMMAND, IDM_OPENFILE, 0);
				}
				else
				{
					GetCursorPos(&lvh.pt);
					TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, lvh.pt.x, lvh.pt.y, 0, hDlg, NULL);
				}
				return TRUE;
			}
		}
		return FALSE;
        default:
            return false;
    }
}


static void EnableControls(HWND hDlg, int id, bool enable)
{
    //CheckDlgButton(hDlg, id, enable);

    switch (id)
    {
    case IDC_CHECK_NAME:
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_FULLNAME), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_CHECK_NOSUFFIX), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_PARTNAME), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_NAMEBEG), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_NAMEEND), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_INCNAME), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_NAMEINC), enable);
        break;

    case IDC_CHECK_SIZE:
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_SIZEEQUAL), enable);
        break;

    case IDC_CHECK_DATE:
        EnableWindow(GetDlgItem(hDlg, IDC_CHECK_YEAR), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_CHECK_MONTH), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_CHECK_DAY), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_CHECK_WEEK), enable);
        break;

    case IDC_CHECK_DATA:
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_DATAEQUAL), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_PARTDATAEQUAL), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_COMBO_DATAFRONT), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_DATABEG), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_DATAEND), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_COMBO_DATAUNIT), enable);
        break;

    case IDC_CHECK_SIZERANGE:
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_SIZERANGE_INC), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_SIZERANGE_IGN), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SIZEBEG), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SIZEEND), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_COMBO_SIZEUNIT), enable);
        break;

    case IDC_CHECK_ATTRIB:
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_ATTRIB_INC), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_ATTRIB_IGN), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_CHECK_ATTR_A), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_CHECK_ATTR_R), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_CHECK_ATTR_S), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_CHECK_ATTR_H), enable);
        break;

    default:
        assert(0);
        break;
    }
}


void SplitString(const wchar_t *src,
                 std::vector<std::wstring>& v,
                 const wchar_t *key,
                 const wchar_t  begin,
                 const wchar_t  end,
                 bool ignoreSpace)
{
    if (!src || src[0] == 0) return;
    
    int len = wcslen(src);
    wchar_t *copy = new wchar_t[len+1];
    if (!copy) return;


    int posl = 0, posr = -1;
    if (ignoreSpace)
    {
        int j = 0;
        for (int i=0; i<len; ++i)
            if (src[i] != L' ')
                copy[j++] = src[i];
        
        copy[j] = 0;
        len = j;
    }
    
    posl = 0, posr = len-1;
    for (int i=0; i<len; ++i)
    {
        if (copy[i] == begin)
            posl = i + 1;
        else if (copy[i] == end)
        {
            posr = i - 1;
            copy[i] = 0;
            break;
        }
    }
    if (posr < posl)
        return;

    int len1 = wcslen(key), j = posl;
    for (int i=posl; i<posr;)
    {
        if (!wcsncmp(&copy[i], key, len1))
        {
            wchar_t bak = copy[i];
            copy[i] = 0;
            v.push_back(std::wstring(&copy[j]));
            i = j = i + len1;
        }
        else
            ++i;
    }

    if (j < posr)
        v.push_back(std::wstring(&copy[j]));

    delete[] copy;
}

void LoadFilterConfigToDialog(HWND hDlg, FileGroup& fg)
{
    TCHAR Buffer[MAX_PATH];

    // 文件名

    if (fg.m_Filter.Switch[FileGroup::FILTER::Compare_FileName]
        == FileGroup::FILTER::Type_Off)
        EnableControls(hDlg, IDC_CHECK_NAME, false);
    else
    {
        CheckDlgButton(hDlg, IDC_CHECK_NAME, 1);
        EnableControls(hDlg, IDC_CHECK_NAME, true);

        switch (fg.m_Filter.Switch[FileGroup::FILTER::Compare_FileName])
        {
        case FileGroup::FILTER::Type_Whole:
            for (int i=IDC_RADIO_FULLNAME; i<=IDC_RADIO_PARTNAME; ++i)
                CheckDlgButton(hDlg, i, i==IDC_RADIO_FULLNAME);
            
            CheckDlgButton(hDlg, IDC_CHECK_NOSUFFIX,
                fg.m_Filter.FileNameWithoutSuffix);

            break;

        case FileGroup::FILTER::Type_RangeMatch:
            for (int i=IDC_RADIO_FULLNAME; i<=IDC_RADIO_PARTNAME; ++i)
                CheckDlgButton(hDlg, i, i==IDC_RADIO_PARTNAME);

            StringCchPrintf(Buffer, MAX_PATH, L"%d",
                fg.m_Filter.FileNameRange.LowerBound);
            SetDlgItemText(hDlg, IDC_EDIT_NAMEBEG, Buffer);

            StringCchPrintf(Buffer, MAX_PATH, L"%d",
                fg.m_Filter.FileNameRange.UpperBound);
            SetDlgItemText(hDlg, IDC_EDIT_NAMEEND, Buffer);

            break;

        case FileGroup::FILTER::Type_Include:
            for (int i=IDC_RADIO_FULLNAME; i<=IDC_RADIO_PARTNAME; ++i)
                CheckDlgButton(hDlg, i, i==IDC_RADIO_INCNAME);

            SetDlgItemText(hDlg, IDC_EDIT_NAMEINC, fg.m_Filter.KeyWordOfFileName.c_str());

            break;

        default:
            assert(0);
            break;
        }
    }

    //------------------------------------------- 文件大小

    if (fg.m_Filter.Switch[FileGroup::FILTER::Compare_FileSize]
        == FileGroup::FILTER::Type_Off)
        EnableControls(hDlg, IDC_CHECK_SIZE, false);
    else
    {
        CheckDlgButton(hDlg, IDC_CHECK_SIZE, 1);
        EnableControls(hDlg, IDC_CHECK_SIZE, true);
        CheckDlgButton(hDlg, IDC_RADIO_SIZEEQUAL, true);
    }

    //------------------------------------------- 文件修改日期

    int status;
    if ((status = fg.m_Filter.Switch[FileGroup::FILTER::Compare_FileDate])
        == FileGroup::FILTER::Type_Off)
        EnableControls(hDlg, IDC_CHECK_DATE, false);
    else
    {
        CheckDlgButton(hDlg, IDC_CHECK_DATE, 1);
        EnableControls(hDlg, IDC_CHECK_DATE, true);

        CheckDlgButton(hDlg, IDC_CHECK_YEAR,  fg.m_Filter.SelectedDate & FileGroup::FILTER::Time_Year);
        CheckDlgButton(hDlg, IDC_CHECK_MONTH, fg.m_Filter.SelectedDate & FileGroup::FILTER::Time_Month);
        CheckDlgButton(hDlg, IDC_CHECK_DAY,   fg.m_Filter.SelectedDate & FileGroup::FILTER::Time_Day);
        CheckDlgButton(hDlg, IDC_CHECK_WEEK,  fg.m_Filter.SelectedDate & FileGroup::FILTER::Time_Week);
    }

    //------------------------------------------- 文件内容

    if ((status = fg.m_Filter.Switch[FileGroup::FILTER::Compare_FileHash])
        == FileGroup::FILTER::Type_Off)
        EnableControls(hDlg, IDC_CHECK_DATA, false);
    else
    {
        CheckDlgButton(hDlg, IDC_CHECK_DATA, 1);
        EnableControls(hDlg, IDC_CHECK_DATA, true);

        if (status == FileGroup::FILTER::Type_Whole)
            for (int i=IDC_RADIO_DATAEQUAL; i<=IDC_RADIO_PARTDATAEQUAL; ++i)
                CheckDlgButton(hDlg, i, i==IDC_RADIO_DATAEQUAL);

        else
        {
            for (int i=IDC_RADIO_DATAEQUAL; i<=IDC_RADIO_PARTDATAEQUAL; ++i)
                CheckDlgButton(hDlg, i, i==IDC_RADIO_PARTDATAEQUAL);

            StringCchPrintf(Buffer, MAX_PATH, L"%d",
                fg.m_Filter.FileDataRange.LowerBound);
            SetDlgItemText(hDlg, IDC_EDIT_DATABEG, Buffer);

            StringCchPrintf(Buffer, MAX_PATH, L"%d",
                fg.m_Filter.FileDataRange.UpperBound);
            SetDlgItemText(hDlg, IDC_EDIT_DATAEND, Buffer);

            // 单位
        }
    }

    //------------------------------------------- 包含的扩展名

    if ((status = fg.m_Filter.Switch[FileGroup::FILTER::Search_IncludeSuffix])
        == FileGroup::FILTER::Type_Off) 
        SetDlgItemText(hDlg, IDC_COMBO_TYPEINC, L"");
    
    else
    {
        std::vector<std::wstring>::iterator it = fg.m_Filter.ContainSuffix.begin();
        std::wstring str = std::wstring((*it++));
        for (; it != fg.m_Filter.ContainSuffix.end(); ++it)
            (str += L" | ") += (*it);
        SetDlgItemText(hDlg, IDC_COMBO_TYPEINC, str.c_str());
    }

    //------------------------------------------- 忽视的扩展名

    if ((status = fg.m_Filter.Switch[FileGroup::FILTER::Search_IgnoreSuffix])
        == FileGroup::FILTER::Type_Off) 
        SetDlgItemText(hDlg, IDC_COMBO_TYPEIGN, L"");
    
    else
    {
        std::vector<std::wstring>::iterator it = fg.m_Filter.IgnoreSuffix.begin();
        std::wstring str = std::wstring((*it++));
        for (; it != fg.m_Filter.IgnoreSuffix.end(); ++it)
            (str += L" | ") += (*it);
        SetDlgItemText(hDlg, IDC_COMBO_TYPEIGN, str.c_str());
    }

    //------------------------------------------- 查找的文件大小范围

    if ((status = fg.m_Filter.Switch[FileGroup::FILTER::Search_FileSize])
        == FileGroup::FILTER::Type_Off)
        EnableControls(hDlg, IDC_CHECK_SIZERANGE, false);
    else
    {
        CheckDlgButton(hDlg, IDC_CHECK_SIZERANGE, 1);
        EnableControls(hDlg, IDC_CHECK_SIZERANGE, true);

        CheckDlgButton(hDlg, IDC_RADIO_SIZERANGE_INC,
            status == FileGroup::FILTER::Type_Include);
        CheckDlgButton(hDlg, IDC_RADIO_SIZERANGE_IGN,
            status == FileGroup::FILTER::Type_Ignore);

        StringCchPrintf(Buffer, MAX_PATH, L"%d",
            fg.m_Filter.FileSizeRange.LowerBound);
        SetDlgItemText(hDlg, IDC_EDIT_SIZEBEG, Buffer);

        StringCchPrintf(Buffer, MAX_PATH, L"%d",
            fg.m_Filter.FileSizeRange.UpperBound);
        SetDlgItemText(hDlg, IDC_EDIT_SIZEEND, Buffer);

        // 单位
    }

    //------------------------------------------- 查找的文件属性

    if ((status = fg.m_Filter.Switch[FileGroup::FILTER::Search_FileAttribute])
        == FileGroup::FILTER::Type_Off)
        EnableControls(hDlg, IDC_CHECK_ATTRIB, false);
    else
    {
        CheckDlgButton(hDlg, IDC_CHECK_ATTRIB, 1);
        EnableControls(hDlg, IDC_CHECK_ATTRIB, true);

        CheckDlgButton(hDlg, IDC_RADIO_ATTRIB_INC,
            status == FileGroup::FILTER::Type_Include);
        CheckDlgButton(hDlg, IDC_RADIO_ATTRIB_IGN,
            status == FileGroup::FILTER::Type_Ignore);

        CheckDlgButton(hDlg, IDC_CHECK_ATTR_A, fg.m_Filter.SelectedAttributes & FileGroup::FILTER::Attrib_Archive);
        CheckDlgButton(hDlg, IDC_CHECK_ATTR_R, fg.m_Filter.SelectedAttributes & FileGroup::FILTER::Attrib_ReadOnly);
        CheckDlgButton(hDlg, IDC_CHECK_ATTR_S, fg.m_Filter.SelectedAttributes & FileGroup::FILTER::Attrib_System);
        CheckDlgButton(hDlg, IDC_CHECK_ATTR_H, fg.m_Filter.SelectedAttributes & FileGroup::FILTER::Attrib_Hide);
    }

    //------------------------------------------- 查找的目录

    std::map<std::wstring, int>::iterator it;
    std::wstring str;
    for (it=fg.m_Filter.SearchDirectory.begin(); it!=fg.m_Filter.SearchDirectory.end(); ++it)
    {
        str.clear();
        //StringCchPrintf(Buffer, MAX_PATH, L"[%d]", it->second);
        //str += Buffer;
        str += it->first;
        SendDlgItemMessage(hDlg, IDC_LIST_DIR, LB_ADDSTRING, 0, (LPARAM)str.c_str());
    }

}

void UpdateFilterConfig(HWND hDlg, FileGroup& fg)
{
    TCHAR Buffer[MAX_PATH];

    // 文件名

    if (!IsDlgButtonChecked(hDlg, IDC_CHECK_NAME))
        fg.m_Filter.Switch[FileGroup::FILTER::Compare_FileName] =
            FileGroup::FILTER::Type_Off;
    else
    {
        if (IsDlgButtonChecked(hDlg, IDC_RADIO_FULLNAME))
        {
            fg.m_Filter.Switch[FileGroup::FILTER::Compare_FileName] =
                FileGroup::FILTER::Type_Whole;
            fg.m_Filter.FileNameWithoutSuffix =
                IsDlgButtonChecked(hDlg, IDC_CHECK_NOSUFFIX);
        }
        else if (IsDlgButtonChecked(hDlg, IDC_RADIO_PARTNAME))
        {
            fg.m_Filter.Switch[FileGroup::FILTER::Compare_FileName] =
                FileGroup::FILTER::Type_RangeMatch;
            GetDlgItemText(hDlg, IDC_EDIT_NAMEBEG, Buffer, MAX_PATH);
            fg.m_Filter.FileNameRange.LowerBound = _wtoi(Buffer);
            GetDlgItemText(hDlg, IDC_EDIT_NAMEEND, Buffer, MAX_PATH);
            fg.m_Filter.FileNameRange.UpperBound = _wtoi(Buffer);
        }
        else if (IsDlgButtonChecked(hDlg, IDC_RADIO_INCNAME))
        {
            fg.m_Filter.Switch[FileGroup::FILTER::Compare_FileName] =
                FileGroup::FILTER::Type_Include;
            GetDlgItemText(hDlg, IDC_EDIT_NAMEINC, Buffer, MAX_PATH);
            fg.m_Filter.KeyWordOfFileName = Buffer;
        }
    }

    //------------------------------------------- 文件大小

    if (!IsDlgButtonChecked(hDlg, IDC_CHECK_SIZE))
        fg.m_Filter.Switch[FileGroup::FILTER::Compare_FileSize] =
            FileGroup::FILTER::Type_Off;
    else
    {
        fg.m_Filter.Switch[FileGroup::FILTER::Compare_FileSize] =
            FileGroup::FILTER::Type_Whole;
    }

    //------------------------------------------- 修改日期

    if (!IsDlgButtonChecked(hDlg, IDC_CHECK_DATE))
        fg.m_Filter.Switch[FileGroup::FILTER::Compare_FileDate] =
            FileGroup::FILTER::Type_Off;
    else
    {
        fg.m_Filter.Switch[FileGroup::FILTER::Compare_FileDate] =
            FileGroup::FILTER::Type_On;
        
        fg.m_Filter.SelectedDate = 0;
        if (IsDlgButtonChecked(hDlg, IDC_CHECK_YEAR))
            fg.m_Filter.SelectedDate |= FileGroup::FILTER::Time_Year;
        if (IsDlgButtonChecked(hDlg, IDC_CHECK_MONTH))
            fg.m_Filter.SelectedDate |= FileGroup::FILTER::Time_Month;
        if (IsDlgButtonChecked(hDlg, IDC_CHECK_DAY))
            fg.m_Filter.SelectedDate |= FileGroup::FILTER::Time_Day;
        if (IsDlgButtonChecked(hDlg, IDC_CHECK_WEEK))
            fg.m_Filter.SelectedDate |= FileGroup::FILTER::Time_Week;
    }

    //------------------------------------------- 文件内容

    if (!IsDlgButtonChecked(hDlg, IDC_CHECK_DATA))
        fg.m_Filter.Switch[FileGroup::FILTER::Compare_FileHash] =
            FileGroup::FILTER::Type_Off;
    else
    {
        if (IsDlgButtonChecked(hDlg, IDC_RADIO_DATAEQUAL))
        {
            fg.m_Filter.Switch[FileGroup::FILTER::Compare_FileHash] =
                FileGroup::FILTER::Type_Whole;
        }
        else
        {
            fg.m_Filter.Switch[FileGroup::FILTER::Compare_FileHash] =
                FileGroup::FILTER::Type_RangeMatch;

            GetDlgItemText(hDlg, IDC_EDIT_DATABEG, Buffer, MAX_PATH);
            fg.m_Filter.FileDataRange.LowerBound = _wtoi(Buffer);
            GetDlgItemText(hDlg, IDC_EDIT_DATAEND, Buffer, MAX_PATH);
            fg.m_Filter.FileDataRange.UpperBound = _wtoi(Buffer);
        }
    }

    //------------------------------------------- 包含类型

    fg.m_Filter.ContainSuffix.clear();
    fg.m_Filter.Switch[FileGroup::FILTER::Search_IncludeSuffix] =
                FileGroup::FILTER::Type_Off;

    int idx;
    if ((idx = SendDlgItemMessage(hDlg, IDC_COMBO_TYPEINC, CB_GETCURSEL, 0, 0))
        != CB_ERR && idx != 0)
    {
        LoadString(g_hInstance, idx+IDS_STRING102, Buffer, MAX_PATH);
        SplitString(Buffer, fg.m_Filter.ContainSuffix, L"|", L'(', L')', true);
        fg.m_Filter.Switch[FileGroup::FILTER::Search_IncludeSuffix] =
                FileGroup::FILTER::Type_On;
    }
    else if (idx == CB_ERR)
    {
        GetDlgItemText(hDlg, IDC_COMBO_TYPEINC, Buffer, MAX_PATH);
        SplitString(Buffer, fg.m_Filter.ContainSuffix, L"|", 0, 0, true);
        if (fg.m_Filter.ContainSuffix.size() > 0)
            fg.m_Filter.Switch[FileGroup::FILTER::Search_IncludeSuffix] =
                FileGroup::FILTER::Type_On;
    }
    else if (idx == 0)
        fg.m_Filter.ContainSuffix.clear();

    //------------------------------------------- 忽略类型

    fg.m_Filter.IgnoreSuffix.clear();
    fg.m_Filter.Switch[FileGroup::FILTER::Search_IgnoreSuffix] = FileGroup::FILTER::Type_Off;

    if ((idx = SendDlgItemMessage(hDlg, IDC_COMBO_TYPEIGN, CB_GETCURSEL, 0, 0))
        != CB_ERR && idx != 0)
    {
        LoadString(g_hInstance, idx+IDS_STRING102, Buffer, MAX_PATH);
        SplitString(Buffer, fg.m_Filter.IgnoreSuffix, L"|", L'(', L')', true);
        fg.m_Filter.Switch[FileGroup::FILTER::Search_IgnoreSuffix] = FileGroup::FILTER::Type_On;
    }
    else if (idx == CB_ERR)
    {
        GetDlgItemText(hDlg, IDC_COMBO_TYPEIGN, Buffer, MAX_PATH);
        SplitString(Buffer, fg.m_Filter.IgnoreSuffix, L"|", 0, 0, true);
        if (fg.m_Filter.IgnoreSuffix.size() > 0)
            fg.m_Filter.Switch[FileGroup::FILTER::Search_IgnoreSuffix] = FileGroup::FILTER::Type_On;
    }
    else if (idx == 0)
        fg.m_Filter.IgnoreSuffix.clear();

    //------------------------------------------- 文件大小范围

    if (!IsDlgButtonChecked(hDlg, IDC_CHECK_SIZERANGE))
        fg.m_Filter.Switch[FileGroup::FILTER::Search_FileSize] =
            FileGroup::FILTER::Type_Off;
    else
    {
        fg.m_Filter.Switch[FileGroup::FILTER::Search_FileSize] =
            (IsDlgButtonChecked(hDlg, IDC_RADIO_SIZERANGE_INC) ?
                FileGroup::FILTER::Type_Include :
                FileGroup::FILTER::Type_Ignore);
        
        GetDlgItemText(hDlg, IDC_EDIT_SIZEBEG, Buffer, MAX_PATH);
        fg.m_Filter.FileSizeRange.LowerBound = _wtoi(Buffer);
        GetDlgItemText(hDlg, IDC_EDIT_SIZEEND, Buffer, MAX_PATH);
        fg.m_Filter.FileSizeRange.UpperBound = _wtoi(Buffer);
    }

    //------------------------------------------- 文件属性

    if (!IsDlgButtonChecked(hDlg, IDC_CHECK_ATTRIB))
        fg.m_Filter.Switch[FileGroup::FILTER::Search_FileAttribute] =
            FileGroup::FILTER::Type_Off;
    else
    {
        fg.m_Filter.Switch[FileGroup::FILTER::Search_FileAttribute] =
            (IsDlgButtonChecked(hDlg, IDC_RADIO_ATTRIB_INC) ?
                FileGroup::FILTER::Type_Include :
                FileGroup::FILTER::Type_Ignore);
        
        fg.m_Filter.SelectedAttributes = 0;

        fg.m_Filter.SelectedAttributes |= IsDlgButtonChecked(hDlg, IDC_CHECK_ATTR_A) ?
            FileGroup::FILTER::Attrib_Archive : 0;
        fg.m_Filter.SelectedAttributes |= IsDlgButtonChecked(hDlg, IDC_CHECK_ATTR_R) ?
            FileGroup::FILTER::Attrib_ReadOnly : 0;
        fg.m_Filter.SelectedAttributes |= IsDlgButtonChecked(hDlg, IDC_CHECK_ATTR_S) ?
            FileGroup::FILTER::Attrib_System : 0;
        fg.m_Filter.SelectedAttributes |= IsDlgButtonChecked(hDlg, IDC_CHECK_ATTR_H) ?
            FileGroup::FILTER::Attrib_Hide : 0;
    }

    //------------------------------------------- 搜索路径

    fg.m_Filter.SearchDirectory.clear();

    idx = SendDlgItemMessage(hDlg, IDC_LIST_DIR, LB_GETCOUNT, 0, 0);
    if (idx != LB_ERR)
    {
        for (int i=0; i<idx; ++i)
        {
            //int times = SendDlgItemMessage(hDlg, IDC_COMBO_TYPEINC, CB_GETCURSEL, 0, 0);
            //if (times == CB_ERR) times = 0;
            //switch (times)
            //{
            //case 0:  times = -1; break;     // 所有
            //case 1:  times =  0; break;     // 当前
            //default: times--;    break;     // X层
            //}
            SendDlgItemMessage(hDlg, IDC_LIST_DIR, LB_GETTEXT, i, (LPARAM)Buffer);
            int len = wcslen(Buffer);
            if (Buffer[len-1] == L'\\')     // 选择分区根目录时最后为反斜杠
                Buffer[len-1] = L'\0';
            fg.m_Filter.SearchDirectory.insert(
                std::make_pair(std::wstring(Buffer), 0/*times*/));
        }
    }
}

bool IsSubString(const wchar_t* child, const wchar_t* parent)
{
    if (!child || !parent)
        return false;

    bool ret;

    while (*child && (ret = *child == *parent))
        ++child, ++parent;

    return ret;
}

bool HashParentDirectoryInList(HWND hList, const wchar_t *dir)
{
    if (!dir)
        return true;

    wchar_t Buf[MAX_PATH];

    int cnt = SendMessage(hList, LB_GETCOUNT, 0, 0);
    for (int i=0; i<cnt; ++i)
    {
        SendMessage(hList, LB_GETTEXT, i, (LPARAM)Buf);
        if (IsSubString(Buf, dir))
            return true;
    }

    return false;
}

static bool Cls_OnCommand_filter(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
    TCHAR Buffer[MAX_PATH];

    switch (id)
    {
    case IDOK:
        UpdateFilterConfig(hDlg, g_FileGroup);

    case IDCANCEL:
        EndDialog(hDlg, 0);
        return TRUE;

    case IDC_BUTTON_ADDDIR:
	    OpenFolder(hDlg, Buffer);
        if (Buffer[0] &&
            LB_ERR == SendDlgItemMessage(hDlg, IDC_LIST_DIR, LB_FINDSTRINGEXACT, -1, (LPARAM)Buffer))
        {
            if (!HashParentDirectoryInList(GetDlgItem(hDlg, IDC_LIST_DIR), Buffer))
            {
                int ans = 0;
                if (LB_ERR != SendDlgItemMessage(hDlg, IDC_LIST_DIR, LB_FINDSTRING, -1, (LPARAM)Buffer))
                    ans = MessageBox(hDlg, L"列表中包含的子文件夹将被剔除", L"提示", MB_YESNO);
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
                MessageBox(hDlg, L"列表中已存在相应的父目录", L"提示", MB_OK | MB_ICONINFORMATION);
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

    // 第三个单选按钮不知怎么没法分到同一组
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
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"最前");
    //SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"最后");

    hCombo = GetDlgItem(hDlg, IDC_COMBO_DATAUNIT);
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"B");
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"KB");
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"MB");
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"GB");
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"TB");
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"PB");
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"EB");

    hCombo = GetDlgItem(hDlg, IDC_COMBO_SIZEUNIT);
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"B");
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"KB");
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"MB");
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"GB");
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"TB");
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"PB");
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"EB");

    hCombo = GetDlgItem(hDlg, IDC_COMBO_SUBDIR);
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"所有子文件夹");
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"当前文件夹");
    for (int i=1; i<100; ++i)
    {
        StringCchPrintf(Buffer, MAX_PATH, L"%d", i);
        SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)Buffer);
    }
    SetDlgItemText(hDlg, IDC_COMBO_SUBDIR, L"所有子文件夹");

    hCombo = GetDlgItem(hDlg, IDC_COMBO_TYPEINC);
    hCombo1 = GetDlgItem(hDlg, IDC_COMBO_TYPEIGN);
    for (UINT i=IDS_STRING102; i<=IDS_STRING107; ++i)
    {
        LoadString(g_hInstance, i, Buffer, MAX_PATH);
        SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)Buffer);
        SendMessage(hCombo1, CB_ADDSTRING, 0, (LPARAM)Buffer);
    }

    SendMessage(hDlg, WM_COMMAND, IDC_CHECK_NAME, 0);
    SendMessage(hDlg, WM_COMMAND, IDC_CHECK_SIZE, 0);
    SendMessage(hDlg, WM_COMMAND, IDC_CHECK_DATE, 0);
    SendMessage(hDlg, WM_COMMAND, IDC_CHECK_DATA, 0);
    LoadFilterConfigToDialog(hDlg, g_FileGroup);
    
    
    // TEST
    SendDlgItemMessage(hDlg, IDC_LIST_DIR, LB_ADDSTRING, 0, 
        (LPARAM)L"E:\\[test]");

    return true;
}

void Cls_OnSysCommand(HWND hwnd, UINT cmd, int x, int y)
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
    }


    return FALSE;
}


DWORD WINAPI Thread(PVOID pvoid)
{

    while (1)
    {
        WaitForSingleObject(g_ThreadSignal, INFINITE);

        // if (...) ExitThread



        g_FileGroup.StartSearchFiles();
    }

    return 0;
}
/*
	// 生成FILEINFO结构的指针以便排序
	if (NULL == (hGlobalI = GlobalAlloc(GPTR, sizeof(DWORD)*dwFileNum))
		|| NULL == (arrdwIndex = (DWORD*)GlobalLock(hGlobalI)))
	{
		MessageBox(0, TEXT("内存分配错误，线程中止！"), 0, MB_ICONERROR);
		ThreadEndRoutine(lpFileInfoList, INVALID_HANDLE_VALUE);
		return 0;
	}

	for (i=0; i<dwFileNum; ++i)
	{
		arrdwIndex[i] = (DWORD)(lpFileInfoList + i*sizeof(FILEINFO));
	}

	if (dwFileNum == 0			// 空文件夹
		|| !(dwMatchedNum = Seclect((PFILEINFO *)arrdwIndex, dwFileNum, pparams->bOptions)))	// 没有匹配文件		
	{
		SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)TEXT("找不到满足条件的文件"));
		ThreadEndRoutine(lpFileInfoList, hGlobalI);
		return 0;
	}
	
	// 计算哈希
	//if (pparams->bOptions & 0x8)	// 需要哈希
	//{
	//	for (i=0;!pparams->bTerminate && i<dwMatchedNum; ++i)
	//	{
	//		StringCbPrintf(szBuffer, 64, TEXT("计算文件哈希(%d/%d)..."), i+1, dwMatchedNum);
	//		SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)szBuffer);
	//		StringCbCopy(szBuffer, MAX_PATH1, ((PFILEINFO)(arrdwIndex[i]))->szPath);
	//		StringCbCat(szBuffer, MAX_PATH1, TEXT("\\"));
	//		StringCbCat(szBuffer, MAX_PATH1, ((PFILEINFO)(arrdwIndex[i]))->wfd.cFileName);
	//		if (((PFILEINFO)(arrdwIndex[i]))->wfd.nFileSizeHigh > pparams->dwHashSizeHigh		// 过滤文件大小
	//			|| ((PFILEINFO)(arrdwIndex[i]))->wfd.nFileSizeLow > pparams->dwHashSizeLow)
	//			continue;
	//		CalculateSha1(szBuffer, ((PFILEINFO)(arrdwIndex[i]))->SHA1, 128, !pparams->bTerminate);
	//	}
	//}

	//if (!pparams->bTerminate)
	//{
	//	StringCbPrintf(szBuffer, 64, TEXT("计算文件哈希(%d/%d)...完成"), dwMatchedNum, dwMatchedNum);
	//	SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)szBuffer);
	//	if (!(pparams->bOptions & 0x7))		// 三个“相同”的选项都没选，按Hash结果筛选相同的文件
	//	{
	//		pparams->bOptions |= 0x10;
	//		Seclect((PFILEINFO *)arrdwIndex, dwFileNum, pparams->bOptions);
	//	}

	//	ListView_DeleteAllItems(hList);
	//	for (i=0; i<dwMatchedNum; ++i)		// 显示
	//	{
	//		InsertListViewItem(hList, (PFILEINFO)arrdwIndex[i], i);
	//	}
	//	SelectSameHash(hList);				// 勾上相同Hash值的文件
	//	
	//	UpdateStatusPart1();
	//}
	//else
	//	SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)TEXT("用户终止"));

	//ThreadEndRoutine(lpFileInfoList, hGlobalI);
	return 0;
}*/



//////////////////////////////////////////////////////////////////////////////////////////////
// 功能  -	右键菜单中哈希选中文件时调用
// pviod -	指向pparams->bOptions的指针，用于终止哈希
//////////////////////////////////////////////////////////////////////////////////////////////
DWORD WINAPI ThreadHashOnly(PVOID pvoid)
{
	//BOOL  * pf = (BOOL *)pvoid;
	//int		i;
	//TCHAR	szBuffer[MAX_PATH1], szBuffer1[MAX_PATH];
    //wchar_t Result[48];
	//SHA_1   Hash;

	//EnableWindow(GetDlgItem(GetParent(hList), IDB_BROWSE), FALSE);
	//EnableWindow(GetDlgItem(GetParent(hList), IDB_SEARCH), FALSE);
	//EnableWindow(GetDlgItem(GetParent(hList), IDB_PAUSE),  FALSE);
	//
	//for (i=ListView_GetItemCount(hList)-1; i>=0; --i)
	//	if (ListView_GetCheckState(hList, i) == TRUE)			// 获取选中的行并哈希
	//	{
	//		ListView_GetItemText(hList, i, 1, szBuffer, MAX_PATH1);
	//		StringCbCat(szBuffer, MAX_PATH1, TEXT("\\"));
	//		ListView_GetItemText(hList, i, 0, szBuffer1, MAX_PATH1);
	//		StringCbCat(szBuffer, MAX_PATH1, szBuffer1);
	//		ListView_SetItemText(hList, i, 5, TEXT("哈希中..."));
	//		Hash.CalculateSha1(szBuffer, Result, MAX_PATH, !(*pf));
	//		ListView_SetItemText(hList, i, 5, Result);
	//	}
	//EnableWindow(GetDlgItem(GetParent(hList), IDB_BROWSE), TRUE);
	//EnableWindow(GetDlgItem(GetParent(hList), IDB_SEARCH), TRUE);
	//EnableWindow(GetDlgItem(GetParent(hList), IDB_PAUSE),  TRUE);
	return 0;
}