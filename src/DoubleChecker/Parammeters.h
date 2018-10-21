#pragma once

#include "Common.h"
#include <string>
#include <vector>

class NameChecker
{
public:
    bool Check();
};

class Parameters
{
public:
    struct NameParam
    {
        enum Type {
            MATCH_DISABLE                       = 0,
            MATCH_WHOLE_NAME                    = 1,
            MATCH_WHOLE_NAME_WITHOUT_SUFFIX     = 2,
            MATCH_STRING                        = 4,
        };
        uint32_t        mSwitch{ MATCH_DISABLE };
        std::wstring    mMatchString;
    };

    struct SizeParam
    {
        enum Type {
            MATCH_DISABLE   = 0,
            MATCH_EXACTLY   = 1,
        };
        uint32_t        mSwitch{ MATCH_DISABLE };
    };

    struct TimeParam
    {
        enum Type {
            MATCH_DISABLE   = 0,
            MATCH_EXACTLY   = 1,
        };
        uint32_t        mSwitch{ MATCH_DISABLE };
    };

    struct ContentParam
    {
        enum Type {
            MATCH_DISABLE           = 0,
            MATCH_WHOLE_FILE        = 1,
        };
        uint32_t        mSwitch{ MATCH_DISABLE };
    };

public:
    NameParam               mName;
    SizeParam               mSize;
    TimeParam               mTime;
    ContentParam            mContent;
    std::vector<FileRecord> mFileList;

public:
    static std::wstring Usage();
    bool ParseCommand(const std::vector<std::wstring>& cmd, std::wstring& error = std::wstring());

private:
    void Reset();
    bool SetupName(const std::vector<std::wstring>& cmd, size_t& index, std::wstring& error);
    bool SetupSize(const std::vector<std::wstring>& cmd, size_t& index, std::wstring& error);
    bool SetupTime(const std::vector<std::wstring>& cmd, size_t& index, std::wstring& error);
    bool SetupContent(const std::vector<std::wstring>& cmd, size_t& index, std::wstring& error);
    bool SetupFileList(const std::wstring& path, std::wstring& error);
};
