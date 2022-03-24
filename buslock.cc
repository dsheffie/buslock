#include <cstdio>
#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <thread>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>

#define PROT (PROT_READ | PROT_WRITE)
#define MAP (MAP_ANONYMOUS|MAP_PRIVATE|MAP_POPULATE)

static const int iters = 1<<16;

using namespace std;
static int thr_online;
static int *lock = nullptr;

static uint64_t *cpu_cycles = nullptr;

static inline double timestamp() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return tv.tv_sec + static_cast<double>(tv.tv_usec)*1e-6;
}

static inline uint64_t read_counter() {
  uint32_t hi=0, lo=0;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return static_cast<uint64_t>(lo)|(static_cast<uint64_t>(hi)<<32);
}

void thr_func(int tid) {
  int *ptr = lock;
  uint64_t start;
  //spin barrier
  __sync_fetch_and_sub(&thr_online, 1);
  while(*reinterpret_cast<volatile int*>(&thr_online)) {
    __builtin_ia32_pause();
  }
  start = read_counter();
  for(int i = 0; i < iters; i+=16) {
     __sync_fetch_and_add(ptr, 1);
     __sync_fetch_and_add(ptr, 1);
     __sync_fetch_and_add(ptr, 1);
     __sync_fetch_and_add(ptr, 1);
     __sync_fetch_and_add(ptr, 1);
     __sync_fetch_and_add(ptr, 1);
     __sync_fetch_and_add(ptr, 1);
     __sync_fetch_and_add(ptr, 1);     
     __sync_fetch_and_add(ptr, 1);
     __sync_fetch_and_add(ptr, 1);
     __sync_fetch_and_add(ptr, 1);
     __sync_fetch_and_add(ptr, 1);
     __sync_fetch_and_add(ptr, 1);
     __sync_fetch_and_add(ptr, 1);
     __sync_fetch_and_add(ptr, 1);
     __sync_fetch_and_add(ptr, 1);     
  }
  cpu_cycles[tid] = (read_counter() - start)/iters;
}

int main() {
  vector<thread> threads;
  int max_thr = sysconf(_SC_NPROCESSORS_ONLN);
  int cl_len = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
  cpu_cycles = new uint64_t[max_thr];
  
  void *ptr = mmap(nullptr, 4096, PROT, MAP, -1, 0);

  std::cout << "OS believes cacheline length is " << cl_len << " bytes\n";

  for(int n_thr = 1; n_thr <= max_thr; ++n_thr) {
    std::cout << "running with " << n_thr << " threads\n";
    for(int disp = cl_len-sizeof(int); disp < cl_len+sizeof(int); ++disp) {
      lock = reinterpret_cast<int*>(reinterpret_cast<char*>(ptr) + disp);
      *lock = n_thr+1;
      thr_online = n_thr;
      for(int thr = 0; thr < n_thr; ++thr) {
	threads.push_back(thread(thr_func, thr));
      }
      
      for(thread &thr : threads) {
	if(thr.joinable()) {
	  thr.join();
	}
      }
      std::cout << "disp = " << disp << ", cycles = " << cpu_cycles[0] << "\n";
      threads.clear();
    }
  }
  munmap(ptr, 4096);
  delete [] cpu_cycles;
  return 0;
}
