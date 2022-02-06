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

double timestamp() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return tv.tv_sec + static_cast<double>(tv.tv_usec)*1e-6;
}

void thr_func(int tid) {
  bool success = false;
  //spin barrier
  __sync_fetch_and_sub(&thr_online, 1);
  while(*reinterpret_cast<volatile int*>(&thr_online)) {
    __builtin_ia32_pause();
  }
  for(int i = 0; i < iters; i++) {
    do {
      int old = *lock;
      success = __sync_bool_compare_and_swap(lock, old, tid);
    }
    while(!success);
    __builtin_ia32_pause();
  }
}

int main() {
  vector<thread> threads;
  int n_thr = sysconf(_SC_NPROCESSORS_ONLN);
  int cl_len = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
  
  void *ptr = mmap(nullptr, 4096, PROT, MAP, -1, 0);

  std::cout << "OS believes cacheline length is " << cl_len << " bytes\n";
  
  for(int disp = cl_len-sizeof(int); disp < cl_len+sizeof(int); ++disp) {
    lock = reinterpret_cast<int*>(reinterpret_cast<char*>(ptr) + disp);
    *lock = n_thr+1;
    thr_online = n_thr;
    double now = timestamp();
    for(int thr = 0; thr < n_thr; ++thr) {
      threads.push_back(thread(thr_func, thr));
    }
    
    for(thread &thr : threads) {
      if(thr.joinable()) {
	thr.join();
      }
    }
    now = timestamp() - now;
    std::cout << "disp = " << disp << ", now = " << now << "\n";
    threads.clear();
  }
  munmap(ptr, 4096);
  return 0;
}
