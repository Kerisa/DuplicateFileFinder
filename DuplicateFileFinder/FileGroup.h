

#pragma once

#include <assert.h>
#include <windows.h>
#include <process.h>
#include <string>
#include <set>
#include <map>
#include <unordered_map>
#include <vector>
#include <Strsafe.h>
#include "Sha1.h"



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
        DWORD NameCrc;
        DWORD PathCrc;
    }
    FileInfo, *pFileInfo;

private:
    // �г������ļ�
    std::vector<std::shared_ptr<FileInfo>> m_List0;

protected:
    // ��ϣ - �����Ͻ��ʹ�ù�ϣ (���Ϲ�ϣʱͬʱ���ϡ���С��)
    std::map<SHA_1::_HashResult, std::vector<std::shared_ptr<FileInfo>>> m_List1H;
    
public:
    Filter m_Filter;
    volatile bool *bTerminate;

    FileGroup();
    void InitFilter();
    bool GetFileSuffix(const std::wstring & name, std::wstring& result) const;
    bool IsFileMatched(const std::wstring & path, const PWIN32_FIND_DATA pwfd);

    // ��������ѡ���ϣ��ʽ
    int PerformHash(const std::wstring& name, PBYTE result, ULONG64 size = 0);

    // �����ļ�ʱ������һ�����
    ULONG64 pMakeSimpleTime(PSYSTEMTIME pst);

    virtual int HashFiles();
    virtual int StoreMatchedFile(const std::wstring & path, const PWIN32_FIND_DATA pwfd, DWORD filenameCrc, DWORD pathCrc);

public:
    int ExportHashData();
    virtual int ExportData();
    std::wstring GetFileNameByCrc(DWORD crc) const;
    std::wstring MakePath(DWORD crc) const;
    int FindFiles(const std::wstring& dirPath, int depth, DWORD parentCrc);
    virtual int StartSearchFiles();
};


class FileGroupS : public FileGroup
{
    // ��С / ���� / (��С + ����)
    std::map<ULONG64, std::vector<std::shared_ptr<FileInfo>>> m_List1S;

public:
    FileGroupS() : FileGroup() { m_List1S.clear(); }
    int HashFiles();
    int ExportData();
    int StoreMatchedFile(const std::wstring & path, const PWIN32_FIND_DATA pwfd, DWORD filenameCrc, DWORD pathCrc);
    int StartSearchFiles();
};

class FileGroupN : public FileGroup
{
    // �ļ���  
    // ���ļ�����crc��������
    std::map<DWORD, std::vector<std::shared_ptr<FileInfo>>> m_List1N;    

public:
    int HashFiles();
    int ExportData();
    int StoreMatchedFile(const std::wstring & path, const PWIN32_FIND_DATA pwfd, DWORD filenameCrc, DWORD pathCrc);
    int StartSearchFiles();
};

class FileGroupSN : public FileGroup
{
    // (��С + �ļ���) / (��С + ���� + �ļ���) / (���� + �ļ���)
    std::multimap<ULONG64, std::map<DWORD, std::vector<std::shared_ptr<FileInfo>>>> m_List2SN;

public:
    int HashFiles();
    int ExportData();
    int StoreMatchedFile(const std::wstring & path, const PWIN32_FIND_DATA pwfd, DWORD filenameCrc, DWORD pathCrc);
    int StartSearchFiles();
};


#define CHECKBOX_SECLECTED 2
#define CHECKBOX_UNSECLECTED 1
struct DataBaseInfo
{
    std::shared_ptr<FileGroup::FileInfo> fi;
    SHA_1::_HashResult hr;
    BYTE checkstate;

    DataBaseInfo(std::shared_ptr<FileGroup::FileInfo>&& _fi, const SHA_1::_HashResult& _hr, BYTE ck = 1)
        : fi(_fi), hr(_hr), checkstate(ck) {}
};


extern std::map<DWORD, std::pair<DWORD, std::wstring>> g_PathList;
extern std::vector<DataBaseInfo> g_DataBase;