// LocalMemoryManager.h
#pragma once
#include <l4/sys/types.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <cassert>

// A simple thread-safe freelist allocator over a user-provided buffer.
// Key properties:
//  - Single global mutex guards ALL metadata.
//  - Allocation writes a small header before the returned pointer to record size.
//  - Free reads size from header (caller-size overload kept for compatibility).
//  - Free-list kept sorted by offset; coalesces with both neighbors.
//  - No dynamic heap allocations for metadata (fixed node pool).
//
// Public API:
//   void* allocate_local(l4_size_t size);
//   void  free_local(void* addr);                     // preferred
//   void  free_local(void* addr, l4_size_t size);   // compatibility (asserts)
//
// Debug builds enforce invariants after each operation.
class LocalMemoryManager
{
public:
  // Compile-time knobs
  static constexpr l4_size_t kAlign    = 16;     // 16-byte alignment
  static constexpr l4_size_t kMaxNodes = 256;    // max free-list fragments

  // Allocation header written immediately before the returned user pointer
  struct Header {
    u_int32_t size;   // aligned user payload size
    u_int32_t magic;  // guard for double-free detection
  };

  static constexpr u_int32_t kAllocMagic = 0xC0FFEE01u;

  // Utility
  static constexpr l4_size_t align_up(l4_size_t x, l4_size_t a) noexcept {
    return (x + (a - 1)) & ~(a - 1);
  }

  // Node for freelist; kept in an intrusive doubly-linked list sorted by offset.
  struct Node {
    l4_size_t off = 0;  // offset from base_
    l4_size_t len = 0;  // length in bytes
    Node* prev = nullptr;
    Node* next = nullptr;
  };

  LocalMemoryManager();
  // Construction: provide the memory arena [buffer_base, buffer_base + buffer_size)
  LocalMemoryManager(void *buffer_base, l4_size_t buffer_size);
  int init(void *buffer_base, l4_size_t buffer_size);
  ~LocalMemoryManager();

  // Prevent accidental copies/moves (each copy would have its own mutex!)
  LocalMemoryManager(const LocalMemoryManager&)            = delete;
  LocalMemoryManager& operator=(const LocalMemoryManager&) = delete;
  LocalMemoryManager(LocalMemoryManager&&)                 = delete;
  LocalMemoryManager& operator=(LocalMemoryManager&&)      = delete;

  // Preferred API: allocator determines size from header on free
  void* allocate_local(l4_size_t size);

  void free_local(void *addr);
/*
  // Compatibility overload: will assert the size matches header (debug) and ignore it
  void free_local(void *addr, l4_size_t caller_size);
*/
  // Optional: expose a blocking allocate that waits until memory is freed
  void* allocate_local_wait(l4_size_t size);

  std::atomic<bool> is_ready{false};

private:
  // ====== Locked helpers: REQUIRE mtx_ to be held ======

  Node* get_node_from_pool_locked();

  void recycle_node_locked(Node* n);

  // Insert [off,len] into free list (sorted) and coalesce with both neighbors
  void insert_and_coalesce_locked(l4_size_t off, l4_size_t len);

  // Find first-fit node with len >= need
  Node* find_fit_locked(l4_size_t need);

  // Unlink node from free list (does not recycle)
  void unlink_locked(Node* n);

  void* allocate_local_locked(l4_size_t req);

  void free_local_locked(void* user);

#ifndef NDEBUG
  void verify_invariants_locked();
#endif

private:
  std::mutex mtx_;
  std::condition_variable cv_;

  // State
  u_int8_t *base_ = nullptr;
  l4_size_t   size_ = 0;

  // Free-list bookkeeping (no heap)
  Node  node_pool_[kMaxNodes]{};
  Node *pool_head_ = nullptr;   // free nodes for bookkeeping
  Node *free_head_ = nullptr;   // free chunks sorted by offset
};
