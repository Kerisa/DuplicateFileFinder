
#pragma once

#include <string>

class FileRecord
{
public:
    //static const int mcValidPartsInRecord = 4;
    static std::string ToUTF8(const FileRecord& fr);
    static bool FromUTF16(const std::wstring & str, FileRecord& fr);

public:
    std::wstring mPath;
    size_t       mNameOffset{ 0 };
    size_t       mSuffixOffset{ 0 };
    uint64_t     mLastWriteTime{ 0 };
    uint64_t     mFileSize{ 0 };
    uint32_t     mCRC{ 0 };
};

#define Assert(x) ((x) || (__debugbreak(), 0))
