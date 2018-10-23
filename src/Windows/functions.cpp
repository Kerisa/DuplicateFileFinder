

#include "functions.h"


extern HWND      g_hList, g_hStatus;
extern HINSTANCE g_hInstance;


std::pair<wchar_t*, uint64_t> szUnit[4] = {
    { L"B", 1, },
    { L"KB", 1024, },
    { L"MB", 1024 * 1024, },
    { L"GB", 1024 * 1024 * 1024, },
};


void UpdateStatusBar(int part, const wchar_t *text)
{
    wchar_t szBuffer[64];

    if (part == 0)
    {
        SendMessage(g_hStatus, SB_SETTEXT, 0, (LPARAM)text);
    }
    else if (part == 1)
    {
        if (!text)
        {
            StringCbPrintf(szBuffer, sizeof(szBuffer), TEXT("选中文件:%d  /  文件总数:%d"),
                GetListViewCheckedCount(g_hList), ListView_GetItemCount(g_hList));
            SendMessage(g_hStatus, SB_SETTEXT, 1, (LPARAM)szBuffer);
        }
        else
            SendMessage(g_hStatus, SB_SETTEXT, 1, (LPARAM)text);
    }
}


std::wstring OpenFolder(HWND hwnd)
{
    BROWSEINFO			bi;
    LPITEMIDLIST		pIDList;
    std::vector<TCHAR>	Buffer(1024);
	
    RtlZeroMemory(&bi, sizeof(BROWSEINFO));
    bi.hwndOwner		= hwnd;
    bi.pszDisplayName	= Buffer.data();
    bi.lpszTitle		= TEXT("选择要分析的文件夹");
    bi.ulFlags			= BIF_NONEWFOLDERBUTTON | BIF_SHAREABLE;

    pIDList	= SHBrowseForFolder(&bi);
    if (pIDList)
    {
        SHGetPathFromIDList(pIDList, Buffer.data());
        //lstrcpy(pReceive, Buffer);
    }
	else
	{
		//pReceive[0] = TEXT('\0');
		Buffer[0] = L'\0';
	}
    return Buffer.data();
}


int GetListViewCheckedCount(HWND g_hList)
{
    int ret = 0;
    for (auto i = g_DataBase.begin(); i != g_DataBase.end(); ++i)
        ret += i->checkstate == CHECKBOX_SECLECTED;
    
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

bool IsValidHash(int index)
{
    int k = 0;
    while (k < SHA_1::SHA1_HASH_SIZE && !g_DataBase[index].hr.b[k]) ++k;
    return k != SHA_1::SHA1_HASH_SIZE;
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



bool HasParentDirectoryInList(HWND g_hList, const std::wstring& dir)
{
    if (dir.empty())
        return true;

    wchar_t Buf[MAX_PATH];

    int cnt = SendMessage(g_hList, LB_GETCOUNT, 0, 0);
    for (int i=0; i<cnt; ++i)
    {
        SendMessage(g_hList, LB_GETTEXT, i, (LPARAM)Buf);
		if (dir.find(Buf) != std::wstring::npos)
        {
			//if (dir.back() == L'\\')  // 是文件路径形式而不是其他符号
                return true;
        }
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
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_INCNAME), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_NAMEINC), enable);
        break;

    case IDC_CHECK_SIZE:
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_SIZEEQUAL), enable);
        break;

    case IDC_CHECK_DATE:
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_DATEEQUAL), enable);
        break;

    case IDC_CHECK_DATA:
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_DATAEQUAL), enable);
        break;

    case IDC_CHECK_SIZERANGE:
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_SIZERANGE_INC), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_SIZERANGE_IGN), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SIZEBEG), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SIZEEND), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_COMBO_SIZEUNIT), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_COMBO_SEARCH_SIZE_UNIT1), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_COMBO_SEARCH_SIZE_UNIT2), enable);
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

	case IDC_CHECK_FILE_TYPE:
		EnableWindow(GetDlgItem(hDlg, IDC_RADIO_TYPE_INC), enable);
		EnableWindow(GetDlgItem(hDlg, IDC_RADIO_TYPE_IGN), enable);
		EnableWindow(GetDlgItem(hDlg, IDC_COMBO_TYPEINC), enable);
		break;

    default:
        assert(0);
        break;
    }
}

