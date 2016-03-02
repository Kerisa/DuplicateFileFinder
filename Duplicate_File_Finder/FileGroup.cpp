
#include "FileGroup.h"
#include <CommCtrl.h>

extern HWND g_hStatus, g_hList;
extern volatile bool bTerminateThread;
extern void UpdateStatusBar(int part, const wchar_t *text);
extern void InsertListViewItem(FileGroup::pFileInfo pfi, int groupid, std::wstring* phash = 0);


FileGroup::FileGroup()
{
    InitFilter();
}


void FileGroup::InitFilter()
{
    memset(m_Filter.Switch, 0, _countof(m_Filter.Switch));
    m_Filter.SelectedAttributes = 0;
    m_Filter.SelectedDate = 0;
    
    memset(&m_Filter.FileNameRange, 0, sizeof(m_Filter.FileNameRange));
    memset(&m_Filter.FileSizeRange, 0, sizeof(m_Filter.FileNameRange));
    memset(&m_Filter.FileDataRange, 0, sizeof(m_Filter.FileNameRange));

    m_Filter.KeyWordOfFileName.clear();
    m_Filter.IgnoreSuffix.clear();
    m_Filter.ContainSuffix.clear();
    m_Filter.SearchDirectory.clear();

    // Default ****************************************************************

    m_Filter.Switch[Filter::Compare_FileSize] = Filter::Type_Whole;
    m_Filter.Switch[Filter::Compare_FileHash] = Filter::Type_Whole;
}


inline bool FileGroup::GetFileSuffix(const std::wstring & name, std::wstring& result) const
{
    bool ret = false;

    std::wstring::size_type pos = name.rfind(L'.');
    if (pos != std::wstring::npos)
    {
        result = std::wstring(name, pos+1);
        ret = true;
    }

    return ret;
}


int FileGroup::StoreMatchedFile(const std::wstring& path, const PWIN32_FIND_DATA pwfd)
{
    SYSTEMTIME st;
    FileTimeToSystemTime(&pwfd->ftLastWriteTime, &st);

    ULONG64 idx = 0, size = ((ULONG64)pwfd->nFileSizeHigh << 32) | pwfd->nFileSizeLow;
    if (m_Filter.Switch[Filter::Compare_FileSize])
        idx = size;
    
    if (m_Filter.Switch[Filter::Compare_FileDate])
        idx += pMakeSimpleTime(&st);
    
    std::wstring name(pwfd->cFileName);
    FileInfo fi;
    fi.Name = std::wstring(name);
    fi.Path = std::wstring(path);
    fi.Size = size;
    fi.CreationTime   = pwfd->ftCreationTime;
    fi.LastWriteTime  = pwfd->ftLastWriteTime;


    m_List0.push_back(fi);

    return 0;
}


ULONG64 FileGroup::pMakeSimpleTime(PSYSTEMTIME pst)
{
    // 31 29 28 24 23 20 19       4 3   0
    // +----+-----+-----+----------+-----+
    // | 3  |  5  |  4  |    16    |     |
    // +----+-----+-----+----------+-----+
    //  星期  日期  月份     年份

    ULONG64 ret = 0;

    if (m_Filter.SelectedDate & m_Filter.Time_Year)
        ret += pst->wYear << 4;

    if (m_Filter.SelectedDate & m_Filter.Time_Month)
        ret += pst->wMonth << 20;

    if (m_Filter.SelectedDate & m_Filter.Time_Day)
        ret += pst->wDay << 24;

    if (m_Filter.SelectedDate & m_Filter.Time_Week)
        ret += pst->wDayOfWeek << 29;

    return ret << 32;
}


