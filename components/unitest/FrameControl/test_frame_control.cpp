#include <gtest/gtest.h>
#include <glog/logging.h>
#include "frame_control.hpp"
#include <iostream>
#include <thread>
#include <chrono>
using namespace easysa::components;

TEST(COMPONENTS, FrameControl) {
/*
* test frame control component
*/
	std::chrono::time_point<std::chrono::steady_clock> start, end;
	start = std::chrono::steady_clock::now();
	FrameController fc(20);
	fc.Start();
	fc.SetFrameRate(30);
	EXPECT_EQ(fc.GetFrameRate(), 30);
	std::this_thread::sleep_for(std::chrono::microseconds(5));
	fc.Control();
	std::this_thread::sleep_for(std::chrono::microseconds(10));
	fc.Control();
	end = std::chrono::steady_clock::now();
	std::chrono::duration<double, std::milli> dur = start - end;
	LOG(INFO) << dur.count();
}