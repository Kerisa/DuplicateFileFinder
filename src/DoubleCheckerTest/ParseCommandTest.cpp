
#include "stdafx.h"
#include <initializer_list>
#include <string>
#include <vector>
#include "ParseCommandTest.h"
#include "../DoubleChecker/Parammeters.h"

using namespace std;

void ParseCommandTest::NameTest()
{
    bool ret;
    Parameters param;

    ret = param.ParseCommand(vector<wstring>{ L"-n", L"..\\test\\filelist1.txt" });
    Assert(ret);
    Assert(param.mName.mSwitch == Parameters::NameParam::MATCH_WHOLE_NAME);
    Assert(param.mContent.mSwitch == Parameters::ContentParam::MATCH_DISABLE);
    Assert(param.mSize.mSwitch == Parameters::SizeParam::MATCH_DISABLE);
    Assert(param.mTime.mSwitch == Parameters::TimeParam::MATCH_DISABLE);


    ret = param.ParseCommand(vector<wstring>{ L"-ne", L"..\\test\\filelist1.txt" });
    Assert(ret);
    Assert(param.mName.mSwitch == Parameters::NameParam::MATCH_WHOLE_NAME_WITHOUT_SUFFIX);
    Assert(param.mContent.mSwitch == Parameters::ContentParam::MATCH_DISABLE);
    Assert(param.mSize.mSwitch == Parameters::SizeParam::MATCH_DISABLE);
    Assert(param.mTime.mSwitch == Parameters::TimeParam::MATCH_DISABLE);


    ret = param.ParseCommand(vector<wstring>{ L"-ns", L"1string+", L"..\\test\\filelist1.txt" });
    Assert(ret);
    Assert(param.mName.mSwitch == Parameters::NameParam::MATCH_STRING);
    Assert(param.mName.mMatchString == L"1string+");
    Assert(param.mContent.mSwitch == Parameters::ContentParam::MATCH_DISABLE);
    Assert(param.mSize.mSwitch == Parameters::SizeParam::MATCH_DISABLE);
    Assert(param.mTime.mSwitch == Parameters::TimeParam::MATCH_DISABLE);


    ret = param.ParseCommand(vector<wstring>{ L"-ns", L"-n", L"..\\test\\filelist1.txt" });
    Assert(ret);

    ret = param.ParseCommand(vector<wstring>{ L"-ns", L"..\\test\\filelist1.txt" });
    Assert(!ret);

    ret = param.ParseCommand(vector<wstring>{ L"-n", L"-n", L"..\\test\\filelist1.txt" });
    Assert(!ret);

    ret = param.ParseCommand(vector<wstring>{ L"-ne", L"-n", L"..\\test\\filelist1.txt" });
    Assert(!ret);
}

void ParseCommandTest::SizeTest()
{
    bool ret;
    Parameters param;

    ret = param.ParseCommand(vector<wstring>{ L"-s", L"..\\test\\filelist1.txt" });
    Assert(ret);
    Assert(param.mName.mSwitch == Parameters::NameParam::MATCH_DISABLE);
    Assert(param.mSize.mSwitch == Parameters::SizeParam::MATCH_EXACTLY);
    Assert(param.mTime.mSwitch == Parameters::TimeParam::MATCH_DISABLE);
    Assert(param.mContent.mSwitch == Parameters::ContentParam::MATCH_DISABLE);

    ret = param.ParseCommand(vector<wstring>{ L"-s", L"-s", L"..\\test\\filelist1.txt" });
    Assert(!ret);
}

void ParseCommandTest::TimeTest()
{
    bool ret;
    Parameters param;

    ret = param.ParseCommand(vector<wstring>{ L"-t", L"..\\test\\filelist1.txt" });
    Assert(ret);
    Assert(param.mName.mSwitch == Parameters::NameParam::MATCH_DISABLE);
    Assert(param.mSize.mSwitch == Parameters::SizeParam::MATCH_DISABLE);
    Assert(param.mTime.mSwitch == Parameters::TimeParam::MATCH_EXACTLY);
    Assert(param.mContent.mSwitch == Parameters::ContentParam::MATCH_DISABLE);


    ret = param.ParseCommand(vector<wstring>{ L"-t", L"-t", L"..\\test\\filelist1.txt" });
    Assert(!ret);
}

