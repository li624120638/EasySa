#ifndef COMPONENTS_MODELLOADER_HPP__
#define COMPONENTS_MODELLOADER_HPP__
#include "memory_queue.hpp"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <thread>
#include <functional>

namespace easysa {
	namespace components {

using Function = std::function<void(void*)>;
struct Task {
	Function function;
	void* arg;
};

class ThreadPool {
public:
	explicit ThreadPool(uint32_t min_thread_num = 1, uint32_t max_thread_num = 1, uint32_t max_queue_num = 10);
	/*
	* init thread pool, call this function after constructor
	*/
	bool Init();
	/*
	* add Task to queue
	*/
	void AddTaskToQueue(std::shared_ptr<Task>);
	virtual ~ThreadPool();
	/*
	*  Get how many thread in working
	*/
	uint32_t GetBusyNum() { return busy_thread_num_; }
	/*
	* Get how may thread live
	*/
	uint32_t GetLiveNum() { return live_thread_num_; }

	uint32_t GetQueueSize() { return task_queue_size; }
	/*
	* manage works
	*/
	void Manager();
private:
	/*
	* workor
	*/
	void Workor();
	void ThreadExit();
private:
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool(ThreadPool&&) = delete;
	ThreadPool& operator = (const ThreadPool&) = delete;
	ThreadPool& operator = (ThreadPool&&) = delete;
private:
	uint32_t max_thread_num_{ 0 };
	uint32_t min_thread_num_{ 0 };
	uint32_t max_queue_num_{ 10 };
	std::atomic<uint32_t> busy_thread_num_{ 0 };
	std::atomic<uint32_t> live_thread_num_{ 0 };
	std::atomic<uint32_t> exit_thread_num_{ 0 };
	uint32_t once_add_thread_num_ = 2;
private:
	std::atomic<uint32_t> task_queue_size{0};
	MemorySafeQueue<std::shared_ptr<Task>> task_queue_;
private:
	std::atomic<bool> running_{ false };
	//std::mutex thread_pool_mtx_;
	std::thread* manager_ = nullptr;
	std::vector<std::thread*> workors_;
};
	} // namespace components
} // namespace easysa
#endif // !COMPONENTS_MODELLOADER_HPP__
