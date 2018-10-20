// FindTest.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <initializer_list>
#include <vector>
#include <string>
#include "../find/Parameters.h"
#include "ParseCommandTest.h"

using namespace std;


void ParseCommandTest::TestCase_Path()
{
    bool ret;
    Parameters param;
    ret = param.ParseCommand(vector<wstring>{ L"." });
    Assert(ret && !param.mSearchPath.empty());

    ret = param.ParseCommand(vector<wstring>{ L".." });
    Assert(ret && !param.mSearchPath.empty());

    ret = param.ParseCommand(vector<wstring>{ L"FindTest.cpp" });
    Assert(!ret);

    ret = param.ParseCommand(vector<wstring>{ L"c:" });
    Assert(ret && param.mSearchPath == L"c:");

    ret = param.ParseCommand(vector<wstring>{ L"D:" });
    Assert(ret && param.mSearchPath == L"D:");

    ret = param.ParseCommand(vector<wstring>{ L"c:\\windows" });
    Assert(ret && param.mSearchPath == L"c:\\windows");

    ret = param.ParseCommand(vector<wstring>{ L"..\\..\\test\\testFolder1" });
    Assert(ret);

    ret = param.ParseCommand(vector<wstring>{ L"..\\..\\test\\testFolder1\\" });
    Assert(ret);

    ret = param.ParseCommand(vector<wstring>{ L"..\\..\\test\\NotExistFolder\\" });
    Assert(!ret);

    ret = param.ParseCommand(vector<wstring>{ L"..\\..\\test\\NotExistFile" });
    Assert(!ret);

    ret = param.ParseCommand(vector<wstring>{ L"..\\..\\test\\ExistEmptyFile" });
    Assert(!ret);
}

void ParseCommandTest::TestCase_Type()
{
    bool ret;
    Parameters param;

    ret = param.ParseCommand(vector<wstring>{ L".", L"-type", L"+(dll)" });
    Assert(ret);
    Assert(!param.mSearchPath.empty());
    Assert(param.mSizeLow == 0);
    Assert(param.mSizeHigh == (uint64_t)-1);
    Assert(!param.mIncludeAttr);
    Assert(param.mAttrib == Parameters::NONE);
    Assert(param.mIncludeType);
    Assert(param.mTypeList.size() == 1);
    Assert(param.mTypeList.find(L"dll") != param.mTypeList.end());


    ret = param.ParseCommand(vector<wstring>{ L".", L"-type", L"-(dll)" });
    Assert(ret);
    Assert(!param.mSearchPath.empty());
    Assert(param.mSizeLow == 0);
    Assert(param.mSizeHigh == (uint64_t)-1);
    Assert(!param.mIncludeAttr);
    Assert(param.mAttrib == Parameters::NONE);
    Assert(!param.mIncludeType);
    Assert(param.mTypeList.size() == 1);
    Assert(param.mTypeList.find(L"dll") != param.mTypeList.end());


    ret = param.ParseCommand(vector<wstring>{ L".", L"-type" });
    Assert(!ret);

    ret = param.ParseCommand(vector<wstring>{ L".", L"-type", L"+dll" });
    Assert(!ret);

    ret = param.ParseCommand(vector<wstring>{ L".", L"-type", L"+()" });
    Assert(!ret);

    ret = param.ParseCommand(vector<wstring>{ L".", L"-type", L"+(5.a)" });
    Assert(!ret);

    ret = param.ParseCommand(vector<wstring>{ L".", L"-type", L"+(filter)" });
    Assert(ret);

    ret = param.ParseCommand(vector<wstring>{ L".", L"-type", L"a(dll)" });
    Assert(!ret);

    ret = param.ParseCommand(vector<wstring>{ L".", L"-type", L"(dll)" });
    Assert(!ret);
}

