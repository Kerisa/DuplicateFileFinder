

#include "functions.h"


extern HWND      g_hList, g_hStatus;
extern HINSTANCE g_hInstance;

extern FileGroup   g_FileGroup, *g_pFileGroup;
extern FileGroupS  g_FileGroupS;
extern FileGroupN  g_FileGroupN;
extern FileGroupSN g_FileGroupSN;


void UpdateStatusBar(int part, const wchar_t *text)
{
    wchar_t szBuffer[64];

    if (part == 0)
    {
        SendMessage(g_hStatus, SB_SETTEXT, 0, (LPARAM)text);
    }
    else if (part == 1)
    {
        StringCbPrintf(szBuffer, sizeof(szBuffer), TEXT("选中文件:%d  /  文件总数:%d"),
            GetListViewCheckedCount(g_hList), ListView_GetItemCount(g_hList));
        SendMessage(g_hStatus, SB_SETTEXT, 1, (LPARAM)szBuffer);
    }
}


void OpenFolder(HWND hwnd, PTSTR pReceive)
{
    BROWSEINFO		bi;
    LPITEMIDLIST	pIDList;
    TCHAR			Buffer[1024];

    assert(pReceive);

    RtlZeroMemory(&bi, sizeof(BROWSEINFO));
    bi.hwndOwner		= hwnd;
    bi.pszDisplayName	= pReceive;
    bi.lpszTitle		= TEXT("选择要分析的文件夹");
    bi.ulFlags			= BIF_NONEWFOLDERBUTTON | BIF_SHAREABLE;

    pIDList	= SHBrowseForFolder(&bi);
    if (pIDList)
    {
        SHGetPathFromIDList(pIDList, Buffer);
        lstrcpy(pReceive, Buffer);
    }
    else
        pReceive[0] = TEXT('\0');
    return;
}


int GetListViewCheckedCount(HWND g_hList)
{
    int i, ret = 0;
    for (i=ListView_GetItemCount(g_hList)-1; i>=0; --i)
    {
        if (ListView_GetCheckState(g_hList, i))
            ++ret;
    }
    return ret;
}


