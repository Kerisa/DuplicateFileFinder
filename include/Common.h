
#pragma once

#include <string>

class FileRecord
{
public:
    std::wstring mPath;
    size_t       mNameOffset{ 0 };
    size_t       mSuffixOffset{ 0 };
    uint64_t     mLastWriteTime{ 0 };
    uint64_t     mFileSize{ 0 };
};

#define Assert(x) ((x) || (__debugbreak(), 0))