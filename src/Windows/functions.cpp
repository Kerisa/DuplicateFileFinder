

#include "functions.h"


void SplitString(const wchar_t *src,
                 std::vector<std::wstring>& v,
                 const wchar_t *key,
                 const wchar_t  begin,
                 const wchar_t  end,
                 bool ignoreSpace)
{
    if (!src || src[0] == 0) return;
    
    int len = wcslen(src);
    wchar_t *copy = new wchar_t[len+1];
    if (!copy) return;


    int posl = 0, posr = -1;
    if (ignoreSpace)
    {
        int j = 0;
        for (int i=0; i<len; ++i)
            if (src[i] != L' ')
                copy[j++] = src[i];
        
        copy[j] = 0;
        len = j;
    }
    
    posl = 0, posr = len-1;
    for (int i=0; i<len; ++i)
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
    for (int i=posl; i<posr;)
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