
#include "stdafx.h"
#include "ParseCommandTest.h"
#include "ListFilesTest.h"
#include <string>
#include <Windows.h>

int _tmain(int argc, TCHAR** argv)
{
    TCHAR drive[32], dir[1024];
    _wsplitpath_s(argv[0], drive, _countof(drive), dir, _countof(dir), nullptr, 0, nullptr, 0);
    std::wstring p(drive);
    p += dir;
    SetCurrentDirectory(p.c_str());

    ParseCommandTest ParseCommand;
    ListFilesTest FindFilesTest;
    return 0;
}

