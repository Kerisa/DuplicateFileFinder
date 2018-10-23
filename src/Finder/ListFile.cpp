
#include "stdafx.h"
#include <cassert>
#include <vector>
#include <string>
#include "Parameters.h"
#include "ListFile.h"
#include <windows.h>

using namespace std;

bool IsFileMatch(WIN32_FIND_DATA* pwfd, Parameters& param)
{
    uint64_t size = ((uint64_t)pwfd->nFileSizeHigh << 32) | pwfd->nFileSizeLow;
    if (size < param.mSizeLow || size > param.mSizeHigh)
        return false;

    bool attrMatch = false;
    if (param.mAttrib & Parameters::ARCHIVE)
        attrMatch |= !!(pwfd->dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE);
    if (param.mAttrib & Parameters::SYSTEM)
        attrMatch |= !!(pwfd->dwFileAttributes & FILE_ATTRIBUTE_SYSTEM);
    if (param.mAttrib & Parameters::HIDDEN)
        attrMatch |= !!(pwfd->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN);
    if (param.mAttrib & Parameters::READONLY)
        attrMatch |= !!(pwfd->dwFileAttributes & FILE_ATTRIBUTE_READONLY);
    if (param.mAttrib & Parameters::NORMAL)
        attrMatch |= !!(pwfd->dwFileAttributes & FILE_ATTRIBUTE_NORMAL);

    if (param.mIncludeAttr ^ attrMatch)
        return false;

    wstring name(pwfd->cFileName);
    size_t dot = name.find_last_of(L'.');
    wstring suffix = dot != wstring::npos ? name.substr(dot+1) : L"";
    bool typeMatch = param.mTypeList.find(suffix) != param.mTypeList.end();
    if (param.mIncludeType ^ typeMatch)
        return false;

    return true;
}

void StoreFile(const wstring& dirPath, WIN32_FIND_DATA* pwfd, vector<FileRecord>& group)
{
    group.emplace_back();
    FileRecord& fr = group.back();

    fr.mPath = dirPath;
    fr.mPath += L"\\";
    fr.mNameOffset = fr.mPath.size();
    group.back().mPath += pwfd->cFileName;
    fr.mNameOffset = fr.mPath.find_last_of(L'.');

    fr.mFileSize = ((uint64_t)pwfd->nFileSizeHigh << 32) | pwfd->nFileSizeLow;
    fr.mLastWriteTime = ((uint64_t)pwfd->ftLastWriteTime.dwHighDateTime << 32) | pwfd->ftLastWriteTime.dwLowDateTime;
}

void FindFiles(const std::wstring& dirPath, int depth, Parameters& param, vector<FileRecord>& group)
{
    HANDLE	hFind;
    WIN32_FIND_DATA wfd;
    std::wstring path;

    path = dirPath + L"\\*.*";
    
    hFind = FindFirstFile(path.c_str(), &wfd);
    if (depth && hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (depth == 0)
                    continue;
                
                if (wcscmp(wfd.cFileName, L".") && wcscmp(wfd.cFileName, L".."))	// 递归查找子文件夹
                {
                    std::wstring search = dirPath;
                    search += L"\\";
                    search += wfd.cFileName;
                    FindFiles(search, depth == -1 ? -1 : depth - 1, param, group);
                }
                else
                {
                    continue;
                }
            }
            else if (IsFileMatch(&wfd, param))	// 文件类型过滤
            {
                StoreFile(dirPath, &wfd, group);
            }
            else
            {
                continue;
            }
            
        } while (FindNextFile(hFind, &wfd));
    }
    FindClose(hFind);
}

std::vector<FileRecord> ListFile(Parameters & param)
{
    vector<FileRecord> group;
    FindFiles(param.mSearchPath, -1, param, group);
    return group;
}
