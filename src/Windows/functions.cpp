

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
            StringCbPrintf(szBuffer, sizeof(szBuffer), TEXT("ѡ���ļ�:%d  /  �ļ�����:%d"),
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
    bi.lpszTitle		= TEXT("ѡ��Ҫ�������ļ���");
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
    TCHAR		szTitle[6][16] = {TEXT("�ļ���"), TEXT("·��"), TEXT("��С"),
                              TEXT("����ʱ��"), TEXT("�޸�ʱ��"), TEXT("��ϣֵ")};
    
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
    lvc.fmt			= LVCFMT_RIGHT;			// ��С�Ҷ���
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
			//if (dir.back() == L'\\')  // ���ļ�·����ʽ��������������
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

    //------------------------------------------- �ļ���

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

    //------------------------------------------- �ļ���С

    if (filter.Switch[Filter::Compare_FileSize] == Filter::Type_Off)
        EnableControls(hDlg, IDC_CHECK_SIZE, false);
    else
    {
        CheckDlgButton(hDlg, IDC_CHECK_SIZE, 1);
        EnableControls(hDlg, IDC_CHECK_SIZE, true);
        CheckDlgButton(hDlg, IDC_RADIO_SIZEEQUAL, true);
    }

    //------------------------------------------- �ļ��޸�����

    int status;
    if ((status = filter.Switch[Filter::Compare_FileDate]) == Filter::Type_Off)
        EnableControls(hDlg, IDC_CHECK_DATE, false);
    else
    {
        CheckDlgButton(hDlg, IDC_CHECK_DATE, 1);
		EnableControls(hDlg, IDC_CHECK_DATE, true);
		CheckDlgButton(hDlg, IDC_RADIO_DATEEQUAL, true);
    }

    //------------------------------------------- �ļ�����

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

    //------------------------------------------- ��������չ��

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

    //------------------------------------------- ���ҵ��ļ���С��Χ

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

        // ��λ
    }

    //------------------------------------------- ���ҵ��ļ�����

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

    //------------------------------------------- ���ҵ�Ŀ¼

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

    //------------------------------------------- �ļ���

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

    //------------------------------------------- �ļ���С

    filter.Switch[Filter::Compare_FileSize] = IsDlgButtonChecked(hDlg, IDC_CHECK_SIZE) ? Filter::Type_Whole : Filter::Type_Off;

    //------------------------------------------- �޸�����

    filter.Switch[Filter::Compare_FileDate] = IsDlgButtonChecked(hDlg, IDC_CHECK_DATE) ? Filter::Type_On : Filter::Type_Off;

    //------------------------------------------- �ļ�����

    filter.Switch[Filter::Compare_FileHash] = IsDlgButtonChecked(hDlg, IDC_CHECK_DATA) ? Filter::Type_Whole : Filter::Type_Off;

    //------------------------------------------- ��������

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

    //------------------------------------------- �ļ���С��Χ

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

    //------------------------------------------- �ļ�����

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

    //------------------------------------------- ����·��

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
            //case 0:  times = -1; break;     // ����
            //case 1:  times =  0; break;     // ��ǰ
            //default: times--;    break;     // X��
            //}
            SendDlgItemMessage(hDlg, IDC_LIST_DIR, LB_GETTEXT, i, (LPARAM)Buffer);
            int len = wcslen(Buffer);
            if (Buffer[len-1] == L'\\')     // ѡ�������Ŀ¼ʱ���Ϊ��б��
                Buffer[len-1] = L'\0';
            filter.mSearchDirectory.insert(
                std::make_pair(std::wstring(Buffer), -1/*times*/));
        }
    }
}
