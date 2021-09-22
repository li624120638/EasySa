#ifndef COMPONENTS_MODEL_LOADER_MEMORY_QUEUE_HPP__
#define COMPONENTS_MODEL_LOADER_MEMORY_QUEUE_HPP__

#include <condition_variable>
#include <mutex>
#include <thread>
#include <queue>
#include <chrono>

namespace easysa {
	namespace components {
template <typename T>
class MemorySafeQueue {
	public:
		MemorySafeQueue() = default;
		MemorySafeQueue(const MemorySafeQueue& queue) = delete;
		MemorySafeQueue& operator = (const MemorySafeQueue& queue) = delete;

		bool TryPop(T& value);
		void WaitAndPop(T& value);
		bool WaitAndTryPop(T& value, const std::chrono::microseconds rel_time);
		void Push(T& new_value);

		bool Empty() {
			std::lock_guard<std::mutex> lock(mtx_);
			return queue_.empty();
		}
		std::size_t Size() {
			std::lock_guard<std::mutex> lock(mtx_);
			return queue_.size();
		}
	private:
		mutable std::mutex mtx_;
		std::queue<T> queue_;
		std::condition_variable not_empty_cond_;
	}; // class ThreadSafeQueue

	template <typename T>
	bool MemorySafeQueue<T>::TryPop(T& value) {
		std::unique_lock<std::mutex> lock(mtx_);
		if (queue_.empty()) return false;
		value = queue_.front();
		queue_.pop();
		return true;
	}

	template <typename T>
	void MemorySafeQueue<T>::WaitAndPop(T& value) {
		std::lock_guard<std::mutex> guard(mtx_);
		not_empty_cond_.wait(lock, [&]() {return !queue_.empty(); });
		value = queue_.front();
		queue_.pop();
	}

	template <typename T>
	bool MemorySafeQueue<T>::WaitAndTryPop(T& value, const std::chrono::microseconds rel_time) {
		std::unique_lock<std::mutex> lock(mtx_);
		if (not_empty_cond_.wait_for(lock, rel_time, [&] { return !queue_.empty(); })) {
			value = queue_.front();
			queue_.pop();
			return true;
		}
		return false;
	}

	template <typename T>
	void MemorySafeQueue<T>::Push(T& new_value) {
		std::unique_lock<std::mutex> lk(mtx_);
		queue_.push(new_value);
		not_empty_cond_.notify_one();
		lk.unlock();
	}
	} // namespace components
} // namespace easysa
#endif // !COMPONENTS_MODEL_LOADER_MEMORY_QUEUE_HPP__
