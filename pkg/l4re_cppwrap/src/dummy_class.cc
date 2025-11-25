
#include "../include/dummy_class.h"
// #include "../include/log_wrapper.h"
#include <iostream>
class DummyClass {
public:
    DummyClass() : data_{0} {}
    void setData(int value) { data_ = value; }
    int getData() const { return data_; } 
private:
    int data_;
};
bool new_dummy_class_(void **obj)
{
    if (obj == nullptr) {
        return false;
    }
    DummyClass* instance = new DummyClass();
    if (instance == nullptr) {
        return false;
    }
    
    *obj = static_cast<void*>(instance);
    std::cout << "DummyClass instance created " << instance << std::endl;
    return true;
}

bool delete_dummy_class_(void **obj)
{
    if (obj == nullptr || *obj == nullptr) {
        return false;
    }
    std::cout << "cout ! Deleting DummyClass instance\n";
    DummyClass* instance = static_cast<DummyClass*>(*obj);
    std::cout<< "DummyClass instance deleted "<< instance << std::endl;
    delete instance;
    *obj = nullptr;
    return true;
}

#ifdef __cplusplus
extern "C" {
#endif
bool new_dummy_class(void **obj)
{
    return new_dummy_class_(obj);
}
bool delete_dummy_class(void **obj)
{
    return delete_dummy_class_(obj);
}



#ifdef __cplusplus
}
#endif