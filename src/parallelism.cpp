// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#include <ket/parallelism.hpp>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "parallel.hpp"

namespace ket {
namespace {

using Body = std::function<void(std::size_t, std::size_t)>;

// Below this many amplitudes the barrier costs more than the work, so we run
// the gate serially regardless of the thread count.
constexpr std::size_t kParallelThreshold = std::size_t{1} << 14;

// A persistent barrier-style pool: the caller acts as thread 0 and the workers
// take the remaining chunks; one notify + one wait synchronizes each dispatch.
class ThreadPool {
 public:
  explicit ThreadPool(unsigned threads) : n_(threads < 1 ? 1 : threads) {
    for (unsigned i = 1; i < n_; ++i) {
      workers_.emplace_back([this, i] { worker(i); });
    }
  }

  ~ThreadPool() {
    {
      std::lock_guard<std::mutex> lk(m_);
      stop_ = true;
      ++gen_;
    }
    cv_.notify_all();
    for (std::thread& t : workers_) {
      if (t.joinable()) t.join();
    }
  }

  unsigned size() const { return n_; }

  void run(std::size_t count, const Body& body) {
    if (n_ == 1) {
      if (count != 0) body(0, count);
      return;
    }
    {
      std::lock_guard<std::mutex> lk(m_);
      body_ = &body;
      count_ = count;
      remaining_ = n_ - 1;
      ++gen_;
    }
    cv_.notify_all();
    run_chunk(0);  // the caller computes its own share
    {
      std::unique_lock<std::mutex> lk(m_);
      done_.wait(lk, [this] { return remaining_ == 0; });
    }
    body_ = nullptr;
  }

 private:
  void run_chunk(unsigned idx) {
    const std::size_t chunk = (count_ + n_ - 1) / n_;
    const std::size_t begin = chunk * idx;
    if (begin >= count_) return;
    const std::size_t end = std::min(begin + chunk, count_);
    (*body_)(begin, end);
  }

  void worker(unsigned idx) {
    std::size_t seen = 0;
    for (;;) {
      std::unique_lock<std::mutex> lk(m_);
      cv_.wait(lk, [this, &seen] { return gen_ != seen || stop_; });
      if (stop_) return;
      seen = gen_;
      lk.unlock();
      run_chunk(idx);
      lk.lock();
      if (--remaining_ == 0) {
        lk.unlock();
        done_.notify_one();
      }
    }
  }

  unsigned n_;
  std::vector<std::thread> workers_;
  std::mutex m_;
  std::condition_variable cv_;
  std::condition_variable done_;
  const Body* body_ = nullptr;
  std::size_t count_ = 0;
  std::size_t gen_ = 0;
  unsigned remaining_ = 0;
  bool stop_ = false;
};

unsigned hardware_threads() {
  const unsigned hc = std::thread::hardware_concurrency();
  return hc != 0 ? hc : 1;
}

// KET_NUM_THREADS as a positive count, or 0 if unset/invalid. Uses _dupenv_s on
// MSVC, where std::getenv raises a deprecation warning our -Werror build
// rejects.
unsigned env_num_threads() {
#ifdef _MSC_VER
  char* buf = nullptr;
  std::size_t len = 0;
  unsigned result = 0;
  if (_dupenv_s(&buf, &len, "KET_NUM_THREADS") == 0 && buf != nullptr) {
    const int v = std::atoi(buf);
    if (v > 0) result = static_cast<unsigned>(v);
  }
  std::free(buf);  // free(nullptr) is a no-op
  return result;
#else
  if (const char* env = std::getenv("KET_NUM_THREADS")) {
    const int v = std::atoi(env);
    if (v > 0) return static_cast<unsigned>(v);
  }
  return 0;
#endif
}

// 0 means "not yet resolved"; resolved lazily from KET_NUM_THREADS or to 1.
std::atomic<unsigned> g_requested{0};
std::unique_ptr<ThreadPool> g_pool;

unsigned effective_threads() {
  unsigned r = g_requested.load();
  if (r == 0) {
    const unsigned env = env_num_threads();
    r = env != 0 ? env : 1;  // serial, deterministic default
    g_requested.store(r);
  }
  return r;
}

// The shared pool, rebuilt when the requested thread count changes. Not safe to
// reconfigure while a simulation is running on another thread.
ThreadPool& pool() {
  const unsigned want = effective_threads();
  if (!g_pool || g_pool->size() != want) {
    g_pool = std::make_unique<ThreadPool>(want);
  }
  return *g_pool;
}

}  // namespace

void set_num_threads(unsigned threads) {
  if (threads == 0) threads = hardware_threads();
  g_requested.store(threads);
}

unsigned num_threads() { return effective_threads(); }

namespace internal {

void parallel_for(std::size_t count, const Body& body) {
  ThreadPool& p = pool();
  if (p.size() == 1 || count < kParallelThreshold) {
    if (count != 0) body(0, count);
    return;
  }
  p.run(count, body);
}

}  // namespace internal

}  // namespace ket
