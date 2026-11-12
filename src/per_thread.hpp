#pragma once
// INFO: Per-thread state behind one initial-exec pointer, instead of a large thread-local object.
// NOTE: The static TLS glibc keeps for dlopen fits a few pointers, not ~120 KB; an initial-exec access is one
//  instruction.

#include <cstdlib>
#include <new>

namespace whitaker {

#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
// NOTE: Without threads there is one thread; a plain object needs no pointer or check.
template <typename T> struct PerThread {
  // NOTE: A member, not a function's static: its destructor is registered at start-up, so get() has no guard.
  static inline constinit T object{};
  [[nodiscard, gnu::always_inline]] static T& get() { return object; }
};
#else
template <typename T> struct PerThread {
  [[gnu::tls_model("initial-exec")]] static inline thread_local T* pointer = nullptr;

  [[nodiscard, gnu::always_inline]] static T& get() {
    if (T* const p = pointer) [[likely]]
      return *p;
    return make();
  }

private:
  struct Owner {
    ~Owner() {
      if (pointer) {
        pointer->~T();
        ::operator delete(pointer, std::align_val_t{alignof(T)});
        pointer = nullptr;
      }
    }
  };
  [[gnu::noinline, gnu::cold]] static T& make() {
    void* const memory = ::operator new(sizeof(T), std::align_val_t{alignof(T)}, std::nothrow);
    // NOTE: As when the C++ runtime cannot allocate a thread's thread-local block.
    if (!memory)
      std::abort();
    pointer = new (memory) T{};
    // NOTE: Ordinary thread-local, touched once per thread: it frees the state when the thread ends.
    static thread_local Owner owner{};
    (void)owner;
    return *pointer;
  }
};
#endif

} // namespace whitaker
