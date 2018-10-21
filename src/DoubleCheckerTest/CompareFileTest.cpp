#include "stdafx.h"
#include <initializer_list>
#include <set>
#include <string>
#include <vector>
#include "Common.h"
#include "CompareFileTest.h"
#include "../DoubleChecker/Compare.h"
#include "../DoubleChecker/Parammeters.h"

using namespace std;

bool GroupCompare(std::vector<std::set<const FileRecord*>>& left, vector<vector<wstring>> right)
{
    for (auto& rg : right)
    {
        std::set<const FileRecord*>* curLeft = nullptr;
        bool found = false;
        for (size_t lg = 0; lg < left.size() && !found; ++lg)
        {
            for (auto fr = left[lg].begin(); fr != left[lg].end(); ++fr)
            {
                if ((*fr)->mPath == rg[0])
                {
                    curLeft = &left[lg];
                    found = true;
                    break;
                }
            }
        }
        if (!found)
            return false;

        if (rg.size() != curLeft->size())
            return false;

        for (auto& fr : rg)
        {
            bool found = false;
            for (auto it = curLeft->begin(); it != curLeft->end() && !found; ++it)
            {
                if ((*it)->mPath == fr)
                    found = true;
            }
            if (!found)
                return false;
        }
    }
    return true;
}

