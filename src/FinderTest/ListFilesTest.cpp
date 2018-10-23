
#include "stdafx.h"
#include <vector>
#include <string>
#include <initializer_list>
#include "../Finder/Parameters.h"
#include "../Finder/ListFile.h"
#include "ListFilesTest.h"

using namespace std;

void ListFilesTest::TestSize()
{
    bool ret;
    Parameters param;
    vector<FileRecord> record;
    ret = param.ParseCommand(vector<wstring>{L"..\\test\\TestFolder1\\"});
    Assert(ret);
    record = ListFile(param);
    Assert(record.size() == 302);


    ret = param.ParseCommand(vector<wstring>{L"..\\test\\TestFolder1\\", L"-size", L"(:)"});
    Assert(ret);
    record = ListFile(param);
    Assert(record.size() == 302);


    ret = param.ParseCommand(vector<wstring>{L"..\\test\\TestFolder1\\", L"-size", L"(2m:)"});
    Assert(ret);
    record = ListFile(param);
    Assert(record.size() == 6);


    ret = param.ParseCommand(vector<wstring>{L"..\\test\\TestFolder1\\", L"-size", L"(100k:1m)"});
    Assert(ret);
    record = ListFile(param);
    Assert(record.size() == 48);


    ret = param.ParseCommand(vector<wstring>{L"..\\test\\TestFolder1\\", L"-size", L"(:854)"});
    Assert(ret);
    record = ListFile(param);
    Assert(record.size() == 30);


    ret = param.ParseCommand(vector<wstring>{L"..\\test\\TestFolder1\\", L"-size", L"(88:88)"});
    Assert(ret);
    record = ListFile(param);
    Assert(record.size() == 4);


    ret = param.ParseCommand(vector<wstring>{L"..\\test\\TestFolder1\\", L"-size", L"(0:0)"});
    Assert(ret);
    record = ListFile(param);
    Assert(record.size() == 6);
}

void ListFilesTest::TestAttr()
{
    bool ret;
    Parameters param;
    vector<FileRecord> record;

    ret = param.ParseCommand(vector<wstring>{L"..\\test\\TestFolder1\\", L"-attr", L"+(a)"});
    Assert(ret);
    record = ListFile(param);
    Assert(record.size() == 302);

    ret = param.ParseCommand(vector<wstring>{L"..\\test\\TestFolder1\\", L"-attr", L"-(a)"});
    Assert(ret);
    record = ListFile(param);
    Assert(record.size() == 0);

    ret = param.ParseCommand(vector<wstring>{L"..\\test\\TestFolder1\\", L"-attr", L"+(r)"});
    Assert(ret);
    record = ListFile(param);
    Assert(record.size() == 2);

    ret = param.ParseCommand(vector<wstring>{L"..\\test\\TestFolder1\\", L"-attr", L"+(rhs)"});
    Assert(ret);
    record = ListFile(param);
    Assert(record.size() == 8);

    ret = param.ParseCommand(vector<wstring>{L"..\\test\\TestFolder1\\", L"-attr", L"-(rhs)"});
    Assert(ret);
    record = ListFile(param);
    Assert(record.size() == 294);
}

void ListFilesTest::TestType()
{
    bool ret;
    Parameters param;
    vector<FileRecord> record;

    ret = param.ParseCommand(vector<wstring>{L"..\\test\\TestFolder1\\", L"-type", L"+(apk,avi,bak,c,chm,cue,dll,doc,docx,dwg,epub)"});
    Assert(ret);
    record = ListFile(param);
    Assert(record.size() == 24);

    ret = param.ParseCommand(vector<wstring>{L"..\\test\\TestFolder1\\", L"-type", L"-(apk,avi,bak,c,chm,cue,dll,doc,docx,dwg,epub)"});
    Assert(ret);
    record = ListFile(param);
    Assert(record.size() == 278);

    ret = param.ParseCommand(vector<wstring>{L"..\\test\\TestFolder1\\", L"-type", L"+(fake)"});
    Assert(ret);
    record = ListFile(param);
    Assert(record.size() == 0);

    ret = param.ParseCommand(vector<wstring>{L"..\\test\\TestFolder1\\", L"-type", L"-(fake)"});
    Assert(ret);
    record = ListFile(param);
    Assert(record.size() == 302);
}
