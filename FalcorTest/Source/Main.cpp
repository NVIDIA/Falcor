#include "Falcor.h"
#include <iostream>
#include "CoreTester.h"

int main()
{
    CoreTester ct;
    
    ct.runTestList(TestList::sGetTests());;
    std::cin.get();
    return 0;
}