void LoadFilterConfigToDialog(HWND hDlg, Filter& filter)
{
    TCHAR Buffer[MAX_PATH];

    //------------------------------------------- 文件名

    if (filter.Switch[Filter::Compare_FileName] == Filter::Type_Off)
        EnableControls(hDlg, IDC_CHECK_NAME, false);
    else
    {
        CheckDlgButton(hDlg, IDC_CHECK_NAME, 1);
        EnableControls(hDlg, IDC_CHECK_NAME, true);

        switch (filter.Switch[Filter::Compare_FileName])
        {
        case Filter::Type_Whole:
            for (int i=IDC_RADIO_FULLNAME; i<=IDC_RADIO_PARTNAME; ++i)
                CheckDlgButton(hDlg, i, i==IDC_RADIO_FULLNAME);
            
            CheckDlgButton(hDlg, IDC_CHECK_NOSUFFIX, filter.FileNameWithoutSuffix);
            break;

        case Filter::Type_Include:
            for (int i=IDC_RADIO_FULLNAME; i<=IDC_RADIO_PARTNAME; ++i)
                CheckDlgButton(hDlg, i, i==IDC_RADIO_INCNAME);

            SetDlgItemText(hDlg, IDC_EDIT_NAMEINC, filter.mKeyWordOfFileName.c_str());
            break;

        default:
            assert(0);
            break;
        }
    }

    //------------------------------------------- 文件大小

    if (filter.Switch[Filter::Compare_FileSize] == Filter::Type_Off)
        EnableControls(hDlg, IDC_CHECK_SIZE, false);
    else
    {
        CheckDlgButton(hDlg, IDC_CHECK_SIZE, 1);
        EnableControls(hDlg, IDC_CHECK_SIZE, true);
        CheckDlgButton(hDlg, IDC_RADIO_SIZEEQUAL, true);
    }

    //------------------------------------------- 文件修改日期

    int status;
    if ((status = filter.Switch[Filter::Compare_FileDate]) == Filter::Type_Off)
        EnableControls(hDlg, IDC_CHECK_DATE, false);
    else
    {
        CheckDlgButton(hDlg, IDC_CHECK_DATE, 1);
		EnableControls(hDlg, IDC_CHECK_DATE, true);
		CheckDlgButton(hDlg, IDC_RADIO_DATEEQUAL, true);
    }

    //------------------------------------------- 文件内容

    if ((status = filter.Switch[Filter::Compare_FileHash]) == Filter::Type_Off)
        EnableControls(hDlg, IDC_CHECK_DATA, false);
    else
    {
        CheckDlgButton(hDlg, IDC_CHECK_DATA, 1);
		EnableControls(hDlg, IDC_CHECK_DATA, true);
		CheckDlgButton(hDlg, IDC_RADIO_DATAEQUAL, true);

		assert((status == Filter::Type_Whole));
		for (int i = IDC_RADIO_DATAEQUAL; i <= IDC_RADIO_PARTDATAEQUAL; ++i)
			CheckDlgButton(hDlg, i, i == IDC_RADIO_DATAEQUAL);
    }

    //------------------------------------------- 包含的扩展名

	if ((status = filter.Switch[Filter::Search_Suffix]) == Filter::Type_Off)
	{
		SetDlgItemText(hDlg, IDC_COMBO_TYPEINC, L"");
		EnableControls(hDlg, IDC_CHECK_FILE_TYPE, false);
		CheckDlgButton(hDlg, IDC_CHECK_FILE_TYPE, FALSE);
	}
    else
    {
		EnableControls(hDlg, IDC_CHECK_FILE_TYPE, true);
		CheckDlgButton(hDlg, IDC_CHECK_FILE_TYPE, TRUE);
		CheckDlgButton(hDlg, IDC_RADIO_TYPE_INC, status == Filter::Type_Include);
		CheckDlgButton(hDlg, IDC_RADIO_TYPE_IGN, status != Filter::Type_Include);
        if (!filter.mSuffix.empty())
        {
			auto it = filter.mSuffix.begin();
            std::wstring str = std::wstring((*it++));
            for (; it != filter.mSuffix.end(); ++it)
                (str += L" | ") += (*it);
            SetDlgItemText(hDlg, IDC_COMBO_TYPEINC, str.c_str());
        }
    }

    //------------------------------------------- 查找的文件大小范围

    if ((status = filter.Switch[Filter::Search_FileSize]) == Filter::Type_Off)
        EnableControls(hDlg, IDC_CHECK_SIZERANGE, false);
    else
    {
        CheckDlgButton(hDlg, IDC_CHECK_SIZERANGE, 1);
        EnableControls(hDlg, IDC_CHECK_SIZERANGE, true);

        CheckDlgButton(hDlg, IDC_RADIO_SIZERANGE_INC, status == Filter::Type_Include);
        CheckDlgButton(hDlg, IDC_RADIO_SIZERANGE_IGN, status == Filter::Type_Ignore);

        StringCchPrintf(Buffer, MAX_PATH, L"%llu", filter.FileSizeRangeLow / szUnit[filter.FileSizeRangeLowUnitIndex].second);
        SetDlgItemText(hDlg, IDC_EDIT_SIZEBEG, Buffer);
        SendDlgItemMessage(hDlg, IDC_COMBO_SEARCH_SIZE_UNIT1, CB_SETCURSEL, (WPARAM)filter.FileSizeRangeLowUnitIndex, 0);

        StringCchPrintf(Buffer, MAX_PATH, L"%llu", filter.FileSizeRangeHigh / szUnit[filter.FileSizeRangeHighUnitIndex].second);
        SetDlgItemText(hDlg, IDC_EDIT_SIZEEND, Buffer);
        SendDlgItemMessage(hDlg, IDC_COMBO_SEARCH_SIZE_UNIT2, CB_SETCURSEL, (WPARAM)filter.FileSizeRangeHighUnitIndex, 0);

        // 单位
    }

    //------------------------------------------- 查找的文件属性

    if ((status = filter.Switch[Filter::Search_FileAttribute])
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

        assert(filter.SelectedAttributes == Filter::Attrib_Normal || !(filter.SelectedAttributes & Filter::Attrib_Normal));
        CheckDlgButton(hDlg, IDC_CHECK_ATTR_A, filter.SelectedAttributes & Filter::Attrib_Archive);
        CheckDlgButton(hDlg, IDC_CHECK_ATTR_R, filter.SelectedAttributes & Filter::Attrib_ReadOnly);
        CheckDlgButton(hDlg, IDC_CHECK_ATTR_S, filter.SelectedAttributes & Filter::Attrib_System);
        CheckDlgButton(hDlg, IDC_CHECK_ATTR_H, filter.SelectedAttributes & Filter::Attrib_Hide);
        CheckDlgButton(hDlg, IDC_CHECK_ATTR_N, filter.SelectedAttributes & Filter::Attrib_Normal);
    }

    //------------------------------------------- 查找的目录

    std::map<std::wstring, int>::iterator it;
    std::wstring str;
    for (it=filter.mSearchDirectory.begin(); it!=filter.mSearchDirectory.end(); ++it)
    {
        str.clear();
        str += it->first;
        SendDlgItemMessage(hDlg, IDC_LIST_DIR, LB_ADDSTRING, 0, (LPARAM)str.c_str());
    }

}


