
#include "UI.h"
#include <thread>
#include <vector>
#include <windows.h>
#include <CommCtrl.h>
#include "resource.h"
#include "functions.h"
#include "FileGroup.h"
#include <strsafe.h>
#include <Shlobj.h>

using namespace std;

HWND        g_hList, g_hStatus, g_hDlgFilter;
static int iClickItem;
HANDLE      g_ThreadSignal, g_hThread;
volatile bool bTerminateThread;
HINSTANCE   g_hInstance;
HMENU       g_hMenu;                             // 全局变量
Filter      g_Filter;

const wchar_t szAppName[20] = L"DuplicateFileFinder";

extern std::vector<DataBaseInfo> g_DataBase;
extern std::vector<std::vector<FileRecord>> g_DataBase2;


#define MAX_PATH1    1024

#define ID_STATUSBAR 1032
#define ID_HEADER    1030
#define ID_LISTVIEW  1031


BOOL  CALLBACK  FilterDialogProc(HWND, UINT, WPARAM, LPARAM);


namespace Detail
{
  std::pair<wchar_t*, uint64_t> szUnit[4] = {
      { L"B", 1, },
      { L"KB", 1024, },
      { L"MB", 1024 * 1024, },
      { L"GB", 1024 * 1024 * 1024, },
  };


  BOOL InitListViewColumns(HWND g_hList)
  {
    int         i;
    LVCOLUMN    lvc;
    int         iWidth[6] = { 200, 380, 75, 130, 130, 80 };
    TCHAR       szTitle[6][16] = { TEXT("文件名"), TEXT("路径"), TEXT("大小"),
                                                         TEXT("创建时间"), TEXT("修改时间"), TEXT("CRC32") };

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;

    for (i = 0; i < 2; ++i)
    {
      lvc.iSubItem = i;
      lvc.cx = iWidth[i];
      lvc.pszText = szTitle[i];
      lvc.cchTextMax = sizeof(lvc.pszText) / sizeof(lvc.pszText[0]);
      if (ListView_InsertColumn(g_hList, i, &lvc) == -1)
        return FALSE;
    }
    lvc.fmt = LVCFMT_RIGHT;         // 大小右对齐
    lvc.iSubItem = 2;
    lvc.cx = iWidth[2];
    lvc.pszText = szTitle[2];
    lvc.cchTextMax = sizeof(lvc.pszText) / sizeof(lvc.pszText[0]);
    if (ListView_InsertColumn(g_hList, 2, &lvc) == -1)
      return FALSE;
    lvc.fmt = LVCFMT_LEFT;
    for (i = 3; i < 6; ++i)
    {
      lvc.iSubItem = i;
      lvc.cx = iWidth[i];
      lvc.pszText = szTitle[i];
      lvc.cchTextMax = sizeof(lvc.pszText) / sizeof(lvc.pszText[0]);
      if (ListView_InsertColumn(g_hList, i, &lvc) == -1)
        return FALSE;
    }
    return TRUE;
  }

  std::vector<int> GetListViewSelectedItems()
  {
    int count = 0;
    int total = ListView_GetSelectedCount(g_hList);
    vector<int> indeies;
    for (int i = 0; count < total && g_DataBase.size(); ++i)
    {
      DWORD state = SendMessage(g_hList, LVM_GETITEMSTATE, i, LVIS_SELECTED);
      if (state & LVIS_SELECTED)
      {
        indeies.push_back(i);
        ++count;
      }
    }
    return indeies;
  }

  std::vector<int> GetListViewCheckedItems()
  {
    vector<int> ret;

    for (size_t i = 0; i != g_DataBase.size(); ++i)
      if (g_DataBase[i].checkstate == CHECKBOX_SECLECTED)
        ret.push_back(i);

    return ret;
  }

  void FillListView(NMLVDISPINFO* pdi, LVITEM* pItem)
  {
    int itemid = pItem->iItem;
    if (itemid >= g_DataBase.size())
      return;

    pdi->item.state = INDEXTOSTATEIMAGEMASK(g_DataBase[itemid].checkstate);
    pdi->item.stateMask = LVIS_STATEIMAGEMASK;

    FileRecord* pfi = g_DataBase[itemid].fi;
    if (pItem->mask & LVIF_TEXT)
    {
      TCHAR Buffer[128] = { 0 };

      switch (pItem->iSubItem)
      {
      case 0: // name
        wcscpy_s(pItem->pszText, pItem->cchTextMax, g_DataBase[itemid].fi->mPath.substr(g_DataBase[itemid].fi->mNameOffset).c_str());
        break;

      case 1: // path
        wcscpy_s(pItem->pszText, pItem->cchTextMax, g_DataBase[itemid].fi->mPath.substr(0, g_DataBase[itemid].fi->mNameOffset).c_str());
        break;

      case 2: // size
        if (g_DataBase[itemid].fi->mFileSize > 966367641ull)        // 约为0.9GB
          StringCbPrintf(Buffer, 120, TEXT("%.2lf GB"), g_DataBase[itemid].fi->mFileSize / (double)0x40000000);
        else if (g_DataBase[itemid].fi->mFileSize > 943718) // 0.9MB
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
      case 5: { // hash
        if (pfi->mCRC != 0)
        {
          TCHAR text[32];
          swprintf_s(text, _countof(text), L"%08x", pfi->mCRC);
          wcscpy_s(pItem->pszText, pItem->cchTextMax, text);
        }
        break;
      }
      default: break;
      }
    }
  }

