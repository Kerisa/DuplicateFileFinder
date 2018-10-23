#pragma once

#include <set>
#include <string>
#include <vector>

class Parameters
{
    class StringCompare
    {
    public:
        bool operator()(const std::wstring& lhs, const std::wstring& rhs) const
        {
            return _wcsicmp(lhs.c_str(), rhs.c_str()) < 0;
        }
    };

public:
    enum {
        NONE        = 0x0,
        READONLY    = 0x1,
        HIDDEN      = 0x2,
        SYSTEM      = 0x4,
        ARCHIVE     = 0x8,
        NORMAL      = 0x10,
    };

    std::wstring                            mSearchPath;
    std::set<std::wstring, StringCompare>   mTypeList;
    uint64_t                                mSizeLow{ 0 };
    uint64_t                                mSizeHigh{ 0xfffffffffffffffful };
    uint32_t                                mAttrib{ NONE };
    bool                                    mIncludeType{ false };
    bool                                    mIncludeAttr{ false };

public:
    static std::wstring Usage();
    bool ParseCommand(const std::vector<std::wstring>& cmd, std::wstring& error = std::wstring());

private:
    void Reset();

    bool ParseType(const std::vector<std::wstring>& cmd, size_t& index, std::wstring& error);
    bool ParseSize(const std::vector<std::wstring>& cmd, size_t& index, std::wstring& error);
    bool ParseAttr(const std::vector<std::wstring>& cmd, size_t& index, std::wstring& error);

    bool mFoundOptionType{ false };
    bool mFoundOptionSize{ false };
    bool mFoundOptionAttr{ false };
};
