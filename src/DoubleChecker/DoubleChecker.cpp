// DoubleChecker.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include <string>
#include <vector>
#include "Parammeters.h"
#include "Compare.h"

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
		cout << "*\n";
        for (auto& fr : list[i])
        {
            string str = FileRecord::ToUTF8(*fr);
            cout << str;
        }
        cout << "\n";
    }
    return 0;
}

