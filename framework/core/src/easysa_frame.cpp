/*************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *************************************************************************/

#include <memory>
#include <string>
#include <unordered_map>
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>
#include "easysa_frame.hpp"
#include "easysa_module.hpp"

namespace easysa {

    SpinLock FrameInfo::spinlock_;
    std::unordered_map<std::string, int> FrameInfo::stream_count_map_;

    static SpinLock s_eos_spinlock_;
    static std::unordered_map<std::string, std::atomic<bool>> s_stream_eos_map_;

    static SpinLock s_remove_spinlock_;
    static std::unordered_map<std::string, bool> s_stream_removed_map_;

    int FrameInfo::flow_depth_ = 0;

    void SetFlowDepth(int flow_depth) { FrameInfo::flow_depth_ = flow_depth; }
    int GetFlowDepth() { return FrameInfo::flow_depth_; }

    bool CheckStreamEosReached(const std::string& stream_id, bool sync) {
        if (sync) {
            while (1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                SpinLockGuard guard(s_eos_spinlock_);
                auto iter = s_stream_eos_map_.find(stream_id);
                if (iter != s_stream_eos_map_.end()) {
                    if (iter->second == true) {
                        s_stream_eos_map_.erase(iter);
                        // LOG(INFO) << "check stream eos reached, stream_id =  " << stream_id;
                        return true;
                    }
                }
                else {
                    return false;
                }
            }
            return false;
        }
        else {
            SpinLockGuard guard(s_eos_spinlock_);
            auto iter = s_stream_eos_map_.find(stream_id);
            if (iter != s_stream_eos_map_.end()) {
                if (iter->second == true) {
                    s_stream_eos_map_.erase(iter);
                    return true;
                }
            }
            return false;
        }
    }

    void SetStreamRemoved(const std::string& stream_id, bool value) {
        SpinLockGuard guard(s_remove_spinlock_);
        auto iter = s_stream_removed_map_.find(stream_id);
        if (iter != s_stream_removed_map_.end()) {
            if (value != true) {
                s_stream_removed_map_.erase(iter);
                return;
            }
            iter->second = true;
        }
        else {
            s_stream_removed_map_[stream_id] = value;
        }
        // LOG(INFO) << "[core]:" << "_____SetStreamRemoved " << stream_id << ":" << s_stream_removed_map_[stream_id];
    }

    bool IsStreamRemoved(const std::string& stream_id) {
        SpinLockGuard guard(s_remove_spinlock_);
        auto iter = s_stream_removed_map_.find(stream_id);
        if (iter != s_stream_removed_map_.end()) {
            // LOG(INFO) << "[core]:" << "_____IsStreamRemoved " << stream_id << ":" << s_stream_removed_map_[stream_id];
            return s_stream_removed_map_[stream_id];
        }
        return false;
    }

    std::shared_ptr<FrameInfo> FrameInfo::Create(const std::string& stream_id, bool eos,
        std::shared_ptr<FrameInfo> payload) {
        if (stream_id == "") {
            LOG(ERROR) << "[core]:" << "FrameInfo::Create() stream_id is empty string.";
            return nullptr;
        }
        std::shared_ptr<FrameInfo> ptr(new (std::nothrow) FrameInfo());
        if (!ptr) {
            LOG(ERROR) << "[core]:" << "FrameInfo::Create() new FrameInfo failed.";
            return nullptr;
        }
        ptr->stream_id = stream_id;
        ptr->payload = payload;
        if (eos) {
            ptr->flags |= easysa::FRAME_FLAG_EOS;
            if (!ptr->payload) {
                SpinLockGuard guard(s_eos_spinlock_);
                s_stream_eos_map_[stream_id] = false;
            }
            return ptr;
        }

        if (flow_depth_ > 0) {
            SpinLockGuard guard(spinlock_);
            auto iter = stream_count_map_.find(stream_id);
            if (iter == stream_count_map_.end()) {
                stream_count_map_[stream_id] = 1;
                // LOG(INFO) << "[core]:" << "FrameInfo::Create() insert stream_id: " << stream_id;
            }
            else {
                int count = stream_count_map_[stream_id];
                if (count >= flow_depth_) {
                    return nullptr;
                }
                stream_count_map_[stream_id] = count + 1;
                // LOG(INFO) << "[core]:" << "FrameInfo::Create() add count stream_id " << stream_id << ":" << count;
            }
        }
        return ptr;
    }

    FrameInfo::~FrameInfo() {
        if (this->IsEos()) {
            if (!this->payload) {
                SpinLockGuard guard(s_eos_spinlock_);
                s_stream_eos_map_[stream_id] = true;
            }
            return;
        }
        /*if (frame.ctx.dev_type == DevContext::INVALID) {
          return;
        }*/
        if (flow_depth_ > 0) {
            SpinLockGuard guard(spinlock_);
            auto iter = stream_count_map_.find(stream_id);
            if (iter != stream_count_map_.end()) {
                int count = iter->second;
                --count;
                if (count <= 0) {
                    stream_count_map_.erase(iter);
                    // LOG(INFO) << "[core]:" << "FrameInfo::~FrameInfo() erase stream_id " << frame.stream_id;
                }
                else {
                    iter->second = count;
                    // LOG(INFO) << "[core]:" << "FrameInfo::~FrameInfo() update stream_id " << frame.stream_id << " : " << count;
                }
            }
            else {
                LOG(ERROR) << "[core]:" << "Invaid stream_id, please check\n";
            }
        }
    }

    void FrameInfo::SetModulesMask(uint64_t mask) {
        SpinLockGuard guard(mask_lock_);
        modules_mask_ = mask;
    }

    uint64_t FrameInfo::MarkPassed(Module* module) {
        SpinLockGuard guard(mask_lock_);
        modules_mask_ |= (uint64_t)1 << module->GetId();
        return modules_mask_;
    }

    uint64_t FrameInfo::GetModulesMask() {
        SpinLockGuard guard(mask_lock_);
        return modules_mask_;
    }

}  // namespace easysa