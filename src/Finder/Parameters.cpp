
#include "stdafx.h"
#include <cassert>
#include <iostream>
#include <regex>
#include <utls/utility.h>
#include "Parameters.h"
#include <windows.h>

using namespace std;

wstring Parameters::Usage()
{
    return
        LR"(finder.exe path <options>
    options:
    -type +/-(type1,type2,...)
        include or exclude matched file suffix

    -size (size1:size2)
        include file size between size1 and size2
        (size1:) for equal or larger than size1, (:size2) for equal or less than size2
        suffix of size:
            empty for Bytes
            'k' : for Kilobytes
            'm' : for Megabytes
            'g' : for Gigabytes

    -attr +/-[rhsan]
        include or exclude matched file attributes
        '+' : for include
        '-' : for exclude

        'r' : for read-only
        'h' : for hidden
        's' : for system
        'a' : for archive
        'n' : for normal, n is valid only when used alone

eg:
    finder c:\\windows -type +(dll,ini) -size (500k:5m) -attr +s
    finder c:\\windows -type +(exe) -size (:500k) -attr -sha
)";
}

bool Parameters::ParseCommand(const vector<wstring>& cmd, wstring& error)
{
    Reset();

    if (cmd.size() < 1)
    {
        error = L"Missing path";
        return false;
    }

    TCHAR bbb[1024];
    GetFullPathName(cmd[0].c_str(), 1024, bbb, NULL);
    mSearchPath.clear();
    DWORD attr = GetFileAttributes(cmd[0].c_str());
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY))
    {
        error = L"[" + cmd[0] + L"] is not a valid directory";
        return false;
    }

    mSearchPath = cmd[0];
    assert(!mSearchPath.empty());
    if (mSearchPath.back() == L'\\')
        mSearchPath.pop_back();

    for (size_t i = 1; i < cmd.size();)
    {
        if (cmd[i] == L"-type")
        {
            if (mFoundOptionType)
            {
                error = L"Multi -type specified";
                return false;
            }

            if (!ParseType(cmd, i, error))
                return false;
            mFoundOptionType = true;
        }
        else if (cmd[i] == L"-size")
        {
            if (mFoundOptionSize)
            {
                error = L"Multi -size specified";
                return false;
            }

            if (!ParseSize(cmd, i, error))
                return false;
            mFoundOptionSize = true;
        }
        else if (cmd[i] == L"-attr")
        {
            if (mFoundOptionAttr)
            {
                error = L"Multi -attr specified";
                return false;
            }

            if (!ParseAttr(cmd, i, error))
                return false;
            mFoundOptionAttr = true;
        }
        else
        {
            error = L"Invalid option ";
            error += cmd[i];
            return false;
        }
    }
    return true;
}

void Parameters::Reset()
{
    mFoundOptionType = false;
    mFoundOptionSize = false;
    mFoundOptionAttr = false;

    mSearchPath.clear();
    mTypeList.clear();
    mSizeLow = 0;
    mSizeHigh = (uint64_t)-1;
    mAttrib = NONE;
    mIncludeType = false;
    mIncludeAttr = false;
}

bool Parameters::ParseType(const std::vector<std::wstring>& cmd, size_t & index, std::wstring& error)
{
    if (index >= cmd.size() - 1)
    {
        error = L"-type: Missing param";
        return false;
    }

    if (cmd[index + 1].empty() || cmd[index + 1][0] != L'+' && cmd[index + 1][0] != L'-')
    {
        error = L"-type: Missing '+' or '-'";
        return false;
    }

    mIncludeType = cmd[index + 1][0] == L'+';

    wregex pattern(L"(\\([[:alnum:]]+(,[[:alnum:]]+)*\\))");
    wsmatch match;
    wstring sub = cmd[index + 1].substr(1);
    if (!regex_match(sub, match, pattern))
    {
        error = L"-type: Invalid type list";
        return false;
    }

    mTypeList.clear();
    auto parts = Utility::Splite(sub, L"(,)");
    for (auto& suffix : parts)
        mTypeList.insert(suffix);

    index += 2;
    return true;
}

