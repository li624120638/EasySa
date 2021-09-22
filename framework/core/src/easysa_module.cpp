/*************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *************************************************************************/
#include "easysa_module.hpp"

#include <memory>
#include <string>
#include <thread>
#include <unordered_map>

#include "easysa_pipeline.hpp"
#include "profiler/pipeline_profiler.hpp"

namespace easysa {

#ifdef UNIT_TEST
    static SpinLock module_id_spinlock_;
    static uint64_t module_id_mask_ = 0;
    static size_t _GetId() {
        SpinLockGuard guard(module_id_spinlock_);
        for (size_t i = 0; i < sizeof(module_id_mask_) * 8; i++) {
            if (!(module_id_mask_ & ((uint64_t)1 << i))) {
                module_id_mask_ |= (uint64_t)1 << i;
                return i;
            }
        }
        return INVALID_MODULE_ID;
    }
    static void _ReturnId(size_t id_) {
        SpinLockGuard guard(module_id_spinlock_);
        if (id_ >= sizeof(module_id_mask_) * 8) {
            return;
        }
        module_id_mask_ &= ~(1 << id_);
    }
#endif

    Module::~Module() {
        std::shared_lock<std::shared_mutex> guard(container_lock_);
        if (container_) {
            container_->ReturnModuleIdx(id_);
        }
        else {
#ifdef UNIT_TEST
            _ReturnId(id_);
#endif
        }
    }

    void Module::SetContainer(Pipeline* container) {
        if (container) {
            {
                std::unique_lock<std::shared_mutex> guard(container_lock_);
                container_ = container;
            }
            GetId();
        }
        else {
            std::unique_lock<std::shared_mutex> guard(container_lock_);
            container_ = nullptr;
            id_ = INVALID_MODULE_ID;
        }
    }

    size_t Module::GetId() {
        if (id_ == INVALID_MODULE_ID) {
            std::shared_lock<std::shared_mutex> guard(container_lock_);
            if (container_) {
                id_ = container_->GetModuleIdx();
            }
            else {
#ifdef UNIT_TEST
                id_ = _GetId();
#endif
            }
        }
        return id_;
    }

    bool Module::PostEvent(EventType type, const std::string& msg) {
        Event event;
        event.type = type;
        event.message = msg;
        event.module_name = name_;

        std::shared_lock<std::shared_mutex> guard(container_lock_);
        if (container_) {
            return container_->GetEventBus()->PostEvent(event);
        }
        else {
            LOG(WARNING) << "[core]:" << "[" << GetName() << "] module's container is not set";
            return false;
        }
    }

    bool Module::PostEvent(Event e) {
        std::shared_lock<std::shared_mutex> guard(container_lock_);
        if (container_) {
            return container_->GetEventBus()->PostEvent(e);
        }
        else {
            LOG(WARNING) << "[core]:" << "[" << GetName() << "] module's container is not set";
            return false;
        }
    }

    int Module::DoTransmitData(std::shared_ptr<FrameInfo> data) {
        if (data->IsEos() && data->payload && IsStreamRemoved(data->stream_id)) {
            // FIMXE
            SetStreamRemoved(data->stream_id, false);
        }
        std::shared_lock<std::shared_mutex> guard(container_lock_);
        if (container_) {
            return container_->ProvideData(this, data);
        }
        else {
            if (HasTransmit()) NotifyObserver(data);
            return 0;
        }
    }

    int Module::DoProcess(std::shared_ptr<FrameInfo> data) {
        bool removed = IsStreamRemoved(data->stream_id);
        if (!removed) {
            // For the case that module is implemented by a pipeline
            if (data->payload && IsStreamRemoved(data->payload->stream_id)) {
                SetStreamRemoved(data->stream_id, true);
                removed = true;
            }
        }

        if (!HasTransmit()) {
            if (!data->IsEos()) {
                if (!removed) {
                    int ret = Process(data);
                    if (ret != 0) {
                        return ret;
                    }
                }
                return DoTransmitData(data);
            }
            else {
                this->OnEos(data->stream_id);
                return DoTransmitData(data);
            }
        }
        else {
            if (removed) {
                data->flags |= FRAME_FLAG_REMOVED;
            }
            return Process(data);
        }
        return -1;
    }

    bool Module::TransmitData(std::shared_ptr<FrameInfo> data) {
        if (!HasTransmit()) {
            return true;
        }
        if (!DoTransmitData(data)) {
            return true;
        }
        return false;
    }

    ModuleProfiler* Module::GetProfiler() {
        std::shared_lock<std::shared_mutex> guard(container_lock_);
        if (container_ && container_->GetProfiler())
            return container_->GetProfiler()->GetModuleProfiler(GetName());
        return nullptr;
    }

    ModuleFactory* ModuleFactory::factory_ = nullptr;

}  // namespace easysa