int FileGroup::ExportHashData()
{
    LVGROUP group;

    int gid         = 1;
    group.cbSize    = sizeof(LVGROUP);
    group.mask      = /*LVGF_HEADER | */LVGF_GROUPID;

    std::map<std::wstring, std::vector<pFileInfo>>::iterator it;
    std::vector<pFileInfo>::iterator it1;
    for (it=m_List1H.begin(); it != m_List1H.end(); ++it)
    {
        if (it->second.size() > 1)
        {
            group.iGroupId  = gid;
            ListView_InsertGroup(g_hList, -1, &group);

            for (it1=it->second.begin(); it1!=it->second.end(); ++it1)
                InsertListViewItem(*it1, gid, const_cast<std::wstring*>(&it->first));
                
            ++gid;
        }
    }

    return 0;
}

int FileGroup::ExportData()
{
    LVGROUP group;

    int gid         = 1;
    group.cbSize    = sizeof(LVGROUP);
    group.mask      = /*LVGF_HEADER | */LVGF_GROUPID;
    

    if (m_Filter.Switch[Filter::Compare_FileHash] != Filter::Type_Off)
    {
        ExportHashData();
    }
    else
    {
        group.iGroupId  = gid;
        ListView_InsertGroup(g_hList, -1, &group);

        std::vector<FileInfo>::iterator it;
        for (it=m_List0.begin(); it != m_List0.end(); ++it)
        {
            InsertListViewItem(&(*it), gid);
        }

    }

    return 0;
}

inline bool _cdecl HashCallback(const long long * const plen1, const long long * const plen2)
{
    return !bTerminateThread;
}


int FileGroup::PerformHash(const std::wstring& name, std::wstring& result, ULONG64 size /*= 0*/)
{
    SHA_1 Hash;
    int   ret;

    // 更新状态栏
    std::wstring status(L"正在哈希: ");
    status += name;
    UpdateStatusBar(0, status.c_str());

    do 
    {
        if (m_Filter.Switch[Filter::Compare_FileHash]
            == Filter::Type_RangeMatch)
        {
            if (size < m_Filter.FileDataRange.UpperBound ||
                m_Filter.FileDataRange.UpperBound < m_Filter.FileDataRange.LowerBound)
            {
                ret = SHA_1::S_INVALID_PARAMETER;
                break;
            }

            if (m_Filter.FileDataRange.Inverted)    // 最后XX字节
                ret = Hash.CalculateSha1(const_cast<wchar_t*>(name.c_str()),
                    size - m_Filter.FileDataRange.UpperBound,
                    m_Filter.FileDataRange.UpperBound - m_Filter.FileDataRange.LowerBound + 1,
                    (SHA_1::CallBack)HashCallback);
        
            else
                ret = Hash.CalculateSha1(const_cast<wchar_t*>(name.c_str()),
                    m_Filter.FileDataRange.LowerBound - 1,
                    m_Filter.FileDataRange.UpperBound - m_Filter.FileDataRange.LowerBound + 1,
                    (SHA_1::CallBack)HashCallback);
        }            
        else
            ret = Hash.CalculateSha1(const_cast<wchar_t*>(name.c_str()),
                (SHA_1::CallBack)HashCallback);
    
    } while (0);
                
    
    if (ret == SHA_1::S_NO_ERR)
        result = Hash.GetHashResult();
    else
        result = L'\0';

    return ret;
}


int FileGroup::HashFiles()
{
    std::vector<FileInfo>::iterator it;
            
    for (it=m_List0.begin(); it!=m_List0.end(); ++it)
    {
        std::wstring str, tmp(it->Path);
        (tmp += L'\\') += it->Name;

        if (PerformHash(tmp, str, it->Size) != SHA_1::S_NO_ERR)
            continue;

        std::map<std::wstring, std::vector<pFileInfo>>::iterator it2;
        if ((it2 = m_List1H.find(str)) != m_List1H.end())
            it2->second.push_back(&(*it));
        else
        {
            std::vector<pFileInfo> v;
            v.push_back(&(*it));
            m_List1H.insert(std::make_pair(str, v));
        }
    }

    return 0;
}


