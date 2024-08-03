#ifndef NEXUSDB_CUSTOM_ALLOCATOR_H
#define NEXUSDB_CUSTOM_ALLOCATOR_H

#include <cstddef>
#include <new>
#include <limits>

namespace nexusdb {

template <typename T>
class CustomAllocator {
public:
    using value_type = T;

    CustomAllocator() noexcept;
    template <class U> CustomAllocator(const CustomAllocator<U>&) noexcept;

    T* allocate(std::size_t n);
    void deallocate(T* p, std::size_t n) noexcept;

    // Other member functions as required by C++ named requirements: Allocator
};

} // namespace nexusdb

#endif // NEXUSDB_CUSTOM_ALLOCATOR_H