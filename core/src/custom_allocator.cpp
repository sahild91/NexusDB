// File: src/custom_allocator.cpp
#include "nexusdb/custom_allocator.h"
#include <cstdlib>
#include <new>

namespace nexusdb {

template <typename T>
CustomAllocator<T>::CustomAllocator() noexcept {}

template <typename T>
template <class U>
CustomAllocator<T>::CustomAllocator(const CustomAllocator<U>&) noexcept {}

template <typename T>
T* CustomAllocator<T>::allocate(std::size_t n) {
    if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
        throw std::bad_alloc();

    if (auto p = static_cast<T*>(std::malloc(n * sizeof(T)))) {
        return p;
    }

    throw std::bad_alloc();
}

template <typename T>
void CustomAllocator<T>::deallocate(T* p, std::size_t) noexcept {
    std::free(p);
}

// Explicit instantiation for common types
template class CustomAllocator<char>;
template class CustomAllocator<int>;
template class CustomAllocator<double>;

} // namespace nexusdb