  int ListViewCustomDraw(LPNMLVCUSTOMDRAW lpNMCustomDraw)
  {
    if (lpNMCustomDraw->nmcd.dwItemSpec < g_DataBase.size())
    {
      lpNMCustomDraw->clrText = g_DataBase[lpNMCustomDraw->nmcd.dwItemSpec].mTextColor;
      lpNMCustomDraw->clrTextBk = g_DataBase[lpNMCustomDraw->nmcd.dwItemSpec].mBKColor;
      return CDRF_NOTIFYITEMDRAW;
    }
    else
    {
      return CDRF_DODEFAULT;
    }
  }

  bool HasParentDirectoryInList(HWND g_hList, const std::wstring& dir)
  {
    if (dir.empty())
      return true;

    wchar_t Buf[MAX_PATH];

    int cnt = SendMessage(g_hList, LB_GETCOUNT, 0, 0);
    for (int i = 0; i < cnt; ++i)
    {
      SendMessage(g_hList, LB_GETTEXT, i, (LPARAM)Buf);
      size_t index = dir.find(Buf);
      if (index != std::wstring::npos && dir[index] == L'\\')
      {
        return true;
      }
    }

    return false;
  }

  std::wstring OpenFolder(HWND hwnd)
  {
    BROWSEINFO          bi;
    LPITEMIDLIST        pIDList;
    std::vector<TCHAR>  Buffer(1024);

    RtlZeroMemory(&bi, sizeof(BROWSEINFO));
    bi.hwndOwner = hwnd;
    bi.pszDisplayName = Buffer.data();
    bi.lpszTitle = TEXT("选择要分析的文件夹");
    bi.ulFlags = BIF_NONEWFOLDERBUTTON | BIF_SHAREABLE;

    pIDList = SHBrowseForFolder(&bi);
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

    case IDC_CHECK_DEL_KEYWORD:
      EnableWindow(GetDlgItem(hDlg, IDC_RADIO_DEL_KEYWORD_INC), enable);
      EnableWindow(GetDlgItem(hDlg, IDC_RADIO_DEL_KEYWORD_EXC), enable);
      EnableWindow(GetDlgItem(hDlg, IDC_EDIT_DEL_KEYWORD_INC), enable && IsDlgButtonChecked(hDlg, IDC_RADIO_DEL_KEYWORD_INC));
      EnableWindow(GetDlgItem(hDlg, IDC_EDIT_DEL_KEYWORD_EXC), enable && IsDlgButtonChecked(hDlg, IDC_RADIO_DEL_KEYWORD_EXC));
      break;

    case IDC_CHECK_DEL_NAMELEN:
      EnableWindow(GetDlgItem(hDlg, IDC_RADIO_DEL_NAMELEN_LONG), enable);
      EnableWindow(GetDlgItem(hDlg, IDC_RADIO_DEL_NAMELEN_SHORT), enable);
      break;

    case IDC_CHECK_DEL_PATHDEP:
      EnableWindow(GetDlgItem(hDlg, IDC_RADIO_DEL_PATHDEP_DEPTH), enable);
      EnableWindow(GetDlgItem(hDlg, IDC_RADIO_DEL_PATHDEP_SHALLOW), enable);
      break;

    case IDC_CHECK_DEL_SEARCHORD:
      EnableWindow(GetDlgItem(hDlg, IDC_RADIO_DEL_SEARCHORD_FRONT), enable);
      EnableWindow(GetDlgItem(hDlg, IDC_RADIO_DEL_SEARCHORD_BACK), enable);
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
        for (int i = IDC_RADIO_FULLNAME; i <= IDC_RADIO_PARTNAME; ++i)
          CheckDlgButton(hDlg, i, i == IDC_RADIO_FULLNAME);

        CheckDlgButton(hDlg, IDC_CHECK_NOSUFFIX, filter.FileNameWithoutSuffix);
        break;

      case Filter::Type_Include:
        for (int i = IDC_RADIO_FULLNAME; i <= IDC_RADIO_PARTNAME; ++i)
          CheckDlgButton(hDlg, i, i == IDC_RADIO_INCNAME);

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
    for (it = filter.mSearchDirectory.begin(); it != filter.mSearchDirectory.end(); ++it)
    {
      str.clear();
      str += it->first;
      SendDlgItemMessage(hDlg, IDC_LIST_DIR, LB_ADDSTRING, 0, (LPARAM)str.c_str());
    }

    //------------------------------------------- 删除策略

    assert((int)!!filter.Switch[Filter::Del_Keyword] + !!filter.Switch[Filter::Del_NameLength] +
      !!filter.Switch[Filter::Del_PathDepth] + !!filter.Switch[Filter::Del_SearchOrder] <= 1);
    if ((status = filter.Switch[Filter::Del_Keyword]) == Filter::Type_Off) {
      EnableControls(hDlg, IDC_CHECK_DEL_KEYWORD, false);
    }
    else {
      CheckDlgButton(hDlg, IDC_CHECK_DEL_KEYWORD, true);
      EnableControls(hDlg, IDC_CHECK_DEL_KEYWORD, true);
      assert(status == Filter::Type_Include || status == Filter::Type_Ignore);
      bool inc = status == Filter::Type_Include;
      CheckDlgButton(hDlg, IDC_RADIO_DEL_KEYWORD_INC, inc);
      CheckDlgButton(hDlg, IDC_RADIO_DEL_KEYWORD_EXC, !inc);
      EnableWindow(GetDlgItem(hDlg, IDC_EDIT_DEL_KEYWORD_INC), inc);
      EnableWindow(GetDlgItem(hDlg, IDC_EDIT_DEL_KEYWORD_EXC), !inc);
      SetDlgItemText(hDlg, IDC_EDIT_DEL_KEYWORD_INC, inc ? filter.mKeyWordOfFileNameForDelete.c_str() : L"");
      SetDlgItemText(hDlg, IDC_EDIT_DEL_KEYWORD_EXC, inc ? L"" : filter.mKeyWordOfFileNameForDelete.c_str());
    }

    if ((status = filter.Switch[Filter::Del_NameLength]) == Filter::Type_Off) {
      EnableControls(hDlg, IDC_CHECK_DEL_NAMELEN, false);
    }
    else {
      CheckDlgButton(hDlg, IDC_CHECK_DEL_NAMELEN, true);
      EnableControls(hDlg, IDC_CHECK_DEL_NAMELEN, true);
      CheckDlgButton(hDlg, IDC_RADIO_DEL_NAMELEN_LONG, status == Filter::Type_Long);
      CheckDlgButton(hDlg, IDC_RADIO_DEL_NAMELEN_SHORT, status == Filter::Type_Short);
    }

    if ((status = filter.Switch[Filter::Del_PathDepth]) == Filter::Type_Off) {
      EnableControls(hDlg, IDC_CHECK_DEL_PATHDEP, false);
    }
    else {
      CheckDlgButton(hDlg, IDC_CHECK_DEL_PATHDEP, true);
      EnableControls(hDlg, IDC_CHECK_DEL_PATHDEP, true);
      CheckDlgButton(hDlg, IDC_RADIO_DEL_PATHDEP_DEPTH, status == Filter::Type_Long);
      CheckDlgButton(hDlg, IDC_RADIO_DEL_PATHDEP_SHALLOW, status == Filter::Type_Short);
    }

    if ((status = filter.Switch[Filter::Del_SearchOrder]) == Filter::Type_Off) {
      EnableControls(hDlg, IDC_CHECK_DEL_SEARCHORD, false);
    }
    else {
      CheckDlgButton(hDlg, IDC_CHECK_DEL_SEARCHORD, true);
      EnableControls(hDlg, IDC_CHECK_DEL_SEARCHORD, true);
      CheckDlgButton(hDlg, IDC_RADIO_DEL_SEARCHORD_FRONT, status == Filter::Type_First);
      CheckDlgButton(hDlg, IDC_RADIO_DEL_SEARCHORD_BACK, status == Filter::Type_Last);
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
      for (int i = 0; i < idx; ++i)
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
        if (Buffer[len - 1] == L'\\')     // 选择分区根目录时最后为反斜杠
          Buffer[len - 1] = L'\0';
        filter.mSearchDirectory.insert(
          std::make_pair(std::wstring(Buffer), -1/*times*/));
      }
    }

