

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
        Compare_FileName,       // 比较文件名
        Compare_FileSize,       // 比较大小
        Compare_FileDate,       // 比较日期
        Compare_FileHash,       // 比较数据
        Search_FileAttribute,   // 文件属性过滤
        Search_FileSize,        // 文件大小过滤
        Search_IgnoreSuffix,    // 文件类型过滤1
        Search_IncludeSuffix,   // 文件类型过滤2
        Search_IncludeZip,      // 搜索Zip压缩包
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

    std::wstring                KeyWordOfFileName;  // 文件名中包含的关键字
    std::vector<std::wstring>   IgnoreSuffix;       // 忽略的后缀名
    std::vector<std::wstring>   ContainSuffix;      // 包含的后缀名
    std::map<std::wstring, int> SearchDirectory;    // 查找目录
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
    // 列出所有文件
    std::vector<std::shared_ptr<FileInfo>> m_List0;

protected:
    // 哈希 - 对以上结果使用哈希 (勾上哈希时同时勾上“大小”)
    std::map<SHA_1::_HashResult, std::vector<std::shared_ptr<FileInfo>>> m_List1H;
    
public:
    Filter m_Filter;
    volatile bool *bTerminate;

    FileGroup();
    void InitFilter();
    bool GetFileSuffix(const std::wstring & name, std::wstring& result) const;
    bool IsFileMatched(const std::wstring & path, const PWIN32_FIND_DATA pwfd);

    // 根据条件选择哈希方式
    int PerformHash(const std::wstring& name, PBYTE result, ULONG64 size = 0);

    // 根据文件时间生成一个标记
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
    // 大小 / 日期 / (大小 + 日期)
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
    // 文件名  
    // 以文件名的crc来做索引
    std::map<DWORD, std::vector<std::shared_ptr<FileInfo>>> m_List1N;    

public:
    int HashFiles();
    int ExportData();
    int StoreMatchedFile(const std::wstring & path, const PWIN32_FIND_DATA pwfd, DWORD filenameCrc, DWORD pathCrc);
    int StartSearchFiles();
};

class FileGroupSN : public FileGroup
{
    // (大小 + 文件名) / (大小 + 日期 + 文件名) / (日期 + 文件名)
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