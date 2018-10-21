
#include "stdafx.h"
#include <cassert>
#include <fstream>
#include <regex>
#include "utls/utility.h"
#include "Parammeters.h"
#include <Windows.h>
#include <shlwapi.h>

using namespace std;

std::wstring Parameters::Usage()
{
    return
        LR"(doublecheck.exe <options> listfile
options:
-n
    file has entire same name

-ne
    file has same name except suffix

-ns str
    file has same sub-string in name

-s
    file has same size

-t
    file has same last modified time

-c
    file has same binary data


PS:
    `n/ns/ne` cannot use together

)";
}


bool Parameters::ParseCommand(const std::vector<std::wstring>& cmd, std::wstring& error)
{
    Reset();
    bool gotFileList = false;
    for (size_t i = 0; i < cmd.size() && !gotFileList; )
    {
        if (cmd[i] == L"-n" || cmd[i] == L"-ns" || cmd[i] == L"-ne")
        {
            if (!SetupName(cmd, i, error))
                return false;
        }
        else if (cmd[i] == L"-s")
        {
            if (!SetupSize(cmd, i, error))
                return false;
        }
        else if (cmd[i] == L"-t")
        {
            if (!SetupTime(cmd, i, error))
                return false;
        }
        else if (cmd[i] == L"-c")
        {
            if (!SetupContent(cmd, i, error))
                return false;
        }
        else
        {
            if (i != cmd.size() - 1)
            {
                error = L"Unknow option " + cmd[i];
                return false;
            }

            if (!SetupFileList(cmd[i], error))
                return false;

            gotFileList = true;
        }
    }

    if (!gotFileList)
    {
        error = L"File list is missing";
        return false;
    }

    if (mContent.mSwitch == ContentParam::MATCH_WHOLE_FILE)
        mSize.mSwitch = SizeParam::MATCH_EXACTLY;

    return true;
}

void Parameters::Reset()
{
    mName.mSwitch = NameParam::MATCH_DISABLE;
    mName.mMatchString.clear();
    mSize.mSwitch = SizeParam::MATCH_DISABLE;
    mTime.mSwitch = TimeParam::MATCH_DISABLE;
    mContent.mSwitch = ContentParam::MATCH_DISABLE;
    mFileList.clear();
}

bool Parameters::SetupName(const std::vector<std::wstring>& cmd, size_t & index, std::wstring & error)
{
    if (mName.mSwitch != NameParam::MATCH_DISABLE)
    {
        error = L"`n/ns/np` cannot use together";
        return false;
    }

    if (cmd[index] == L"-n")
    {
        mName.mSwitch = NameParam::MATCH_WHOLE_NAME;
        ++index;
        return true;
    }
    else if (cmd[index] == L"-ne")
    {
        mName.mSwitch = NameParam::MATCH_WHOLE_NAME_WITHOUT_SUFFIX;
        ++index;
        return true;
    }
    else if (cmd[index] == L"-ns")
    {
        if (index > cmd.size() - 2)
        {
            error = L"Match string is missing";
            return false;
        }
        mName.mSwitch = NameParam::MATCH_STRING;
        mName.mMatchString = cmd[index + 1];
        index += 2;
        return true;
    }
    else
    {
        error = L"Error name option";
        return false;
    }
}

bool Parameters::SetupSize(const std::vector<std::wstring>& cmd, size_t & index, std::wstring & error)
{
    if (mSize.mSwitch != SizeParam::MATCH_DISABLE)
    {
        error = L"Multi size option specified";
        return false;
    }

    mSize.mSwitch = SizeParam::MATCH_EXACTLY;
    ++index;
    return true;
}

bool Parameters::SetupTime(const std::vector<std::wstring>& cmd, size_t & index, std::wstring & error)
{
    if (mTime.mSwitch != TimeParam::MATCH_DISABLE)
    {
        error = L"Multi time option specified";
        return false;
    }

    mTime.mSwitch = TimeParam::MATCH_EXACTLY;
    ++index;
    return true;
}

bool Parameters::SetupContent(const std::vector<std::wstring>& cmd, size_t & index, std::wstring & error)
{
    if (mContent.mSwitch != ContentParam::MATCH_DISABLE)
    {
        error = L"Multi content option specified";
        return false;
    }

    if (cmd[index] == L"-c")
    {
        mContent.mSwitch = ContentParam::MATCH_WHOLE_FILE;
        ++index;
        return true;
    }
    else
    {
        error = L"Error content option";
        return false;
    }
}

bool Parameters::SetupFileList(const std::wstring & path, std::wstring& error)
{
    ifstream in(path, ios::binary);
    if (!in.is_open())
    {
        error = path + L" open failed";
        return false;
    }

    in.seekg(0, ios::end);
    uint64_t length = in.tellg();
    assert(length < 0x100000000llu);
    in.seekg(0, ios::beg);
    vector<char> data((size_t)length + 2);
    in.read(data.data(), length);
    in.close();
    auto parts = Utility::Splite((const wchar_t*)data.data(), L"\r\n");
    for (auto& name : parts)
    {
        if (PathFileExists(name.c_str()))
        {
            mFileList.emplace_back();
            FileRecord& fr = mFileList.back();
            fr.mPath = name;
            fr.mSuffixOffset = PathFindExtension(name.c_str()) - name.c_str();
            if (fr.mSuffixOffset < name.size())
                ++fr.mSuffixOffset;
            fr.mNameOffset = PathFindFileName(name.c_str()) - name.c_str();
        }
    }

    if (mFileList.empty())
    {
        error = L"Not a valid file list";
        return false;
    }
    else
    {
        return true;
    }
}
