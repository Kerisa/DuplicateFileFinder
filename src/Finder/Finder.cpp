// Find.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <utls/utility.h>
#include <iostream>
#include "Parameters.h"
#include "ListFile.h"

using namespace std;

int _tmain(int argc, wchar_t** argv)
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

    auto list = ListFile(param);
    setlocale(LC_ALL, "zh-cn");
    for (auto& r : list)
    {
        char buf[1024] = { 0 };
        size_t conved = 0;
        wcstombs_s(&conved, buf, sizeof(buf), r.mPath.c_str(), r.mPath.size() * sizeof(wchar_t));
        cout << buf << "\n";
    }
    cout << endl;
    return 0;
}