BOOL InitListViewColumns(HWND g_hList)
{
    int			i;
    LVCOLUMN	lvc;
    int			iWidth[6]  = {195, 195, 75, 130, 130, 340};
    TCHAR		szTitle[6][16] = {TEXT("文件名"), TEXT("路径"), TEXT("大小"),
                              TEXT("创建时间"), TEXT("修改时间"), TEXT("哈希值")};
    
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt	 = LVCFMT_LEFT;

    for (i=0; i<2; ++i)
    {
        lvc.iSubItem	= i;
        lvc.cx			= iWidth[i];
        lvc.pszText		= szTitle[i];
        lvc.cchTextMax	= sizeof(lvc.pszText) / sizeof(lvc.pszText[0]);
        if (ListView_InsertColumn(g_hList, i, &lvc) == -1)
            return FALSE;
    }
    lvc.fmt			= LVCFMT_RIGHT;			// 大小右对齐
    lvc.iSubItem	= 2;
    lvc.cx			= iWidth[2];
    lvc.pszText		= szTitle[2];
    lvc.cchTextMax	= sizeof(lvc.pszText) / sizeof(lvc.pszText[0]);
    if (ListView_InsertColumn(g_hList, 2, &lvc) == -1)
        return FALSE;
    lvc.fmt	 = LVCFMT_LEFT;
    for (i=3; i<6; ++i)
    {
        lvc.iSubItem	= i;
        lvc.cx			= iWidth[i];
        lvc.pszText		= szTitle[i];
        lvc.cchTextMax	= sizeof(lvc.pszText) / sizeof(lvc.pszText[0]);
        if (ListView_InsertColumn(g_hList, i, &lvc) == -1)
            return FALSE;
    }
    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////////////
// <DlgProc中调用>
// 功能		- 仅保留结果列表里哈希相同的文件项
// g_hList	- ListView句柄
//////////////////////////////////////////////////////////////////////////////////////////////
int DelDifferentHash(HWND g_hList)
{
    static const int BufSize = 128;
    TCHAR szBuffer[BufSize], szBuffer1[BufSize];
    int iItemCount, iDelCount, i, j;

    iDelCount = 0;
    iItemCount = ListView_GetItemCount(g_hList);

    for (i=j=0; j<iItemCount;)
    {
        j = i + 1;
        ListView_GetItemText(g_hList, i, 5, szBuffer, BufSize);
        if (!szBuffer[0])
        {
            ++i;
            continue;
        }
        do
        {
            if (j == iItemCount)		// 删除最后一个单独的文件
                goto _DELETE;
            ListView_GetItemText(g_hList, j, 5, szBuffer1, BufSize);
        
            if (!lstrcmp(szBuffer, szBuffer1))			// 大小不同哈希必然不同，因此可以删除
                ++j;
            else
            {
                if (j == i+1)
                {
_DELETE:			ListView_DeleteItem(g_hList, i);
                    ++iDelCount;
                    --j;
                    --iItemCount;
                }
                else
                    i = j;
                break;
            }
        }while (j<iItemCount);
    }
    return iDelCount;
}


int SelectSameHash(HWND g_hList)
{
    static const int BufSize = 128;

    TCHAR szBuffer[BufSize], szBuffer1[BufSize];
    int i, iItemCount, iCheckCount;

    iItemCount = ListView_GetItemCount(g_hList);
    for (i=0; i<iItemCount; ++i)
        ListView_SetCheckState(g_hList, i, FALSE);

    for (i=0,iCheckCount=0; i<iItemCount-1;)
    {
        ListView_GetItemText(g_hList, i, 5, szBuffer, BufSize);
        ListView_GetItemText(g_hList, i+1, 5, szBuffer1, BufSize);
        if (!szBuffer[0] || !szBuffer1[0] || lstrcmp(szBuffer, szBuffer1))
        {
            ++i;
            continue;
        }	
        do
        {
            ListView_SetCheckState(g_hList, ++i, TRUE);
            ++iCheckCount;
            ListView_GetItemText(g_hList, i+1, 5, szBuffer1, BufSize);
        }while(i<iItemCount-1 && !lstrcmp(szBuffer, szBuffer1));
    }
    return iCheckCount;
}


void pInsertListViewItem(HWND g_hList, FileGroup::pFileInfo pfi, std::wstring* phash, int groupid, DWORD index)			// 分组显示 : LVIF_GROUPID
{
    TCHAR		Buffer[128];
    LVITEM		lvI;

    RtlZeroMemory(&lvI, sizeof(LVITEM));
    lvI.iItem		= index;
    lvI.mask		= LVIF_TEXT | LVIF_GROUPID;
    lvI.cchTextMax	= 1024;
    lvI.iGroupId    = groupid;

    lvI.iSubItem = 0;
    lvI.pszText  = const_cast<wchar_t*>(pfi->Name.c_str());
    ListView_InsertItem(g_hList, &lvI);                           // 先对subitem=0使用InsertItem


    ListView_SetItemText(g_hList, 0, 1, const_cast<wchar_t*>(pfi->Path.c_str()));
    

    if (pfi->Size > 966367641)		// 约为0.9GB
    {
        StringCbPrintf(Buffer, 120, TEXT("%.2lf GB"), pfi->Size/(double)0x40000000);
    }
    else if (pfi->Size > 943718)	// 0.9MB
    {
        StringCbPrintf(Buffer, 120, TEXT("%.2lf MB"), pfi->Size / (double)0x100000);
    }
    else if (pfi->Size > 921)        // 0.9KB
    {
        StringCbPrintf(Buffer, 120, TEXT("%.2lf KB"), pfi->Size / (double)0x400);
    }
    else
    {
        StringCbPrintf(Buffer, 120, TEXT("%d B"), pfi->Size);
    }
    ListView_SetItemText(g_hList, 0, 2, Buffer);


    SYSTEMTIME st;
    FILETIME ft;
    
    FileTimeToLocalFileTime(&pfi->CreationTime, &ft);
    FileTimeToSystemTime(&ft, &st);
    StringCbPrintf(Buffer, 120, TEXT("%04d/%02d/%02d %02d:%02d:%02d"),
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    ListView_SetItemText(g_hList, 0, 3, Buffer);


    FileTimeToLocalFileTime(&pfi->LastWriteTime, &ft);
    FileTimeToSystemTime(&ft, &st);
    StringCbPrintf(Buffer, 120, TEXT("%04d/%02d/%02d %02d:%02d:%02d"),
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    ListView_SetItemText(g_hList, 0, 4, Buffer);


    ListView_SetItemText(g_hList, 0, 5, phash ? (LPTSTR)phash->c_str() : 0);
}


void InsertListViewItem(FileGroup::pFileInfo pfi, int groupid, std::wstring* phash)
{
    pInsertListViewItem(g_hList, pfi, phash, groupid, 0);
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


bool IsSubString(const wchar_t* child, const wchar_t* parent, int pos)
{
    if (!child || !parent)
        return false;

    bool ret;

    while (*child && (ret = *child == *parent))
        ++child, ++parent;

    return ret;
}


bool HasParentDirectoryInList(HWND g_hList, const wchar_t *dir)
{
    if (!dir)
        return true;

    wchar_t Buf[MAX_PATH];

    int cnt = SendMessage(g_hList, LB_GETCOUNT, 0, 0);
    for (int i=0; i<cnt; ++i)
    {
        SendMessage(g_hList, LB_GETTEXT, i, (LPARAM)Buf);
        if (IsSubString(Buf, dir))
            return true;
    }

    return false;
}


void EnableControls(HWND hDlg, int id, bool enable)
{
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
        EnableWindow(GetDlgItem(hDlg, IDC_CHECK_ATTR_N), enable);
        break;

    default:
        assert(0);
        break;
    }
}


void DetermineStructType(HWND hDlg)
{
    bool name = IsDlgButtonChecked(hDlg, IDC_CHECK_NAME);
    bool size = IsDlgButtonChecked(hDlg, IDC_CHECK_SIZE);
    bool date = IsDlgButtonChecked(hDlg, IDC_CHECK_DATE);

    if (name && (size || date))
        g_pFileGroup = &g_FileGroupSN;

    else if (!name && (size || date))
        g_pFileGroup = &g_FileGroupS;

    else if (name && !size && !date)
        g_pFileGroup = &g_FileGroupN;

    // 只选中文件内容完全相同的话先按大小筛
    else if (!name && !size && !date &&
        IsDlgButtonChecked(hDlg, IDC_RADIO_DATAEQUAL))
        g_pFileGroup = &g_FileGroupS;
    else
        g_pFileGroup = &g_FileGroup;
}


void LoadFilterConfigToDialog(HWND hDlg, FileGroup& fg)
{
    TCHAR Buffer[MAX_PATH];

    //------------------------------------------- 文件名

    if (fg.m_Filter.Switch[Filter::Compare_FileName]
        == Filter::Type_Off)
        EnableControls(hDlg, IDC_CHECK_NAME, false);
    else
    {
        CheckDlgButton(hDlg, IDC_CHECK_NAME, 1);
        EnableControls(hDlg, IDC_CHECK_NAME, true);

        switch (fg.m_Filter.Switch[Filter::Compare_FileName])
        {
        case Filter::Type_Whole:
            for (int i=IDC_RADIO_FULLNAME; i<=IDC_RADIO_PARTNAME; ++i)
                CheckDlgButton(hDlg, i, i==IDC_RADIO_FULLNAME);
            
            CheckDlgButton(hDlg, IDC_CHECK_NOSUFFIX,
                fg.m_Filter.FileNameWithoutSuffix);

            break;

        case Filter::Type_RangeMatch:
            for (int i=IDC_RADIO_FULLNAME; i<=IDC_RADIO_PARTNAME; ++i)
                CheckDlgButton(hDlg, i, i==IDC_RADIO_PARTNAME);

            StringCchPrintf(Buffer, MAX_PATH, L"%d",
                fg.m_Filter.FileNameRange.LowerBound);
            SetDlgItemText(hDlg, IDC_EDIT_NAMEBEG, Buffer);

            StringCchPrintf(Buffer, MAX_PATH, L"%d",
                fg.m_Filter.FileNameRange.UpperBound);
            SetDlgItemText(hDlg, IDC_EDIT_NAMEEND, Buffer);

            break;

        case Filter::Type_Include:
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

    if (fg.m_Filter.Switch[Filter::Compare_FileSize]
        == Filter::Type_Off)
        EnableControls(hDlg, IDC_CHECK_SIZE, false);
    else
    {
        CheckDlgButton(hDlg, IDC_CHECK_SIZE, 1);
        EnableControls(hDlg, IDC_CHECK_SIZE, true);
        CheckDlgButton(hDlg, IDC_RADIO_SIZEEQUAL, true);
    }

    //------------------------------------------- 文件修改日期

    int status;
    if ((status = fg.m_Filter.Switch[Filter::Compare_FileDate])
        == Filter::Type_Off)
        EnableControls(hDlg, IDC_CHECK_DATE, false);
    else
    {
        CheckDlgButton(hDlg, IDC_CHECK_DATE, 1);
        EnableControls(hDlg, IDC_CHECK_DATE, true);

        CheckDlgButton(hDlg, IDC_CHECK_YEAR,  fg.m_Filter.SelectedDate & Filter::Time_Year);
        CheckDlgButton(hDlg, IDC_CHECK_MONTH, fg.m_Filter.SelectedDate & Filter::Time_Month);
        CheckDlgButton(hDlg, IDC_CHECK_DAY,   fg.m_Filter.SelectedDate & Filter::Time_Day);
        CheckDlgButton(hDlg, IDC_CHECK_WEEK,  fg.m_Filter.SelectedDate & Filter::Time_Week);
    }

    //------------------------------------------- 文件内容

    if ((status = fg.m_Filter.Switch[Filter::Compare_FileHash])
        == Filter::Type_Off)
        EnableControls(hDlg, IDC_CHECK_DATA, false);
    else
    {
        CheckDlgButton(hDlg, IDC_CHECK_DATA, 1);
        EnableControls(hDlg, IDC_CHECK_DATA, true);

        if (status == Filter::Type_Whole)
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

            
            SendDlgItemMessage(hDlg, IDC_COMBO_DATAFRONT, CB_SETCURSEL,
                fg.m_Filter.FileDataRange.Inverted, 0);
        }
    }

    //------------------------------------------- 包含的扩展名

    if ((status = fg.m_Filter.Switch[Filter::Search_IncludeSuffix])
        == Filter::Type_Off) 
        SetDlgItemText(hDlg, IDC_COMBO_TYPEINC, L"");
    
    else
    {
        std::vector<std::wstring>::iterator it = fg.m_Filter.ContainSuffix.begin();
        if (it != fg.m_Filter.ContainSuffix.end())
        {
            std::wstring str = std::wstring((*it++));
            for (; it != fg.m_Filter.ContainSuffix.end(); ++it)
                (str += L" | ") += (*it);
            SetDlgItemText(hDlg, IDC_COMBO_TYPEINC, str.c_str());
        }
    }

    //------------------------------------------- 忽视的扩展名

    if ((status = fg.m_Filter.Switch[Filter::Search_IgnoreSuffix])
        == Filter::Type_Off) 
        SetDlgItemText(hDlg, IDC_COMBO_TYPEIGN, L"");
    
    else
    {
        std::vector<std::wstring>::iterator it = fg.m_Filter.IgnoreSuffix.begin();
        if (it != fg.m_Filter.IgnoreSuffix.end())
        {
            std::wstring str = std::wstring((*it++));
            for (; it != fg.m_Filter.IgnoreSuffix.end(); ++it)
                (str += L" | ") += (*it);
            SetDlgItemText(hDlg, IDC_COMBO_TYPEIGN, str.c_str());
        }
    }

    //------------------------------------------- 查找的文件大小范围

    if ((status = fg.m_Filter.Switch[Filter::Search_FileSize])
        == Filter::Type_Off)
        EnableControls(hDlg, IDC_CHECK_SIZERANGE, false);
    else
    {
        CheckDlgButton(hDlg, IDC_CHECK_SIZERANGE, 1);
        EnableControls(hDlg, IDC_CHECK_SIZERANGE, true);

        CheckDlgButton(hDlg, IDC_RADIO_SIZERANGE_INC,
            status == Filter::Type_Include);
        CheckDlgButton(hDlg, IDC_RADIO_SIZERANGE_IGN,
            status == Filter::Type_Ignore);

        StringCchPrintf(Buffer, MAX_PATH, L"%d",
            fg.m_Filter.FileSizeRange.LowerBound);
        SetDlgItemText(hDlg, IDC_EDIT_SIZEBEG, Buffer);

        StringCchPrintf(Buffer, MAX_PATH, L"%d",
            fg.m_Filter.FileSizeRange.UpperBound);
        SetDlgItemText(hDlg, IDC_EDIT_SIZEEND, Buffer);

        // 单位
    }

    //------------------------------------------- 查找的文件属性

    if ((status = fg.m_Filter.Switch[Filter::Search_FileAttribute])
        == Filter::Type_Off)
        EnableControls(hDlg, IDC_CHECK_ATTRIB, false);
    else
    {
        CheckDlgButton(hDlg, IDC_CHECK_ATTRIB, 1);
        EnableControls(hDlg, IDC_CHECK_ATTRIB, true);

        CheckDlgButton(hDlg, IDC_RADIO_ATTRIB_INC,
            status == Filter::Type_Include);
        CheckDlgButton(hDlg, IDC_RADIO_ATTRIB_IGN,
            status == Filter::Type_Ignore);

        CheckDlgButton(hDlg, IDC_CHECK_ATTR_A, fg.m_Filter.SelectedAttributes & Filter::Attrib_Archive);
        CheckDlgButton(hDlg, IDC_CHECK_ATTR_R, fg.m_Filter.SelectedAttributes & Filter::Attrib_ReadOnly);
        CheckDlgButton(hDlg, IDC_CHECK_ATTR_S, fg.m_Filter.SelectedAttributes & Filter::Attrib_System);
        CheckDlgButton(hDlg, IDC_CHECK_ATTR_H, fg.m_Filter.SelectedAttributes & Filter::Attrib_Hide);
        CheckDlgButton(hDlg, IDC_CHECK_ATTR_N, fg.m_Filter.SelectedAttributes & Filter::Attrib_Normal);
    }

    //------------------------------------------- 查找的目录

    std::map<std::wstring, int>::iterator it;
    std::wstring str;
    for (it=fg.m_Filter.SearchDirectory.begin(); it!=fg.m_Filter.SearchDirectory.end(); ++it)
    {
        str.clear();
        str += it->first;
        SendDlgItemMessage(hDlg, IDC_LIST_DIR, LB_ADDSTRING, 0, (LPARAM)str.c_str());
    }

}


void UpdateFilterConfig(HWND hDlg, FileGroup& fg)
{
    TCHAR Buffer[MAX_PATH];

    //------------------------------------------- 文件名

    if (!IsDlgButtonChecked(hDlg, IDC_CHECK_NAME))
        fg.m_Filter.Switch[Filter::Compare_FileName] =
            Filter::Type_Off;
    else
    {
        if (IsDlgButtonChecked(hDlg, IDC_RADIO_FULLNAME))
        {
            fg.m_Filter.Switch[Filter::Compare_FileName] =
                Filter::Type_Whole;
            fg.m_Filter.FileNameWithoutSuffix =
                IsDlgButtonChecked(hDlg, IDC_CHECK_NOSUFFIX);
        }
        else if (IsDlgButtonChecked(hDlg, IDC_RADIO_PARTNAME))
        {
            fg.m_Filter.Switch[Filter::Compare_FileName] =
                Filter::Type_RangeMatch;
            GetDlgItemText(hDlg, IDC_EDIT_NAMEBEG, Buffer, MAX_PATH);
            fg.m_Filter.FileNameRange.LowerBound = _wtoi(Buffer);
            GetDlgItemText(hDlg, IDC_EDIT_NAMEEND, Buffer, MAX_PATH);
            fg.m_Filter.FileNameRange.UpperBound = _wtoi(Buffer);
        }
        else if (IsDlgButtonChecked(hDlg, IDC_RADIO_INCNAME))
        {
            fg.m_Filter.Switch[Filter::Compare_FileName] =
                Filter::Type_Include;
            GetDlgItemText(hDlg, IDC_EDIT_NAMEINC, Buffer, MAX_PATH);
            fg.m_Filter.KeyWordOfFileName = Buffer;
        }
    }

    //------------------------------------------- 文件大小

    if (!IsDlgButtonChecked(hDlg, IDC_CHECK_SIZE))
        fg.m_Filter.Switch[Filter::Compare_FileSize] =
            Filter::Type_Off;
    else
    {
        fg.m_Filter.Switch[Filter::Compare_FileSize] =
            Filter::Type_Whole;
    }

    //------------------------------------------- 修改日期

    if (!IsDlgButtonChecked(hDlg, IDC_CHECK_DATE))
        fg.m_Filter.Switch[Filter::Compare_FileDate] =
            Filter::Type_Off;
    else
    {
        fg.m_Filter.Switch[Filter::Compare_FileDate] =
            Filter::Type_On;
        
        fg.m_Filter.SelectedDate = 0;
        if (IsDlgButtonChecked(hDlg, IDC_CHECK_YEAR))
            fg.m_Filter.SelectedDate |= Filter::Time_Year;
        if (IsDlgButtonChecked(hDlg, IDC_CHECK_MONTH))
            fg.m_Filter.SelectedDate |= Filter::Time_Month;
        if (IsDlgButtonChecked(hDlg, IDC_CHECK_DAY))
            fg.m_Filter.SelectedDate |= Filter::Time_Day;
        if (IsDlgButtonChecked(hDlg, IDC_CHECK_WEEK))
            fg.m_Filter.SelectedDate |= Filter::Time_Week;
    }

    //------------------------------------------- 文件内容

    if (!IsDlgButtonChecked(hDlg, IDC_CHECK_DATA))
        fg.m_Filter.Switch[Filter::Compare_FileHash] =
            Filter::Type_Off;
    else
    {
        if (IsDlgButtonChecked(hDlg, IDC_RADIO_DATAEQUAL))
        {
            fg.m_Filter.Switch[Filter::Compare_FileHash] =
                Filter::Type_Whole;
            fg.m_Filter.Switch[Filter::Compare_FileSize] =  // 附上文件大小
                Filter::Type_Whole;
        }
        else
        {
            fg.m_Filter.Switch[Filter::Compare_FileHash] =
                Filter::Type_RangeMatch;

            GetDlgItemText(hDlg, IDC_EDIT_DATABEG, Buffer, MAX_PATH);
            fg.m_Filter.FileDataRange.LowerBound = _wtoi(Buffer);
            GetDlgItemText(hDlg, IDC_EDIT_DATAEND, Buffer, MAX_PATH);
            fg.m_Filter.FileDataRange.UpperBound = _wtoi(Buffer);

            int idx = SendDlgItemMessage(hDlg, IDC_COMBO_DATAFRONT, CB_GETCURSEL, 0, 0);
            fg.m_Filter.FileDataRange.Inverted = (bool)idx;
        }
    }

    //------------------------------------------- 包含类型

    fg.m_Filter.ContainSuffix.clear();
    fg.m_Filter.Switch[Filter::Search_IncludeSuffix] =
                Filter::Type_Off;

    int idx;
    if ((idx = SendDlgItemMessage(hDlg, IDC_COMBO_TYPEINC, CB_GETCURSEL, 0, 0)) != CB_ERR)
    {
        LoadString(g_hInstance, idx+IDS_STRING103, Buffer, MAX_PATH);
        SplitString(Buffer, fg.m_Filter.ContainSuffix, L"|", L'(', L')', true);
        fg.m_Filter.Switch[Filter::Search_IncludeSuffix] =
                Filter::Type_On;
    }
    else if (idx == CB_ERR)
    {
        GetDlgItemText(hDlg, IDC_COMBO_TYPEINC, Buffer, MAX_PATH);
        SplitString(Buffer, fg.m_Filter.ContainSuffix, L"|", 0, 0, true);
        if (fg.m_Filter.ContainSuffix.size() > 0)
            fg.m_Filter.Switch[Filter::Search_IncludeSuffix] =
                Filter::Type_On;
    }
    

    //------------------------------------------- 忽略类型

    fg.m_Filter.IgnoreSuffix.clear();
    fg.m_Filter.Switch[Filter::Search_IgnoreSuffix] = Filter::Type_Off;

    if ((idx = SendDlgItemMessage(hDlg, IDC_COMBO_TYPEIGN, CB_GETCURSEL, 0, 0)) != CB_ERR)
    {
        LoadString(g_hInstance, idx+IDS_STRING103, Buffer, MAX_PATH);
        SplitString(Buffer, fg.m_Filter.IgnoreSuffix, L"|", L'(', L')', true);
        fg.m_Filter.Switch[Filter::Search_IgnoreSuffix] = Filter::Type_On;
    }
    else if (idx == CB_ERR)
    {
        GetDlgItemText(hDlg, IDC_COMBO_TYPEIGN, Buffer, MAX_PATH);
        SplitString(Buffer, fg.m_Filter.IgnoreSuffix, L"|", 0, 0, true);
        if (fg.m_Filter.IgnoreSuffix.size() > 0)
            fg.m_Filter.Switch[Filter::Search_IgnoreSuffix] = Filter::Type_On;
    }


    //------------------------------------------- 文件大小范围

    if (!IsDlgButtonChecked(hDlg, IDC_CHECK_SIZERANGE))
        fg.m_Filter.Switch[Filter::Search_FileSize] =
            Filter::Type_Off;
    else
    {
        fg.m_Filter.Switch[Filter::Search_FileSize] =
            (IsDlgButtonChecked(hDlg, IDC_RADIO_SIZERANGE_INC) ?
                Filter::Type_Include :
                Filter::Type_Ignore);
        
        GetDlgItemText(hDlg, IDC_EDIT_SIZEBEG, Buffer, MAX_PATH);
        fg.m_Filter.FileSizeRange.LowerBound = _wtoi(Buffer);
        GetDlgItemText(hDlg, IDC_EDIT_SIZEEND, Buffer, MAX_PATH);
        fg.m_Filter.FileSizeRange.UpperBound = _wtoi(Buffer);
    }

    //------------------------------------------- 文件属性

    if (!IsDlgButtonChecked(hDlg, IDC_CHECK_ATTRIB))
        fg.m_Filter.Switch[Filter::Search_FileAttribute] =
            Filter::Type_Off;
    else
    {
        fg.m_Filter.Switch[Filter::Search_FileAttribute] =
            (IsDlgButtonChecked(hDlg, IDC_RADIO_ATTRIB_INC) ?
                Filter::Type_Include :
                Filter::Type_Ignore);
        
        fg.m_Filter.SelectedAttributes = 0;

        fg.m_Filter.SelectedAttributes |= IsDlgButtonChecked(hDlg, IDC_CHECK_ATTR_A) ?
            Filter::Attrib_Archive : 0;
        fg.m_Filter.SelectedAttributes |= IsDlgButtonChecked(hDlg, IDC_CHECK_ATTR_R) ?
            Filter::Attrib_ReadOnly : 0;
        fg.m_Filter.SelectedAttributes |= IsDlgButtonChecked(hDlg, IDC_CHECK_ATTR_S) ?
            Filter::Attrib_System : 0;
        fg.m_Filter.SelectedAttributes |= IsDlgButtonChecked(hDlg, IDC_CHECK_ATTR_H) ?
            Filter::Attrib_Hide : 0;
        fg.m_Filter.SelectedAttributes |= IsDlgButtonChecked(hDlg, IDC_CHECK_ATTR_N) ?
            Filter::Attrib_Normal : 0;
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
                std::make_pair(std::wstring(Buffer), -1/*times*/));
        }
    }
}
