// DoubleChecker.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include <string>
#include <vector>
#include "Parammeters.h"
#include "Compare.h"
#include <Windows.h>

using namespace std;

int _tmain(int argc, TCHAR** argv)
{
    vector<wstring> cmd;
    for (int i = 1; i < argc; ++i)
    {
        cmd.push_back(argv[i]);
    }

    if (cmd.empty() || cmd.size() == 1 && (cmd[1] == L"--help" || cmd[1] == L"-h"))
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

    auto list = CompareFile(param);

    for (size_t i = 0; i < list.size(); ++i)
    {
        cout << "group " << i << ":\n-------------------------\n";
        for (auto& fr : list[i])
        {
            int n = WideCharToMultiByte(CP_UTF8, NULL, fr->mPath.c_str(), -1, NULL, NULL, NULL, NULL);
            vector<char> mcb(n);
            WideCharToMultiByte(CP_UTF8, NULL, fr->mPath.c_str(), -1, mcb.data(), n, NULL, NULL);
            cout.write(mcb.data(), n - 1);

            wchar_t wbuf[1024];
            swprintf_s(wbuf, _countof(wbuf), L"|%lld|%lld\n", fr->mFileSize, fr->mLastWriteTime);
            n = WideCharToMultiByte(CP_UTF8, NULL, wbuf, -1, NULL, NULL, NULL, NULL);
            mcb.resize(n);
            WideCharToMultiByte(CP_UTF8, NULL, wbuf, -1, mcb.data(), n, NULL, NULL);
            cout.write(mcb.data(), n - 1);

        }
        cout << "\n";
    }
    return 0;
}