void UpdateFilterConfig(HWND hDlg, Filter& filter)
{
    TCHAR Buffer[MAX_PATH];
	int idx;

    //------------------------------------------- 文件名

    if (!IsDlgButtonChecked(hDlg, IDC_CHECK_NAME))
        filter.Switch[Filter::Compare_FileName] =
            Filter::Type_Off;
    else
    {
        if (IsDlgButtonChecked(hDlg, IDC_RADIO_FULLNAME))
        {
            filter.Switch[Filter::Compare_FileName] =
                Filter::Type_Whole;
            filter.FileNameWithoutSuffix =
                IsDlgButtonChecked(hDlg, IDC_CHECK_NOSUFFIX);
        }
        else if (IsDlgButtonChecked(hDlg, IDC_RADIO_INCNAME))
        {
            filter.Switch[Filter::Compare_FileName] =
                Filter::Type_Include;
            GetDlgItemText(hDlg, IDC_EDIT_NAMEINC, Buffer, MAX_PATH);
            filter.mKeyWordOfFileName = Buffer;
        }
    }

    //------------------------------------------- 文件大小

    filter.Switch[Filter::Compare_FileSize] = IsDlgButtonChecked(hDlg, IDC_CHECK_SIZE) ? Filter::Type_Whole : Filter::Type_Off;

    //------------------------------------------- 修改日期

    filter.Switch[Filter::Compare_FileDate] = IsDlgButtonChecked(hDlg, IDC_CHECK_DATE) ? Filter::Type_On : Filter::Type_Off;

    //------------------------------------------- 文件内容

    filter.Switch[Filter::Compare_FileHash] = IsDlgButtonChecked(hDlg, IDC_CHECK_DATA) ? Filter::Type_Whole : Filter::Type_Off;

    //------------------------------------------- 包含类型

    filter.mSuffix.clear();
    filter.Switch[Filter::Search_Suffix] = Filter::Type_Off;
	
	if (!IsDlgButtonChecked(hDlg, IDC_CHECK_FILE_TYPE))
		filter.Switch[Filter::Search_Suffix] = Filter::Type_Off;
	else
	{
		if (IsDlgButtonChecked(hDlg, IDC_RADIO_TYPE_INC))
			filter.Switch[Filter::Search_Suffix] = Filter::Type_Include;
		else if (IsDlgButtonChecked(hDlg, IDC_RADIO_TYPE_IGN))
			filter.Switch[Filter::Search_Suffix] = Filter::Type_Ignore;
		else
			assert(0);

		if ((idx = SendDlgItemMessage(hDlg, IDC_COMBO_TYPEINC, CB_GETCURSEL, 0, 0)) != CB_ERR)
		{
			LoadString(g_hInstance, idx + IDS_STRING103, Buffer, MAX_PATH);
			SplitString(Buffer, filter.mSuffix, L"|", L'(', L')', true);
		}
		else if (idx == CB_ERR)
		{
			GetDlgItemText(hDlg, IDC_COMBO_TYPEINC, Buffer, MAX_PATH);
			SplitString(Buffer, filter.mSuffix, L"|", 0, 0, true);
		}
	}

    //------------------------------------------- 文件大小范围

    if (!IsDlgButtonChecked(hDlg, IDC_CHECK_SIZERANGE))
        filter.Switch[Filter::Search_FileSize] =
            Filter::Type_Off;
    else
    {
        filter.Switch[Filter::Search_FileSize] =
            (IsDlgButtonChecked(hDlg, IDC_RADIO_SIZERANGE_INC) ?
                Filter::Type_Include :
                Filter::Type_Ignore);
        
        GetDlgItemText(hDlg, IDC_EDIT_SIZEBEG, Buffer, MAX_PATH);
        DWORD nSel = SendDlgItemMessage(hDlg, IDC_COMBO_SEARCH_SIZE_UNIT1, CB_GETCURSEL, 0, 0);
        filter.FileSizeRangeLow = _wtoll(Buffer);
        if (nSel >= 0 && nSel < _countof(szUnit))
        {
            filter.FileSizeRangeLowUnitIndex = nSel;
            filter.FileSizeRangeLow *= szUnit[nSel].second;
        }
        else
        {
            filter.FileSizeRangeLowUnitIndex = 0;
        }

        GetDlgItemText(hDlg, IDC_EDIT_SIZEEND, Buffer, MAX_PATH);
        if (wcslen(Buffer) > 0)
        {
            DWORD nSel = SendDlgItemMessage(hDlg, IDC_COMBO_SEARCH_SIZE_UNIT2, CB_GETCURSEL, 0, 0);
            filter.FileSizeRangeHigh = _wtoll(Buffer);
            if (nSel >= 0 && nSel < _countof(szUnit))
            {
                filter.FileSizeRangeHighUnitIndex = nSel;
                filter.FileSizeRangeHigh *= szUnit[nSel].second;
            }
            else
            {
                filter.FileSizeRangeHighUnitIndex = 0;
            }
        }
        else
        {
            filter.FileSizeRangeHigh = (uint64_t)-1;
        }
    }

    //------------------------------------------- 文件属性

    if (!IsDlgButtonChecked(hDlg, IDC_CHECK_ATTRIB))
        filter.Switch[Filter::Search_FileAttribute] =
            Filter::Type_Off;
    else
    {
        filter.Switch[Filter::Search_FileAttribute] =
            (IsDlgButtonChecked(hDlg, IDC_RADIO_ATTRIB_INC) ?
                Filter::Type_Include :
                Filter::Type_Ignore);
        
        filter.SelectedAttributes = 0;

        filter.SelectedAttributes |= IsDlgButtonChecked(hDlg, IDC_CHECK_ATTR_A) ?
            Filter::Attrib_Archive : 0;
        filter.SelectedAttributes |= IsDlgButtonChecked(hDlg, IDC_CHECK_ATTR_R) ?
            Filter::Attrib_ReadOnly : 0;
        filter.SelectedAttributes |= IsDlgButtonChecked(hDlg, IDC_CHECK_ATTR_S) ?
            Filter::Attrib_System : 0;
        filter.SelectedAttributes |= IsDlgButtonChecked(hDlg, IDC_CHECK_ATTR_H) ?
            Filter::Attrib_Hide : 0;

        filter.SelectedAttributes = IsDlgButtonChecked(hDlg, IDC_CHECK_ATTR_N) ?
            Filter::Attrib_Normal : filter.SelectedAttributes;

        assert(filter.SelectedAttributes == Filter::Attrib_Normal || !(filter.SelectedAttributes & Filter::Attrib_Normal));
    }

    //------------------------------------------- 搜索路径

    filter.mSearchDirectory.clear();

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
            filter.mSearchDirectory.insert(
                std::make_pair(std::wstring(Buffer), -1/*times*/));
        }
    }
}