bool FileGroup::IsFileMatched(const std::wstring & path, const PWIN32_FIND_DATA pwfd)
{
    bool ret = true;
    
    // 后缀名
    if (m_Filter.Switch[m_Filter.Search_IncludeSuffix] != Filter::Type_Off)
    {
        std::wstring suf;
        if (!GetFileSuffix(pwfd->cFileName, suf) || !suf.size())
        {
            ret &= false;
            return ret;
        }

        bool e = false;
        for (int i=m_Filter.ContainSuffix.size()-1; i>=0; --i)
            e |= !wcscmp(m_Filter.ContainSuffix[i].c_str(), suf.c_str());
        
        if (!e)
        {
            ret &= false;
            return ret;
        }
    }

    if (m_Filter.Switch[m_Filter.Search_IgnoreSuffix] != Filter::Type_Off)
    {
        std::wstring suf;
        if (!GetFileSuffix(pwfd->cFileName, suf) || !suf.size())
        {
            ret &= false;
            return ret;
        }

        bool e = true;
        for (int i=m_Filter.IgnoreSuffix.size()-1; i>=0; --i)
            e &= wcscmp(m_Filter.IgnoreSuffix[i].c_str(), suf.c_str());
        
        if (!e)
        {
            ret &= false;
            return ret;
        }
    }

    // Zip
    //if (m_Filter.Switch[m_Filter.Search_IncludeZip])
    //{
    //    std::wstring *suf = 0;
    //    if (!GetFileSuffix(pwfd->cFileName, suf) || !suf)
    //    {
    //        ret &= false;
    //        return ret;
    //    }

    //    if (wcscmp(suf->c_str(), L"zip"))
    //    {
    //        ret &= false;
    //        return ret;
    //    }
    //}

    // 文件大小
    if (m_Filter.Switch[m_Filter.Search_FileSize])
    {
        ULARGE_INTEGER ui;
        ui.LowPart  = pwfd->nFileSizeLow;
        ui.HighPart = pwfd->nFileSizeHigh;
        if (ui.QuadPart < m_Filter.FileSizeRange.LowerBound ||
            ui.QuadPart > m_Filter.FileSizeRange.UpperBound)
        {
            if (m_Filter.Switch[m_Filter.Search_FileSize] == m_Filter.Type_Include)
            {
                ret &= false;
                return ret;
            }
        }
        else
        {
            if (m_Filter.Switch[m_Filter.Search_FileSize] == m_Filter.Type_Ignore)
            {
                ret &= false;
                return ret;
            }
        }
    }

    // 文件属性
    static const int FileAttr[] = {
        FILE_ATTRIBUTE_NORMAL, FILE_ATTRIBUTE_ARCHIVE, FILE_ATTRIBUTE_HIDDEN,
        FILE_ATTRIBUTE_READONLY, FILE_ATTRIBUTE_SYSTEM
    };

    bool include = true;
    if (m_Filter.Switch[m_Filter.Search_FileAttribute]
        != Filter::Type_Off)
    {
        if (pwfd->dwFileAttributes == INVALID_FILE_ATTRIBUTES)
        {
            ret &= false;
            // LOG
            return ret;
        }

        if (m_Filter.Switch[m_Filter.Search_FileAttribute] == m_Filter.Type_Include)
        {
            include = false;
            for (int i=0; i<_countof(FileAttr); ++i)
                if (m_Filter.SelectedAttributes & FileAttr[i])  // 属性被勾选
                {
                    if (pwfd->dwFileAttributes & FileAttr[i])
                    {
                        include = true;
                        break;
                    }
                }
        }
        else
        {
            include = true;
            for (int i=0; i<_countof(FileAttr); ++i)
                if (m_Filter.SelectedAttributes & FileAttr[i])  // 属性被勾选
                {
                    if (pwfd->dwFileAttributes & FileAttr[i])
                    {
                        include = false;
                        break;
                    }
                }
        }
    }
    ret &= include;

    return ret;
}