void CompareFileTest::CompareTest()
{
    bool ret;
    Parameters param;
    std::vector<std::set<const FileRecord*>> list;

    ret = param.ParseCommand(vector<wstring>{L"-t", L"..\\test\\filelist2.txt" });
    Assert(ret);
    list = CompareFile(param);
    ret = GroupCompare(list, vector<vector<wstring>>{
        { L"D:\\TestFolder2\\cn_windows_10_enterprise_x64_dvd.sha", L"D:\\TestFolder2\\cnamsl.sha", L"D:\\TestFolder2\\cn" },
        { L"D:\\TestFolder2\\1\\cn_windows_10_enterprise_x64_dvd_6846957.sha", L"D:\\TestFolder2\\cn_windows_10_enterprise_x64_dvd_6846957.txt" },
        { L"D:\\TestFolder2\\windows_10.sha", L"D:\\TestFolder2\\windows_10 - 副本.sha" }
    });
    Assert(ret);


    ret = param.ParseCommand(vector<wstring>{L"-s", L"..\\test\\filelist2.txt" });
    Assert(ret);
    list = CompareFile(param);
    ret = GroupCompare(list, vector<vector<wstring>>{
        { L"D:\\TestFolder2\\1.!bt", L"D:\\TestFolder2\\1.bak"},
        { L"D:\\TestFolder2\\cn", L"D:\\TestFolder2\\cn_windows_10_enterprise_x64_dvd.sha",
          L"D:\\TestFolder2\\1\\cn_windows_10_enterprise_x64_dvd_6846957.sha", L"D:\\TestFolder2\\cn_windows_10_enterprise_x64_dvd_6846957.txt",
          L"D:\\TestFolder2\\cnamsl.sha", L"D:\\TestFolder2\\data_modified\\cn_windows_10_enterprise_x64_dvd_6846957.sha",
          L"D:\\TestFolder2\\data_modified.sha", L"D:\\TestFolder2\\windows_10.sha" },
    });
    Assert(ret);


    ret = param.ParseCommand(vector<wstring>{L"-n", L"..\\test\\filelist2.txt" });
    Assert(ret);
    list = CompareFile(param);
    ret = GroupCompare(list, vector<vector<wstring>>{
        { L"D:\\TestFolder2\\1\\cn_windows_10_enterprise_x64_dvd_6846957.sha",
          L"D:\\TestFolder2\\data_modified\\cn_windows_10_enterprise_x64_dvd_6846957.sha"
        },
    });
    Assert(ret);


    ret = param.ParseCommand(vector<wstring>{L"-ns", L"windows", L"..\\test\\filelist2.txt" });
    Assert(ret);
    list = CompareFile(param);
    ret = GroupCompare(list, vector<vector<wstring>>{
        { L"D:\\TestFolder2\\1\\cn_windows_10_enterprise_x64_dvd_6846957.sha", L"D:\\TestFolder2\\cn_windows_10_enterprise_x64_dvd_6846957.txt",
            L"D:\\TestFolder2\\data_modified\\cn_windows_10_enterprise_x64_dvd_6846957.sha", L"D:\\TestFolder2\\windows_10.sha",
            L"D:\\TestFolder2\\windows_10 - 副本.sha", L"D:\\TestFolder2\\cn_windows_10_enterprise_x64_dvd.sha"
        },
    });
    Assert(ret);


    ret = param.ParseCommand(vector<wstring>{L"-ne", L"..\\test\\filelist2.txt" });
    Assert(ret);
    list = CompareFile(param);
    ret = GroupCompare(list, vector<vector<wstring>>{
        { L"D:\\TestFolder2\\1.!bt", L"D:\\TestFolder2\\1.bak"},
        { L"D:\\TestFolder2\\1\\cn_windows_10_enterprise_x64_dvd_6846957.sha", L"D:\\TestFolder2\\cn_windows_10_enterprise_x64_dvd_6846957.txt",
          L"D:\\TestFolder2\\data_modified\\cn_windows_10_enterprise_x64_dvd_6846957.sha"},
    });
    Assert(ret);


    ret = param.ParseCommand(vector<wstring>{L"-c", L"..\\test\\filelist2.txt" });
    Assert(ret);
    list = CompareFile(param);
    ret = GroupCompare(list, vector<vector<wstring>>{
        { L"D:\\TestFolder2\\1.!bt", L"D:\\TestFolder2\\1.bak"},
        { L"D:\\TestFolder2\\cn", L"D:\\TestFolder2\\cn_windows_10_enterprise_x64_dvd.sha", L"D:\\TestFolder2\\cn_windows_10_enterprise_x64_dvd_6846957.txt",
            L"D:\\TestFolder2\\1\\cn_windows_10_enterprise_x64_dvd_6846957.sha", L"D:\\TestFolder2\\cnamsl.sha",
            L"D:\\TestFolder2\\windows_10.sha"
        },
    });
    Assert(ret);


    ////////////////////////////////////////////////////////////////////////////


    ret = param.ParseCommand(vector<wstring>{L"-s", L"-n", L"..\\test\\filelist2.txt" });
    Assert(ret);
    list = CompareFile(param);
    ret = GroupCompare(list, vector<vector<wstring>>{
        { L"D:\\TestFolder2\\1\\cn_windows_10_enterprise_x64_dvd_6846957.sha",
          L"D:\\TestFolder2\\data_modified\\cn_windows_10_enterprise_x64_dvd_6846957.sha"
        },
    });
    Assert(ret);


    ret = param.ParseCommand(vector<wstring>{L"-s", L"-ne", L"..\\test\\filelist2.txt" });
    Assert(ret);
    list = CompareFile(param);
    ret = GroupCompare(list, vector<vector<wstring>>{
        { L"D:\\TestFolder2\\1.!bt", L"D:\\TestFolder2\\1.bak"},
        { L"D:\\TestFolder2\\1\\cn_windows_10_enterprise_x64_dvd_6846957.sha", L"D:\\TestFolder2\\cn_windows_10_enterprise_x64_dvd_6846957.txt",
            L"D:\\TestFolder2\\data_modified\\cn_windows_10_enterprise_x64_dvd_6846957.sha" },
    });
    Assert(ret);


    ret = param.ParseCommand(vector<wstring>{L"-s", L"-ns", L"windows", L"..\\test\\filelist2.txt" });
    Assert(ret);
    list = CompareFile(param);
    ret = GroupCompare(list, vector<vector<wstring>>{
        { L"D:\\TestFolder2\\1\\cn_windows_10_enterprise_x64_dvd_6846957.sha", L"D:\\TestFolder2\\cn_windows_10_enterprise_x64_dvd_6846957.txt",
            L"D:\\TestFolder2\\data_modified\\cn_windows_10_enterprise_x64_dvd_6846957.sha", L"D:\\TestFolder2\\cn_windows_10_enterprise_x64_dvd.sha",
            L"D:\\TestFolder2\\windows_10.sha"
        },
    });
    Assert(ret);


    ret = param.ParseCommand(vector<wstring>{L"-s", L"-t", L"..\\test\\filelist2.txt" });
    Assert(ret);
    list = CompareFile(param);
    ret = GroupCompare(list, vector<vector<wstring>>{
        { L"D:\\TestFolder2\\1\\cn_windows_10_enterprise_x64_dvd_6846957.sha", L"D:\\TestFolder2\\cn_windows_10_enterprise_x64_dvd_6846957.txt", },
        { L"D:\\TestFolder2\\cn", L"D:\\TestFolder2\\cnamsl.sha", L"D:\\TestFolder2\\cn_windows_10_enterprise_x64_dvd.sha" },
    });
    Assert(ret);


    ret = param.ParseCommand(vector<wstring>{L"-s", L"-c", L"..\\test\\filelist2.txt" });
    Assert(ret);
    list = CompareFile(param);
    ret = GroupCompare(list, vector<vector<wstring>>{
        { L"D:\\TestFolder2\\1.!bt", L"D:\\TestFolder2\\1.bak"},
        { L"D:\\TestFolder2\\cn", L"D:\\TestFolder2\\cn_windows_10_enterprise_x64_dvd.sha", L"D:\\TestFolder2\\cn_windows_10_enterprise_x64_dvd_6846957.txt",
            L"D:\\TestFolder2\\1\\cn_windows_10_enterprise_x64_dvd_6846957.sha", L"D:\\TestFolder2\\cnamsl.sha",
            L"D:\\TestFolder2\\windows_10.sha"
        },
    });
    Assert(ret);


    ret = param.ParseCommand(vector<wstring>{L"-t", L"-n", L"..\\test\\filelist2.txt" });
    Assert(ret);
    list = CompareFile(param);
    ret = GroupCompare(list, vector<vector<wstring>>{ });
    Assert(ret);


    ret = param.ParseCommand(vector<wstring>{L"-t", L"-ne", L"..\\test\\filelist2.txt" });
    Assert(ret);
    list = CompareFile(param);
    ret = GroupCompare(list, vector<vector<wstring>>{
        { L"D:\\TestFolder2\\1\\cn_windows_10_enterprise_x64_dvd_6846957.sha", L"D:\\TestFolder2\\cn_windows_10_enterprise_x64_dvd_6846957.txt", },
    });
    Assert(ret);


    ret = param.ParseCommand(vector<wstring>{L"-t", L"-ns", L"windows", L"..\\test\\filelist2.txt" });
    Assert(ret);
    list = CompareFile(param);
    ret = GroupCompare(list, vector<vector<wstring>>{
        { L"D:\\TestFolder2\\1\\cn_windows_10_enterprise_x64_dvd_6846957.sha", L"D:\\TestFolder2\\cn_windows_10_enterprise_x64_dvd_6846957.txt", },
        { L"D:\\TestFolder2\\windows_10.sha", L"D:\\TestFolder2\\windows_10 - 副本.sha", },
    });
    Assert(ret);


    ret = param.ParseCommand(vector<wstring>{L"-t", L"-c", L"..\\test\\filelist2.txt" });
    Assert(ret);
    list = CompareFile(param);
    ret = GroupCompare(list, vector<vector<wstring>>{
        { L"D:\\TestFolder2\\cn", L"D:\\TestFolder2\\cn_windows_10_enterprise_x64_dvd.sha",  L"D:\\TestFolder2\\cnamsl.sha" },
        {
            L"D:\\TestFolder2\\cn_windows_10_enterprise_x64_dvd_6846957.txt",
            L"D:\\TestFolder2\\1\\cn_windows_10_enterprise_x64_dvd_6846957.sha",
        },
    });
    Assert(ret);


    ret = param.ParseCommand(vector<wstring>{L"-n", L"-c", L"..\\test\\filelist2.txt" });
    Assert(ret);
    list = CompareFile(param);
    ret = GroupCompare(list, vector<vector<wstring>>{ });
    Assert(ret);


    ret = param.ParseCommand(vector<wstring>{L"-ne", L"-c", L"..\\test\\filelist2.txt" });
    Assert(ret);
    list = CompareFile(param);
    ret = GroupCompare(list, vector<vector<wstring>>{
        { L"D:\\TestFolder2\\1.!bt", L"D:\\TestFolder2\\1.bak"},
        { L"D:\\TestFolder2\\cn_windows_10_enterprise_x64_dvd_6846957.txt", L"D:\\TestFolder2\\1\\cn_windows_10_enterprise_x64_dvd_6846957.sha" },
    });
    Assert(ret);


    ret = param.ParseCommand(vector<wstring>{L"-ns", L"windows", L"-c", L"..\\test\\filelist2.txt" });
    Assert(ret);
    list = CompareFile(param);
    ret = GroupCompare(list, vector<vector<wstring>>{
        {
          L"D:\\TestFolder2\\cn_windows_10_enterprise_x64_dvd.sha", L"D:\\TestFolder2\\cn_windows_10_enterprise_x64_dvd_6846957.txt",
          L"D:\\TestFolder2\\1\\cn_windows_10_enterprise_x64_dvd_6846957.sha", L"D:\\TestFolder2\\windows_10.sha"
        },
    });
    Assert(ret);
}
