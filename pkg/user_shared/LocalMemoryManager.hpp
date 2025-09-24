// LocalMemoryManager.hpp
#pragma once
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
  LocalMemoryManager(){}
  // Construction: provide the memory arena [buffer_base, buffer_base + buffer_size)
  LocalMemoryManager(void *buffer_base, l4_size_t buffer_size)
  : base_(static_cast<u_int8_t*>(buffer_base)), size_(buffer_size)
  {
    std::scoped_lock lk(mtx_);
    // Initialize node pool list
    pool_head_ = nullptr;
    for (l4_size_t i = 0; i < kMaxNodes; ++i) {
      node_pool_[i].prev = nullptr;
      node_pool_[i].next = pool_head_;
      pool_head_ = &node_pool_[i];
    }
    free_head_ = nullptr;

    // Seed a single free range
    insert_and_coalesce_locked(/*off*/0, /*len*/size_);
    is_ready.store(true, std::memory_order_release);
#ifndef NDEBUG
    verify_invariants_locked();
#endif
  }

  int init(void *buffer_base, l4_size_t buffer_size)
  {
    base_ = static_cast<u_int8_t*>(buffer_base);
    size_ = buffer_size;
    std::scoped_lock lk(mtx_);
    // Initialize node pool list
    pool_head_ = nullptr;
    for (l4_size_t i = 0; i < kMaxNodes; ++i) {
      node_pool_[i].prev = nullptr;
      node_pool_[i].next = pool_head_;
      pool_head_ = &node_pool_[i];
    }
    free_head_ = nullptr;

    // Seed a single free range
    insert_and_coalesce_locked(/*off*/0, /*len*/size_);
    is_ready.store(true, std::memory_order_release);
#ifndef NDEBUG
    verify_invariants_locked();
#endif
    return 0;
  }

  ~LocalMemoryManager() = default;

  // Prevent accidental copies/moves (each copy would have its own mutex!)
  LocalMemoryManager(const LocalMemoryManager&)            = delete;
  LocalMemoryManager& operator=(const LocalMemoryManager&) = delete;
  LocalMemoryManager(LocalMemoryManager&&)                 = delete;
  LocalMemoryManager& operator=(LocalMemoryManager&&)      = delete;

  // Preferred API: allocator determines size from header on free
  void* allocate_local(l4_size_t size) {
    std::scoped_lock lk(mtx_);
    return allocate_local_locked(size);
  }

  void free_local(void *addr) {
    std::scoped_lock lk(mtx_);
    free_local_locked(addr);
  }
/*
  // Compatibility overload: will assert the size matches header (debug) and ignore it
  void free_local(void *addr, l4_size_t caller_size) {
    (void)caller_size;
    std::scoped_lock lk(mtx_);
#ifndef NDEBUG
    if (addr) {
      auto *hdr = reinterpret_cast<Header*>(static_cast<u_int8_t*>(addr) - sizeof(Header));
      assert(hdr->magic == kAllocMagic && "free_local(addr, size): invalid magic (double free or bad pointer)");
      assert(hdr->size == align_up(caller_size, kAlign) && "free_local(addr, size): size mismatch");
    }
#endif
    free_local_locked(addr);
  }
*/
  // Optional: expose a blocking allocate that waits until memory is freed
  void* allocate_local_wait(l4_size_t size) {
    std::unique_lock<std::mutex> lk(mtx_);
    for (;;) {
      if (void* p = allocate_local_locked(size))
        return p;
      cv_.wait(lk);
    }
  }

  std::atomic<bool> is_ready{false};

