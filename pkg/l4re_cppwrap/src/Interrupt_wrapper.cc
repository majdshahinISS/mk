#include <l4/re/error_helper>
#include <l4/re/util/object_registry>
#include <l4/io/io.h>
#include <l4/sys/irq>
#include <memory>
#include <map>
#include <iostream>

#include "../include/Interrupt_wrapper.h"
#ifdef __cplusplus
extern "C" {
#endif
class InterruptPL : public L4::Irqep_t<InterruptPL>
{
public:
    InterruptPL(L4Re::Util::Registry_server<> *server,
        void * obj,
        int irqnum,
        void(*handler)(void*))
        :handler_(handler),_icu(l4io_request_icu()), irqnum(irqnum), _obj(obj), _server(server)
    {
        L4Re::chkcap(_irq = server->registry()->register_irq_obj(this));
        L4Re::chksys(_icu->bind(irqnum, _irq));
        _irq->unmask();
    }
    virtual void handle_irq()
    {
        
        if (handler_)
            handler_(_obj);
        
        _irq->unmask();
    }
    ~InterruptPL()
    {
        if (_irq.is_valid()) {
            _irq->detach();           // detach from interrupt source
        }
        // Optionally mask or unbind via ICU if needed — depending on platform
        if (_icu.is_valid()) {
            _icu->unbind(irqnum, _irq);       // or call ICU-specific unbind/mask
        }
        // Then unregister from registry:
        if (_server) {
            _server->registry()->unregister_obj(this);
        }
        
        // REVIEW 
        ////L4Re::Env::env()->rm()->detach(reinterpret_cast<l4_addr_t>(_addr), 0);
        // _server->registry()->unregister_obj(this); 
        // No need to manually unregister — the object goes away,
        // registry_server removes stale caps automatically.
        std::cout<<"InterruptPL is destructed\n";
    }

private:
    void * _obj;
    void (*handler_)(void* obj);
    int irqnum;
    L4::Cap<L4::Irq> _irq;
    L4::Cap<L4::Icu> _icu;
    L4Re::Util::Registry_server<> *_server;
};

static std::map<int,std::shared_ptr<InterruptPL>>  interrupts_map;

bool Interrupt_add(void *server_, void * obj, int irqnum, void(*interrupt_handler)(void*))
{
    if(server_ == nullptr)
        return false;
    L4Re::Util::Registry_server<> * server = (L4Re::Util::Registry_server<>*) server_;
    if((server == nullptr)|| (interrupt_handler == nullptr))
        return false;
    std::shared_ptr<InterruptPL> irq_wrapper = std::make_shared<InterruptPL>(server, obj, irqnum, interrupt_handler);
        
    interrupts_map.insert(std::make_pair(irqnum, irq_wrapper));
    
    return true;
}

bool Interrupt_remove(int irqnum)
{
    // from interrupts_map get the value using key
    // 
    auto it = interrupts_map.find(irqnum);
    if (it == interrupts_map.end())
        return false;

    interrupts_map.erase(it);   // destructor is called → cleanup done

    return true;
}

bool Interrupt_exist(int irqnum)
{
    // 
    auto it = interrupts_map.find(irqnum);
    if (it == interrupts_map.end())
        return false;
    else
        return true;
}

#ifdef __cplusplus
}
#endif