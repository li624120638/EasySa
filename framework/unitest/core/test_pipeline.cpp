#include <gtest/gtest.h>
#include <glog/logging.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <ctime>
#include <future>
#include <list>
#include <memory>
#include <mutex>
#include <random>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "easysa_frame.hpp"
#include "easysa_pipeline.hpp"

namespace easysa {
	class MsgObserver : StreamMsgObserver {
	public:
		enum StopFlag { STOP_BY_EOS = 0, STOP_BY_ERROR};
		void Update(const StreamMsg& smsg) override {
			if (stop_) return;
			if (smsg.type == StreamMsgType::EOS_MSG) {
				LOG(INFO) << "[core_unitest]:" << "[observer] received EOS_MSG from stream_id" << smsg.stream_id;
				eos_stream_id_.insert(smsg.stream_id);
				if (eos_stream_id_.size() == stream_cnt_) {
					stop_.store(true);
					wakener_.set_value(STOP_BY_EOS);
				}
			} else if (smsg.type == StreamMsgType::ERROR_MSG) {
				LOG(INFO) << "[core_unitest]:" << "[observer] received ERROR_MSG";
				stop_.store(true);
				wakener_.set_value(STOP_BY_ERROR);
			}
		}

		StopFlag WaitForStop() {
			StopFlag stop_flag = wakener_.get_future().get();
			pipeline_->Stop();
			return stop_flag;
		}
	private:
		std::size_t stream_cnt_;
		std::shared_ptr<Pipeline> pipeline_ = nullptr;
		std::set<std::string> eos_stream_id_;
		std::atomic<bool> stop_{ false };
		std::promise<StopFlag> wakener_;
	};

	static constexpr int DataFramePtrKey = 0;
	struct DataFrame {
		int frame_id;
	};

	static const int __MIN_STREAM_CNT__ = 1;
	static const int __MAX_STREAM_CNT__ = 64;

	static const int __MIN_FRAME_CNT__ = 100;
	static const int __MAX_FRAME_CNT__ = 1200;

	static const std::vector<std::vector<std::list<int>>> g_neighbor_list = {
										/*
										* 0 --> 1 --> 2
										*/
										{{1}, {2}, {}},
										/*
										*  0 --> 1 --> 2
										*        |
										*          --> 3  
										*/
										{{1}, {2, 3}, {}, {}},
										/*
										* 0 --> 1 --> 2
										*       | 
                                        *		  --> 3	--> 4
										*             |
										*               --> 5
										*/
										{ {1}, {2, 3}, {}, {4, 5}, {}, {}},
										/*
										* 0 --> 1 --> 2
										*       |
										*		  --> 3	--> 4 -- |
										*             |          | --> 6 
										*               --> 5 -- |
										*/
										{{1}, {2, 3}, {}, {4, 5}, {6}, {6},{}},
										/*
										* 0 --> 1 --> 2 ---------|
										*       |                | --> 6
										*		  --> 3	--> 4 -- |
										*             |          
										*               --> 5 
										*/
										{{1},{2, 3}, {6}, {4, 5}, {6}, {}, {}}
	                                     };
	class TestProcessor : public Module {
	public:
		explicit TestProcessor(const std::string& name, int chns) :Module(name) { cnts_.resize(chns); }
		bool Open(ModuleParamSet param_set) override {
			opened_ = true;
			return true;
		}
		void Close() override {
			opened_ = false;
		}
		bool Process(std::shared_ptr<FrameInfo> data) override {
			EXPECT_EQ(true, opened_);
			EXPECT_NE((uint32_t)1, FrameFlag::FRAME_FLAG_EOS & data->flags);
			uint32_t chn_idx = std::atoi(data->stream_id.c_str());  // data->channel_idx;  FIXME
			cnts_[chn_idx]++;
			return true;
		}
		std::vector<uint64_t> GetCnts() const { return cnts_; }
	private:
		bool opened_ = false;
		std::vector<uint64_t> cnts_;
		static std::atomic<int> id_;
	}; // class TestProcessor

	class TestProcessorFailure : public TestProcessor {
	public:
		explicit TestProcessorFailure(int chns, int failure_ret_num)
			: TestProcessor("TestProcessorFailure", chns), e_(time(NULL)), failure_ret_num_(failure_ret_num) {
			std::uniform_int_distribution<> failure_frame_randomer(0, __MIN_FRAME_CNT__ - 1);
			std::uniform_int_distribution<> failure_chn_randomer(0, chns - 1);
			failure_chn_ = failure_chn_randomer(e_);
			failure_frame_ = failure_frame_randomer(e_);
		}
		bool Process(std::shared_ptr<FrameInfo> data) override {
			uint32_t chn_idx = std::atoi(data->stream_id.c_str());  // data->channel_idx;  FIXMEi
			std::shared_ptr<DataFrame> frame = easysa::any_cast<std::shared_ptr<DataFrame>>(data->datas[0]);
			int64_t frame_idx = frame->frame_id;
			if (static_cast<int>(chn_idx) == failure_chn_ && frame_idx == failure_frame_) {
				return failure_ret_num_;
			}
			TestProcessor::Process(data);
			return true;
		}
		void SetFailureFrameIdx(int idx) { failure_frame_ = idx; }

	private:
		std::default_random_engine e_;
		int failure_chn_ = -1;
		int failure_frame_ = -1;
		int failure_ret_num_ = -1;
	};  // class TestProcessorFailure

	class TestProvider : public TestProcessor {
	public:
		explicit TestProvider(int chns, Pipeline* pipeline) : TestProcessor("TestProvider", chns), pipeline_(pipeline) {
			EXPECT_TRUE(nullptr != pipeline);
			EXPECT_GT(chns, 0);
			std::default_random_engine e(time(NULL));
			std::uniform_int_distribution<> randomer(__MIN_FRAME_CNT__, __MAX_FRAME_CNT__);
			frame_cnts_.clear();
			for (int i = 0; i < chns; ++i) {
				frame_cnts_.push_back(randomer(e));
			}
		}

		void StartSendData() {
			threads_.clear();
			for (size_t i = 0; i < frame_cnts_.size(); ++i) {
				threads_.push_back(std::thread(&TestProvider::ThreadFunc, this, i));
			}
		}

		void StopSendData() {
			for (auto& it : threads_) {
				if (it.joinable()) {
					it.join();
				}
			}
		}

		std::vector<uint64_t> GetFrameCnts() const { return frame_cnts_; }

	private:
		void ThreadFunc(int chn_idx) {
			int64_t frame_idx = 0;
			uint64_t frame_cnt = frame_cnts_[chn_idx];
			while (frame_cnt--) {
				auto data = FrameInfo::Create(std::to_string(chn_idx));
				data->SetStreamIndex(chn_idx);
				auto frame = std::make_shared<DataFrame>();
				frame->frame_id = frame_idx++;
				data->datas[DataFramePtrKey] = frame;
				if (!pipeline_->ProvideData(this, data)) {
					return;
				}
				if (frame_cnt == 0) {
					data = FrameInfo::Create(std::to_string(chn_idx), true);
					data->SetStreamIndex(chn_idx);
					pipeline_->ProvideData(this, data);
					LOG(INFO) << "[core_unitest]:" << "Send EOS:" << chn_idx << " frame id :" << frame_idx;
				}
			}
		}
		std::vector<std::thread> threads_;
		std::vector<uint64_t> frame_cnts_;
		Pipeline* pipeline_ = nullptr;
	};  // class TestProvider
} // namespace easysa