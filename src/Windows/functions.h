
#pragma once

#include <string>
#include <vector>

void SplitString                (const wchar_t *src,
                                 std::vector<std::wstring>& v,
                                 const wchar_t *key,
                                 const wchar_t  begin,
                                 const wchar_t  end,
                                 bool ignoreSpace
                                 );