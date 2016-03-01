

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
    
    
    // 根据过滤条件确定使用何种容器存储临时数据
    int m_StoreType;
    
    int pFindFiles (const std::wstring& dirPath, int depth);

    // 筛选出容器中满足其他条件的文件进行哈希
    int pHashFiles1S  ();
    int pHashFiles2SN ();
    int pHashFiles1N  ();
    int pHashFiles0   ();

    // 根据条件选择哈希方式
    int PerformHash (const std::wstring& name, std::wstring& result, ULONG64 size = 0);
    
    // 根据文件时间生成一个标记
    ULONG64 pMakeSimpleTime (PSYSTEMTIME pst);

public:
    
	struct FILTER
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
    } m_Filter;

    FileGroup();
    void    InitFilter       ();
    bool    GetFileSuffix    (const std::wstring & name, std::wstring& result) const;
    int     FindFiles        (const std::wstring& dirPath, int depth);
    int     HashFiles        ();
    int     ExportData       ();
    bool    IsFileMatched    (const std::wstring & path, const PWIN32_FIND_DATA pwfd);
    int     StoreMatchedFile (const std::wstring & path, const PWIN32_FIND_DATA pwfd);
    int     StartSearchFiles ();
};


