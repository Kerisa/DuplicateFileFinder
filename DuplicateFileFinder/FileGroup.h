

#pragma once

#include <assert.h>
#include <windows.h>
#include <process.h>
#include <string>
#include <set>
#include <map>
#include <vector>
#include <Strsafe.h>
#include "Sha1.h"


class FileGroup;


class PerformanceAnalyse
{
};

class Log
{
};

struct Filter
{
    enum {
        Compare_FileName,       // �Ƚ��ļ���
        Compare_FileSize,       // �Ƚϴ�С
        Compare_FileDate,       // �Ƚ�����
        Compare_FileHash,       // �Ƚ�����
        Search_FileAttribute,   // �ļ����Թ���
        Search_FileSize,        // �ļ���С����
        Search_IgnoreSuffix,    // �ļ����͹���1
        Search_IncludeSuffix,   // �ļ����͹���2
        Search_IncludeZip,      // ����Zipѹ����
        RuleSwitchEnd,

        Type_Off = 0,
        Type_On  = 1,
        Type_Ignore,
        Type_Include,
        Type_Whole,
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

    int   Switch[RuleSwitchEnd];
    DWORD SelectedAttributes;
    DWORD SelectedDate;
    int   FileNameWithoutSuffix;
    struct {
        ULONG64 UpperBound;
        ULONG64 LowerBound; // 1 based (not zero)
        bool    Inverted;
    } FileNameRange, FileSizeRange, FileDataRange;

    std::wstring                KeyWordOfFileName;  // �ļ����а����Ĺؼ���
    std::vector<std::wstring>   IgnoreSuffix;       // ���Եĺ�׺��
    std::vector<std::wstring>   ContainSuffix;      // �����ĺ�׺��
    std::map<std::wstring, int> SearchDirectory;    // ����Ŀ¼
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
        FILETIME CreationTime;
        FILETIME LastWriteTime;
        std::wstring Path;
        std::wstring Name;
    }
    FileInfo, *pFileInfo;

    // �г������ļ�
    std::vector<FileInfo>                                   m_List0;

    // ��ϣ - �����Ͻ��ʹ�ù�ϣ (���Ϲ�ϣʱͬʱ���ϡ���С��)
    std::map<std::wstring, std::vector<pFileInfo>>          m_List1H;
    
public:
    Filter m_Filter;
    volatile bool *bTerminate;

    FileGroup();
    void    InitFilter       ();
    bool    GetFileSuffix    (const std::wstring & name, std::wstring& result) const;
    int     FindFiles        (const std::wstring& dirPath, int depth);
    bool    IsFileMatched    (const std::wstring & path, const PWIN32_FIND_DATA pwfd);
    int PerformHash (const std::wstring& name, std::wstring& result, ULONG64 size = 0);// ��������ѡ���ϣ��ʽ
    ULONG64 pMakeSimpleTime (PSYSTEMTIME pst);// �����ļ�ʱ������һ�����
    int     ExportHashData       ();
    virtual int     HashFiles        ();
    virtual int     ExportData       ();
    virtual int     StoreMatchedFile (const std::wstring & path, const PWIN32_FIND_DATA pwfd);
    virtual int     StartSearchFiles ();
};


class FileGroupS : public FileGroup
{
public:
    // ��С / ���� / ��С + ����
    std::map<ULONG64, std::vector<FileInfo>> m_List1S;
    FileGroupS() : FileGroup() { m_List1S.clear(); }
    int     HashFiles        ();
    int     ExportData       ();
    int     StoreMatchedFile (const std::wstring & path, const PWIN32_FIND_DATA pwfd);
    int     StartSearchFiles ();
};

class FileGroupN : public FileGroup
{
public:
    // �ļ���
    std::map<DWORD, std::vector<FileInfo>> m_List1N;    // ���ļ�����crc��������

    int     HashFiles        ();
    int     ExportData       ();
    int     StoreMatchedFile (const std::wstring & path, const PWIN32_FIND_DATA pwfd);
    int     StartSearchFiles ();
};

class FileGroupSN : public FileGroup
{
public:
    // ��С + �ļ��� / ��С + ���� + �ļ��� / ���� + �ļ���
    std::multimap<ULONG64, std::map<DWORD, std::vector<FileInfo>>> m_List2SN;

    int     HashFiles        ();
    int     ExportData       ();
    int     StoreMatchedFile (const std::wstring & path, const PWIN32_FIND_DATA pwfd);
    int     StartSearchFiles ();
};