int FileGroup::FindFiles(const std::wstring& dirPath, int depth)
{
    HANDLE	hFind;
    WIN32_FIND_DATA wfd;
    std::wstring path, search;

	path = dirPath + L"\\*.*";

    // 更新状态栏
    std::wstring status(L"正在查找: ");
    status += dirPath;
    UpdateStatusBar(0, status.c_str());

    hFind = FindFirstFile(path.c_str(), &wfd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (depth != 0 &&
                    wcscmp(wfd.cFileName, L".") && wcscmp(wfd.cFileName, L".."))	// 递归查找子文件夹
				{
					search = dirPath;
                    search += L"\\";
                    search += wfd.cFileName;
					FindFiles(search, depth==-1 ? -1 : depth-1);
				}
			}
			else if (IsFileMatched(dirPath, &wfd))	// 文件类型过滤
			{
                StoreMatchedFile(dirPath, &wfd);
			}
		}
        while (FindNextFile(hFind, &wfd));
	}
	FindClose(hFind);

    return 0;
}


int FileGroup::StartSearchFiles()
{
    m_List0.clear();
    m_List1H.clear();

    // 设置状态栏
    UpdateStatusBar(0, L"查找文件...");

    std::map<std::wstring, int>::iterator it;
    for (it=m_Filter.SearchDirectory.begin(); it!=m_Filter.SearchDirectory.end(); ++it)
    {
        FindFiles(it->first, 100);
    }

    // 设置状态栏
    UpdateStatusBar(0, L"查找完成");

    if (m_Filter.Switch[Filter::Compare_FileHash] != Filter::Type_Off)
    {
        HashFiles();
    }
    
    // 设置状态栏
    UpdateStatusBar(0, L"正在生成列表");

    ExportData();

    // 设置状态栏
    UpdateStatusBar(0, L"操作结束");

    m_List0.clear();
    m_List1H.clear();
    return 0;
}


//*****************************************************************************

int FileGroupS::HashFiles()
{
    std::map<ULONG64, std::vector<FileInfo>>::iterator it;
    for (it=m_List1S.begin(); it!=m_List1S.end(); ++it)
        if (it->second.size() > 1)
        {
            std::vector<FileInfo>::iterator it1;
            
            for (it1=it->second.begin(); it1!=it->second.end(); ++it1)
            {
                std::wstring str, tmp(it1->Path);
                (tmp += L'\\') += it1->Name;

                if (PerformHash(tmp, str, it1->Size) != SHA_1::S_NO_ERR)
                    continue;

                std::map<std::wstring, std::vector<pFileInfo>>::iterator it2;
                if ((it2 = m_List1H.find(str)) != m_List1H.end())
                    it2->second.push_back(&(*it1));
                else
                {
                    std::vector<pFileInfo> v;
                    v.push_back(&(*it1));
                    m_List1H.insert(std::make_pair(str, v));
                }
            }
        }

    return 0;
}

int FileGroupS::ExportData()
{
    LVGROUP group;

    int gid         = 1;
    group.cbSize    = sizeof(LVGROUP);
    group.mask      = /*LVGF_HEADER | */LVGF_GROUPID;
    

    if (m_Filter.Switch[Filter::Compare_FileHash] != Filter::Type_Off)
    {
        ExportHashData();
    }
    else
    {
        std::map<ULONG64, std::vector<FileInfo>>::iterator it;
        std::vector<FileInfo>::iterator it1;
        for (it=m_List1S.begin(); it != m_List1S.end(); ++it)
        {
            if (it->second.size() > 1)
            {
                group.iGroupId  = gid;
                ListView_InsertGroup(g_hList, -1, &group);

                for (it1=it->second.begin(); it1!=it->second.end(); ++it1)
                    InsertListViewItem(&(*it1), gid);

                ++gid;
            }
        }
    }

    return 0;
}