    //------------------------------------------- 删除策略

    filter.Switch[Filter::Del_Keyword] = Filter::Type_Off;
    if (IsDlgButtonChecked(hDlg, IDC_CHECK_DEL_KEYWORD)) {
      if (IsDlgButtonChecked(hDlg, IDC_RADIO_DEL_KEYWORD_INC)) {
        filter.Switch[Filter::Del_Keyword] = Filter::Type_Include;
        GetDlgItemText(hDlg, IDC_EDIT_DEL_KEYWORD_INC, Buffer, MAX_PATH);
        filter.mKeyWordOfFileNameForDelete = Buffer;
      }
      else {
        filter.Switch[Filter::Del_Keyword] = Filter::Type_Ignore;
        GetDlgItemText(hDlg, IDC_EDIT_DEL_KEYWORD_EXC, Buffer, MAX_PATH);
        filter.mKeyWordOfFileNameForDelete = Buffer;
      }
      assert(!filter.mKeyWordOfFileNameForDelete.empty());
    }

    filter.Switch[Filter::Del_NameLength] = Filter::Type_Off;
    if (IsDlgButtonChecked(hDlg, IDC_CHECK_DEL_NAMELEN)) {
      filter.Switch[Filter::Del_NameLength] = IsDlgButtonChecked(hDlg, IDC_RADIO_DEL_NAMELEN_LONG) ? Filter::Type_Long : Filter::Type_Short;
    }

