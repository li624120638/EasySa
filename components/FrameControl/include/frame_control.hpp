#ifndef FRAME_CONTROL_HPP
#include <chrono>
#include <thread>

namespace easysa {
	namespace components {
class FrameController {
public:
	explicit FrameController(uint32_t frame_rate) :
							 frame_rate_(frame_rate){}
	void Start() { start_ = std::chrono::steady_clock::now(); }
	inline uint32_t GetFrameRate() const { return frame_rate_; }
	inline void SetFrameRate(uint32_t frame_rate) {
		frame_rate_ = frame_rate;
	}
	void Control() {
		if (frame_rate_ == 0) return;
		double delay = 1000.0 / frame_rate_;
		end_ = std::chrono::steady_clock::now();
		std::chrono::duration<double, std::milli> diff = end_ - start_;
		auto gap = delay - diff.count() - time_gap_;
		if (gap > 0) {
			std::chrono::duration<double, std::milli> dura(gap);
			std::this_thread::sleep_for(dura);
			time_gap_ = 0;
		}
		else {
			time_gap_ = -gap;
		}
	}
private:
	uint32_t frame_rate_{25};
	double time_gap_ = 0;
	std::chrono::time_point<std::chrono::steady_clock> start_, end_;
}; // class FrameController

	} // namespace components
} // namespace easysa
#endif // !FRAME_CONTROL_HPP
