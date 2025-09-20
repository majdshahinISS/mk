

# if 0


#include <memory>
class DS_Side
{
private:
  std::unique_ptr<DataspaceOwner> this_side;  // null initially
  // new_data_callback_t incomming_new_data_handler;
  // free_your_data_callback_t free_your_data_handler; // handel free data req. from the other side
public:
  L4::Cap<IDataspaceOwner> other_side ;
  DS_Side(
    L4Re::Util::Registry_server<> *server = nullptr,
    const char *this_side_ipc_name = nullptr, 
    const char *other_side_ipc_name = nullptr,
    new_data_callback_t new_data_callback = nullptr,            // handle new available data from the other side
    free_your_data_callback_t free_your_data_callback = nullptr // handle free data req. from the other side
  ) 
  {
    if (server != nullptr && this_side_ipc_name != nullptr)
    {
        this_side = std::make_unique<DataspaceOwner>(
        server, 
        this_side_ipc_name,
        new_data_callback,
        free_your_data_callback // handle free data req. from the other side
      ); // name must be 11 characters long maximum
    }
    else 
      this_side = nullptr;
    if (other_side_ipc_name != nullptr)
    {
      other_side =  L4::Cap<IDataspaceOwner>();
      other_side =L4Re::Env::env()->get_cap<IDataspaceOwner>(other_side_ipc_name);
      if (!other_side.is_valid()) {
        std::printf("Failed to get dss capability\n");
        return ;
      }
    }
    //else
      //other_side = nullptr;
  }

};



#endif 