    filter.Switch[Filter::Del_PathDepth] = Filter::Type_Off;
    if (IsDlgButtonChecked(hDlg, IDC_CHECK_DEL_PATHDEP)) {
      filter.Switch[Filter::Del_PathDepth] = IsDlgButtonChecked(hDlg, IDC_RADIO_DEL_PATHDEP_DEPTH) ? Filter::Type_Long : Filter::Type_Short;
    }

    filter.Switch[Filter::Del_SearchOrder] = Filter::Type_Off;
    if (IsDlgButtonChecked(hDlg, IDC_CHECK_DEL_SEARCHORD)) {
      filter.Switch[Filter::Del_SearchOrder] = IsDlgButtonChecked(hDlg, IDC_RADIO_DEL_SEARCHORD_FRONT) ? Filter::Type_First : Filter::Type_Last;
    }
    assert((int)!!filter.Switch[Filter::Del_Keyword] + !!filter.Switch[Filter::Del_NameLength] +
      !!filter.Switch[Filter::Del_PathDepth] + !!filter.Switch[Filter::Del_SearchOrder] <= 1);
  }
}

void UpdateStatusBar(int part, const wchar_t* text)
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
        Detail::GetListViewCheckedCount(g_hList), ListView_GetItemCount(g_hList));
      SendMessage(g_hStatus, SB_SETTEXT, 1, (LPARAM)szBuffer);
    }
    else
      SendMessage(g_hStatus, SB_SETTEXT, 1, (LPARAM)text);
  }
}