void ParseCommandTest::ContentTest()
{
    bool ret;
    Parameters param;

    ret = param.ParseCommand(vector<wstring>{ L"-c", L"..\\test\\filelist1.txt" });
    Assert(ret);
    Assert(param.mName.mSwitch == Parameters::NameParam::MATCH_DISABLE);
    Assert(param.mSize.mSwitch == Parameters::SizeParam::MATCH_EXACTLY);
    Assert(param.mTime.mSwitch == (Parameters::TimeParam::MATCH_DISABLE));
    Assert(param.mContent.mSwitch == Parameters::ContentParam::MATCH_WHOLE_FILE);

    ret = param.ParseCommand(vector<wstring>{ L"-c", L"-c", L"..\\test\\filelist1.txt" });
    Assert(!ret);
}

void ParseCommandTest::FileListTest()
{
    bool ret;
    Parameters param;

    ret = param.ParseCommand(vector<wstring>{ L"..\\test\\filelist1.txt" });
    Assert(ret);
    Assert(param.mFileList.size() == 304);

    ret = param.ParseCommand(vector<wstring>{ L"..\\test\\ExistEmptyFile" });
    Assert(!ret);
    Assert(param.mFileList.size() == 0);

    ret = param.ParseCommand(vector<wstring>{ L"..\\test\\NotExistFile" });
    Assert(!ret);
    Assert(param.mFileList.size() == 0);
}

void ParseCommandTest::CombineTest()
{
    bool ret;
    Parameters param;

    ret = param.ParseCommand(vector<wstring>{ L"-c", L"-s", L"-t", L"-ns", L"samPle", L"..\\test\\filelist1.txt" });
    Assert(ret);
    Assert(param.mName.mSwitch == Parameters::NameParam::MATCH_STRING);
    Assert(param.mName.mMatchString == L"samPle");
    Assert(param.mSize.mSwitch == Parameters::SizeParam::MATCH_EXACTLY);
    Assert(param.mTime.mSwitch == (Parameters::TimeParam::MATCH_EXACTLY));
    Assert(param.mContent.mSwitch == Parameters::ContentParam::MATCH_WHOLE_FILE);


    // ½»»»Ë³Ðò
    ret = param.ParseCommand(vector<wstring>{ L"-c", L"-ns", L"samPle", L"-s", L"-t", L"..\\test\\filelist1.txt" });
    Assert(ret);
    Assert(param.mName.mSwitch == Parameters::NameParam::MATCH_STRING);
    Assert(param.mName.mMatchString == L"samPle");
    Assert(param.mSize.mSwitch == Parameters::SizeParam::MATCH_EXACTLY);
    Assert(param.mTime.mSwitch == (Parameters::TimeParam::MATCH_EXACTLY));
    Assert(param.mContent.mSwitch == Parameters::ContentParam::MATCH_WHOLE_FILE);


    ret = param.ParseCommand(vector<wstring>{ L"-c", L"-s", L"-t", L"-n", L"..\\test\\filelist1.txt" });
    Assert(ret);
    Assert(param.mName.mSwitch == Parameters::NameParam::MATCH_WHOLE_NAME);
    Assert(param.mSize.mSwitch == Parameters::SizeParam::MATCH_EXACTLY);
    Assert(param.mTime.mSwitch == (Parameters::TimeParam::MATCH_EXACTLY));
    Assert(param.mContent.mSwitch == Parameters::ContentParam::MATCH_WHOLE_FILE);


    ret = param.ParseCommand(vector<wstring>{ L"..\\test\\filelist1.txt" });
    Assert(ret);
    Assert(param.mName.mSwitch == Parameters::NameParam::MATCH_DISABLE);
    Assert(param.mSize.mSwitch == Parameters::SizeParam::MATCH_DISABLE);
    Assert(param.mTime.mSwitch == (Parameters::TimeParam::MATCH_DISABLE));
    Assert(param.mContent.mSwitch == Parameters::ContentParam::MATCH_DISABLE);


    ret = param.ParseCommand(vector<wstring>{ L"-s", L"-t", L"-ns", L"..\\test\\filelist1.txt", L"..\\test\\filelist1.txt" });
    Assert(ret);
    Assert(param.mName.mSwitch == Parameters::NameParam::MATCH_STRING);
    Assert(param.mName.mMatchString == L"..\\test\\filelist1.txt");
    Assert(param.mSize.mSwitch == Parameters::SizeParam::MATCH_EXACTLY);
    Assert(param.mTime.mSwitch == (Parameters::TimeParam::MATCH_EXACTLY));
    Assert(param.mContent.mSwitch == Parameters::ContentParam::MATCH_DISABLE);


    // file list miss
    ret = param.ParseCommand(vector<wstring>{ L"-c", L"(0:1)", L"-s", L"-t", L"-ns", L"..\\test\\filelist1.txt" });
    Assert(!ret);

    // file list miss
    ret = param.ParseCommand(vector<wstring>{ L"-ns", L"SAMpLE", L"-t", L"-s" });
    Assert(!ret);
}
