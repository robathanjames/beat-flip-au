#include "AllocationGuard.h"

#include <atomic>
#include <cstdlib>
#include <new>

namespace
{
std::atomic<bool> tracking { false };
std::atomic<int> count { 0 };
}

// Separate translation unit keeps this allocation probe out of the test/DSP optimizer.
void* operator new (std::size_t size)
{
    if (tracking.load()) ++count;
    if (auto* memory = std::malloc (size == 0 ? 1 : size)) return memory;
    throw std::bad_alloc();
}
void* operator new[] (std::size_t size) { return ::operator new (size); }
void operator delete (void* memory) noexcept { std::free (memory); }
void operator delete[] (void* memory) noexcept { std::free (memory); }
void operator delete (void* memory, std::size_t) noexcept { std::free (memory); }
void operator delete[] (void* memory, std::size_t) noexcept { std::free (memory); }

namespace allocationGuard
{
void start() noexcept { count.store (0); tracking.store (true); }
int stop() noexcept { tracking.store (false); return count.load(); }
}
