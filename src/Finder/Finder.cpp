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
    for (auto& r : list)
    {
        string str = FileRecord::ToUTF8(r);
        cout << str;
    }
    cout << endl;
    return 0;
}
