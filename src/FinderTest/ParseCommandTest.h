#pragma once

class ParseCommandTest
{
    void TestCase_Path();
    void TestCase_Type();
    void TestCase_Size();
    void TestCase_Attr();
    void TestCase_Multi();

public:
    ParseCommandTest()
    {
        TestCase_Path();
        TestCase_Type();
        TestCase_Size();
        TestCase_Attr();
        TestCase_Multi();
    }
};