bool Parameters::ParseSize(const std::vector<std::wstring>& cmd, size_t & index, wstring& error)
{
    if (index >= cmd.size() - 1)
    {
        error = L"-size: Missing param";
        return false;
    }

    wregex pattern(L"(\\((\\d+[kmg]?)?:(\\d+[kmg]?)?\\))");
    wsmatch match;
    if (!regex_match(cmd[index + 1], match, pattern))
    {
        error = L"-size: Invalid size range";
        return false;
    }

    uint32_t shift[26] = { 0 };
    shift[L'k' - L'a'] = 1024;
    shift[L'm' - L'a'] = 1024 * 1024;
    shift[L'g' - L'a'] = 1024 * 1024 * 1024;
    auto parts = Utility::Splite(cmd[index + 1], L"(:)");
    if (parts.size() == 2)
    {
        wchar_t unit = parts[0][parts[0].size() - 1];
        mSizeLow = (uint64_t)_wtoi(parts[0].c_str()) * (unit > L'a' ? shift[unit - L'a'] : 1);
        unit = parts[1][parts[1].size() - 1];
        mSizeHigh = (uint64_t)_wtoi(parts[1].c_str()) * (unit > L'a' ? shift[unit - L'a'] : 1);

        if (mSizeLow > mSizeHigh)
        {
            error = L"-size: Low bound is greater than high bound";
            return false;
        }
    }
    else if (parts.size() == 1)
    {
        if (cmd[index + 1][1] == L':')
        {
            mSizeLow = 0;
            wchar_t unit = parts[0][parts[0].size() - 1];
            mSizeHigh = _wtoi(parts[0].c_str()) * (unit > L'a' ? shift[unit - L'a'] : 1);
        }
        else
        {
            mSizeHigh = (uint64_t)-1;
            wchar_t unit = parts[0][parts[0].size() - 1];
            mSizeLow = _wtoi(parts[0].c_str()) * (unit > L'a' ? shift[unit - L'a'] : 1);
        }
    }
    else if (parts.size() == 0)
    {
        mSizeLow = 0;
        mSizeHigh = (uint64_t)-1;
    }
    else
    {
        error = L"-size: Invalid size range";
        return false;
    }

    index += 2;
    return true;
}

bool Parameters::ParseAttr(const std::vector<std::wstring>& cmd, size_t & index, wstring& error)
{
    if (index >= cmd.size() - 1)
    {
        error = L"-attr: Missing param";
        return false;
    }

    if (cmd[index + 1].empty() || cmd[index + 1][0] != L'+' && cmd[index + 1][0] != L'-')
    {
        error = L"-attr: Missing '+' or '-'";
        return false;
    }

    mIncludeAttr = cmd[index + 1][0] == L'+';

    wregex pattern(L"(\\([rhsan]+\\))");
    wsmatch match;
    wstring sub = cmd[index + 1].substr(1);
    if (!regex_match(sub, match, pattern))
    {
        error = L"-attr: Invalid attributes list";
        return false;
    }

    mAttrib = 0;
    auto parts = Utility::Splite(sub, L"()");
    for (auto& a : parts[0])
    {
        switch (a)
        {
        case L'a': mAttrib |= Parameters::ARCHIVE; break;
        case L'r': mAttrib |= Parameters::READONLY; break;
        case L'h': mAttrib |= Parameters::HIDDEN; break;
        case L's': mAttrib |= Parameters::SYSTEM; break;
        case L'n': mAttrib |= Parameters::NORMAL; break;
        default: error = L"-attr: Invalid attribute '" + a; error += L"'"; return false;
        }
    }
    assert(mAttrib != 0);

    if ((mAttrib & Parameters::NORMAL) && mAttrib != Parameters::NORMAL)
    {
        error = L"`Normal` attribute can only used alone";
        return false;
    }

    index += 2;
    return true;
}
