/*************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *************************************************************************/

#ifndef FRAMEWORK_CORE_INCLUDE_EASYSA_ALLOCATOR_HPP_
#define FRAMEWORK_CORE_INCLUDE_EASYSA_ALLOCATOR_HPP_

#include <atomic>
#include <memory>
#include <new>
#include "easysa_common.hpp"
#include "util/easysa_queue.hpp"

 /**
  *  @file easysa_allocator.hpp
  *
  *  This file contains a declaration of  memory allocator.
  */
namespace easysa {

	class MemoryAllocator : public NonCopyable {
	public:
		explicit MemoryAllocator(int device_id) : device_id_(device_id) {}
		virtual ~MemoryAllocator() = default;
		virtual void* alloc(size_t size, int timeout_ms = 0) = 0;
		virtual void free(void* p) = 0;
		int device_id() const { return device_id_; }
		void set_device_id(int device_id) { device_id_ = device_id; }
	protected:
		int device_id_ = -1;
		std::mutex mutex_;
	};

	class CpuAllocator : public MemoryAllocator {
	public:
		CpuAllocator() : MemoryAllocator(-1) {}
		~CpuAllocator() = default;

		void* alloc(size_t size, int timeout_ms = 0) override;
		void free(void* p) override;
	};

	// helper functions
	std::shared_ptr<void> MemAlloc(size_t size, std::shared_ptr<MemoryAllocator> allocator);
	std::shared_ptr<void> CpuMemAlloc(size_t size);

}  // namespace easysa

#endif // FRAMEWORK_CORE_INCLUDE_EASYSA_ALLOCATOR_HPP_