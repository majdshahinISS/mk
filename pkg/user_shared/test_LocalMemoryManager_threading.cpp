#if 0

// LocalMemoryManager.hpp
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>

class LocalMemoryManager
{
public:
  // Compile-time knobs
  static constexpr std::size_t kAlign    = 16;     // 16-byte alignment
  static constexpr std::size_t kMaxNodes = 128;    // max free-list fragments
  std::atomic<bool> is_ready{false};
  LocalMemoryManager(void *buffer_base, std::size_t buffer_size)
  : base_(static_cast<std::uint8_t*>(buffer_base)), size_(buffer_size)
  {
    reset();
    if((base_ != nullptr) && (buffer_size != 0))
        is_ready.store(true);
  }

  LocalMemoryManager()
  {

  }

  int init(void *buffer_base, std::size_t buffer_size)
  {
    base_ = static_cast<std::uint8_t*>(buffer_base);
    size_ = buffer_size;
    reset();
    if((base_ != nullptr) && (buffer_size != 0))
        is_ready.store(true);
    
    return 0;
  }

  void wait_for_is_ready()
  {
    while(is_ready.load()== false)
        sleep(1);
  }

  // Allocate a sub-buffer of at least `size` bytes; returns nullptr on failure.
  void* allocate_local(std::size_t size)
  {
    if (is_ready.load() == false)
    {
        std::printf("LocalMemoryManager is not ready\n");
        return nullptr;
    }
    std::lock_guard<std::mutex> lk(mtx_);
    if (!base_ || size == 0) return nullptr;
    const std::size_t need = align_up(size, kAlign);

    Node *prev = nullptr, *cur = free_head_;
    while (cur)
    {
      const std::size_t aligned_off = align_up(cur->off, kAlign);
      const std::size_t padding     = aligned_off - cur->off;

      if (padding <= cur->size && need <= (cur->size - padding))
      {
        const std::size_t right_off  = aligned_off + need;
        const std::size_t right_size = (cur->off + cur->size) - right_off;

        if (padding == 0 && right_size == 0)
        {
          // Use whole node.
          remove_node(prev, cur);
          free_node(cur);
        }
        else if (padding == 0)
        {
          // Keep right remainder in place.
          cur->off  = right_off;
          cur->size = right_size;
        }
        else if (right_size == 0)
        {
          // Keep left padding as the node.
          cur->size = padding;
        }
        else
        {
          // Split into left padding (keep cur) + right remainder (new node).
          Node *right = alloc_node();
          if (!right) return nullptr; // out of bookkeeping nodes
          right->off  = right_off;
          right->size = right_size;
          right->next = cur->next;
          cur->size   = padding;
          cur->next   = right;
        }
        return base_ + aligned_off;
      }
      prev = cur;
      cur  = cur->next;
    }
    return nullptr; // no fit
  }

  // Free a previously allocated sub-buffer at `addr` with `size` bytes.
  void free_local(void *addr, std::size_t size)
  {
    if (is_ready.load() == false)
    {
        std::printf("LocalMemoryManager is not ready\n");
        return;
    }
    std::lock_guard<std::mutex> lk(mtx_);

    if (!addr || size == 0 || !base_) return;

    auto p = reinterpret_cast<std::uintptr_t>(addr);
    auto b = reinterpret_cast<std::uintptr_t>(base_);
    if (p < b || p >= b + size_) return; // out of range; ignore

    Node *n = alloc_node();
    if (!n) return; // bookkeeping exhausted; conservatively ignore
    n->off  = static_cast<std::size_t>(p - b);
    n->size = align_up(size, kAlign);
    n->next = nullptr;

    // Insert sorted by offset.
    if (!free_head_ || n->off < free_head_->off)
    {
      n->next = free_head_;
      free_head_ = n;
    }
    else
    {
      Node *cur = free_head_;
      while (cur->next && cur->next->off < n->off) cur = cur->next;
      n->next = cur->next;
      cur->next = n;
    }

    // Coalesce adjacent nodes.
    coalesce();
  }

  // Reset back to a single free chunk (entire buffer free).
  void reset()
  {
    // rebuild node free-pool
    for (std::size_t i = 0; i < kMaxNodes; ++i) node_pool_[i].next = (i+1<kMaxNodes) ? &node_pool_[i+1] : nullptr;
    pool_head_ = &node_pool_[0];
    free_head_ = nullptr;

    // one big free chunk
    Node *root = alloc_node();
    if (root)
    {
      root->off  = 0;
      root->size = size_;
      root->next = nullptr;
      free_head_ = root;
    }
  }

private:
  struct Node {
    std::size_t off;
    std::size_t size;
    Node *next;
  };

  static constexpr std::size_t align_up(std::size_t x, std::size_t a)
  { return (x + (a - 1)) & ~(a - 1); }

  Node* alloc_node()
  {
    if (!pool_head_) return nullptr;
    Node *n = pool_head_;
    pool_head_ = pool_head_->next;
    n->next = nullptr;
    return n;
  }

