#ifndef EVBUFFER_H
#define EVBUFFER_H
#include "../include/tlog.h"
#include <mutex>
#include <condition_variable>
#include <cstdio>

template<typename T,size_t buflen> class EVBuffer
{
public:
   EVBuffer():count_(0),write_index_(0),read_index_(0)
   {
   }

   void insert(const T& v)
   {
      tlog(LOG_EVBuffer_insert, "Inserting element");
      std::unique_lock guard(mutex_);
      if(count_ == buflen)
      {
         tlog(LOG_EVBuffer_insert, "Buffer full, waiting for capacity");
         capacity_available_.wait(guard, [this]{return (this->count_ < buflen);});
         tlog(LOG_EVBuffer_insert, "Capacity available");
      }
      content_[write_index_]=v;
      write_index_ = (write_index_ +1)%buflen;
      count_++;
      tlog(LOG_EVBuffer_insert, "Element inserted, count=%zu", count_);
      element_available_.notify_one();
   }

   T remove()
   {
      tlog(LOG_EVBuffer_remove, "Attempting to remove element");
      fflush(stdout);
      std::unique_lock guard(mutex_);
      if(count_ == 0)
      {
         tlog(LOG_EVBuffer_remove, "Buffer empty, waiting for element (count=%zu)", count_);
         element_available_.wait(guard, [this]{return (this->count_ > 0);});
         tlog(LOG_EVBuffer_remove, "Element available!");
      }
      T ret = content_[read_index_];
      read_index_ = (read_index_ +1)%buflen;
      count_--;
      tlog(LOG_EVBuffer_remove, "Element removed, count=%zu", count_);
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
