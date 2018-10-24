

#pragma once

#include <array>
#include <cassert>
#include <windows.h>
#include <process.h>
#include <string>
#include <set>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>
#include <Strsafe.h>
#include "Sha1.h"
#include "common.h"


struct Filter
{
    enum {
        Compare_FileName,       // 比较文件名
        Compare_FileSize,       // 比较大小
        Compare_FileDate,       // 比较日期
        Compare_FileHash,       // 比较数据
        Search_FileAttribute,   // 文件属性过滤
        Search_FileSize,        // 文件大小过滤
        Search_Suffix,			// 文件类型过滤2
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
        Attrib_Archive  = FILE_ATTRIBUTE_ARCHIVE,
        Attrib_Normal   = FILE_ATTRIBUTE_NORMAL,
    };

    std::array<int, RuleSwitchEnd>   Switch;
    DWORD SelectedAttributes;
    int   FileNameWithoutSuffix;
	uint64_t FileSizeRangeLow;
	uint64_t FileSizeRangeHigh;
    size_t FileSizeRangeLowUnitIndex;
    size_t FileSizeRangeHighUnitIndex;

    std::wstring                mKeyWordOfFileName;  // 文件名中包含的关键字
    std::vector<std::wstring>   mSuffix;      // 包含的后缀名
    std::map<std::wstring, int> mSearchDirectory;    // 查找目录
};


#define CHECKBOX_SECLECTED 2
#define CHECKBOX_UNSECLECTED 1
struct DataBaseInfo
{
    FileRecord* fi;
    SHA_1::_HashResult hr;
    BYTE checkstate;
	int mFistInGroup;

    DataBaseInfo(FileRecord* _fi, const SHA_1::_HashResult& _hr, int fistInGroup = 0, BYTE ck = 1)
        : fi(_fi), hr(_hr), checkstate(ck), mFistInGroup(fistInGroup) {}
};


extern std::vector<DataBaseInfo> g_DataBase;
extern std::vector<std::vector<FileRecord>> g_DataBase2;