int FileGroupS::StoreMatchedFile(const std::wstring & path, const PWIN32_FIND_DATA pwfd)
{
    SYSTEMTIME st;
    FileTimeToSystemTime(&pwfd->ftLastWriteTime, &st);

    ULONG64 idx = 0, size = ((ULONG64)pwfd->nFileSizeHigh << 32) | pwfd->nFileSizeLow;
    if (m_Filter.Switch[Filter::Compare_FileSize])
        idx = size;
    
    if (m_Filter.Switch[Filter::Compare_FileDate])
        idx += pMakeSimpleTime(&st);
    
    std::wstring name(pwfd->cFileName);
    FileInfo fi;
    fi.Name = std::wstring(name);
    fi.Path = std::wstring(path);
    fi.Size = size;
    fi.CreationTime   = pwfd->ftCreationTime;
    fi.LastWriteTime  = pwfd->ftLastWriteTime;

    std::map<ULONG64, std::vector<FileInfo>>::iterator it = m_List1S.find(idx);
    if (it != m_List1S.end())
        it->second.push_back(fi);

    else
    {
        std::vector<FileInfo> v;
        v.push_back(fi);
        m_List1S.insert(std::make_pair(idx, v));
    }

    return 0;
}

int FileGroupS::StartSearchFiles()
{
    m_List1S.clear();
    m_List1H.clear();

    // 设置状态栏
    UpdateStatusBar(0, L"查找文件...");

    std::map<std::wstring, int>::iterator it;
    for (it=m_Filter.SearchDirectory.begin(); it!=m_Filter.SearchDirectory.end(); ++it)
    {
        FindFiles(it->first, 100);
    }

    // 设置状态栏
    UpdateStatusBar(0, L"查找完成");

    if (m_Filter.Switch[Filter::Compare_FileHash] != Filter::Type_Off)
    {
        HashFiles();
    }
    
    // 设置状态栏
    UpdateStatusBar(0, L"正在生成列表");

    ExportData();

    // 设置状态栏
    UpdateStatusBar(0, L"操作结束");

    m_List1S.clear();
    m_List1H.clear();
    return 0;
}

//*****************************************************************************


int FileGroupN::HashFiles()
{
    std::map<std::wstring, std::vector<FileInfo>>::iterator it;
    for (it=m_List1N.begin(); it!=m_List1N.end(); ++it)
        if (it->second.size() > 1)
        {
            std::vector<FileInfo>::iterator it1;
            
            for (it1=it->second.begin(); it1!=it->second.end(); ++it1)
            {
                std::wstring str, tmp(it1->Path);
                (tmp += L'\\') += it1->Name;

                if (PerformHash(tmp, str, it1->Size) != SHA_1::S_NO_ERR)
                    continue;

                std::map<std::wstring, std::vector<pFileInfo>>::iterator it2;
                if ((it2 = m_List1H.find(str)) != m_List1H.end())
                    it2->second.push_back(&(*it1));
                else
                {
                    std::vector<pFileInfo> v;
                    v.push_back(&(*it1));
                    m_List1H.insert(std::make_pair(str, v));
                }
            }
        }


    return 0;
}

int FileGroupN::ExportData()
{
    LVGROUP group;

    int gid         = 1;
    group.cbSize    = sizeof(LVGROUP);
    group.mask      = /*LVGF_HEADER | */LVGF_GROUPID;
    

    if (m_Filter.Switch[Filter::Compare_FileHash] != Filter::Type_Off)
    {
        ExportHashData();
    }
    else
    {
        std::map<std::wstring, std::vector<FileInfo>>::iterator it;
        std::vector<FileInfo>::iterator it1;
        for (it=m_List1N.begin(); it!=m_List1N.end(); ++it)
        {
            if (it->second.size() > 1)
            {
                group.iGroupId  = gid;
                ListView_InsertGroup(g_hList, -1, &group);

                for (it1=it->second.begin(); it1!=it->second.end(); ++it1)
                    InsertListViewItem(&(*it1), gid);
                    
                ++gid;
            }
        }
    }

    return 0;
}

