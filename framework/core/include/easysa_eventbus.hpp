/*************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *************************************************************************/

#ifndef FRAMEWORK_CORE_INCLUDE_EASYSA_EVENTBUS_HPP_
#define FRAMEWORK_CORE_INCLUDE_EASYSA_EVENTBUS_HPP_

 /**
  *  @file cnstream_eventbus.hpp
  *
  *  This file contains a declaration of the EventBus class.
  */

#include <atomic>
#include <functional>
#include <list>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

#include "easysa_common.hpp"
#include "util/easysa_queue.hpp"

namespace easysa {
	class Pipeline;
 /**
 * @brief Flags to specify the way in which bus watchers handled one event.
 */
	enum EventHandleFlag {
		EVENT_HANDLE_NULL,          ///< The event is not handled.
		EVENT_HANDLE_INTERCEPTION,  ///< The bus watcher is informed, and the event is intercepted.
		EVENT_HANDLE_SYNCED,        ///< The bus watcher is informed, and then other bus watchers are informed.
		EVENT_HANDLE_STOP           ///< A poll event is stopped.
	};
/**
 * @brief A structure holding the event information.
 */
	struct Event {
		EventType type;             ///< The event type.
		std::string stream_id;      ///< The stream that posts this event.
		std::string message;        ///< Additional event messages.
		std::string module_name;    ///< The module that posts this event.
		std::thread::id thread_id;  ///< The thread id from which the event is posted.
	};

/**
 * @brief The bus watcher function.
 *
 * @param event The event polled from the event bus.
 * @param Pipeline The module that is watching.
 *
 * @return Returns the flag that specifies how the event is handled.
 */
	using BusWatcher = std::function<EventHandleFlag(const Event&)>;
/**
* @brief the Event bus that transmits event from modules to pipeline
**/
	class EventBus : private NonCopyable {
	public:
		friend class Pipeline;
		/*
		*@brief start or stop an event bus thread
		*/
		bool Start();
		void Stop();
		/*
		* @brief Add BusWatcher to event bus
		* @param func the bus watcher to be added
		* @return the number of bus watcher had been added
		*/
		uint32_t AddBusWatch(BusWatcher func);
		/*
		* @brief Posts an event to a bus
		* @return true if this function run sucessfully.Otherwise, return false
		*/
		bool PostEvent(Event event);
	private:
		EventBus() = default;
		~EventBus();
		/*
		* @brief Polls an event from bus
		* @note This function is blocked until an event or bus is stopped
		*/
		Event PollEvent();
		/*
		* @brief Get all bus watcher from bus
		* @return a list with pairs of bus watcher and module
		*/
		const std::list<BusWatcher>& EventBus::GetBusWatchers() const;
		/*
		*
		* @brief removes all bus watcher
		*/
		void ClearAllWatchers();
		/*
		* @brief Checks if the bus is running
		*/
		bool IsRunning();
		void EventLoop();
	private:
		std::mutex watcher_mtx_;
		ThreadSafeQueue<Event> queue_;
		std::list<BusWatcher> bus_watchers_;
		std::thread event_thread_;
		std::atomic<bool> running_{ false };
	}; // class EventBus
} // namespace easysa
#endif // FRAMEWORK_CORE_INCLUDE_EASYSA_EVENTBUS_HPP_