/*************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *************************************************************************/

#include "conveyor.hpp"

#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include "connector.hpp"

#pragma warning(disable:4267)
namespace easysa {

    Conveyor::Conveyor(size_t max_size) : max_size_(max_size) {
    }

    uint32_t Conveyor::GetBufferSize() {
        std::unique_lock<std::mutex> lk(data_mutex_);
        return dataq_.size();
    }

    bool Conveyor::PushDataBuffer(FrameInfoPtr data) {
        std::unique_lock<std::mutex> lk(data_mutex_);
        if (dataq_.size() < max_size_) {
            dataq_.push(data);
            //lk.unlock();
            notempty_cond_.notify_one();
            fail_time_ = 0;
            return true;
        }
        fail_time_ += 1;
        return false;
    }

    uint64_t Conveyor::GetFailTime() {
        std::unique_lock<std::mutex> lk(data_mutex_);
        return fail_time_;
    }

    FrameInfoPtr Conveyor::PopDataBuffer() {
        std::unique_lock<std::mutex> lk(data_mutex_);
        FrameInfoPtr data = nullptr;
        if (notempty_cond_.wait_for(lk, rel_time_, [&] { return !dataq_.empty(); })) {
            data = dataq_.front();
            dataq_.pop();
            return data;
        }
        return data;
    }

    std::vector<FrameInfoPtr> Conveyor::PopAllDataBuffer() {
        std::unique_lock<std::mutex> lk(data_mutex_);
        std::vector<FrameInfoPtr> vec_data;
        FrameInfoPtr data = nullptr;
        while (!dataq_.empty()) {
            data = dataq_.front();
            dataq_.pop();
            vec_data.push_back(data);
        }
        return vec_data;
    }

}  // namespace easysa