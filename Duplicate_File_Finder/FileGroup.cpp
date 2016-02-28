
#include "FileGroup.h"

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

    //*************************************************************************

    m_Filter.Switch[FILTER::Compare_FileSize] = FILTER::Type_Whole;
    m_Filter.Switch[FILTER::Compare_FileHash] = FILTER::Type_Whole;

    m_StoreType = StoreType_size;
}

int FileGroup::StoreMatchedFile(const std::wstring& path, const PWIN32_FIND_DATA pwfd)
{
    SYSTEMTIME st;
    FileTimeToSystemTime(&pwfd->ftLastWriteTime, &st);

    ULONG64 idx = 0, size = ((ULONG64)pwfd->nFileSizeHigh << 32) | pwfd->nFileSizeLow;
    if (m_Filter.Switch[FILTER::Compare_FileSize])
        idx = size;
    
    if (m_Filter.Switch[FILTER::Compare_FileDate])
        idx += MakeSimpleTime(&st);
    
    std::wstring name(pwfd->cFileName);
    FileInfo fi;
    fi.Name = std::wstring(name);
    fi.Path = std::wstring(path);
    fi.Size = size;
    fi.st   = st;


    switch (m_StoreType)
    {
    case StoreType_size:
    case StoreType_size_date:
    case StoreType_date:
        {
            std::map<ULONG64, std::vector<FileInfo>>::iterator it = m_List1S.find(idx);
            if (it != m_List1S.end())
                it->second.push_back(fi);
            else
            {
                std::vector<FileInfo> v;
                v.push_back(fi);
                m_List1S.insert(std::make_pair(idx, v));
            }
        }
        break;


    case StoreType_size_name:
    case StoreType_date_name:
    case StoreType_size_date_name:
        {
            std::multimap<ULONG64, std::map<std::wstring, std::vector<FileInfo>>>::iterator it;
            std::map<std::wstring, std::vector<FileInfo>>::iterator it1;

            switch (m_Filter.Switch[m_Filter.Compare_FileName])
            {
            case FILTER::Type_Whole:
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

            case FILTER::Type_Include:
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

            case FILTER::Type_RangeMatch:
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
        }
        break;

    case StoreType_name:
        {
            std::map<std::wstring, std::vector<FileInfo>>::iterator it;
            std::vector<FileInfo>::iterator it1;

            switch (m_Filter.Switch[m_Filter.Compare_FileName])
            {
            case FILTER::Type_Whole:
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

            case FILTER::Type_Include:
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

            case FILTER::Type_RangeMatch:
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
        }
        break;

    case StoreType_All:
        m_List0.push_back(fi);
        break;

    case StoreType_hash:
    default:
        assert(0);
        break;
    }

    return 0;
}


ULONG64 FileGroup::MakeSimpleTime(PSYSTEMTIME pst)
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


int FileGroup::HashFiles()
{
    if (m_Filter.Switch[FILTER::Compare_FileHash] == FILTER::Type_Off)
        return 0;

    switch (m_StoreType)
    {
    case StoreType_size:
    case StoreType_date:
    case StoreType_size_date:
        pHashFiles1S();
        break;

    case StoreType_size_name:
    case StoreType_date_name:
    case StoreType_size_date_name:
        pHashFiles2SN();
        break;

    case StoreType_name:
        pHashFiles1N();
        break;

    case StoreType_All:
        pHashFiles0();
        break;

    case StoreType_hash:
    default:
        assert(0);
        break;
    }

    return 0;
}

int FileGroup::ExportData()
{
    if (m_Filter.Switch[FileGroup::FILTER::Compare_FileHash] != FileGroup::FILTER::Type_Off)
    {
        std::map<std::wstring, std::vector<pFileInfo>>::iterator it;
        std::vector<pFileInfo>::iterator it1;
        for (it=m_List1H.begin(); it != m_List1H.end(); ++it)
        {
            if (it->second.size() > 1)
            {
                for (it1=it->second.begin(); it1!=it->second.end(); ++it1)
                    InsertListViewItem(*it1, const_cast<std::wstring*>(&it->first));
            }
        }
    }
    else switch (m_StoreType)
    {
    case StoreType_size:
    case StoreType_date:
    case StoreType_size_date:
        {
            std::map<ULONG64, std::vector<FileInfo>>::iterator it;
            std::vector<FileInfo>::iterator it1;
            for (it=m_List1S.begin(); it != m_List1S.end(); ++it)
            {
                if (it->second.size() > 1)
                {
                    for (it1=it->second.begin(); it1!=it->second.end(); ++it1)
                       InsertListViewItem(&(*it1));
                }
            }
        }
        break;

    case StoreType_size_name:
    case StoreType_date_name:
    case StoreType_size_date_name:
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
                        for (it2=it1->second.begin(); it2!=it1->second.end(); ++it2)
                           InsertListViewItem(&(*it2));
                    }
                }
            }
        }
        break;

    case StoreType_hash:
        assert(0);
        break;

    case StoreType_name:
        {
            std::map<std::wstring, std::vector<FileInfo>>::iterator it;
            std::vector<FileInfo>::iterator it1;
            for (it=m_List1N.begin(); it!=m_List1N.end(); ++it)
            {
                if (it->second.size() > 1)
                {
                    for (it1=it->second.begin(); it1!=it->second.end(); ++it1)
                        InsertListViewItem(&(*it1));
                }
            }
        }
        break;

    case StoreType_All:
        {
            std::vector<FileInfo>::iterator it;
            for (it=m_List0.begin(); it != m_List0.end(); ++it)
            {
                InsertListViewItem(&(*it));
            }
        }
        break;

    default:
        assert(0);
        break;
    }

    return 0;
}

