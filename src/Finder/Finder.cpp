// Find.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <utls/utility.h>
#include <iostream>
#include "Parameters.h"
#include "ListFile.h"
#include <Windows.h>

using namespace std;

int _tmain(int argc, wchar_t** argv)
{
    vector<wstring> cmd;
    for (int i = 1; i < argc; ++i)
    {
        cmd.push_back(argv[i]);
    }

    if (cmd.empty() || cmd.size() == 1 && (cmd[0] == L"--help" || cmd[0] == L"-h"))
    {
        wcout << Parameters::Usage();
        return 1;
    }

    Parameters param;
    wstring err;
    if (!param.ParseCommand(cmd, err))
    {
        wcout << err << L"\n\n";
        return 1;
    }

    auto list = ListFile(param);
    setlocale(LC_ALL, "zh-cn");
    for (auto& r : list)
    {
        int n = WideCharToMultiByte(CP_UTF8, NULL, r.mPath.c_str(), -1, NULL, NULL, NULL, NULL);
        vector<char> mcb(n);
        WideCharToMultiByte(CP_UTF8, NULL, r.mPath.c_str(), -1, mcb.data(), n, NULL, NULL);
        cout.write(mcb.data(), n - 1);

        wchar_t wbuf[128];
        swprintf_s(wbuf, _countof(wbuf), L"|%lld|%lld\n", r.mFileSize, r.mLastWriteTime);
        n = WideCharToMultiByte(CP_UTF8, NULL, wbuf, -1, NULL, NULL, NULL, NULL);
        mcb.resize(n);
        WideCharToMultiByte(CP_UTF8, NULL, wbuf, -1, mcb.data(), n, NULL, NULL);
        cout.write(mcb.data(), n - 1);
    }
    cout << endl;
    return 0;
}
