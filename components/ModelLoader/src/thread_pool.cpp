#include "thread_pool.hpp"
#include <glog/logging.h>
#include <chrono>

namespace easysa {
	namespace components {
ThreadPool::ThreadPool(uint32_t min_thread_num, uint32_t max_thread_num, uint32_t max_queue_num) {
	min_thread_num_ = min_thread_num;
	max_thread_num_ = max_thread_num;
	max_queue_num_ = max_queue_num;
}
bool ThreadPool::Init() {
	workors_.resize(max_thread_num_, nullptr);
	live_thread_num_ = 0;
	busy_thread_num_ = 0;
	exit_thread_num_ = 0;
	task_queue_size = 0;
	running_ = true;
	for (size_t i = 0; i < min_thread_num_; i++) {
		workors_[i] = new std::thread(&ThreadPool::Workor, this);
		if (workors_[i] == nullptr) return false;
	}
	manager_ = new std::thread(&ThreadPool::Manager, this);
	if (manager_ == nullptr) {
		return false;
	}
	return true;
}
void ThreadPool::Manager() {
	while (running_)
	{	// check status after 3 sec
		std::this_thread::sleep_for(std::chrono::seconds(3));
		/*
		* add thread
		* task_queue_size greater than live_thread_num_
		* && live_thread_num_< max_thread_num_
		*/
		int counter = 0;
		if (task_queue_size > live_thread_num_ && live_thread_num_ < max_thread_num_) {
			for (size_t i = 0; i < max_queue_num_ && counter < once_add_thread_num_ &&
				live_thread_num_ < max_thread_num_; ++i) {
				if (workors_[i] == nullptr) {
					workors_[i] = new std::thread(&ThreadPool::Workor, this);
					++counter;
					++live_thread_num_;
				}
			}
		}
		/*
		*  �����߳�
		* busy_thread_number * 2 < live_thread_num_ 
		* && live_thread_num_ > min_thread_num_
		*/
		if (busy_thread_num_ * 2 < live_thread_num_ &&
			live_thread_num_ > min_thread_num_) {
			// ���߳���ɱ����Ҫ����ɱ��
			exit_thread_num_ = once_add_thread_num_;
		}
	}
}
void ThreadPool::Workor() {
	while (true) {
		std::shared_ptr<Task> task = nullptr;
		while (running_ && task == nullptr)
		{
			task_queue_.WaitAndTryPop(task, (std::chrono::microseconds)1);
		}
		if (exit_thread_num_ > 0 && task == nullptr) {
			/*
			* ���ﲻ�ܻ��ߵ��߳��Ƿ�С��min_thread_num ��Ӧ����
			* exit_thread_num�𽥱�Ϊ0
			*/
			--exit_thread_num_;
			if (live_thread_num_ > min_thread_num_) {
				--live_thread_num_;
				ThreadExit();
			}
			break;
		}
		if (task == nullptr) {
			// ���������������������ʱ���Żᴥ��
			break;
		}
		//LOG(INFO) << "[Thread pool] excute function";
		task_queue_size -= 1;
		++busy_thread_num_;
		task->function(task->arg);
		--busy_thread_num_;
	}
}

void ThreadPool::ThreadExit() {
	std::thread::id id = std::this_thread::get_id();
	for (auto& it : workors_) {
		if (it != nullptr && it->get_id() == id) {
			it->detach();
			it = nullptr;
			break;
		}
	}
}

void ThreadPool::AddTaskToQueue(std::shared_ptr<Task> task) {
	while (running_ && task_queue_size == max_queue_num_)
	{ // ����������˵����
		// ����
		std::this_thread::sleep_for(std::chrono::microseconds(1));
	}
	if (!running_) {
		return;
	}
	task_queue_.Push(task);
	task_queue_size++;
	//LOG(INFO) << "[thread pool]: add task to pool ";
}

ThreadPool::~ThreadPool() {
	while (GetQueueSize() > 0) {
		std::this_thread::sleep_for(std::chrono::microseconds(5));
	}
	running_ = false;
	if (manager_) {
		if (manager_->joinable()) {
			manager_->join();
			manager_ = nullptr;
		}
	}
	for (auto& it : workors_) {
		if (it && it->joinable()) {
			it->join();
			it = nullptr;
		}
	}
}
	} // namespace components
} // namespace easysa