bool Cls_OnCommand_main(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
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

    if (id == IDM_OPENFILE)
    {
      ShellExecute(0, 0, g_DataBase[iClickItem].fi->mPath.c_str(), NULL, NULL, SW_SHOW);
    }
    else
    {
      StringCbCopy(szBuffer1, _countof(szBuffer1), TEXT("确认删除"));
      StringCbCat(szBuffer1, _countof(szBuffer1), g_DataBase[iClickItem].fi->mPath.c_str());
      if (IDNO == MessageBox(hDlg, szBuffer1, TEXT("提示"), MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING))
        return TRUE;

      vector<wchar_t> doubleNull(g_DataBase[iClickItem].fi->mPath.begin(), g_DataBase[iClickItem].fi->mPath.end());
      doubleNull.push_back(L'\0');
      doubleNull.push_back(L'\0');
      SHFILEOPSTRUCT fo = { 0 };
      fo.wFunc = FO_DELETE;
      fo.pFrom = doubleNull.data();
      fo.fFlags = FOF_NO_UI | FOF_ALLOWUNDO;

      //DeleteFile(g_DataBase[iClickItem].fi->mPath.c_str());
      if (SHFileOperation(&fo) || !GetLastError())
      {
        ListView_DeleteItem(g_hList, iClickItem);
        g_DataBase.erase(g_DataBase.begin() + iClickItem);
        UpdateStatusBar(1, 0);
      }
      else
      {
        MessageBox(hDlg, TEXT("无法删除文件，请手动删除"), TEXT("提示"), MB_ICONEXCLAMATION);
      }
    }
    return TRUE;

  case IDM_EXPLORER:
    if (iClickItem < g_DataBase.size() && iClickItem >= 0)
    {
      wstring cmd(L"/select,\"");
      cmd += g_DataBase[iClickItem].fi->mPath.c_str();
      cmd += L"\"";
      ShellExecute(0, 0, L"explorer.exe", cmd.c_str(), 0, SW_SHOW);
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
        vector<wchar_t> doubleNull(g_DataBase[i].fi->mPath.begin(), g_DataBase[i].fi->mPath.end());
        doubleNull.push_back(L'\0');
        doubleNull.push_back(L'\0');
        SHFILEOPSTRUCT fo = { 0 };
        fo.wFunc = FO_DELETE;
        fo.pFrom = doubleNull.data();
        fo.fFlags = FOF_NO_UI | FOF_ALLOWUNDO;

        //DeleteFile(g_DataBase[i].fi->mPath.c_str());
        if (SHFileOperation(&fo) || !GetLastError())
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

  case IDM_HASHALL: {
    auto list = Detail::GetListViewCheckedItems();
    std::thread([list]() {
      if (!list.empty())
      {
        for (size_t ii = 0; ii < list.size(); ++ii)
        {
          TCHAR msg[128];
          swprintf_s(msg, _countof(msg), L"正在计算 CRC32...(%d/%d)", ii + 1, list.size());
          UpdateStatusBar(0, msg);

          g_DataBase[list[ii]].fi->mCRC = GetFileCrc(g_DataBase[list[ii]].fi->mPath);
          SendMessage(g_hList, LVM_REDRAWITEMS, list[ii], list[ii]);
        }
        UpdateStatusBar(0, L"CRC32 计算完成");

        int color[2] = { RGB(192,255,255), RGB(255, 204, 192) };
        int colorIdx = 0;
        for (size_t i = 0; i < g_DataBase.size(); )
        {
          DWORD crc32 = g_DataBase[i].fi->mCRC;
          g_DataBase[i].mBKColor = RGB(255, 255, 255);
          if (!g_DataBase[i].mFistInGroup || crc32 == 0)
          {
            SendMessage(g_hList, LVM_REDRAWITEMS, i, i);
            ++i;
            continue;
          }

          size_t sameCrcInGroup = 0;
          size_t j = i + 1;
          for (; j < g_DataBase.size() && !g_DataBase[j].mFistInGroup; ++j)
          {
            if (g_DataBase[j].fi->mCRC == crc32)
            {
              ++sameCrcInGroup;
              g_DataBase[j].mBKColor = color[colorIdx];
              g_DataBase[j].mTextColor = RGB(0, 0, 0);
              SendMessage(g_hList, LVM_REDRAWITEMS, j, j);
            }
          }

          if (sameCrcInGroup > 0)
          {
            g_DataBase[i].mBKColor = color[colorIdx];
            g_DataBase[i].mTextColor = RGB(0, 0, 0);
            colorIdx ^= 1;
          }

          SendMessage(g_hList, LVM_REDRAWITEMS, i, i);
          i = j;
        }
      }
    }).detach();
    return TRUE;
  }

  case IDM_SELALL:
  case IDM_USELALL:
    for (size_t i = 0; i < g_DataBase.size(); ++i)
      g_DataBase[i].checkstate =
      id == IDM_SELALL ? CHECKBOX_SECLECTED : CHECKBOX_UNSECLECTED;
    UpdateStatusBar(1, 0);
    InvalidateRect(g_hList, NULL, TRUE);
    return TRUE;

  case IDM_SELECT_DUP_IN_GROUP:
    for (size_t i = 0; i < g_DataBase.size(); )
    {
      g_DataBase[i].checkstate = CHECKBOX_UNSECLECTED;
      SendMessage(g_hList, LVM_REDRAWITEMS, i, i);
      if (!g_DataBase[i].mFistInGroup)
      {
        ++i;
        continue;
      }

      size_t j = i + 1;
      while (j < g_DataBase.size() && !g_DataBase[j].mFistInGroup)
      {
        g_DataBase[j].checkstate = CHECKBOX_SECLECTED;
        SendMessage(g_hList, LVM_REDRAWITEMS, j, j);
        ++j;
      }
      i = j;
    }
    return TRUE;

  case IDM_SELECT_DUP_IN_GROUP_VIA_CRC:
    for (size_t i = 0; i < g_DataBase.size(); )
    {
      g_DataBase[i].checkstate = CHECKBOX_UNSECLECTED;
      SendMessage(g_hList, LVM_REDRAWITEMS, i, i);
      if (!g_DataBase[i].mFistInGroup)
      {
        ++i;
        continue;
      }

      DWORD crc32 = g_DataBase[i].fi->mCRC;
      if (crc32 == 0)
      {
        ++i;
        continue;
      }

      size_t j = i + 1;
      while (j < g_DataBase.size() && !g_DataBase[j].mFistInGroup)
      {
        g_DataBase[j].checkstate = CHECKBOX_UNSECLECTED;
        if (g_DataBase[j].fi->mCRC == crc32)
        {
          g_DataBase[j].checkstate = CHECKBOX_SECLECTED;
          SendMessage(g_hList, LVM_REDRAWITEMS, j, j);
        }
        ++j;
      }
      i = j;
    }
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


bool Cls_OnCreate(HWND hDlg, LPCREATESTRUCT lpCreateStruct)
{
  // 设置菜单
  SetMenu(hDlg, LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_MENU2)));

  RECT rc;
  GetClientRect(hDlg, &rc);

  // 初始化 Status Bar
  g_hStatus = CreateWindowEx(0, STATUSCLASSNAME, (PCTSTR)NULL, SBARS_SIZEGRIP | WS_CHILD | WS_VISIBLE,
    0, 0, 0, 0, hDlg, (HMENU)NULL, g_hInstance, NULL);

  static int StatusBarSetPart[2];
  StatusBarSetPart[0] = (rc.right - rc.left) * 4 / 5;
  StatusBarSetPart[1] = -1;
  SendMessage(g_hStatus, SB_SETPARTS, 2, (LPARAM)StatusBarSetPart);

  RECT sbrc;
  POINT lppt;
  GetWindowRect(g_hStatus, &sbrc);
  lppt.y = sbrc.top;
  lppt.x = sbrc.right;
  ScreenToClient(hDlg, &lppt);


  // 初始化List View    大小在这里微调……
  g_hList = CreateWindowEx(0, WC_LISTVIEW, NULL,
    LVS_REPORT | WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_ALIGNTOP | LVS_OWNERDATA,
    0, 0, rc.right - rc.left, lppt.y,
    hDlg, (HMENU)ID_LISTVIEW, (HINSTANCE)GetWindowLong(hDlg, GWL_HINSTANCE), NULL);
  ListView_EnableGroupView(g_hList, TRUE);
  ListView_SetExtendedListViewStyleEx(g_hList, 0, LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
  if (!Detail::InitListViewColumns(g_hList))
    assert(0);

  // 设置焦点
  SetFocus(GetDlgItem(hDlg, IDB_BROWSE));
  return false;
}


void Cls_OnClose(HWND hwnd)
{
  EndDialog(hwnd, 0);
}


void Cls_OnSysCommand0(HWND hwnd, UINT cmd, int x, int y)
{
  if (cmd == SC_CLOSE)
  {
    SendMessage(g_hDlgFilter, WM_SYSCOMMAND, SC_CLOSE, 0);
    PostQuitMessage(0);
  }
}

void Cls_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
  RECT sbrc;
  GetWindowRect(g_hStatus, &sbrc);
  MoveWindow(g_hStatus, 0, cy - sbrc.bottom + sbrc.top, cx, sbrc.bottom - sbrc.top, TRUE);
  MoveWindow(g_hList, 0, 0, cx, cy - sbrc.bottom + sbrc.top, TRUE);
}