void ParseCommandTest::TestCase_Size()
{
    bool ret;
    Parameters param;

    ret = param.ParseCommand(vector<wstring>{ L".", L"-size", L"(5:5g)" });
    Assert(ret);
    Assert(!param.mSearchPath.empty());
    Assert(param.mSizeLow == 5);
    Assert(param.mSizeHigh == (uint64_t)5 * 1024 * 1024 * 1024);
    Assert(!param.mIncludeAttr);
    Assert(param.mAttrib == Parameters::NONE);
    Assert(!param.mIncludeType);
    Assert(param.mTypeList.empty());


    ret = param.ParseCommand(vector<wstring>{ L".", L"-size", L"(05:55)" });
    Assert(ret);
    Assert(!param.mSearchPath.empty());
    Assert(param.mSizeLow == 5);
    Assert(param.mSizeHigh == 55);
    Assert(!param.mIncludeAttr);
    Assert(param.mAttrib == Parameters::NONE);
    Assert(!param.mIncludeType);
    Assert(param.mTypeList.empty());


    ret = param.ParseCommand(vector<wstring>{ L".", L"-size", L"(15k:5m)" });
    Assert(ret);
    Assert(!param.mSearchPath.empty());
    Assert(param.mSizeLow == 15 * 1024);
    Assert(param.mSizeHigh == 5 * 1024 * 1024);
    Assert(!param.mIncludeAttr);
    Assert(param.mAttrib == Parameters::NONE);
    Assert(!param.mIncludeType);
    Assert(param.mTypeList.empty());


    ret = param.ParseCommand(vector<wstring>{ L".", L"-size", L"(:5m)" });
    Assert(ret);
    Assert(!param.mSearchPath.empty());
    Assert(param.mSizeLow == 0);
    Assert(param.mSizeHigh == 5 * 1024 * 1024);
    Assert(!param.mIncludeAttr);
    Assert(param.mAttrib == Parameters::NONE);
    Assert(!param.mIncludeType);
    Assert(param.mTypeList.empty());


    ret = param.ParseCommand(vector<wstring>{ L".", L"-size", L"(50k:)" });
    Assert(ret);
    Assert(!param.mSearchPath.empty());
    Assert(param.mSizeLow == 50 * 1024);
    Assert(param.mSizeHigh == (uint64_t)-1);
    Assert(!param.mIncludeAttr);
    Assert(param.mAttrib == Parameters::NONE);
    Assert(!param.mIncludeType);
    Assert(param.mTypeList.empty());


    ret = param.ParseCommand(vector<wstring>{ L".", L"-size", L"(:)" });
    Assert(ret);
    Assert(!param.mSearchPath.empty());
    Assert(param.mSizeLow == 0);
    Assert(param.mSizeHigh == (uint64_t)-1);
    Assert(!param.mIncludeAttr);
    Assert(param.mAttrib == Parameters::NONE);
    Assert(!param.mIncludeType);
    Assert(param.mTypeList.empty());


    ret = param.ParseCommand(vector<wstring>{ L".", L"-size" });
    Assert(!ret);

    ret = param.ParseCommand(vector<wstring>{ L".", L"-size", L"()" });
    Assert(!ret);

    ret = param.ParseCommand(vector<wstring>{ L".", L"-size", L"(5a:)" });
    Assert(!ret);

    ret = param.ParseCommand(vector<wstring>{ L".", L"-size", L"[5a:]" });
    Assert(!ret);

    ret = param.ParseCommand(vector<wstring>{ L".", L"-size", L"(k:k)" });
    Assert(!ret);

    ret = param.ParseCommand(vector<wstring>{ L".", L"-size", L"+(5k:)" });
    Assert(!ret);

    ret = param.ParseCommand(vector<wstring>{ L".", L"-size", L"(5k:)+" });
    Assert(!ret);

    ret = param.ParseCommand(vector<wstring>{ L".", L"-size", L"(5.5k:)" });
    Assert(!ret);

    ret = param.ParseCommand(vector<wstring>{ L".", L"-size", L"(3.5:10)" });
    Assert(!ret);

    ret = param.ParseCommand(vector<wstring>{ L".", L"-size", L"5k:5m" });
    Assert(!ret);
}

