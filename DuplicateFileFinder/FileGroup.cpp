

#include "FileGroup.h"
#include "crc32.h"
#include <CommCtrl.h>
#include <memory>

extern HWND g_hStatus, g_hList;
extern volatile bool bTerminateThread;

extern void UpdateStatusBar(int part, const wchar_t *text);


std::map<DWORD, std::pair<DWORD, std::wstring>> g_PathList;
std::vector<DataBaseInfo> g_DataBase;

FileGroup::FileGroup()
{
    create_crc_table();
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


int FileGroup::StoreMatchedFile(const std::wstring& path, const PWIN32_FIND_DATA pwfd, DWORD crc, DWORD pathCrc)
{
    SYSTEMTIME st;
    FileTimeToSystemTime(&pwfd->ftLastWriteTime, &st);

    ULONG64 idx = 0, size = ((ULONG64)pwfd->nFileSizeHigh << 32) | pwfd->nFileSizeLow;
    if (m_Filter.Switch[Filter::Compare_FileSize])
        idx = size;
    
    if (m_Filter.Switch[Filter::Compare_FileDate])
        idx += pMakeSimpleTime(&st);
    
    auto pfi = std::shared_ptr<FileInfo>(new FileInfo);
    pfi->Size = size;
    pfi->NameCrc = crc;
    pfi->PathCrc = pathCrc;
    pfi->CreationTime = pwfd->ftCreationTime;
    pfi->LastWriteTime = pwfd->ftLastWriteTime;

    m_List0.push_back(pfi);

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
    for (auto it1 = m_List1H.begin(); it1 != m_List1H.end(); ++it1)
    {
        if (it1->second.size() > 1)
        {
            for (auto it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
            {
                g_DataBase.push_back(
                    DataBaseInfo(std::shared_ptr<FileInfo>(*it2), it1->first));
            }
        }
    }

    return 0;
}

std::wstring FileGroup::MakePath(DWORD crc) const
{
    std::wstring path;
    DWORD id = crc;

    auto & pair= g_PathList.find(id);
    if (pair != g_PathList.end())
    {
        id = pair->second.first;
        path = pair->second.second;

        for (; (pair = g_PathList.find(id)) != g_PathList.end(); id = pair->second.first)
            path = pair->second.second + L'\\' + path;
    }
    return static_cast<std::wstring>(path);
}

std::wstring FileGroup::GetFileNameByCrc(DWORD crc) const
{
    auto &r = g_PathList.find(crc);
    return r != g_PathList.end() ? r->second.second : std::wstring();
}

int FileGroup::ExportData()
{
    if (m_Filter.Switch[Filter::Compare_FileHash] != Filter::Type_Off)
    {
        ExportHashData();
    }
    else
    {
        for (auto it1 = m_List0.begin(); it1 != m_List0.end(); ++it1)
        {
            g_DataBase.push_back(
                DataBaseInfo(std::shared_ptr<FileInfo>(*it1), SHA_1::_HashResult()));
        }
    }
    return 0;
}

inline bool _cdecl HashCallback(const long long * const plen1, const long long * const plen2)
{
    return !bTerminateThread;
}


int FileGroup::PerformHash(const std::wstring& name, PBYTE result, ULONG64 size /*= 0*/)
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
        memcpy(result, Hash.GetHashResultHex(), SHA_1::SHA1_HASH_SIZE);
    else
        *result = 0;

    return ret;
}


int FileGroup::HashFiles()
{
    for (auto it=m_List0.begin(); it!=m_List0.end(); ++it)
    {
        std::wstring tmp(MakePath((*it)->PathCrc));
        (tmp += L'\\') += GetFileNameByCrc((*it)->NameCrc);

        SHA_1::_HashResult h;
        int ret = PerformHash(tmp, h.b, (*it)->Size);
        if (ret == SHA_1::S_TERMINATED)
            return 1;
        else if (ret != SHA_1::S_NO_ERR)
            continue;

        m_List1H[h].push_back(*it);
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


int FileGroup::FindFiles(const std::wstring& dirPath, int depth, DWORD parentCrc)
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
    if (depth && hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            DWORD id = 0;
            if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (depth != 0 &&
                    wcscmp(wfd.cFileName, L".") && wcscmp(wfd.cFileName, L".."))	// 递归查找子文件夹
                {
                    id = CRC32_4((PBYTE)wfd.cFileName, 0, wcslen(wfd.cFileName)*sizeof(wchar_t));

                    search = dirPath;
                    search += L"\\";
                    search += wfd.cFileName;
                    FindFiles(search, depth == -1 ? -1 : depth - 1, id);
                }
                else
                    continue;
            }
            else if (IsFileMatched(dirPath, &wfd))	// 文件类型过滤
            {
                id = CRC32_4((PBYTE)wfd.cFileName, 0, wcslen(wfd.cFileName)*sizeof(wchar_t));
                StoreMatchedFile(dirPath, &wfd, id, parentCrc);
            }
            else
                continue;

            // 记录路径            
            g_PathList[id] = std::make_pair(parentCrc, std::wstring(wfd.cFileName));

        }
        while ((!bTerminate || bTerminate && !(*bTerminate)) &&
            FindNextFile(hFind, &wfd));
    }
    FindClose(hFind);

    return 0;
}


