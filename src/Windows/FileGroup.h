

#pragma once

#include <array>
#include <cassert>
#include <windows.h>
#include <string>
#include <map>
#include <vector>
#include "common.h"


struct Filter
{
    enum {
        Compare_FileName,       // �Ƚ��ļ���
        Compare_FileSize,       // �Ƚϴ�С
        Compare_FileDate,       // �Ƚ�����
        Compare_FileHash,       // �Ƚ�����
        Search_FileAttribute,   // �ļ����Թ���
        Search_FileSize,        // �ļ���С����
        Search_Suffix,			// �ļ����͹���2
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

    std::wstring                mKeyWordOfFileName;  // �ļ����а����Ĺؼ���
    std::vector<std::wstring>   mSuffix;      // �����ĺ�׺��
    std::map<std::wstring, int> mSearchDirectory;    // ����Ŀ¼

	std::wstring				MakeCompareCommand();
	std::vector<std::wstring>	MakeSearchCommand();
};


#define CHECKBOX_SECLECTED 2
#define CHECKBOX_UNSECLECTED 1
struct DataBaseInfo
{
    FileRecord* fi;
    BYTE checkstate;
    DWORD mBKColor{ RGB(255,255,255) };
    DWORD mTextColor{ RGB(0,0,0) };
	int mFistInGroup;

    DataBaseInfo(FileRecord* _fi, int fistInGroup = 0, BYTE ck = 1)
        : fi(_fi), checkstate(ck), mFistInGroup(fistInGroup) {}
};


extern std::vector<DataBaseInfo> g_DataBase;
extern std::vector<std::vector<FileRecord>> g_DataBase2;