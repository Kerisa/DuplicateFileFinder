

#pragma once

#include <assert.h>
#include <windows.h>
#include <process.h>
#include <string>
#include <set>
#include <map>
#include <vector>
#include <Strsafe.h>
#include <ShlObj.h>
#include <Commdlg.h>
#include <CommCtrl.h>
#include "Sha1.h"
#include "resource.h"


#pragma comment (lib, "Comctl32.lib")

#define ID_STATUSBAR 1032
#define ID_HEADER    1030
#define ID_LISTVIEW  1031
#define MAX_PATH1	 1024


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class FileGroup;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
DWORD WINAPI	Thread(PVOID);				// 声明
DWORD WINAPI	ThreadHashOnly(PVOID);
BOOL  CALLBACK	MainDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL  CALLBACK	FilterDialogProc(HWND, UINT, WPARAM, LPARAM);
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void OpenFolder(HWND hwnd, PTSTR pReceive);
BOOL InitListViewColumns(HWND hList);
//BOOL SetNewParam(HWND hwnd, PPARAMS p);
int SelectSameHash(HWND hList);
//DWORD Seclect(PFILEINFO pfi[], DWORD dwFileNum, BYTE bOptions);
//void ThreadEndRoutine(PBYTE lpVirMem, HGLOBAL hIndex);
int DelDifferentHash(HWND hList);

void LoadFilterConfigToDialog(HWND hDlg, FileGroup& fg);
void UpdateFilterConfig(HWND hDlg, FileGroup& fg);


class PerformanceAnalyse
{
};



class FileGroup
{
public:
    enum {
        StoreType_size = 1,
        StoreType_size_date,
        StoreType_size_name,
        StoreType_size_date_name,
        StoreType_hash,
        StoreType_date,
        StoreType_date_name,
        StoreType_name,
        StoreType_All
    };

    typedef struct _FileInfo
    {
        ULONG64 Size;
        SYSTEMTIME st;
        std::wstring Path;
        std::wstring Name;
        //wchar_t      Sha1[42];

        _FileInfo(ULONG64 size, PSYSTEMTIME pst, const std::wstring& path, std::wstring& name, wchar_t *sha1 = 0)
            : Size(size)
        {
            Path = std::wstring(path);
            Name = std::wstring(name);
            st = *pst;
            //if (sha1)
            //    wcscpy_s(sha1, _countof(sha1), sha1);
            //else
            //    sha1[0] = l'\0';
        }

        _FileInfo(ULONG64 size, PSYSTEMTIME pst, const std::wstring& path, wchar_t *name, wchar_t *sha1 = 0)
        {
             _FileInfo(size, pst, path, std::wstring(name), sha1);
        }

        _FileInfo() {}
        
    }
    FileInfo, *pFileInfo;

    // 大小 / 日期 / 大小 + 日期
    std::map<ULONG64, std::vector<FileInfo>>                                m_List1S;

    // 大小 + 文件名 / 大小 + 日期 + 文件名 / 日期 + 文件名
    std::multimap<ULONG64, std::map<std::wstring, std::vector<FileInfo>>>   m_List2SN;

    // 文件名
    std::map<std::wstring, std::vector<FileInfo>>                           m_List1N;
    
    // 列出所有文件
    std::vector<FileInfo>                                                   m_List0;

    // 哈希 - 对以上结果使用哈希 (勾上哈希时同时勾上“大小”)
    std::map<std::wstring, std::vector<pFileInfo>>                           m_List1H;
    

    int m_StoreType;
    
    int pFindFiles(const std::wstring& dirPath, int depth);

    int pHashFiles1S();
    int pHashFiles2SN();
    int pHashFiles1N();
    int pHashFiles0();
    int PerformHash(const std::wstring& name, std::wstring& result, ULONG64 size = 0);

public:
    
	struct FILTER
    {
        enum {
            Compare_FileName,
            Compare_FileSize,
            Compare_FileDate,
            Compare_FileHash,
            Search_FileAttribute,
            Search_FileSize,
            Search_IgnoreSuffix,
            Search_IncludeSuffix,
            Search_IncludeZip,
            RuleSwitchEnd,

            Type_Off = 0,
            Type_On  = 1,
            Type_Ignore,
            Type_Include,
            Type_Whole,
            Type_PerfectMatch,
            Type_RangeMatch,

            Attrib_ReadOnly = FILE_ATTRIBUTE_READONLY,
            Attrib_System   = FILE_ATTRIBUTE_SYSTEM,
            Attrib_Hide     = FILE_ATTRIBUTE_HIDDEN,
            Attrib_Normal   = FILE_ATTRIBUTE_NORMAL,
            Attrib_Archive  = FILE_ATTRIBUTE_ARCHIVE,

            Time_Year       = 1,
            Time_Month      = 2,
            Time_Day        = 4,
            Time_Week       = 8,
        };

        int Switch[RuleSwitchEnd];
        DWORD SelectedAttributes;
        DWORD SelectedDate;
        int FileNameWithoutSuffix;
        struct {
            ULONG64 UpperBound;
            ULONG64 LowerBound; // 1 based (not zero)
            bool    Inverted;
        } FileNameRange, FileSizeRange, FileDataRange;

        std::wstring KeyWordOfFileName;
        std::vector<std::wstring> IgnoreSuffix;
        std::vector<std::wstring> ContainSuffix;
        std::map<std::wstring, int> SearchDirectory;
    } m_Filter;

    FileGroup();

    void InitFilter();
    int FindFiles(const std::wstring& dirPath, int depth);
    //int CheckFilter();
    int HashFiles();
    int ExportData();
    ULONG64 MakeSimpleTime(PSYSTEMTIME pst);
    bool IsFileMatched   (const std::wstring & path, const PWIN32_FIND_DATA pwfd);
    int  StoreMatchedFile(const std::wstring & path, const PWIN32_FIND_DATA pwfd);
    int StartSearchFiles();

    //friend void InsertListViewItem(FileGroup::pFileInfo pfi);
    friend void InsertListViewItem(FileGroup::pFileInfo pfi, std::wstring* phash = 0);
    friend void LoadFilterConfigToDialog(HWND hDlg, FileGroup& fg);
    friend void UpdateFilterConfig(HWND hDlg, FileGroup& fg);

    bool GetFileSuffix(const std::wstring & name, std::wstring& result) const
    {
        bool ret = false;

        std::wstring::size_type pos = name.rfind(L'.');
        if (pos != std::wstring::npos)
        {
            result = std::wstring(name, pos+1);
            ret = true;
        }

        return ret;
    }
};


