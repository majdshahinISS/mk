#ifndef EVBUFFER_H
#define EVBUFFER_H

#include <mutex>
#include <condition_variable>

template<typename T,size_t buflen> class EVBuffer
{
public:
   EVBuffer():count_(0),write_index_(0),read_index_(0)
   {
   }

   void insert(const T& v)
   {
      std::unique_lock guard(mutex_);
      if(count_ == buflen)
      {
         capacity_available_.wait(guard, [this]{return (this->count_ < buflen);});
      }
      content_[write_index_]=v;
      write_index_ = (write_index_ +1)%buflen;
      count_++;
      element_available_.notify_one();
   }

   T remove()
   {
      std::unique_lock guard(mutex_);
      if(count_ == 0)
      {
         element_available_.wait(guard, [this]{return (this->count_ > 0);});
      }
      T ret = content_[read_index_];
      read_index_ = (read_index_ +1)%buflen;
      count_--;
      capacity_available_.notify_one();
      return ret;
   }

private:
   T content_[buflen];
   std::mutex mutex_;
   std::condition_variable element_available_;
   std::condition_variable capacity_available_;
   size_t count_;
   size_t write_index_;
   size_t read_index_;
};
#endif
