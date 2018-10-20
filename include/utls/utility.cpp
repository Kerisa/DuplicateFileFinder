#pragma once


#include <algorithm>
#include <iterator>
#include <set>
#include <string>
#include "utility.h"

namespace Utility
{
    std::vector<std::wstring> Splite(const std::wstring& _str, const std::wstring& _delim)
    {
        std::vector<std::wstring> out;
        std::set<wchar_t> delim;
        std::transform(_delim.begin(), _delim.end(), std::inserter(delim, delim.end()), [](const std::wstring::value_type& c) { return c; });

        size_t iLastPos = 0;
        for (size_t i = 0; i < _str.size() && iLastPos < _str.size(); )
        {
            auto it = delim.find(_str[i]);
            if (it != delim.end())
            {
                if (i > iLastPos)
                {
                    out.push_back(_str.substr(iLastPos, i - iLastPos));
                }

                // iLastPos移至下一个非delim字符上
                for (iLastPos = i + 1; iLastPos < _str.size() && delim.find(_str[iLastPos]) != delim.end(); ++iLastPos)
                    ;
                i = iLastPos + 1;
                continue;
            }
            ++i;
        }
        if (iLastPos < _str.size())
        {
            out.push_back(_str.substr(iLastPos, _str.size() - iLastPos));
        }

        return out;
    }
}