LRESULT Cls_OnListViewNotify(HWND hDlg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  bHandled = FALSE;

  if (((NMHDR*)lParam)->idFrom != ID_LISTVIEW)
    return FALSE;

  switch (((NMHDR*)lParam)->code)
  {
  case LVN_KEYDOWN:
    switch (((LPNMLVKEYDOWN)lParam)->wVKey)
    {
    case VK_RETURN:
      iClickItem = ListView_GetSelectionMark(g_hList);
      SendMessage(hDlg, WM_COMMAND, IDM_OPENFILE, 0);
      bHandled = TRUE;
      return TRUE;

    case VK_DELETE:
      iClickItem = ListView_GetSelectionMark(g_hList);
      SendMessage(hDlg, WM_COMMAND, IDM_DELETE, 0);
      bHandled = TRUE;
      return TRUE;

      // 使用空格键切换checkbox
    case VK_SPACE: {
      auto indeies = Detail::GetListViewSelectedItems();
      for (auto i : indeies)
      {
        g_DataBase[i].checkstate ^= 0x3;
        SendMessage(g_hList, LVM_REDRAWITEMS, i, i);
      }
      UpdateStatusBar(1, nullptr);
      bHandled = TRUE;
      return TRUE;
    }
    }
    break;

  case NM_CLICK: {
    NMITEMACTIVATE* ia = (NMITEMACTIVATE*)lParam;
    LVHITTESTINFO lhti;
    lhti.pt = ia->ptAction;
    SendMessage(g_hList, LVM_SUBITEMHITTEST, 0, (LPARAM)&lhti);

    if (lhti.flags == LVHT_ONITEMSTATEICON && lhti.iSubItem == 0)
    {
      g_DataBase[lhti.iItem].checkstate ^= 0x3;
      SendMessage(g_hList, LVM_REDRAWITEMS, lhti.iItem, lhti.iItem);
    }

    UpdateStatusBar(1, nullptr);
    bHandled = TRUE;
    return TRUE;
  }

  case NM_CUSTOMDRAW: {
    bHandled = TRUE;
    return Detail::ListViewCustomDraw((LPNMLVCUSTOMDRAW)lParam);
  }

  case NM_RCLICK:
  case NM_DBLCLK: {
    LVHITTESTINFO lvh;
    GetCursorPos(&lvh.pt);
    ScreenToClient(hDlg, &lvh.pt);
    iClickItem = SendMessage(g_hList, LVM_SUBITEMHITTEST, 0, (LPARAM)&lvh);
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
  }

  case LVN_GETDISPINFO:
  {
    NMLVDISPINFO* pdi = reinterpret_cast<NMLVDISPINFO*>(lParam);
    LVITEMW* pItem = &pdi->item;
    Detail::FillListView(pdi, pItem);
  }
  }

  return FALSE;
}