bool _cdecl HashCallback(const long long * const plen1, const long long * const plen2)
{
    return true;
}


int FileGroup::PerformHash(const std::wstring& name, std::wstring& result, ULONG64 size /*= 0*/)
{
    SHA_1 Hash;
    int   ret;

    do 
    {
        if (m_Filter.Switch[FileGroup::FILTER::Compare_FileHash]
            == FileGroup::FILTER::Type_RangeMatch)
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


int FileGroup::pHashFiles1S()
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

int FileGroup::pHashFiles2SN()
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

int FileGroup::pHashFiles1N()
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

int FileGroup::pHashFiles0()
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
    if (m_Filter.Switch[m_Filter.Search_IncludeSuffix] != FILTER::Type_Off)
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

    if (m_Filter.Switch[m_Filter.Search_IgnoreSuffix] != FILTER::Type_Off)
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
        != FILTER::Type_Off)
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
    if (m_Filter.Switch[m_Filter.Compare_FileHash] != FileGroup::FILTER::Type_Off)     // 哈希
        m_StoreType = StoreType_hash;
    
    if (m_Filter.Switch[m_Filter.Compare_FileSize])
    {
        if (m_Filter.Switch[m_Filter.Compare_FileDate])     // 大小 + 日期
        {
            if (m_Filter.Switch[m_Filter.Compare_FileName])
                m_StoreType = StoreType_size_date_name;
            else
                m_StoreType = StoreType_size_date;
        }
        else if (m_Filter.Switch[m_Filter.Compare_FileName])    // 大小 + 文件名
        {
            m_StoreType = StoreType_size_name;
        }
        else    // 大小
        {
            m_StoreType = StoreType_size;
        }
    }
    else if (m_Filter.Switch[m_Filter.Compare_FileDate])
    {
        if (m_Filter.Switch[m_Filter.Compare_FileName])
            m_StoreType = StoreType_date_name;
        else
            m_StoreType = StoreType_date;

    }
    else if (m_Filter.Switch[m_Filter.Compare_FileName])
    {
        m_StoreType = StoreType_name;
    }
    else
    {
        m_StoreType = StoreType_All;
    }
    
    return pFindFiles(dirPath, depth);
}

int FileGroup::pFindFiles(const std::wstring& dirPath, int depth)
{
    HANDLE	hFind;
    WIN32_FIND_DATA wfd;
    std::wstring path, search;

	path = dirPath + L"\\*.*";

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
					pFindFiles(search, depth==-1 ? -1 : depth-1);
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
    m_List1S.clear();
    m_List2SN.clear();
    m_List1N.clear();
    m_List0.clear();
    m_List1H.clear();

    std::map<std::wstring, int>::iterator it;
    for (it=m_Filter.SearchDirectory.begin(); it!=m_Filter.SearchDirectory.end(); ++it)
    {
        FindFiles(it->first, 100);
    }

    if (m_Filter.Switch[FileGroup::FILTER::Compare_FileHash] != FileGroup::FILTER::Type_Off)
    {
        HashFiles();
    }

    ExportData();

    return 0;
}