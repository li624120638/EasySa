/*************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *************************************************************************/

#ifndef FRAMEWORK_CORE_INCLUDE_UTIL_EASYSA_SPINLOCK_HPP_
#define FRAMEWORK_CORE_INCLUDE_UTIL_EASYSA_SPINLOCK_HPP_
#define NOMINMAX
#include <windows.h>
#include <atomic>

namespace easysa {
	class SpinLock {
	   public:
		void Lock() {
			while (lock_.test_and_set(std::memory_order_acquire)){}
		}
		void Unlock() { lock_.clear(std::memory_order_release); }
	private:
		std::atomic_flag lock_ = ATOMIC_FLAG_INIT;
	}; // class SpinLock

	class SpinLockGuard {
	public:
		explicit SpinLockGuard(SpinLock& lk) :lock_(lk) { lock_.Lock(); }
		~SpinLockGuard() { lock_.Unlock(); }
	private:
		SpinLock& lock_;
	}; // class SpinLockGuard
} // namespace easysa
#endif // FRAMEWORK_CORE_INCLUDE_UTIL_EASYSA_RWLOCK_HPP_
