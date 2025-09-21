#if 0
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
  // unsigned delay_ms = 1000u + ((unsigned)tidb * 37u) % 2000u;
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
   enum { BUF_SIZE = 300 * 1024 };
  static uint8_t backing[BUF_SIZE];
  LocalMemoryManager mm(backing, (size_t)BUF_SIZE);

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