/*************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *************************************************************************/

#ifndef FRAMEWORK_CORE_INCLUDE_UTIL_EASYSA_RWLOCK_HPP_
#define FRAMEWORK_CORE_INCLUDE_UTIL_EASYSA_RWLOCK_HPP_

/*
* there use shared_mutex to realize read and write lock
*/

#include <thread>
#include <mutex>
#include <shared_mutex>
namespace easysa {
	/**
	* just a example for c++14 rwlock
	*/
	class RwExample {
	public:
		void Write() {
			std::unique_lock<std::shared_mutex> lock(shared_mtx_);
			// do your thing
		}
		void Read() {
			std::shared_lock<std::shared_mutex> lock(shared_mtx_);
			// do yout thing
		}
	private:
		mutable std::shared_mutex shared_mtx_;
	}; // class RwExample
} // namespace easysa

#endif // FRAMEWORK_CORE_INCLUDE_UTIL_EASYSA_RWLOCK_HPP_