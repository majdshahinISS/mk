#include "Test.h"

// Constructor that takes a string and a void function with no parameters
Test::Test(const std::string& testName, std::function<void()> func) 
    : name(testName), testFunction(func) 
{
    std::cout << "Created test: " << name << std::endl;
}

// Method to run the test
void Test::run() const{
    std::cout << "Running test: " << name << std::endl;
    testFunction();
}

// Getter for the test name
std::string Test::getName() const {
    return name;
}