//////////////////////////////////////////////////////////////////////////////////////

//                                  过滤条件窗口                                     //

//////////////////////////////////////////////////////////////////////////////////////



bool Cls_OnCommand_filter(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
  switch (id)
  {
  case IDOK:
    if (!IsDlgButtonChecked(hDlg, IDC_CHECK_NAME) && !IsDlgButtonChecked(hDlg, IDC_CHECK_SIZE) &&
      !IsDlgButtonChecked(hDlg, IDC_CHECK_DATE) && !IsDlgButtonChecked(hDlg, IDC_CHECK_DATA))
    {
      MessageBox(hDlg, L"至少选择一个匹配条件", szAppName, MB_ICONWARNING);
      return TRUE;
    }
    if (IsDlgButtonChecked(hDlg, IDC_CHECK_DEL_KEYWORD)) {
      TCHAR buf_inc[MAX_PATH] = { 0 };
      TCHAR buf_exc[MAX_PATH] = { 0 };
      GetDlgItemText(hDlg, IDC_EDIT_DEL_KEYWORD_INC, buf_inc, MAX_PATH);
      GetDlgItemText(hDlg, IDC_EDIT_DEL_KEYWORD_EXC, buf_exc, MAX_PATH);
      bool empty = wcslen(buf_inc) == 0 && IsDlgButtonChecked(hDlg, IDC_RADIO_DEL_KEYWORD_INC);
      empty |= wcslen(buf_exc) == 0 && IsDlgButtonChecked(hDlg, IDC_RADIO_DEL_KEYWORD_EXC);
      if (empty) {
        MessageBox(hDlg, L"请输入优先匹配的关键字", szAppName, MB_ICONWARNING);
        return TRUE;
      }
    }
    Detail::UpdateFilterConfig(hDlg, g_Filter);

  case IDCANCEL:
    EndDialog(hDlg, 0);
    return TRUE;

  case IDC_BUTTON_ADDDIR: {
    std::wstring Buffer = Detail::OpenFolder(hDlg);
    if (Buffer[0] &&
      LB_ERR == SendDlgItemMessage(hDlg, IDC_LIST_DIR, LB_FINDSTRINGEXACT, -1, (LPARAM)Buffer.c_str()))
    {
      if (!Detail::HasParentDirectoryInList(GetDlgItem(hDlg, IDC_LIST_DIR), Buffer))
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
    Detail::EnableControls(hDlg, id, !!IsDlgButtonChecked(hDlg, id));
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
    break;

  case IDC_CHECK_DEL_KEYWORD:
  case IDC_CHECK_DEL_NAMELEN:
  case IDC_CHECK_DEL_PATHDEP:
  case IDC_CHECK_DEL_SEARCHORD: {
    bool b1 = IsDlgButtonChecked(hDlg, IDC_CHECK_DEL_KEYWORD);
    bool b2 = IsDlgButtonChecked(hDlg, IDC_CHECK_DEL_NAMELEN);
    bool b3 = IsDlgButtonChecked(hDlg, IDC_CHECK_DEL_PATHDEP);
    bool b4 = IsDlgButtonChecked(hDlg, IDC_CHECK_DEL_SEARCHORD);
    CheckDlgButton(hDlg, IDC_CHECK_DEL_KEYWORD,   id == IDC_CHECK_DEL_KEYWORD ? b1 : false);
    CheckDlgButton(hDlg, IDC_CHECK_DEL_NAMELEN,   id == IDC_CHECK_DEL_NAMELEN ? b2 : false);
    CheckDlgButton(hDlg, IDC_CHECK_DEL_PATHDEP,   id == IDC_CHECK_DEL_PATHDEP ? b3 : false);
    CheckDlgButton(hDlg, IDC_CHECK_DEL_SEARCHORD, id == IDC_CHECK_DEL_SEARCHORD ? b4 : false);
    Detail::EnableControls(hDlg, IDC_CHECK_DEL_KEYWORD,   !!IsDlgButtonChecked(hDlg, IDC_CHECK_DEL_KEYWORD));
    Detail::EnableControls(hDlg, IDC_CHECK_DEL_NAMELEN,   !!IsDlgButtonChecked(hDlg, IDC_CHECK_DEL_NAMELEN));
    Detail::EnableControls(hDlg, IDC_CHECK_DEL_PATHDEP,   !!IsDlgButtonChecked(hDlg, IDC_CHECK_DEL_PATHDEP));
    Detail::EnableControls(hDlg, IDC_CHECK_DEL_SEARCHORD, !!IsDlgButtonChecked(hDlg, IDC_CHECK_DEL_SEARCHORD));
    return TRUE;
  }
  case IDC_RADIO_DEL_KEYWORD_INC:
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_DEL_KEYWORD_INC), IsDlgButtonChecked(hDlg, id));
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_DEL_KEYWORD_EXC), !IsDlgButtonChecked(hDlg, id));
    break;

  case IDC_RADIO_DEL_KEYWORD_EXC:
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_DEL_KEYWORD_INC), !IsDlgButtonChecked(hDlg, id));
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_DEL_KEYWORD_EXC), IsDlgButtonChecked(hDlg, id));
    break;
  }

  return FALSE;
}


