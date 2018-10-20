#pragma once

#include <string>
#include <vector>

class Parameters;

class FileRecord
{
public:
    std::wstring mPath;
    size_t       mNameOffset    { 0 };
    size_t       mSuffixOffset  { 0 };
    uint64_t     mLastWriteTime { 0 };
    uint64_t     mFileSize      { 0 };
};

std::vector<FileRecord> ListFile(Parameters& param);