  void free_node(Node *n)
  {
    n->next   = pool_head_;
    pool_head_ = n;
  }

  void remove_node(Node *prev, Node *cur)
  {
    if (!prev) free_head_ = cur->next;
    else       prev->next = cur->next;
  }

  void coalesce()
  {
    Node *cur = free_head_;
    while (cur && cur->next)
    {
      if (cur->off + cur->size == cur->next->off)
      {
        Node *n = cur->next;
        cur->size += n->size;
        cur->next  = n->next;
        free_node(n);
      }
      else cur = cur->next;
    }
  }

  std::mutex mtx_;

  // State
  std::uint8_t *base_ = nullptr;
  std::size_t   size_ = 0;

  // Free-list bookkeeping (no heap)
  Node  node_pool_[kMaxNodes]{};
  Node *pool_head_ = nullptr;   // free nodes for bookkeeping
  Node *free_head_ = nullptr;   // free chunks sorted by offset
};

//////////////////////////////////

// TEST


/// @brief ///////////////////////////////////////////
/// @return /

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>   // usleep
#include <time.h>     // nanosleep (optional)
#include <stdarg.h>


typedef struct {
  LocalMemoryManager* mm;
  uint8_t             tid_byte;    // 1..255
  size_t              alloc_size;  // per-thread size
} ThreadArg;

static pthread_mutex_t g_print_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_err_mtx   = PTHREAD_MUTEX_INITIALIZER;
static int g_errors = 0;

static void report(const char* fmt, ...)
{
  pthread_mutex_lock(&g_print_mtx);
  va_list ap; va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
  fputc('\n', stdout);
  va_end(ap);
  pthread_mutex_unlock(&g_print_mtx);
}

static void add_error(void)
{
  pthread_mutex_lock(&g_err_mtx);
  ++g_errors;
  pthread_mutex_unlock(&g_err_mtx);
}

static void* worker(void* arg_void)
{
  ThreadArg* arg = (ThreadArg*)arg_void;
  LocalMemoryManager* mm = arg->mm;
  const uint8_t tidb = arg->tid_byte;
  const size_t SZ = arg->alloc_size;

  // 1) allocate
  void* p = mm->allocate_local(SZ);
  if (!p) {
    report("[T%u] allocation FAILED for %zu bytes", (unsigned)tidb, SZ);
    add_error();
    return NULL;
  }

  // 2) fill
  uint8_t* b = (uint8_t*)p;
  for (size_t i = 0; i < SZ; ++i) b[i] = tidb;

  // 3) wait 1..3 seconds (deterministic per thread)
  //unsigned delay_ms = 1000u + ((unsigned)tidb * 37u) % 2000u;
  //usleep(delay_ms * 1000u);

  // 4) verify
  size_t bad_idx = (size_t)-1;
  for (size_t i = 0; i < SZ; ++i) {
    if (b[i] != tidb) { bad_idx = i; break; }
  }
  if (bad_idx != (size_t)-1) {
    report("[T%u] CORRUPTION at +%zu : got %u, expected %u",
           (unsigned)tidb, bad_idx, (unsigned)b[bad_idx], (unsigned)tidb);
    add_error();
  }

  // 5) free
  mm->free_local(p, SZ);
  return NULL;
}

int test3()
{
   enum { BUF_SIZE = 10 * 1024 };
  static uint8_t backing[BUF_SIZE];
  LocalMemoryManager mm;
  mm.init(backing, (size_t)BUF_SIZE);
  mm.wait_for_is_ready();
  // 255 threads × 32 bytes = 8160 bytes (fits with 16B alignment & metadata)
  enum { N_THREADS = 255 };
  const size_t PER_THREAD = 1024;

  pthread_t tids[N_THREADS];
  ThreadArg args[N_THREADS];

  // spawn
  for (int i = 0; i < N_THREADS; ++i) {
    args[i].mm         = &mm;
    args[i].tid_byte   = (uint8_t)(i + 1); // 1..255
    args[i].alloc_size = PER_THREAD;

    int rc = pthread_create(&tids[i], NULL, worker, &args[i]);
    if (rc != 0) {
      report("pthread_create failed at i=%d (rc=%d)", i, rc);
      return 1;
    }
  }

  // join
  for (int i = 0; i < N_THREADS; ++i) {
    int rc = pthread_join(tids[i], NULL);
    if (rc != 0) {
      report("pthread_join failed at i=%d (rc=%d)", i, rc);
      return 1;
    }
  }

  // sanity: try near-full allocation after everyone freed
  void* big = mm.allocate_local(BUF_SIZE - 256);
  if (!big) {
    report("[FAIL] Could not allocate big block after test.");
    return 1;
  }
  mm.free_local(big, BUF_SIZE - 256);

  if (g_errors == 0) {
    report("[OK] No corruption detected across %d threads.", N_THREADS);
    return 0;
  } else {
    report("[WARN] Detected %d corruption event(s).", g_errors);
    return 2;
  }
}



#endif