int FileGroup::StartSearchFiles()
{
    m_List0.clear();
    m_List1H.clear();

    g_DataBase.clear();
    g_PathList.clear();

    // 设置状态栏
    UpdateStatusBar(0, L"查找文件...");
    UpdateStatusBar(1, L"");

    do
    {
        for (auto it=m_Filter.SearchDirectory.begin(); it!=m_Filter.SearchDirectory.end(); ++it)
        {
            DWORD id = CRC32_4((PBYTE)it->first.c_str(), 0, wcslen(it->first.c_str())*sizeof(wchar_t));
            g_PathList[id] = std::make_pair(0, it->first);
            FindFiles(it->first, 100, id);
        }

        if (bTerminate && *bTerminate)
            break;

        // 设置状态栏
        UpdateStatusBar(0, L"查找完成");

        if (m_Filter.Switch[Filter::Compare_FileHash] != Filter::Type_Off)
        {
            HashFiles();
        }

        if (bTerminate && *bTerminate)
            break;
    
        // 设置状态栏
        UpdateStatusBar(0, L"正在生成列表");

        ExportData();
        
        ListView_SetItemCount(g_hList, g_DataBase.size());

    } while(0);

    // 设置状态栏
    UpdateStatusBar(0, L"操作结束");
    UpdateStatusBar(1, nullptr);

    m_List0.clear();
    m_List1H.clear();
    return 0;
}


//*****************************************************************************

int FileGroupS::HashFiles()
{
    for (auto it=m_List1S.begin(); it!=m_List1S.end(); ++it)
        if (it->second.size() > 1)
        {
            for (auto it1=it->second.begin(); it1!=it->second.end(); ++it1)
            {
                std::wstring tmp(MakePath((*it1)->PathCrc));
                (tmp += L'\\') += GetFileNameByCrc((*it1)->NameCrc);

                SHA_1::_HashResult h;
                int ret = PerformHash(tmp, h.b, (*it1)->Size);
                if (ret == SHA_1::S_TERMINATED)
                    return 1;
                else if (ret != SHA_1::S_NO_ERR)
                    continue;

                m_List1H[h].push_back(*it1);
            }
        }

    return 0;
}

