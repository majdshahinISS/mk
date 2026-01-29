#include "LocalMemoryManager.h"

LocalMemoryManager::LocalMemoryManager() {}

LocalMemoryManager::LocalMemoryManager(void *buffer_base, l4_size_t buffer_size)
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

int LocalMemoryManager::init(void *buffer_base, l4_size_t buffer_size)
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

LocalMemoryManager::~LocalMemoryManager() = default;

void* LocalMemoryManager::allocate_local(l4_size_t size) {
  std::scoped_lock lk(mtx_);
  return allocate_local_locked(size);
}

void LocalMemoryManager::free_local(void *addr) {
  std::scoped_lock lk(mtx_);
  free_local_locked(addr);
}

void* LocalMemoryManager::allocate_local_wait(l4_size_t size) {
  std::unique_lock<std::mutex> lk(mtx_);
  for (;;) {
    if (void* p = allocate_local_locked(size))
      return p;
    cv_.wait(lk);
  }
}

LocalMemoryManager::Node* LocalMemoryManager::get_node_from_pool_locked() {
  if (!pool_head_) return nullptr;
  Node* n = pool_head_;
  pool_head_ = n->next;
  n->prev = n->next = nullptr;
  n->off = n->len = 0;
  return n;
}

void LocalMemoryManager::recycle_node_locked(Node* n) {
  if (!n) return;
  n->prev = nullptr;
  n->next = pool_head_;
  pool_head_ = n;
}

void LocalMemoryManager::insert_and_coalesce_locked(l4_size_t off, l4_size_t len) {
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

LocalMemoryManager::Node* LocalMemoryManager::find_fit_locked(l4_size_t need) {
  for (Node* n = free_head_; n; n = n->next) {
    if (n->len >= need) return n;
  }
  return nullptr;
}

void LocalMemoryManager::unlink_locked(Node* n) {
  if (!n) return;
  if (n->prev) n->prev->next = n->next; else free_head_ = n->next;
  if (n->next) n->next->prev = n->prev;
  n->prev = n->next = nullptr;
}

void* LocalMemoryManager::allocate_local_locked(l4_size_t req) {
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

void LocalMemoryManager::free_local_locked(void* user) {
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
void LocalMemoryManager::verify_invariants_locked() {
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
