#pragma once

class ParseCommandTest
{
    void NameTest();
    void SizeTest();
    void TimeTest();
    void ContentTest();
    void FileListTest();
    void CombineTest();

public:
    ParseCommandTest()
    {
        NameTest();
        SizeTest();
        TimeTest();
        ContentTest();
        FileListTest();
        CombineTest();
    }
};