int FileGroupS::ExportData()
{
    if (m_Filter.Switch[Filter::Compare_FileHash] != Filter::Type_Off)
    {
        ExportHashData();
    }
    else
    {
        for (auto it1 = m_List1S.begin(); it1 != m_List1S.end(); ++it1)
        {
            if (it1->second.size() > 1)
            {
                for (auto it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
                {
                    g_DataBase.push_back(
                        DataBaseInfo(std::shared_ptr<FileInfo>(*it2), SHA_1::_HashResult()));
                }
            }
        }
    }

    return 0;
}

int FileGroupS::StoreMatchedFile(const std::wstring & path, const PWIN32_FIND_DATA pwfd, DWORD crc, DWORD pathCrc)
{
    SYSTEMTIME st;
    FileTimeToSystemTime(&pwfd->ftLastWriteTime, &st);

    ULONG64 idx = 0, size = ((ULONG64)pwfd->nFileSizeHigh << 32) | pwfd->nFileSizeLow;
    if (m_Filter.Switch[Filter::Compare_FileSize])
        idx = size;
    
    if (m_Filter.Switch[Filter::Compare_FileDate])
        idx += pMakeSimpleTime(&st);
    
    auto pfi = std::shared_ptr<FileInfo>(new FileInfo);
    pfi->Size = size;
    pfi->NameCrc = crc;
    pfi->PathCrc = pathCrc;
    pfi->CreationTime = pwfd->ftCreationTime;
    pfi->LastWriteTime = pwfd->ftLastWriteTime;

    m_List1S[idx].push_back(pfi);

    return 0;
}

int FileGroupS::StartSearchFiles()
{
    m_List1S.clear();
    m_List1H.clear();

    g_PathList.clear();
    g_DataBase.clear();

    // 设置状态栏
    UpdateStatusBar(0, L"查找文件...");
    UpdateStatusBar(1, L"");

    do
    {
        for (auto it=m_Filter.SearchDirectory.begin(); it!=m_Filter.SearchDirectory.end(); ++it)
        {
            DWORD id = CRC32_4((PBYTE)it->first.c_str(), 0, wcslen(it->first.c_str())*sizeof(wchar_t));
            g_PathList[id] = std::make_pair(0, it->first);
            FindFiles(it->first, 100, id);
        }

        if (bTerminate && *bTerminate)
            break;
    
        // 设置状态栏
        UpdateStatusBar(0, L"查找完成");

        if (m_Filter.Switch[Filter::Compare_FileHash] != Filter::Type_Off)
        {
            HashFiles();
        }
    
        if (bTerminate && *bTerminate)
            break;

        // 设置状态栏
        UpdateStatusBar(0, L"正在生成列表");

        ExportData();

        ListView_SetItemCount(g_hList, g_DataBase.size());
    
    } while (0);

    // 设置状态栏
    UpdateStatusBar(0, L"操作结束");
    UpdateStatusBar(1, nullptr);

    m_List1S.clear();
    m_List1H.clear();
    return 0;
}

//*****************************************************************************


int FileGroupN::HashFiles()
{
    for (auto it=m_List1N.begin(); it!=m_List1N.end(); ++it)
        if (it->second.size() > 1)
        {
            for (auto it1=it->second.begin(); it1!=it->second.end(); ++it1)
            {
                std::wstring tmp(MakePath((*it1)->PathCrc));
                (tmp += L'\\') += GetFileNameByCrc((*it1)->NameCrc);

                SHA_1::_HashResult h;
                int ret = PerformHash(tmp, h.b, (*it1)->Size);
                if (ret == SHA_1 ::S_TERMINATED)
                    return 1;
                else if (ret != SHA_1::S_NO_ERR)
                    continue;

                m_List1H[h].push_back(*it1);
            }
        }
    return 0;
}

int FileGroupN::ExportData()
{
    if (m_Filter.Switch[Filter::Compare_FileHash] != Filter::Type_Off)
    {
        ExportHashData();
    }
    else
    {
        for (auto it1 = m_List1N.begin(); it1 != m_List1N.end(); ++it1)
        {
            if (it1->second.size() > 1)
            {
                for (auto it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
                {
                    g_DataBase.push_back(
                        DataBaseInfo(std::shared_ptr<FileInfo>(*it2), SHA_1::_HashResult()));
                }
            }
        }
    }

    return 0;
}

int FileGroupN::StoreMatchedFile(const std::wstring & path, const PWIN32_FIND_DATA pwfd, DWORD crc, DWORD pathCrc)
{
    SYSTEMTIME st;
    FileTimeToSystemTime(&pwfd->ftLastWriteTime, &st);

    ULONG64 idx = 0, size = ((ULONG64)pwfd->nFileSizeHigh << 32) | pwfd->nFileSizeLow;
    if (m_Filter.Switch[Filter::Compare_FileSize])
        idx = size;
    
    if (m_Filter.Switch[Filter::Compare_FileDate])
        idx += pMakeSimpleTime(&st);
    
    std::wstring name(pwfd->cFileName);
    auto pfi = std::shared_ptr<FileInfo>(new FileInfo);
    pfi->Size = size;
    pfi->NameCrc = crc;
    pfi->PathCrc = pathCrc;
    pfi->CreationTime = pwfd->ftCreationTime;
    pfi->LastWriteTime = pwfd->ftLastWriteTime;

    std::map<DWORD, std::vector<FileInfo>>::iterator it;
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

            DWORD crc = CRC32_4((PBYTE)name.c_str(), 0, name.size() * 2);

            m_List1N[crc].push_back(pfi);
        }
        break;

    case Filter::Type_Include:
        if (name.find(m_Filter.KeyWordOfFileName) != std::wstring::npos)
        {
            // 这里 map 只有一对键值<keyname - vector>
            m_List1N[0].push_back(pfi);
        }
        break;

    case Filter::Type_RangeMatch:
        int len = m_Filter.FileNameRange.UpperBound - m_Filter.FileNameRange.LowerBound + 1;

        if (len < 0
            || name.size() < len
            || m_Filter.FileNameRange.LowerBound < 1
            || name.size() < m_Filter.FileNameRange.UpperBound - 1)
            break;

        DWORD crc = CRC32_4((PBYTE)name.c_str() + m_Filter.FileNameRange.LowerBound * 2 - 2,
            0, len * 2);

        m_List1N[crc].push_back(pfi);

        break;
    }

    return 0;
}

