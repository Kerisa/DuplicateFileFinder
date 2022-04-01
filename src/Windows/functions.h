
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

std::wstring GetFileName(const std::wstring& s);

size_t GetPathDepth(const std::wstring& s);