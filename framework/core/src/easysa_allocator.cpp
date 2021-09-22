/*************************************************************************
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *************************************************************************/
#include "easysa_allocator.hpp"

#include <exception>
#include <memory>

namespace easysa {

    // helper funcs
    class AllocDeleter final {
    public:
        explicit AllocDeleter(std::shared_ptr<MemoryAllocator> allocator)
            : allocator_(allocator) {}

        void operator()(void* ptr) {
            allocator_->free(ptr);
        }
    private:
        std::shared_ptr<MemoryAllocator> allocator_;
    };

    std::shared_ptr<void> MemAlloc(size_t size, std::shared_ptr<MemoryAllocator> allocator) {
        if (allocator) {
            std::shared_ptr<void> ds(allocator->alloc(size), AllocDeleter(allocator));
            return ds;
        }
        return nullptr;
    }

    std::shared_ptr<void> CpuMemAlloc(size_t size) {
        std::shared_ptr<MemoryAllocator> allocator = std::make_shared<CpuAllocator>();
        return MemAlloc(size, allocator);
    }

    // cpu Var-size allocator
    void* CpuAllocator::alloc(size_t size, int timeout_ms) {
        size_t alloc_size = (size + 4095) / 4096 * 4096;
        timeout_ms = timeout_ms;  // disable warning
        return static_cast<void*> (new (std::nothrow) unsigned char[alloc_size]);
    }

    void CpuAllocator::free(void* p) {
        unsigned char* ptr = static_cast<unsigned char*>(p);
        delete[]ptr;
    };

}  // namespace easysa