int FileGroupN::StartSearchFiles()
{
    m_List1N.clear();
    m_List1H.clear();

    g_PathList.clear();
    g_DataBase.clear();

    // 设置状态栏
    UpdateStatusBar(0, L"查找文件...");
    UpdateStatusBar(1, L"");

    do
    {
        std::map<std::wstring, int>::iterator it;
        for (it=m_Filter.SearchDirectory.begin(); it!=m_Filter.SearchDirectory.end(); ++it)
        {
            DWORD id = CRC32_4((PBYTE)it->first.c_str(), 0, wcslen(it->first.c_str())*sizeof(wchar_t));
            g_PathList[id] = std::make_pair(0, it->first);
            FindFiles(it->first, 100, id);
        }

        if (bTerminate && *bTerminate)
            break;

        // 设置状态栏
        UpdateStatusBar(0, L"查找完成");

        if (m_Filter.Switch[Filter::Compare_FileHash] != Filter::Type_Off)
        {
            HashFiles();
        }
    
        if (bTerminate && *bTerminate)
            break;

        // 设置状态栏
        UpdateStatusBar(0, L"正在生成列表");

        ExportData();

        ListView_SetItemCount(g_hList, g_DataBase.size());

    } while (0);

    // 设置状态栏
    UpdateStatusBar(0, L"操作结束");
    UpdateStatusBar(1, nullptr);

    m_List1N.clear();
    m_List1H.clear();
    return 0;
}



//*****************************************************************************



int FileGroupSN::HashFiles()
{
    for (auto it=m_List2SN.begin(); it!=m_List2SN.end(); ++it)
    {
        for (auto it1=it->second.begin(); it1!=it->second.end(); ++it1)
        {
            if (it1->second.size() > 1)
            {
                for (auto it2=it1->second.begin(); it2!=it1->second.end(); ++it2)
                {
                    std::wstring tmp(MakePath((*it2)->PathCrc));
                    (tmp += L'\\') += GetFileNameByCrc((*it2)->NameCrc);

                    SHA_1::_HashResult h;
                    int ret = PerformHash(tmp, h.b, (*it2)->Size);
                    if (ret == SHA_1::S_TERMINATED)
                        return 1;
                    else if (ret != SHA_1::S_NO_ERR)
                        continue;

                    m_List1H[h].push_back(*it2);
                }
            }
        }
    }
    return 0;
}

