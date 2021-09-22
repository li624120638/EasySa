#ifndef FRAMEWORK_CORE_SRC_CONVEYOR_HPP_
#define FRAMEWORK_CORE_SRC_CONVEYOR_HPP_

#include <memory>
#include <vector>
#include <queue>
#include <condition_variable>

#include "easysa_frame.hpp"

namespace easysa {

	using FrameInfoPtr = std::shared_ptr<FrameInfo>;

	class Connector;

	/**
	 * @brief Conveyor is used to transmit data between two modules.
	 *
	 * Conveyor belongs to Connector.
	 * Each Connect could have several conveyors which depends on the paramllelism of each module.
	 *
	 * Conveyor has one buffer queue for transmitting data from one module to another.
	 * The upstream node module will push data to buffer queue, and the downstream node will pop data from buffer queue.
	 *
	 * The capacity of buffer queue could be set in configuration json file (see README for more information of
	 * configuration json file). If there is no element in buffer queue, the downstream node will wait to pop and
	 * be blocked. On contrary, if the queue is full, the upstream node will wait to push and be blocked.
	 */
	class Conveyor : private NonCopyable {
	public:
		Conveyor(size_t max_size);
		~Conveyor() = default;
		bool PushDataBuffer(FrameInfoPtr data);
		FrameInfoPtr PopDataBuffer();
		std::vector<FrameInfoPtr> PopAllDataBuffer();
		uint32_t GetBufferSize();
		uint64_t GetFailTime();

	private:
#ifdef UNIT_TEST
	public:
#endif

	private:
		std::queue<FrameInfoPtr> dataq_;
		size_t max_size_;
		uint64_t fail_time_ = 0;
		std::mutex data_mutex_;
		std::condition_variable notempty_cond_;
		const std::chrono::milliseconds rel_time_{ 20 };
	};  // class Conveyor

}  // namespace easysa

#endif  // FRAMEWORK_CORE_SRC_CONVEYOR_HPP_