/*************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *************************************************************************/

#include "easysa_eventbus.hpp"

#include <list>
#include <memory>
#include <thread>
#include <utility>
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>

#include "easysa_pipeline.hpp"

namespace easysa {

    EventBus::~EventBus() {
        Stop();
    }

    bool EventBus::IsRunning() {
        return running_.load();
    }

    bool EventBus::Start() {
        running_.store(true);
        event_thread_ = std::thread(&EventBus::EventLoop, this);
        return true;
    }

    void EventBus::Stop() {
        if (IsRunning()) {
            running_.store(false);
            if (event_thread_.joinable()) {
                event_thread_.join();
            }
        }
    }

    // @return The number of bus watchers that has been added to this event bus.
    uint32_t EventBus::AddBusWatch(BusWatcher func) {
        std::lock_guard<std::mutex> lk(watcher_mtx_);
        bus_watchers_.push_front(func);
        return bus_watchers_.size(); // disable warning
    }

    void EventBus::ClearAllWatchers() {
        std::lock_guard<std::mutex> lk(watcher_mtx_);
        bus_watchers_.clear();
    }

    const std::list<BusWatcher>& EventBus::GetBusWatchers() const {
        return bus_watchers_;
    }

    bool EventBus::PostEvent(Event event) {
        if (!running_.load()) {
            LOG(WARNING) << "[core]:" << "Post event failed, pipeline not running";
            return false;
        }
        // LOG(INFO) << "[core]:" << "Recieve Event from [" << event.module->GetName() << "] :" << event.message;
        queue_.Push(event);
#ifdef UNIT_TEST
        if (unit_test) {
            test_eventq_.Push(event);
            unit_test = false;
        }
#endif
        return true;
    }

    Event EventBus::PollEvent() {
        Event event;
        event.type = EVENT_INVALID;
        while (running_.load()) {
            if (queue_.WaitAndTryPop(event, std::chrono::milliseconds(100))) {
                break;
            }
        }
        if (!running_.load()) event.type = EVENT_STOP;
        return event;
    }

    void EventBus::EventLoop() {
        const std::list<BusWatcher>& kWatchers = GetBusWatchers();
        EventHandleFlag flag = EVENT_HANDLE_NULL;

        // SetThreadName("cn-EventLoop", pthread_self());
        // start loop
        while (IsRunning()) {
            Event event = PollEvent();
            if (event.type == EVENT_INVALID) {
                LOG(INFO) << "[core]:" << "[EventLoop] event type is invalid";
                break;
            }
            else if (event.type == EVENT_STOP) {
                LOG(INFO) << "[core]:" << "[EventLoop] Get stop event";
                break;
            }
            std::unique_lock<std::mutex> lk(watcher_mtx_);
            for (auto& watcher : kWatchers) {
                flag = watcher(event);
                if (flag == EVENT_HANDLE_INTERCEPTION || flag == EVENT_HANDLE_STOP) {
                    break;
                }
            }
            if (flag == EVENT_HANDLE_STOP) {
                break;
            }
        }
        LOG(INFO) << "[core]:" << "Event bus exit.";
    }

#ifdef UNIT_TEST
    Event EventBus::PollEventToTest() {
        Event event;
        event.type = EVENT_INVALID;
        while (running_.load()) {
            if (test_eventq_.WaitAndTryPop(event, std::chrono::milliseconds(100))) {
                break;
            }
        }
        if (!running_.load()) event.type = EVENT_STOP;
        return event;
    }
#endif

}  // namespace easysa