int FileGroupSN::ExportData()
{
    if (m_Filter.Switch[Filter::Compare_FileHash] != Filter::Type_Off)
    {
        ExportHashData();
    }
    else
    {
        for (auto it1 = m_List2SN.begin(); it1 != m_List2SN.end(); ++it1)
        {
            for (auto it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
            {
                if (it2->second.size() > 1) 
                {
                    for (auto it3 = it2->second.begin(); it3 != it2->second.end(); ++it3)
                        g_DataBase.push_back(
                            DataBaseInfo(std::shared_ptr<FileInfo>(*it3), SHA_1::_HashResult()));
                }
            }
        }
    }

    return 0;
}

int FileGroupSN::StoreMatchedFile(const std::wstring & path, const PWIN32_FIND_DATA pwfd, DWORD crc, DWORD pathCrc)
{
    SYSTEMTIME st;
    FileTimeToSystemTime(&pwfd->ftLastWriteTime, &st);

    ULONG64 idx = 0, size = ((ULONG64)pwfd->nFileSizeHigh << 32) | pwfd->nFileSizeLow;
    if (m_Filter.Switch[Filter::Compare_FileSize])
        idx = size;
    
    if (m_Filter.Switch[Filter::Compare_FileDate])
        idx += pMakeSimpleTime(&st);
    
    std::wstring name(pwfd->cFileName);
    auto pfi = std::shared_ptr<FileInfo>(new FileInfo);
    pfi->Size = size;
    pfi->NameCrc = crc;
    pfi->PathCrc = pathCrc;
    pfi->CreationTime = pwfd->ftCreationTime;
    pfi->LastWriteTime = pwfd->ftLastWriteTime;

    std::multimap<ULONG64, std::map<DWORD, std::vector<std::shared_ptr<FileInfo>>>>::iterator it;
    std::map<DWORD, std::vector<std::shared_ptr<FileInfo>>>::iterator it1;

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
            
            DWORD crc = CRC32_4((PBYTE)name.c_str(), 0, name.size() * 2);
            auto range = m_List2SN.equal_range(idx);
                    
            bool find = false;
            for (it=range.first; it!=range.second; ++it)
            {
                if ((it1=it->second.find(crc)) != it->second.end())
                {
                    it1->second.push_back(pfi);
                    find = true;
                    break;
                }
            }
                    
            if (!find)
            {
                std::vector<std::shared_ptr<FileInfo>> v;
                v.push_back(pfi);
                std::map<DWORD, std::vector<std::shared_ptr<FileInfo>>> m;
                m.insert(std::make_pair(crc, v));
                m_List2SN.insert(std::make_pair(idx, m));
            }

        }
        break;

    case Filter::Type_Include:
        if (name.find(m_Filter.KeyWordOfFileName) != std::wstring::npos)
        {
            auto range = m_List2SN.equal_range(idx);
                    
            bool find = false;
            for (it=range.first; it!=range.second; ++it)
            {
                if ((it1=it->second.find(0)) != it->second.end())
                {
                    it1->second.push_back(pfi);
                    find = true;
                    break;
                }
            }
                    
            if (!find)
            {
                std::vector<std::shared_ptr<FileInfo>> v;
                v.push_back(pfi);
                std::map<DWORD, std::vector<std::shared_ptr<FileInfo>>> m;
                m.insert(std::make_pair(0, v));
                m_List2SN.insert(std::make_pair(idx, m));
            }
        }
        break;

    case Filter::Type_RangeMatch:
        try
        {
            int len = m_Filter.FileNameRange.UpperBound - m_Filter.FileNameRange.LowerBound + 1;

            if (len < 0
                || name.size() < len
                || m_Filter.FileNameRange.LowerBound < 1
                || name.size() < m_Filter.FileNameRange.UpperBound - 1)
                break;

            DWORD crc = CRC32_4((PBYTE)name.c_str() + m_Filter.FileNameRange.LowerBound * 2 - 2,
                0, len * 2);

            auto range = m_List2SN.equal_range(idx);
                    
            bool find = false;
            for (it=range.first; it!=range.second; ++it)
            {
                if ((it1=it->second.find(crc)) != it->second.end())
                {
                    it1->second.push_back(pfi);
                    find = true;
                    break;
                }
            }
                    
            if (!find)
            {
                std::vector<std::shared_ptr<FileInfo>> v;
                v.push_back(pfi);
                std::map<DWORD, std::vector<std::shared_ptr<FileInfo>>> m;
                m.insert(std::make_pair(crc, v));
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

    g_PathList.clear();
    g_DataBase.clear();

    // 设置状态栏
    UpdateStatusBar(0, L"查找文件...");
    UpdateStatusBar(1, L"");

    do
    {
        std::map<std::wstring, int>::iterator it;
        for (it=m_Filter.SearchDirectory.begin(); it!=m_Filter.SearchDirectory.end(); ++it)
        {
            DWORD id = CRC32_4((PBYTE)it->first.c_str(), 0, wcslen(it->first.c_str())*sizeof(wchar_t));
            g_PathList[id] = std::make_pair(0, it->first);
            FindFiles(it->first, 100, id);
        }

        if (bTerminate && *bTerminate)
            break;

        // 设置状态栏
        UpdateStatusBar(0, L"查找完成");

        if (m_Filter.Switch[Filter::Compare_FileHash] != Filter::Type_Off)
        {
            HashFiles();
        }
    
        if (bTerminate && *bTerminate)
            break;

        // 设置状态栏
        UpdateStatusBar(0, L"正在生成列表");

        ExportData();

        ListView_SetItemCount(g_hList, g_DataBase.size());
    
    } while(0);

    // 设置状态栏
    UpdateStatusBar(0, L"操作结束");
    UpdateStatusBar(1, nullptr);

    m_List2SN.clear();
    m_List1H.clear();
    return 0;
}
