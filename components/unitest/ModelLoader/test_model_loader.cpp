#include <glog/logging.h>
#include <gtest/gtest.h>
#include <chrono>
#include <memory>
#include <thread>
#include "thread_pool.hpp"
using namespace easysa::components;
static int test_num = 0;
void PrintValue(void* value) {
	int val = *((int*)value);
	test_num++;
	//LOG(INFO) << "[Threadpool]" << val;
}
TEST(COMPONENTS, ThreadPool) {
	ThreadPool pool(2, 10, 20);
	size_t task_num = 20;
	EXPECT_TRUE(pool.Init());
	for (size_t i = 0; i < task_num; ++i) {
		std::shared_ptr<Task> task =
			std::make_shared<Task>();
		task->function = PrintValue;
		task->arg = &task_num;
		pool.AddTaskToQueue(task);
	}
	//std::this_thread::sleep_for(std::chrono::seconds(20));
}