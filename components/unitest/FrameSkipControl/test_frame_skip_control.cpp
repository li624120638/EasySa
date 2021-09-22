#include <gtest/gtest.h>
#include "frame_skip_control.hpp"
#include <iostream>
using namespace easysa::components;

TEST(COMPONENTS, FrameSkip) {
	constexpr int N = 5;
	std::vector<std::string> modules_name = {
		"source", "detection", "fer", "etvh", "osd"
	};
	/*
	* source dection and osd not skip frame,
	* fer skip one frame after deal two frame
	* etvh skip one frame after deal three frame
	*/ 
	std::vector<uint32_t> modules_skip = {
		0, 0, 2, 3, 0
	};
	FrameSkipControl<N> fsc;
	std::vector<std::pair<std::string, uint32_t>> cfg;
	cfg.resize(N);
	cfg.resize(N);
	for (int i = 0; i < N; ++i) {
		cfg[i] = std::make_pair(modules_name[i], modules_skip[i]);
	}
	EXPECT_TRUE(fsc.Init(cfg));
	for (size_t i = 0; i < 10; ++i) {
		auto bs = fsc.GetSkipBitSetNow();
		//std::cout << bs << std::endl;
		// LOG(INFO) << bs;
	}
	for (size_t i = 0; i < 10; ++i) {
		auto vec = fsc.GetSkipVectorNow();
		//std::cout << bs << std::endl;
		for (auto& it : vec) {
			// LOG(INFO) << it;
		}
	}
}