private:
  // ====== Locked helpers: REQUIRE mtx_ to be held ======

  Node* get_node_from_pool_locked() {
    if (!pool_head_) return nullptr;
    Node* n = pool_head_;
    pool_head_ = n->next;
    n->prev = n->next = nullptr;
    n->off = n->len = 0;
    return n;
  }

  void recycle_node_locked(Node* n) {
    if (!n) return;
    n->prev = nullptr;
    n->next = pool_head_;
    pool_head_ = n;
  }

  // Insert [off,len] into free list (sorted) and coalesce with both neighbors
  void insert_and_coalesce_locked(l4_size_t off, l4_size_t len) {
    assert(off + len <= size_);

    // Find insertion position
    Node* cur = free_head_;
    Node* prev = nullptr;
    while (cur && cur->off < off) {
      prev = cur;
      cur = cur->next;
    }

    Node* n = get_node_from_pool_locked();
    if (!n) {
      assert(false && "Node pool exhausted; increase kMaxNodes");
      return;
    }
    n->off = off;
    n->len = len;

    // Link n between prev and cur
    n->prev = prev;
    n->next = cur;
    if (prev) prev->next = n; else free_head_ = n;
    if (cur)  cur->prev = n;

    // Coalesce with next
    if (n->next && (n->off + n->len == n->next->off)) {
      n->len += n->next->len;
      Node* del = n->next;
      n->next = del->next;
      if (del->next) del->next->prev = n;
      recycle_node_locked(del);
    }
    // Coalesce with prev
    if (n->prev && (n->prev->off + n->prev->len == n->off)) {
      n->prev->len += n->len;
      Node* del = n;
      n = n->prev; // merged into prev
      n->next = del->next;
      if (del->next) del->next->prev = n;
      recycle_node_locked(del);
    }
  }

  // Find first-fit node with len >= need
  Node* find_fit_locked(l4_size_t need) {
    for (Node* n = free_head_; n; n = n->next) {
      if (n->len >= need) return n;
    }
    return nullptr;
  }

  // Unlink node from free list (does not recycle)
  void unlink_locked(Node* n) {
    if (!n) return;
    if (n->prev) n->prev->next = n->next; else free_head_ = n->next;
    if (n->next) n->next->prev = n->prev;
    n->prev = n->next = nullptr;
  }

  void* allocate_local_locked(l4_size_t req) {
    if (req == 0) return nullptr;
    const l4_size_t ua   = align_up(req, kAlign);
    const l4_size_t need = ua + sizeof(Header);

    Node* n = find_fit_locked(need);
    if (!n) {
#ifndef NDEBUG
      verify_invariants_locked();
#endif
      return nullptr;
    }

    const l4_size_t off = n->off;
    if (n->len == need) {
      // exact fit: unlink and recycle node
      unlink_locked(n);
      recycle_node_locked(n);
    } else {
      // split in place: keep remainder in n
      n->off += need;
      n->len -= need;
    }

    // write header
    auto *hdr = reinterpret_cast<Header*>(base_ + off);
    hdr->size  = static_cast<u_int32_t>(ua);
    hdr->magic = kAllocMagic;

#ifndef NDEBUG
    verify_invariants_locked();
#endif
    return static_cast<void*>(reinterpret_cast<u_int8_t*>(hdr) + sizeof(Header));
  }

  void free_local_locked(void* user) {
    if (!user) return;
    auto *user_p = static_cast<u_int8_t*>(user);
    auto *hdr = reinterpret_cast<Header*>(user_p - sizeof(Header));

    // Basic guard checks
    assert(hdr->magic == kAllocMagic && "free_local: invalid magic (double free or bad pointer)");
    const l4_size_t ua = hdr->size;
    assert(ua == align_up(ua, kAlign) && "header size must be aligned");
    const l4_size_t off = reinterpret_cast<u_int8_t*>(hdr) - base_;
    assert(off < size_ && off + ua + sizeof(Header) <= size_ && "free_local: out of range");

    // poison header to help catch double frees
    hdr->magic = 0;

    insert_and_coalesce_locked(off, ua + sizeof(Header));

    // Wake one waiter if any
    cv_.notify_one();

#ifndef NDEBUG
    verify_invariants_locked();
#endif
  }

#ifndef NDEBUG
  void verify_invariants_locked() {
    // Check free list sortedness, non-overlap, bounds, and link symmetry
    l4_size_t last_end = 0;
    Node* prev = nullptr;
    for (Node* n = free_head_; n; n = n->next) {
      assert(n->prev == prev && "prev link mismatch");
      assert(!n->prev || n->prev->off < n->off);
      assert(last_end <= n->off && "overlapping free nodes");
      assert(n->off + n->len <= size_ && "free node out of arena bounds");
      last_end = n->off + n->len;
      prev = n;
    }
  }
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
