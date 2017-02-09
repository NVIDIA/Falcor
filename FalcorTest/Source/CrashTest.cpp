#include "CrashTest.h"

void CrashTest::addTests()
{
    addTestToList<TestThrow>();
    addTestToList<TestVector>();
    addTestToList<TestAssert>();
    addTestToList<TestCrashHandling>();
}

testing_func(CrashTest, TestThrow)
{
    throw std::exception("error message");
    return test_fail("Test proceeded past throw");
}

testing_func(CrashTest, TestVector)
{
    std::vector<char> testVec;
    testVec.resize(10);
    testVec.at(100);
    return test_fail("Test proceeded past operation that should have thrown");
}

testing_func(CrashTest, TestAssert)
{
    assert(false);
    return test_fail("Test proceeded past assert");
}

testing_func(CrashTest, TestCrashHandling)
{
    //Really only testing that this gets run despite previous tets crashing
    return test_pass();
}

int main()
{
    CrashTest ct;
    ct.init();
    ct.run();
    return 0;
}