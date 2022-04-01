

#include "functions.h"
#include <windows.h>

void SplitString(const wchar_t* src,
  std::vector<std::wstring>& v,
  const wchar_t* key,
  const wchar_t  begin,
  const wchar_t  end,
  bool ignoreSpace)
{
  if (!src || src[0] == 0) return;

  int len = wcslen(src);
  wchar_t* copy = new wchar_t[len + 1];
  if (!copy) return;


  int posl = 0, posr = -1;
  if (ignoreSpace)
  {
    int j = 0;
    for (int i = 0; i < len; ++i)
      if (src[i] != L' ')
        copy[j++] = src[i];

    copy[j] = 0;
    len = j;
  }

  posl = 0, posr = len - 1;
  for (int i = 0; i < len; ++i)
  {
    if (copy[i] == begin)
      posl = i + 1;
    else if (copy[i] == end)
    {
      posr = i - 1;
      copy[i] = 0;
      break;
    }
  }
  if (posr < posl)
    return;

  int len1 = wcslen(key), j = posl;
  for (int i = posl; i < posr;)
  {
    if (!wcsncmp(&copy[i], key, len1))
    {
      wchar_t bak = copy[i];
      copy[i] = 0;
      v.push_back(std::wstring(&copy[j]));
      i = j = i + len1;
    }
    else
      ++i;
  }

  if (j < posr)
    v.push_back(std::wstring(&copy[j]));

  delete[] copy;
}

std::wstring GetFileName(const std::wstring& s)
{
  std::vector<wchar_t> name(s.size() + 1);
  _wsplitpath_s(s.c_str(), nullptr, 0, nullptr, 0, name.data(), name.size(), nullptr, 0);
  return std::wstring(name.data());
}

size_t GetPathDepth(const std::wstring& s)
{
  std::vector<wchar_t> dir(s.size() + 1);
  _wsplitpath_s(s.c_str(), nullptr, 0, dir.data(), dir.size(), nullptr, 0, nullptr, 0);
  std::vector<std::wstring> result;
  SplitString(dir.data(), result, L"\\", 0, 0, true);
  return result.size();
}
