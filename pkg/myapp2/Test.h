#ifndef TEST_H
#define TEST_H

#include <iostream>
#include <string>
#include <functional>

class Test {
    private:
        std::string name;
        std::function<void()> testFunction;
    
    public:
        // Constructor that takes a string and a void function with no parameters
        Test(const std::string& testName, std::function<void()> func) ;
    
        // Method to run the test
        void run() const;
    
        // Getter for the test name
        std::string getName() const ;
    };

#endif // TEST_H