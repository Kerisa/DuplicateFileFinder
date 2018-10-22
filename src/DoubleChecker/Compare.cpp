#include "stdafx.h"
#include <algorithm>
#include <cassert>
#include <iterator>
#include <fstream>
#include <functional>
#include <map>
#include <set>
#include <vector>
#include "Compare.h"
#include "Common.h"
#include "Parammeters.h"
#include "crc32/crc.h"
#include <Windows.h>

using namespace std;

uint32_t GetFileCrc(const std::wstring& path)
{
    ifstream in(path, ios::binary);
    if (!in.is_open())
    {
        assert("cannot open file" && 0);
        return 0;
    }

    in.seekg(0, ios::end);
    ifstream::streampos length = in.tellg();
    in.seekg(0, ios::beg);

    char buf[1024] = { 0 };
    uint32_t crc = 0;
    while (length > 0)
    {
        if (length >= sizeof(buf))
        {
            in.read(buf, sizeof(buf));
            crc = CRC32((unsigned char*)buf, crc, sizeof(buf));
            length -= sizeof(buf);
        }
        else
        {
            in.read(buf, length);
            crc = CRC32((unsigned char*)buf, crc, length);
            break;
        }
    }
    in.close();
    return crc;
}

void FillFileRecord(Parameters & param)
{
    for (auto& fr : param.mFileList)
    {
        if (fr.mLastWriteTime == 0)
        {
            HANDLE hFile = CreateFile(fr.mPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                FILETIME ft;
                GetFileTime(hFile, NULL, NULL, &ft);
                fr.mLastWriteTime = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;

                DWORD sizeHigh;
                DWORD sizeLow = GetFileSize(hFile, &sizeHigh);
                fr.mFileSize = ((uint64_t)sizeHigh << 32) | sizeLow;

                CloseHandle(hFile);
            }
        }
    }
}

void SortAndDivide(vector<const FileRecord*>& ref, std::vector<std::set<const FileRecord*>>& groups,
    std::function<bool(const FileRecord* lhs, const FileRecord* rhs)> isLess,
    std::function<bool(const FileRecord* lhs, const FileRecord* rhs)> isEqual)
{
    sort(ref.begin(), ref.end(), isLess);
    for (int i = 0; i < ref.size(); )
    {
        set<const FileRecord*> g;
        g.insert(ref[i]);
        int k = i + 1;
        for (; k < ref.size(); ++k)
        {
            if (isEqual(ref[k], ref[i]))
            {
                g.insert(ref[k]);
            }
            else
            {
                break;
            }
        }
        if (g.size() > 1)
            groups.emplace_back(g);

        i = k;
    }
}

std::vector<std::set<const FileRecord*>> CompareFile(Parameters & param)
{
    //FillFileRecord(param);

    vector<const FileRecord*> ref;
    transform(param.mFileList.begin(), param.mFileList.end(), back_inserter(ref), [](const FileRecord& fr) { return &fr; });

    std::vector<std::set<const FileRecord*>> groups;

    SortAndDivide(ref, groups, [param](const FileRecord* lhs, const FileRecord* rhs) {

        if (param.mSize.mSwitch == Parameters::SizeParam::MATCH_EXACTLY && lhs->mFileSize != rhs->mFileSize)
        {
            return lhs->mFileSize < rhs->mFileSize;
        }
        else if (param.mTime.mSwitch == Parameters::TimeParam::MATCH_EXACTLY && lhs->mLastWriteTime != rhs->mLastWriteTime)
        {
            return lhs->mLastWriteTime < rhs->mLastWriteTime;
        }
        else
        {
            switch (param.mName.mSwitch)
            {
            case Parameters::NameParam::MATCH_WHOLE_NAME:
                return lhs->mPath.substr(lhs->mNameOffset) < rhs->mPath.substr(rhs->mNameOffset);
            case Parameters::NameParam::MATCH_WHOLE_NAME_WITHOUT_SUFFIX:
                return lhs->mPath.substr(lhs->mNameOffset, lhs->mSuffixOffset - lhs->mNameOffset) < rhs->mPath.substr(rhs->mNameOffset, rhs->mSuffixOffset - rhs->mNameOffset);
            case Parameters::NameParam::MATCH_STRING: {
                bool b1 = lhs->mPath.substr(lhs->mNameOffset).find(param.mName.mMatchString) != wstring::npos;
                bool b2 = rhs->mPath.substr(rhs->mNameOffset).find(param.mName.mMatchString) != wstring::npos;
                return b1 < b2;
            }
            }
        }

        return lhs < rhs;
    },
    [param](const FileRecord* lhs, const FileRecord* rhs){

        bool pass1 = param.mSize.mSwitch == Parameters::SizeParam::MATCH_DISABLE || param.mSize.mSwitch == Parameters::SizeParam::MATCH_EXACTLY && lhs->mFileSize == rhs->mFileSize;
        bool pass2 = param.mTime.mSwitch == Parameters::TimeParam::MATCH_DISABLE || param.mTime.mSwitch == Parameters::TimeParam::MATCH_EXACTLY && lhs->mLastWriteTime == rhs->mLastWriteTime;
        bool pass31 = param.mName.mSwitch == Parameters::NameParam::MATCH_WHOLE_NAME && lhs->mPath.substr(lhs->mNameOffset) == rhs->mPath.substr(rhs->mNameOffset);
        bool pass32 = param.mName.mSwitch == Parameters::NameParam::MATCH_WHOLE_NAME_WITHOUT_SUFFIX && lhs->mPath.substr(lhs->mNameOffset, lhs->mSuffixOffset - lhs->mNameOffset) == rhs->mPath.substr(rhs->mNameOffset, rhs->mSuffixOffset - rhs->mNameOffset);
        bool pass33 = param.mName.mSwitch == Parameters::NameParam::MATCH_STRING && lhs->mPath.substr(lhs->mNameOffset).find(param.mName.mMatchString) != wstring::npos && rhs->mPath.substr(rhs->mNameOffset).find(param.mName.mMatchString) != wstring::npos;
        bool pass3 = param.mName.mSwitch == Parameters::NameParam::MATCH_DISABLE || pass31 || pass32 || pass33;

        return pass1 && pass2 && pass3;
    });
    

    if (param.mContent.mSwitch == Parameters::ContentParam::MATCH_WHOLE_FILE)
    {
        std::vector<std::set<const FileRecord*>> newGroups;
        for (auto& g : groups)
        {
            std::map<uint32_t, vector<const FileRecord*>> tmp;
            for (auto& fr : g)
            {
                uint32_t crc = GetFileCrc(fr->mPath);
                tmp[crc].push_back(fr);
            }
            for (auto it = tmp.begin(); it != tmp.end(); ++it)
                if (it->second.size() > 1)
                {
                    std::set<const FileRecord*> n;
                    copy(it->second.begin(), it->second.end(), inserter(n, n.end()));
                    newGroups.push_back(n);
                }

        }

        groups = newGroups;
    }
    

    return groups;
}
