// DoubleCheckerTest.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "ParseCommandTest.h"
#include "CompareFileTest.h"
#include <string>
#include <windows.h>

int _tmain(int argc, TCHAR** argv)
{
    TCHAR drive[32], dir[1024];
    _wsplitpath_s(argv[0], drive, _countof(drive), dir, _countof(dir), nullptr, 0, nullptr, 0);
    std::wstring p(drive);
    p += dir;
    SetCurrentDirectory(p.c_str());

    ParseCommandTest parseCommandTest;
    CompareFileTest compareFileTest;
    return 0;
}