int FileGroupN::StoreMatchedFile(const std::wstring & path, const PWIN32_FIND_DATA pwfd)
{
    SYSTEMTIME st;
    FileTimeToSystemTime(&pwfd->ftLastWriteTime, &st);

    ULONG64 idx = 0, size = ((ULONG64)pwfd->nFileSizeHigh << 32) | pwfd->nFileSizeLow;
    if (m_Filter.Switch[Filter::Compare_FileSize])
        idx = size;
    
    if (m_Filter.Switch[Filter::Compare_FileDate])
        idx += pMakeSimpleTime(&st);
    
    std::wstring name(pwfd->cFileName);
    FileInfo fi;
    fi.Name = std::wstring(name);
    fi.Path = std::wstring(path);
    fi.Size = size;
    fi.CreationTime   = pwfd->ftCreationTime;
    fi.LastWriteTime  = pwfd->ftLastWriteTime;

    std::map<std::wstring, std::vector<FileInfo>>::iterator it;
    std::vector<FileInfo>::iterator it1;

    switch (m_Filter.Switch[m_Filter.Compare_FileName])
    {
    case Filter::Type_Whole:
        {
            std::size_t pos;
            if (m_Filter.FileNameWithoutSuffix
                && (pos = name.rfind(L'.')) != std::wstring::npos)
            {
                name = name.substr(0, pos);
            }

            if ((it = m_List1N.find(name)) != m_List1N.end())
                it->second.push_back(fi);
            else
            {
                std::vector<FileInfo> v;
                v.push_back(fi);
                m_List1N.insert(std::make_pair(name, v));
            }
        }
        break;

    case Filter::Type_Include:
        if (name.find(m_Filter.KeyWordOfFileName) != std::wstring::npos)
        {
            if ((it = m_List1N.find(m_Filter.KeyWordOfFileName)) != m_List1N.end())
                it->second.push_back(fi);
            else
            {
                std::vector<FileInfo> v;
                v.push_back(fi);
                m_List1N.insert(std::make_pair(m_Filter.KeyWordOfFileName, v));
            }
        }
        break;

    case Filter::Type_RangeMatch:
        try
        {
            std::wstring part = std::wstring(name, m_Filter.FileNameRange.LowerBound - 1,
                m_Filter.FileNameRange.UpperBound - m_Filter.FileNameRange.LowerBound + 1);
            if (part.size() <
                m_Filter.FileNameRange.UpperBound - m_Filter.FileNameRange.LowerBound + 1)
                break;

            if ((it = m_List1N.find(part)) != m_List1N.end())
                    it->second.push_back(fi);
            else
            {
                std::vector<FileInfo> v;
                v.push_back(fi);
                m_List1N.insert(std::make_pair(part, v));
            }
        }
        catch (...) { }


        break;
    }

    return 0;
}

int FileGroupN::StartSearchFiles()
{
    m_List1N.clear();
    m_List1H.clear();

    // 设置状态栏
    UpdateStatusBar(0, L"查找文件...");

    std::map<std::wstring, int>::iterator it;
    for (it=m_Filter.SearchDirectory.begin(); it!=m_Filter.SearchDirectory.end(); ++it)
    {
        FindFiles(it->first, 100);
    }

    // 设置状态栏
    UpdateStatusBar(0, L"查找完成");

    if (m_Filter.Switch[Filter::Compare_FileHash] != Filter::Type_Off)
    {
        HashFiles();
    }
    
    // 设置状态栏
    UpdateStatusBar(0, L"正在生成列表");

    ExportData();

    // 设置状态栏
    UpdateStatusBar(0, L"操作结束");

    m_List1N.clear();
    m_List1H.clear();
    return 0;
}


//*****************************************************************************



