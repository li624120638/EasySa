/*************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *************************************************************************/

#ifndef FRAMEWORK_CORE_SRC_CONNECTOR_HPP_
#define FRAMEWORK_CORE_SRC_CONNECTOR_HPP_

#include <atomic>
#include <memory>
#include <vector>

#include "easysa_frame.hpp"

namespace easysa {

	using FrameInfoPtr = std::shared_ptr<FrameInfo>;

	class Conveyor;

	/**
	 * @brief Connects two modules. Transmits data between modules through Conveyor(s).
	 *
	 * The number of Conveyors of Connector depends on the paramllelism of downstream node module.
	 * The parallelism of each module could be set in configuration json file (see README for
	 * more information of configuration json file).
	 *
	 * Connector could be blocked to balance the various speed of different modules in the same pipeline.
	 *
	 *  -----------                                                   -----------
	 * |           |       /---------------------------------\       |           |
	 * |           |      |             connector             |      |           |
	 * |           | push |                                   | pop  |           |
	 * |   module  |----->|  -----------conveyor1------------ |----->|   module  |
	 * |     A     |----->|  -----------conveyor2------------ |----->|     B     |
	 * |           |----->|  -----------conveyor3------------ |----->|           |
	 * |           |      |              ... ...              |      |           |
	 * |           |       \---------------------------------/       |           |
	 *  -----------                                                   -----------
	 */
	class Connector : private NonCopyable {
	public:
		/**
		 * @brief Connector constructor.
		 * @param
		 *   [conveyor_count]: the conveyor num of this connector.
		 *   [conveyor_capacity]: the maximum buffer number of a conveyor.
		 */
		explicit Connector(const size_t conveyor_count, size_t conveyor_capacity = 20);
		~Connector();

		const size_t GetConveyorCount() const;
		size_t GetConveyorCapacity() const;
		bool IsConveyorFull(int conveyor_idx) const;
		bool IsConveyorEmpty(int conveyor_idx) const;
		size_t GetConveyorSize(int conveyor_idx) const;
		uint64_t GetFailTime(int conveyor_idx) const;

		FrameInfoPtr PopDataBufferFromConveyor(int conveyor_idx);
		bool PushDataBufferToConveyor(int conveyor_idx, FrameInfoPtr data);

		void Start();
		void Stop();
		bool IsStopped();
		void EmptyDataQueue();

	private:
		Conveyor* GetConveyorByIdx(int idx) const;
		Conveyor* GetConveyor(int conveyor_idx) const;

		std::vector<Conveyor*> conveyors_;
		size_t conveyor_capacity_ = 20;
		std::vector<uint64_t> fail_times_;
		std::atomic<bool> stop_{ false };
	};  // class Connector

}  // namespace easysa

#endif  // FRAMEWORK_CORE_SRC_CONNECTOR_HPP_