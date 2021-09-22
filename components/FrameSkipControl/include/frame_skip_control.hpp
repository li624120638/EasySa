#ifndef COMPONENTS_FRAME_SKIP_CONTROL_HPP_
#define COMPONENTS_FRAME_SKIP_CONTROL_HPP_
#include <glog/logging.h>
#include <atomic>
#include <vector>
#include <bitset>
/*
*
*	@brief this file for create one bitset for
*   finger whether skip module when Date frame flow in module
*	template  int N, represent how long bitset will be created
*	after call GetSkipBitSetNow();
*/
namespace easysa {
	namespace components {
		template<int N>
		class FrameSkipControl {
		public:
			FrameSkipControl() {
				running_ = true;
			}
			/*
			* param[in] cfg have some pair<name, C> which contains the module name and
			* a number which repensent when passed N frame to skip.
			* 
			* eg. C = 0, no frame skip moduele, so the represent bit in bitset
			* always 0
			* C = 1, when pass one frame, the next frame will be pass, so
			* the next frame data`s bitset in that will be set 1.
			* C = 2, when pass two frame, the next frame will be pass.
			* [Notice]:if bit is 1, represent this frame will skip this module,
			* if bit is 0,represent this frame won`t skip this module.
			*/
			bool Init(const std::vector<std::pair<std::string, uint32_t>>& cfg) {
				status_.resize(module_num_, 0);
				skip_info_.resize(module_num_, 0);
				cfg_ = cfg;
				size_t i = 0;
				for (auto& it : cfg_) {
					skip_info_[i] = it.second;
					++i;
				}
				init_ = true;
				return true;
			}
			std::bitset<N> GetSkipBitSetNow() {
				std::bitset<N> bs;
				if (init_ && running_) {
					for (size_t i = 0; i < module_num_; ++i) {
						status_[i] = (status_[i] + 1) % (skip_info_[i] + 1);
						if (1 == status_[i]) {
							bs[i] = 1;
						}
					}
					return bs;
				}
				LOG(ERROR) << "FrameSkipControler not inital or running";
				return bs;
			}
			std::vector<uint32_t> GetSkipVectorNow() {
				std::vector<uint32_t> vec;
				vec.resize(module_num_, 0);
				if (init_ && running_) {
					for (size_t i = 0; i < module_num_; ++i) {
						status_[i] = (status_[i] + 1) % (skip_info_[i] + 1);
						if (1 == status_[i]) {
							vec[i] = 1;
						}
					}
					return vec;
				}
				LOG(ERROR) << "FrameSkipControler not inital or running";
				return vec;
			}
			virtual ~FrameSkipControl() {
				running_ = false;
				skip_info_.clear();
				status_.clear();
				cfg_.clear();
			}
		private:
			std::atomic<bool> init_{ false };
			std::atomic<bool> running_{ false };
			uint32_t module_num_ = N;
			std::vector<uint32_t> skip_info_;
			std::vector<uint32_t> status_;
			using CFG = std::vector<std::pair<std::string, uint32_t>>;
			CFG cfg_;
		}; // class FrameSkipControl
	} // namespace components
}// namespace easysa

#endif // !COMPONENTS_FRAME_SKIP_CONTROL_HPP_