int FileGroupSN::HashFiles()
{
    std::multimap<ULONG64, std::map<std::wstring, std::vector<FileInfo>>>::iterator it;
    for (it=m_List2SN.begin(); it!=m_List2SN.end(); ++it)
    {
        std::map<std::wstring, std::vector<FileInfo>>::iterator it1;
        for (it1=it->second.begin(); it1!=it->second.end(); ++it1)
        {
            if (it1->second.size() > 1)
            {
                std::vector<FileInfo>::iterator it2;
                for (it2=it1->second.begin(); it2!=it1->second.end(); ++it2)
                {
                    std::wstring str, tmp(it2->Path);
                    (tmp += L'\\') += it2->Name;

                    if (PerformHash(tmp, str, it2->Size) != SHA_1::S_NO_ERR)
                        continue;

                    std::map<std::wstring, std::vector<pFileInfo>>::iterator it3;
                    if ((it3 = m_List1H.find(str)) != m_List1H.end())
                        (*it3).second.push_back(&(*it2));
                    else
                    {
                        std::vector<pFileInfo> v;
                        v.push_back(&(*it2));
                        m_List1H.insert(std::make_pair(str, v));
                    }
                }
            }
        }
    }
    return 0;
}

int FileGroupSN::ExportData()
{
    LVGROUP group;

    int gid         = 1;
    group.cbSize    = sizeof(LVGROUP);
    group.mask      = /*LVGF_HEADER | */LVGF_GROUPID;
    

    if (m_Filter.Switch[Filter::Compare_FileHash] != Filter::Type_Off)
    {
        ExportHashData();
    }
    else
    {
        std::multimap<ULONG64, std::map<std::wstring, std::vector<FileInfo>>>::iterator it;
        std::map<std::wstring, std::vector<FileInfo>>::iterator it1;
        std::vector<FileInfo>::iterator it2;
        for (it=m_List2SN.begin(); it != m_List2SN.end(); ++it)
        {
            for (it1=it->second.begin(); it1!=it->second.end(); ++it1)
            {
                if (it1->second.size() > 1)
                {
                    group.iGroupId  = gid;
                    ListView_InsertGroup(g_hList, -1, &group);

                    for (it2=it1->second.begin(); it2!=it1->second.end(); ++it2)
                        InsertListViewItem(&(*it2), gid);

                    ++gid;
                }
            }
        }
    }

    return 0;
}