void ParseCommandTest::TestCase_Attr()
{
    bool ret;
    Parameters param;

    ret = param.ParseCommand(vector<wstring>{ L".", L"-attr", L"+(ashr)" });
    Assert(ret);
    Assert(!param.mSearchPath.empty());
    Assert(param.mSizeLow == 0);
    Assert(param.mSizeHigh == (uint64_t)-1);
    Assert(param.mIncludeAttr);
    Assert(param.mAttrib == (Parameters::ARCHIVE | Parameters::SYSTEM | Parameters::READONLY | Parameters::HIDDEN));
    Assert(!param.mIncludeType);
    Assert(param.mTypeList.empty());


    ret = param.ParseCommand(vector<wstring>{ L".", L"-attr", L"-(ashr)" });
    Assert(ret);
    Assert(!param.mSearchPath.empty());
    Assert(param.mSizeLow == 0);
    Assert(param.mSizeHigh == (uint64_t)-1);
    Assert(!param.mIncludeAttr);
    Assert(param.mAttrib == (Parameters::ARCHIVE | Parameters::SYSTEM | Parameters::READONLY | Parameters::HIDDEN));
    Assert(!param.mIncludeType);
    Assert(param.mTypeList.empty());


    ret = param.ParseCommand(vector<wstring>{ L".", L"-attr", L"-(ar)" });
    Assert(ret);
    Assert(!param.mSearchPath.empty());
    Assert(param.mSizeLow == 0);
    Assert(param.mSizeHigh == (uint64_t)-1);
    Assert(!param.mIncludeAttr);
    Assert(param.mAttrib == (Parameters::ARCHIVE | Parameters::READONLY));
    Assert(!param.mIncludeType);
    Assert(param.mTypeList.empty());


    ret = param.ParseCommand(vector<wstring>{ L".", L"-attr", L"+(ara)" });
    Assert(ret);
    Assert(param.mIncludeAttr);
    Assert(param.mAttrib == (Parameters::ARCHIVE | Parameters::READONLY));


    ret = param.ParseCommand(vector<wstring>{ L".", L"-attr", L"+()" });
    Assert(!ret);

    ret = param.ParseCommand(vector<wstring>{ L".", L"-attr", L"+ashr" });
    Assert(!ret);

    ret = param.ParseCommand(vector<wstring>{ L".", L"-attr", L"^(ashr)" });
    Assert(!ret);

    ret = param.ParseCommand(vector<wstring>{ L".", L"-attr", L"-(f)" });
    Assert(!ret);

    ret = param.ParseCommand(vector<wstring>{ L".", L"-attr", L"-[as]" });
    Assert(!ret);

    ret = param.ParseCommand(vector<wstring>{ L".", L"-attr", L"-(avs)" });
    Assert(!ret);
}

void ParseCommandTest::TestCase_Multi()
{
    bool ret;
    Parameters param;

    ret = param.ParseCommand(vector<wstring>{ L".", L"-type", L"+(dll,exe)", L"-attr", L"+(ars)", L"-size", L"(:10m)" });
    Assert(ret);
    Assert(!param.mSearchPath.empty());
    Assert(param.mSizeLow == 0);
    Assert(param.mSizeHigh == 10 * 1024 * 1024);
    Assert(param.mIncludeAttr);
    Assert(param.mAttrib == (Parameters::ARCHIVE | Parameters::SYSTEM | Parameters::READONLY));
    Assert(param.mIncludeType);
    Assert(param.mTypeList.size() == 2);
    Assert(param.mTypeList.find(L"dll") != param.mTypeList.end());
    Assert(param.mTypeList.find(L"exe") != param.mTypeList.end());


    ret = param.ParseCommand(vector<wstring>{ L".", L"-type", L"-(ini)", L"-attr", L"+(s)", L"-size", L"(:10m)" });
    Assert(ret);
    Assert(!param.mSearchPath.empty());
    Assert(param.mSizeLow == 0);
    Assert(param.mSizeHigh == 10 * 1024 * 1024);
    Assert(param.mIncludeAttr);
    Assert(param.mAttrib == (Parameters::SYSTEM));
    Assert(!param.mIncludeType);
    Assert(param.mTypeList.size() == 1);
    Assert(param.mTypeList.find(L"ini") != param.mTypeList.end());


    ret = param.ParseCommand(vector<wstring>{ L".", L"-type", L"-(mp3)", L"-size", L"(:10m)", L"-attr", L"+(s)" });
    Assert(ret);
    Assert(!param.mSearchPath.empty());
    Assert(param.mSizeLow == 0);
    Assert(param.mSizeHigh == 10 * 1024 * 1024);
    Assert(param.mIncludeAttr);
    Assert(param.mAttrib == (Parameters::SYSTEM));
    Assert(!param.mIncludeType);
    Assert(param.mTypeList.size() == 1);
    Assert(param.mTypeList.find(L"mp3") != param.mTypeList.end());
}