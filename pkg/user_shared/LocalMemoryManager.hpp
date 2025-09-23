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
        usleep(100);
  }

  // Allocate a sub-buffer of at least `size` bytes; returns nullptr on failure.
  void* allocate_local(std::size_t size)
  {
    if (is_ready.load() == false)
    {
        std::printf("LocalMemoryManager is not ready\n");
        return nullptr;
    }
    //else
    //  std::printf("LocalMemoryManager is ready :) \n");
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

