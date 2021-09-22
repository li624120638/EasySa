/*************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *************************************************************************/

#include "data_source.hpp"

#include <algorithm>
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <glog/logging.h>

#include "profiler/module_profiler.hpp"

namespace easysa {

    DataSource::DataSource(const std::string& name) : SourceModule(name) {}

    DataSource::~DataSource() {}

    static int GetDeviceId(ModuleParamSet paramSet) {
        if (paramSet.find("device_id") == paramSet.end()) return -1;
        std::stringstream ss;
        int device_id;
        ss << paramSet["device_id"];
        ss >> device_id;
        /*check device_id valid or not,FIXME*/
        return device_id;
    }

    bool DataSource::Open(ModuleParamSet paramSet) {
        if (paramSet.find("output_type") != paramSet.end()) {
            std::string out_type = paramSet["output_type"];
            if (out_type == "cpu") {
                param_.output_type_ = OUTPUT_CPU;
            }
            else if (out_type == "cuda") {
                param_.output_type_ = OUTPUT_CUDA;
            }
            else {
                LOG(ERROR) << "[source]:" << "output_type " << paramSet["output_type"] << " not supported";
                return false;
            }
            if (param_.output_type_ == OUTPUT_CUDA) {
                param_.device_id_ = GetDeviceId(paramSet);
                if (param_.device_id_ < 0) {
                    LOG(ERROR) << "[source]:" << "output_type CUDA : device_id must be set";
                    return false;
                }
            }
        }

        if (paramSet.find("interval") != paramSet.end()) {
            std::stringstream ss;
            int interval;
            ss << paramSet["interval"];
            ss >> interval;
            if (interval <= 0) {
                LOG(ERROR) << "[source]:" << "interval : invalid";
                return false;
            }
            param_.interval_ = interval;
        }

        if (paramSet.find("decoder_type") != paramSet.end()) {
            std::string dec_type = paramSet["decoder_type"];
            if (dec_type == "cpu") {
                param_.decoder_type_ = DECODER_CPU;
            }
            else if (dec_type == "cuda") {
                param_.decoder_type_ = DECODER_CUDA;
            }
            else {
                LOG(ERROR) << "[source]:" << "decoder_type " << paramSet["decoder_type"] << " not supported";
                return false;
            }
            if (dec_type == "cuda") {
                param_.device_id_ = GetDeviceId(paramSet);
                if (param_.device_id_ < 0) {
                    LOG(ERROR) << "[source]:" << "decoder_type CUDA : device_id must be set";
                    return false;
                }
            }
        }

        // TODO : use cuda

        if (paramSet.find("input_buf_number") != paramSet.end()) {
            std::string ibn_str = paramSet["input_buf_number"];
            std::stringstream ss;
            ss << paramSet["input_buf_number"];
            ss >> param_.input_buf_number_;
        }

        if (paramSet.find("output_buf_number") != paramSet.end()) {
            std::string obn_str = paramSet["output_buf_number"];
            std::stringstream ss;
            ss << paramSet["output_buf_number"];
            ss >> param_.output_buf_number_;
        }
        return true;
    }

    bool DataSource::Close() { RemoveSources(); return true; }

}  // namespace easysa