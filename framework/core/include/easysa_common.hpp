/*************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *************************************************************************/

#ifndef FRAMEWORK_CORE_INCLUDE_EASYSA_COMMON_HPP_
#define FRAMEWORK_CORE_INCLUDE_EASYSA_COMMON_HPP_

#include <limits.h>
#include <thread>
#include <windows.h>

#include <atomic>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace easysa {

    /**
     * @brief Flag to specify how bus watcher handle a single event.
     */
    enum EventType {
        EVENT_INVALID,  ///< An invalid event type.
        EVENT_ERROR,    ///< An error event.
        EVENT_WARNING,  ///< A warning event.
        EVENT_EOS,      ///< An EOS event.
        EVENT_STOP,     ///< Stops an event that is called by application layer usually.
        EVENT_STREAM_ERROR,  ///< A stream error event.
        EVENT_TYPE_END  ///< Reserved for your custom events.
    };


    class NonCopyable {
    protected:
        NonCopyable() = default;
        ~NonCopyable() = default;

    private:
        NonCopyable(const NonCopyable&) = delete;
        NonCopyable(NonCopyable&&) = delete;
        NonCopyable& operator=(const NonCopyable&) = delete;
        NonCopyable& operator=(NonCopyable&&) = delete;
    };

    /*helper functions
     */
    /*
    static const pthread_t invalid_pthread_tid = static_cast<pthread_t>(-1);
    inline void SetThreadName(const std::string& name, pthread_t thread = invalid_pthread_tid) {
        // name length should be less than 16 bytes 
        if (name.empty() || name.size() >= 16) {
            return;
        }
        if (thread == invalid_pthread_tid) {
            prctl(PR_SET_NAME, name.c_str());
            return;
        }
        pthread_setname_np(thread, name.c_str());
    }

    inline std::string GetThreadName(pthread_t thread = invalid_pthread_tid) {
        char name[80];
        if (thread == invalid_pthread_tid) {
            prctl(PR_GET_NAME, name);
            return name;
        }
        pthread_getname_np(thread, name, 80);
        return name;
    }
     */
    /*
    struct tagTHREADNAME_INFO {
        DWORD dwType; // must be 0x1000
        LPCSTR szName; // pointer to name (in user addr space)
        DWORD dwThreadID; // thread ID (-1=caller thread)
        DWORD dwFlags; // reserved for future use, must be zero
    };
    // -1 represent this thread
     inline void SetThreadName(DWORD dwThreadID, LPCSTR szThreadName) {
        tagTHREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = szThreadName;
        info.dwThreadID = dwThreadID;
        info.dwFlags = 0;
        __try
        {
            RaiseException(0x406D1388, 0, sizeof(info) / sizeof(DWORD), (DWORD*)&info);
        }
        __except (EXCEPTION_CONTINUE_EXECUTION)
        {
        }
     }
     */
     inline std::string GetThreadName(const std::thread::id& thd_id) {
         //char name[80];
         std::string name;
         /*
         if (thread == invalid_pthread_tid) {
             prctl(PR_GET_NAME, name);
             return name;
         }
         std::thread::native_handle = 
         pthread_getname_np(thread, name, 80);
         */   
         return name;
     }
    /*pipeline capacities*/
    constexpr size_t INVALID_MODULE_ID = (size_t)(-1);
    uint32_t GetMaxModuleNumber();

    constexpr uint32_t INVALID_STREAM_IDX = (uint32_t)(-1);
    uint32_t GetMaxStreamNumber();

    /**
     * Limit the resource for each stream,
     * there will be no more than "flow_depth" frames simultaneously.
     * Disabled by default.
     */
    void SetFlowDepth(int flow_depth);
    int GetFlowDepth();

    /*for force-remove-source*/
    bool CheckStreamEosReached(const std::string& stream_id, bool sync = true);
    void SetStreamRemoved(const std::string& stream_id, bool value = true);
    bool IsStreamRemoved(const std::string& stream_id);

    /**
     * @brief Converts number to string
     *
     * @param number the number
     * @param width Padding with zero
     * @return Returns string
     */
    template <typename T>
    std::string NumToFormatStr(const T& number, uint32_t width) {
        std::stringstream ss;
        ss << std::setw(width) << std::setfill('0') << number;
        return ss.str();
    }
}  // namespace cnstream

#endif  // FRAMEWORK_CORE_INCLUDE_EASYSA_COMMON_HPP_