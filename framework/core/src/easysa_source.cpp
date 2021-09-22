/*************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *************************************************************************/
#include <bitset>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "easysa_source.hpp"
#include "easysa_eventbus.hpp"
#include "easysa_pipeline.hpp"
#include "profiler/module_profiler.hpp"
#include "util/easysa_spinlock.hpp"


namespace easysa {

#ifdef UNIT_TEST

    /*default */
    static SpinLock stream_idx_lock;
    static std::unordered_map<std::string, uint32_t> stream_idx_map;
    static std::bitset<MAX_STREAM_NUM> stream_bitset(0);

    static uint32_t _GetStreamIndex(const std::string& stream_id) {
        SpinLockGuard guard(stream_idx_lock);
        auto search = stream_idx_map.find(stream_id);
        if (search != stream_idx_map.end()) {
            return search->second;
        }

        for (uint32_t i = 0; i < GetMaxStreamNumber(); i++) {
            if (!stream_bitset[i]) {
                stream_bitset.set(i);
                stream_idx_map[stream_id] = i;
                return i;
            }
        }
        return INVALID_STREAM_IDX;
    }

    static int _ReturnStreamIndex(const std::string& stream_id) {
        SpinLockGuard guard(stream_idx_lock);
        auto search = stream_idx_map.find(stream_id);
        if (search == stream_idx_map.end()) {
            return -1;
        }
        uint32_t stream_idx = search->second;
        if (stream_idx >= GetMaxStreamNumber()) {
            return -1;
        }
        stream_bitset.reset(stream_idx);
        stream_idx_map.erase(search);
        return 0;
    }
#endif

    uint32_t SourceModule::GetStreamIndex(const std::string& stream_id) {
        std::shared_lock<std::shared_mutex> guard(container_lock_);
        if (container_) return container_->GetStreamIndex(stream_id);
#ifdef UNIT_TEST
        return _GetStreamIndex(stream_id);
#endif
        return INVALID_STREAM_IDX;
    }

    void SourceModule::ReturnStreamIndex(const std::string& stream_id) {
        std::shared_lock<std::shared_mutex> guard(container_lock_);
        if (container_) container_->ReturnStreamIndex(stream_id);
#ifdef UNIT_TEST
        _ReturnStreamIndex(stream_id);
#endif
    }

    bool SourceModule::AddSource(std::shared_ptr<SourceHandler> handler) {
        if (!handler) {
            return false;
        }
        std::string stream_id = handler->GetStreamId();
        std::unique_lock<std::mutex> lock(mtx_);
        if (source_map_.find(stream_id) != source_map_.end()) {
            LOG(ERROR) << "[core]:" << "Duplicate stream_id\n";
            return false;
        }

        if (source_map_.size() >= GetMaxStreamNumber()) {
            LOG(ERROR) << "[core]:" << handler->GetStreamId()
                << " doesn't add to pipeline because of maximum limitation: " << GetMaxStreamNumber();
            return false;
        }

        handler->SetStreamUniqueIdx(source_idx_);
        source_idx_++;

        SetStreamRemoved(stream_id, false);
        if (handler->Open() != true) {
            LOG(ERROR) << "[core]:" << "source Open failed";
            return false;
        }
        source_map_[stream_id] = handler;
        return 0;
    }

    int SourceModule::RemoveSource(std::shared_ptr<SourceHandler> handler, bool force) {
        if (!handler) {
            return -1;
        }
        return RemoveSource(handler->GetStreamId(), force);
    }

    std::shared_ptr<SourceHandler> SourceModule::GetSourceHandler(const std::string& stream_id) {
        std::unique_lock<std::mutex> lock(mtx_);
        if (source_map_.find(stream_id) == source_map_.cend()) {
            return nullptr;
        }
        return source_map_[stream_id];
    }

    int SourceModule::RemoveSource(const std::string& stream_id, bool force) {
        SetStreamRemoved(stream_id, force);
        // Close handler first
        {
            std::unique_lock<std::mutex> lock(mtx_);
            auto iter = source_map_.find(stream_id);
            if (iter == source_map_.end()) {
                LOG(WARNING) << "[core]:" << "source does not exist\n";
                return 0;
            }
            iter->second->Close();
        }
        // wait for eos reached
        CheckStreamEosReached(stream_id, force);
        SetStreamRemoved(stream_id, false);
        {
            std::unique_lock<std::mutex> lock(mtx_);
            auto iter = source_map_.find(stream_id);
            if (iter == source_map_.end()) {
                LOG(WARNING) << "[core]:" << "source does not exist\n";
                return 0;
            }
            source_map_.erase(iter);
        }
        return 0;
    }

    int SourceModule::RemoveSources(bool force) {
        {
            std::unique_lock<std::mutex> lock(mtx_);
            for (auto& iter : source_map_) {
                SetStreamRemoved(iter.first, force);
            }
        }
        {
            std::unique_lock<std::mutex> lock(mtx_);
            for (auto& iter : source_map_) {
                iter.second->Close();
            }
        }
        {
            std::unique_lock<std::mutex> lock(mtx_);
            for (auto& iter : source_map_) {
                CheckStreamEosReached(iter.first, force);
                SetStreamRemoved(iter.first, false);
            }
            source_map_.clear();
        }
        return 0;
    }

    bool SourceModule::SendData(std::shared_ptr<FrameInfo> data) {
        if (!data->IsEos() && IsStreamRemoved(data->stream_id)) {
            return false;
        }
        auto profiler = this->GetProfiler();
        if (profiler && !data->IsEos()) {
            profiler->RecordProcessEnd(kPROCESS_PROFILER_NAME, std::make_pair(data->stream_id, data->timestamp));
        }
        return this->TransmitData(data);
    }

}  // namespace easysa