/*************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *************************************************************************/
/*
* @brief thead safe queue
* @author cyssmile
* https://www.cnblogs.com/cyssmile/p/14574440.html
*/
#ifndef FRAMEWORK_CORE_INCLUDE_UTIL_EASYSA_QUEUE_HPP_
#define FRAMEWORK_CORE_INCLUDE_UTIL_EASYSA_QUEUE_HPP_

#include <condition_variable>
#include <mutex>
#include <thread>
#include <queue>
#include <chrono>

namespace easysa {

template <typename T>
class ThreadSafeQueue {
public:
	ThreadSafeQueue() = default;
	ThreadSafeQueue(const ThreadSafeQueue& queue) = delete;
	ThreadSafeQueue& operator = (const ThreadSafeQueue& queue) = delete;

	bool TryPop(T& value);
	void WaitAndPop(T& value);
	bool WaitAndTryPop(T& value, const std::chrono::microseconds rel_time);
	void Push(const T& new_value);

	bool Empty() {
		std::lock_guard<std::mutex> lock(mtx_);
		return queue_.empty();
	}
	std::size_t Size() {
		std::lock_guard<std::shared_lock> lock(mtx_);
		return queue_.size();
	}
private:
	mutable std::mutex mtx_;
	std::queue<T> queue_;
	std::condition_variable not_empty_cond_;
}; // class ThreadSafeQueue

template <typename T>
bool ThreadSafeQueue<T>::TryPop(T& value) {
	std::unique_lock<std::mutex> lock(mtx_);
	if (queue_.empty()) return false; 
	value = queue_.front();
	queue_.pop();
	return true;
}

template <typename T>
void ThreadSafeQueue<T>::WaitAndPop(T& value) {
	std::unique_lock<std::mutex> lock(mtx_);
	not_empty_cond_.wait(lock, [&]() {return !queue_.empty(); });
	value = queue_.front();
	queue_.pop();
}

template <typename T>
bool ThreadSafeQueue<T>::WaitAndTryPop(T& value, const std::chrono::microseconds rel_time) {
	std::unique_lock<std::mutex> lock(mtx_);
	if (not_empty_cond_.wait_for(lock, rel_time, [&] { return queue_.empty(); })) {
		value = queue_.front();
		queue_.pop();
		return true;
	}
	return false;
}

template <typename T>
void ThreadSafeQueue<T>::Push(const T& new_value) {
	std::unique_lock<std::mutex> lock(mtx_);
	queue_.push(new_value);
	lock.unlock();
	not_empty_cond_.notify_one();
}
} // namespace easysa

#endif FRAMEWORK_CORE_INCLUDE_UTIL_EASYSA_QUEUE_HPP_