bool Cls_OnInitDialog_filter(HWND hDlg, HWND hwndFocus, LPARAM lParam)
{
  HWND hCombo, hCombo1;
  TCHAR Buffer[MAX_PATH];

  g_hDlgFilter = hDlg;

  // 初始化文件类型下拉框
  hCombo = GetDlgItem(hDlg, IDC_COMBO_TYPEINC);
  hCombo1 = GetDlgItem(hDlg, IDC_COMBO_TYPEIGN);
  for (UINT i = IDS_STRING103; i <= IDS_STRING107; ++i)
  {
    LoadString(g_hInstance, i, Buffer, MAX_PATH);
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)Buffer);
    SendMessage(hCombo1, CB_ADDSTRING, 0, (LPARAM)Buffer);
  }

  // 初始化大小单位下拉框
  hCombo = GetDlgItem(hDlg, IDC_COMBO_SEARCH_SIZE_UNIT1);
  hCombo1 = GetDlgItem(hDlg, IDC_COMBO_SEARCH_SIZE_UNIT2);
  for (UINT i = 0; i < _countof(Detail::szUnit); ++i)
  {
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)Detail::szUnit[i].first);
    SendMessage(hCombo1, CB_ADDSTRING, 0, (LPARAM)Detail::szUnit[i].first);
  }
  SendMessage(hCombo, CB_SETCURSEL, 0, 0);
  SendMessage(hCombo1, CB_SETCURSEL, 0, 0);

  // 设置默认匹配规则
  SendMessage(hDlg, WM_COMMAND, IDC_CHECK_NAME, 1);
  SendMessage(hDlg, WM_COMMAND, IDC_CHECK_SIZE, 1);
  SendMessage(hDlg, WM_COMMAND, IDC_RADIO_FULLNAME, 1);
  SendMessage(hDlg, WM_COMMAND, IDC_CHECK_DATE, 0);
  SendMessage(hDlg, WM_COMMAND, IDC_CHECK_DATA, 0);

  CheckDlgButton(hDlg, IDC_RADIO_INCNAME, FALSE);
  CheckDlgButton(hDlg, IDC_RADIO_FULLNAME, TRUE);
  CheckDlgButton(hDlg, IDC_RADIO_SIZEEQUAL, TRUE);
  CheckDlgButton(hDlg, IDC_RADIO_DATEEQUAL, TRUE);
  CheckDlgButton(hDlg, IDC_RADIO_DATAEQUAL, TRUE);

  CheckDlgButton(hDlg, IDC_RADIO_TYPE_IGN, FALSE);
  CheckDlgButton(hDlg, IDC_RADIO_TYPE_INC, TRUE);
  CheckDlgButton(hDlg, IDC_RADIO_SIZERANGE_IGN, FALSE);
  CheckDlgButton(hDlg, IDC_RADIO_SIZERANGE_INC, TRUE);
  CheckDlgButton(hDlg, IDC_RADIO_ATTRIB_IGN, FALSE);
  CheckDlgButton(hDlg, IDC_RADIO_ATTRIB_INC, TRUE);

  CheckDlgButton(hDlg, IDC_RADIO_DEL_KEYWORD_INC, TRUE);
  CheckDlgButton(hDlg, IDC_RADIO_DEL_NAMELEN_LONG, TRUE);
  CheckDlgButton(hDlg, IDC_RADIO_DEL_PATHDEP_DEPTH, TRUE);
  CheckDlgButton(hDlg, IDC_RADIO_DEL_SEARCHORD_FRONT, TRUE);

  Detail::LoadFilterConfigToDialog(hDlg, g_Filter);

  return true;
}


void Cls_OnSysCommand(HWND hwnd, UINT cmd, int x, int y)
{
  if (cmd == SC_CLOSE)
    EndDialog(hwnd, 0);
}