#include "CrashTest.h"

CrashTest::CrashTest()
{
    REGISTER_NAME;
}

void CrashTest::addTests()
{
    addTestToList<TestThrow>();
    addTestToList<TestVector>();
    //addTestToList<TestAssert>();
    addTestToList<TestCrashHandling>();
}

TESTING_FUNC(CrashTest, TestThrow)
{
    throw std::exception("error message");
    return TestBase::TestData(TestBase::TestResult::Fail, mName, "Test proceeded past throw");
}

TESTING_FUNC(CrashTest, TestVector)
{
    std::vector<char> testVec;
    testVec.resize(10);
    testVec.at(100);
    return TestBase::TestData(TestBase::TestResult::Fail, mName, "Test proceeded past operation that should have thrown");
}

TESTING_FUNC(CrashTest, TestAssert)
{
    assert(false);
    return TestBase::TestData(TestBase::TestResult::Fail, mName, "Test proceeded past assert");
}

TESTING_FUNC(CrashTest, TestCrashHandling)
{
    //Really only testing that this gets run despite previous tets crashing
    return TestBase::TestData(TestBase::TestResult::Pass, mName);
}

int main()
{
    CrashTest ct;
    ct.init();
    ct.run();
    return 0;
}