/*************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *************************************************************************/
#ifndef FRAMEWORK_CORE_INCLUDE_EASYSA_CONFIG_HPP_
#define FRAMEWORK_CORE_INCLUDE_EASYSA_CONFIG_HPP_

 /**
  * @file easysa_config.hpp
  *
  * This file contains a declaration of the ModuleConfig class.
  */
#define NOMINMAX
#include <windows.h>
#include <string.h>

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "easysa_common.hpp"

namespace easysa {

    static constexpr char kPROFILER_CONFIG_NAME[] = "profiler_config";

    struct ProfilerConfig {
        bool enable_profiling = false;
        bool enable_tracing = false;
        size_t trace_event_capacity = 100000;

        /**
         * Parses members from JSON string.
         *
         * @return Returns true if the JSON file has been parsed successfully. Otherwise, returns false.
         */
        bool ParseByJSONStr(const std::string& jstr);

        /**
         * Parses members from JSON file.
         *
         * @return Returns true if the JSON file has been parsed successfully. Otherwise, returns false.
         */
        bool ParseByJSONFile(const std::string& jfname);
    };  // struct ProfilerConfig

    /// Module parameter set.
    using ModuleParamSet = std::unordered_map<std::string, std::string>;

#define JSON_DIR_PARAM_NAME "json_file_dir"
    /**
     * @brief The configuration parameters of a module.
     *
     * You can use ``ModuleConfig`` to add modules in a pipeline.
     * The module configuration can be in JSON file.
     *
     * @code
     * "name(ModuleConfig::name)": {
     *   custom_params(ModuleConfig::parameters): {
     *     "key0": "value",
     *     "key1": "value",
     *     ...
     *   }
     *  "parallelism(ModuleConfig::parallelism)": 3,
     *  "max_input_queue_size(ModuleConfig::maxInputQueueSize)": 20,
     *  "class_name(ModuleConfig::className)": "Inferencer",
     *  "next_modules": ["module0(ModuleConfig::name)", "module1(ModuleConfig::name)", ...],
     * }
     * @endcode
     *
     * @see Pipeline::AddModuleConfig.
     */
    struct ModuleConfig {
        std::string name;  ///< The name of the module.
        std::unordered_map<std::string, std::string>
            parameters;   ///< The key-value pairs. The pipeline passes this value to the ModuleConfig::name module.
        int parallelism;  ///< Module parallelism. It is equal to module thread number and the data queue for input data.
        int maxInputQueueSize;          ///< The maximum size of the input data queues.
        std::string className;          ///< The class name of the module.
        std::vector<std::string> next;  ///< The name of the downstream modules.
        bool showPerfInfo;              ///< Whether to show performance information or not.

        /**
         * Parses members from JSON string except ModuleConfig::name.
         *
         * @return Returns true if the JSON file has been parsed successfully. Otherwise, returns false.
         */
        bool ParseByJSONStr(const std::string& jstr);

        /**
         * Parses members from JSON file except ModuleConfig::name.
         *
         * @return Returns true if the JSON file has been parsed successfully. Otherwise, returns false.
         */
        bool ParseByJSONFile(const std::string& jfname);
    };

    /**
     * Parses pipeline configs from json-config-file.
     *
     * @return Returns true if the JSON file has been parsed successfully. Otherwise, returns false.
     */
    bool ConfigsFromJsonFile(const std::string& config_file,
        std::vector<ModuleConfig>* pmodule_configs,
        ProfilerConfig* pprofiler_config);

    /**
     * @brief Gets the complete path of a file.
     *
     * If the path you set is an absolute path, returns the absolute path.
     * If the path you set is a relative path, retuns the path that appends the relative path
     * to the specified JSON file path.
     *
     * @param path The path relative to the JSON file or an absolute path.
     * @param param_set The module parameters. The JSON file path is one of the parameters.
     *
     * @return Returns the complete path of a file.
     */
    std::string GetPathRelativeToTheJSONFile(const std::string& path, const ModuleParamSet& param_set);

}  // namespace easysa

#endif // !FRAMEWORK_CORE_INCLUDE_EASYSA_CONFIG_HPP_
