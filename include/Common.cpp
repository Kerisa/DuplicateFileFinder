
#pragma once

#include <cassert>
#include <vector>
#include "Common.h"
#include "utls/utility.h"
#include <Windows.h>
#include <Shlwapi.h>


#pragma comment(lib, "Shlwapi.lib")

using namespace std;

std::string FileRecord::ToUTF8(const FileRecord & fr)
{
    string str;

    int n = WideCharToMultiByte(CP_UTF8, NULL, fr.mPath.c_str(), -1, NULL, NULL, NULL, NULL);
    vector<char> mcb(n + 1);
    WideCharToMultiByte(CP_UTF8, NULL, fr.mPath.c_str(), -1, mcb.data(), n, NULL, NULL);
    str = mcb.data();

    wchar_t wbuf[1024];
    swprintf_s(wbuf, _countof(wbuf), L"|%lld|%lld|%08x\n", fr.mFileSize, fr.mLastWriteTime, fr.mCRC);
    n = WideCharToMultiByte(CP_UTF8, NULL, wbuf, -1, NULL, NULL, NULL, NULL);
    mcb.resize(n + 1);
    WideCharToMultiByte(CP_UTF8, NULL, wbuf, -1, mcb.data(), n, NULL, NULL);
    str += mcb.data();
    return str;
}

bool FileRecord::FromUTF16(const std::wstring& str, FileRecord& fr)
{
    auto parts = Utility::Splite(str, L"|");
    if (parts.size() != 4)      // 无效的行
    {
        assert("Invalid row" && 0);
        return false;
    }

    if (PathFileExists(parts[0].c_str()))
    {
        fr.mPath = parts[0];
        fr.mSuffixOffset = PathFindExtension(parts[0].c_str()) - parts[0].c_str();
        if (fr.mSuffixOffset < parts[0].size())
            ++fr.mSuffixOffset;
        fr.mNameOffset = PathFindFileName(parts[0].c_str()) - parts[0].c_str();

        fr.mFileSize = _wtoll(parts[1].c_str());
        fr.mLastWriteTime = _wtoll(parts[2].c_str());
        fr.mCRC = static_cast<uint32_t>(wcstoll(parts[3].c_str(), nullptr, 16));
    }

    return true;
}