int FileGroupSN::StoreMatchedFile(const std::wstring & path, const PWIN32_FIND_DATA pwfd)
{
    SYSTEMTIME st;
    FileTimeToSystemTime(&pwfd->ftLastWriteTime, &st);

    ULONG64 idx = 0, size = ((ULONG64)pwfd->nFileSizeHigh << 32) | pwfd->nFileSizeLow;
    if (m_Filter.Switch[Filter::Compare_FileSize])
        idx = size;
    
    if (m_Filter.Switch[Filter::Compare_FileDate])
        idx += pMakeSimpleTime(&st);
    
    std::wstring name(pwfd->cFileName);
    FileInfo fi;
    fi.Name = std::wstring(name);
    fi.Path = std::wstring(path);
    fi.Size = size;
    fi.CreationTime   = pwfd->ftCreationTime;
    fi.LastWriteTime  = pwfd->ftLastWriteTime;

    std::multimap<ULONG64, std::map<std::wstring, std::vector<FileInfo>>>::iterator it;
    std::map<std::wstring, std::vector<FileInfo>>::iterator it1;

    switch (m_Filter.Switch[m_Filter.Compare_FileName])
    {
    case Filter::Type_Whole:
        {
            std::size_t pos;
            if (m_Filter.FileNameWithoutSuffix
                && (pos = name.rfind(L'.')) != std::wstring::npos)
            {
                name = name.substr(0, pos);
            }
                    
            std::pair<std::multimap<ULONG64, std::map<std::wstring, std::vector<FileInfo>>>::iterator,
                std::multimap<ULONG64, std::map<std::wstring, std::vector<FileInfo>>>::iterator> range;
            range = m_List2SN.equal_range(idx);
                    
            bool find = false;
            for (it=range.first; it!=range.second; ++it)
            {
                if ((it1=it->second.find(name)) != it->second.end())
                {
                    it1->second.push_back(fi);
                    find = true;
                    break;
                }
            }
                    
            if (!find)
            {
                std::vector<FileInfo> v;
                v.push_back(fi);
                std::map<std::wstring, std::vector<FileInfo>> m;
                m.insert(std::make_pair(name, v));
                m_List2SN.insert(std::make_pair(idx, m));
            }

        }
        break;

    case Filter::Type_Include:
        if (name.find(m_Filter.KeyWordOfFileName) != std::wstring::npos)
        {
            std::pair<std::multimap<ULONG64, std::map<std::wstring, std::vector<FileInfo>>>::iterator,
                std::multimap<ULONG64, std::map<std::wstring, std::vector<FileInfo>>>::iterator> range;
            range = m_List2SN.equal_range(idx);
                    
            bool find = false;
            for (it=range.first; it!=range.second; ++it)
            {
                if ((it1=it->second.find(m_Filter.KeyWordOfFileName)) != it->second.end())
                {
                    it1->second.push_back(fi);
                    find = true;
                    break;
                }
            }
                    
            if (!find)
            {
                std::vector<FileInfo> v;
                v.push_back(fi);
                std::map<std::wstring, std::vector<FileInfo>> m;
                m.insert(std::make_pair(m_Filter.KeyWordOfFileName, v));
                m_List2SN.insert(std::make_pair(idx, m));
            }
        }
        break;

    case Filter::Type_RangeMatch:
        try
        {
            std::wstring part = std::wstring(name, m_Filter.FileNameRange.LowerBound - 1,
                m_Filter.FileNameRange.UpperBound - m_Filter.FileNameRange.LowerBound + 1);
            if (part.size() <
                m_Filter.FileNameRange.UpperBound - m_Filter.FileNameRange.LowerBound + 1)
                break;

            std::pair<std::multimap<ULONG64, std::map<std::wstring, std::vector<FileInfo>>>::iterator,
                std::multimap<ULONG64, std::map<std::wstring, std::vector<FileInfo>>>::iterator> range;
            range = m_List2SN.equal_range(idx);
                    
            bool find = false;
            for (it=range.first; it!=range.second; ++it)
            {
                if ((it1=it->second.find(part)) != it->second.end())
                {
                    it1->second.push_back(fi);
                    find = true;
                    break;
                }
            }
                    
            if (!find)
            {
                std::vector<FileInfo> v;
                v.push_back(fi);
                std::map<std::wstring, std::vector<FileInfo>> m;
                m.insert(std::make_pair(part, v));
                m_List2SN.insert(std::make_pair(idx, m));
            }
        }
        catch (...) { }

        break;
    }

    return 0;
}

int FileGroupSN::StartSearchFiles()
{
    m_List2SN.clear();
    m_List1H.clear();

    // 设置状态栏
    UpdateStatusBar(0, L"查找文件...");

    std::map<std::wstring, int>::iterator it;
    for (it=m_Filter.SearchDirectory.begin(); it!=m_Filter.SearchDirectory.end(); ++it)
    {
        FindFiles(it->first, 100);
    }

    // 设置状态栏
    UpdateStatusBar(0, L"查找完成");

    if (m_Filter.Switch[Filter::Compare_FileHash] != Filter::Type_Off)
    {
        HashFiles();
    }
    
    // 设置状态栏
    UpdateStatusBar(0, L"正在生成列表");

    ExportData();

    // 设置状态栏
    UpdateStatusBar(0, L"操作结束");

    m_List2SN.clear();
    m_List1H.clear();
    return 0;
}
