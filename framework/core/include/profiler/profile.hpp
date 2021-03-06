/*************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *************************************************************************/
#ifndef FRAMEWORK_CORE_INCLUDE_PROFILER_PROFILE_HPP_
#define FRAMEWORK_CORE_INCLUDE_PROFILER_PROFILE_HPP_

#include <string>
#include <utility>
#include <vector>

namespace easysa {

    // Performance statistics of stream.
    struct StreamProfile {
        std::string stream_name;         ///< stream name.
        uint64_t counter = 0;            ///< frame counter, it is equal to `completed` plus `dropped`.
        uint64_t completed = 0;          ///< completed frame counter.
        int64_t dropped = 0;             ///< dropped frame counter.
        double latency = 0.0;            ///< average latency. (ms)
        double maximum_latency = 0.0;    ///< maximum latency. (ms)
        double minimum_latency = 0.0;    ///< minimum latency. (ms)
        double fps = 0.0;                ///< fps.

        /*
         * StreamProfile constructor.
         */
        StreamProfile() = default;
        /**
         * StreamProfile copy constructor.
         *
         * @param it which instance copy from.
         */
        StreamProfile(const StreamProfile& it) = default;
        /**
         * StreamProfile operator =.
         *
         * @param it Which instance copy from.
         *
         * @return Returns a lvalue reference to the current instance.
         */
        StreamProfile& operator=(const StreamProfile& it) = default;
        /**
         * StreamProfile move constructor.
         *
         * @param it which instance move from.
         */
        inline StreamProfile(StreamProfile&& it) {
            *this = std::forward<StreamProfile>(it);
        }
        /**
         * StreamProfile operator =.
         *
         * @param it Which instance move from.
         *
         * @return Returns a lvalue reference to the current instance.
         */
        inline StreamProfile& operator=(StreamProfile&& it) {
            stream_name = std::move(it.stream_name);
            counter = it.counter;
            completed = it.completed;
            dropped = it.dropped;
            latency = it.latency;
            maximum_latency = it.maximum_latency;
            minimum_latency = it.minimum_latency;
            fps = it.fps;
            return *this;
        }
    };  // struct StreamProfile

    // Performance statistics of process.
    struct ProcessProfile {
        std::string process_name;                      ///< process name.
        uint64_t counter = 0;                          ///< frame counter, it is equal to `completed` plus `dropped`.
        uint64_t completed = 0;                        ///< completed frame counter.
        int64_t dropped = 0;                           ///< dropped frame counter.
        int64_t ongoing = 0;                           ///< number of frame being processed.
        double latency = 0.0;                          ///< average latency. (ms)
        double maximum_latency = 0.0;                  ///< maximum latency. (ms)
        double minimum_latency = 0.0;                  ///< minimum latency. (ms)
        double fps = 0.0;                              ///< fps.
        std::vector<StreamProfile> stream_profiles;    ///< stream profiles.

        /*
         * ProcessProfile constructor.
         */
        ProcessProfile() = default;
        /**
         * ProcessProfile copy constructor.
         *
         * @param it which instance copy from.
         */
        ProcessProfile(const ProcessProfile& it) = default;
        /**
         * ProcessProfile operator =.
         *
         * @param it Which instance copy from.
         *
         * @return Returns a lvalue reference to the current instance.
         */
        ProcessProfile& operator=(const ProcessProfile& it) = default;
        /**
         * ProcessProfile move constructor.
         *
         * @param it which instance move from.
         */
        inline ProcessProfile(ProcessProfile&& it) noexcept {
            *this = std::forward<ProcessProfile>(it);
        }
        /**
         * ProcessProfile operator =.
         *
         * @param it Which instance move from.
         *
         * @return Returns a lvalue reference to the current instance.
         */
        inline ProcessProfile& operator=(ProcessProfile&& it) {
            process_name = std::move(it.process_name);
            stream_profiles = std::move(it.stream_profiles);
            counter = it.counter;
            completed = it.completed;
            ongoing = it.ongoing;
            dropped = it.dropped;
            latency = it.latency;
            maximum_latency = it.maximum_latency;
            minimum_latency = it.minimum_latency;
            fps = it.fps;
            return *this;
        }
    };  // struct ProcessProfile

    // Performance statistics of module.
    struct ModuleProfile {
        std::string module_name;                         ///< module name.
        std::vector<ProcessProfile> process_profiles;    ///< process profiles.

        /*
         * ModuleProfile constructor.
         */
        ModuleProfile() = default;
        /**
         * ModuleProfile copy constructor.
         *
         * @param it which instance copy from.
         */
        ModuleProfile(const ModuleProfile& it) = default;
        /**
         * ModuleProfile operator =.
         *
         * @param it Which instance copy from.
         *
         * @return Returns a lvalue reference to the current instance.
         */
        ModuleProfile& operator=(const ModuleProfile& it) = default;
        /**
         * ModuleProfile move constructor.
         *
         * @param it which instance move from.
         */
        inline ModuleProfile(ModuleProfile&& it) {
            *this = std::forward<ModuleProfile>(it);
        }
        /**
         * ModuleProfile operator =.
         *
         * @param it Which instance move from.
         *
         * @return Returns a lvalue reference to the current instance.
         */
        inline ModuleProfile& operator=(ModuleProfile&& it) {
            module_name = std::move(it.module_name);
            process_profiles = std::move(it.process_profiles);
            return *this;
        }
    };  // struct ModuleProfile

    // Performance statistics of pipeline.
    struct PipelineProfile {
        std::string pipeline_name;                       ///< pipeline name.
        std::vector<ModuleProfile> module_profiles;      ///< module profiles.
        ProcessProfile overall_profile;                  ///< profile of the whole pipeline.

        /*
         * PipelineProfile constructor.
         */
        PipelineProfile() = default;
        /**
         * PipelineProfile copy constructor.
         *
         * @param it which instance copy from.
         */
        PipelineProfile(const PipelineProfile& it) = default;
        /**
         * PipelineProfile operator =.
         *
         * @param it Which instance copy from.
         *
         * @return Returns a lvalue reference to the current instance.
         */
        PipelineProfile& operator=(const PipelineProfile& it) = default;
        /**
         * PipelineProfile move constructor.
         *
         * @param it which instance move from.
         */
        inline PipelineProfile(PipelineProfile&& it) {
            *this = std::forward<PipelineProfile>(it);
        }
        /**
         * PipelineProfile operator =.
         *
         * @param it Which instance move from.
         *
         * @return Returns a lvalue reference to the current instance.
         */
        inline PipelineProfile& operator=(PipelineProfile&& it) {
            pipeline_name = std::move(it.pipeline_name);
            module_profiles = std::move(it.module_profiles);
            overall_profile = std::move(it.overall_profile);
            return *this;
        }
    };  // struct PipelineProfile

}  // namespace easysa
#endif // FRAMEWORK_CORE_INCLUDE_PROFILER_PROFILE_HPP_
