#include <libyuv.h>

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cmath>
#include <cstring>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>
#include <glog/logging.h>

#include "easysa_frame_va.hpp"
#include "easysa_module.hpp"

namespace easysa {

    DataFrame::~DataFrame() {
        cuda_data.reset();
        cpu_data.reset();

        if (nullptr != deAllocator_) {
            deAllocator_.reset();
        }
        if (nullptr != bgr_mat) {
            delete bgr_mat, bgr_mat = nullptr;
        }

    }
    /*
    namespace color_cvt {
        static
            cv::Mat BGRToBGR(const DataFrame& frame) {
            // const cv::Mat bgr(frame.height, frame.stride[0], CV_8UC3, const_cast<void*>(frame.data[0]->GetCpuData()));
            //return bgr(cv::Rect(0, 0, frame.width, frame.height)).clone();
            return frame.src_mat.clone();
        }

        static
            cv::Mat RGBToBGR(const DataFrame& frame) {
            const cv::Mat rgb(frame.height, frame.stride[0], CV_8UC3, const_cast<void*>(frame.data[0]->GetCpuData()));
            cv::Mat bgr;
            cv::cvtColor(rgb, bgr, cv::COLOR_RGB2BGR);
            return bgr(cv::Rect(0, 0, frame.width, frame.height)).clone();
        }

        static
            cv::Mat YUV420SPToBGR(const DataFrame& frame, bool nv21) {
            const uint8_t* y_plane = reinterpret_cast<const uint8_t*>(frame.data[0]->GetCpuData());
            const uint8_t* uv_plane = reinterpret_cast<const uint8_t*>(frame.data[1]->GetCpuData());
            int width = frame.width;
            int height = frame.height;
            int y_stride = frame.stride[0];
            int uv_stride = frame.stride[1];
            cv::Mat bgr(height, width, CV_8UC3);
            uint8_t* dst_bgr24 = bgr.data;
            int dst_stride = width * 3;
            // kYvuH709Constants make it to BGR
            if (nv21)
                libyuv::NV21ToRGB24Matrix(y_plane, y_stride, uv_plane, uv_stride,
                    dst_bgr24, dst_stride, &libyuv::kYvuH709Constants, width, height);
            else
                libyuv::NV12ToRGB24Matrix(y_plane, y_stride, uv_plane, uv_stride,
                    dst_bgr24, dst_stride, &libyuv::kYvuH709Constants, width, height);
            return bgr;
        }

        static inline
            cv::Mat NV12ToBGR(const DataFrame& frame) {
            return YUV420SPToBGR(frame, false);
        }

        static inline
            cv::Mat NV21ToBGR(const DataFrame& frame) {
            return YUV420SPToBGR(frame, true);
        }

        static inline
            cv::Mat FrameToImageBGR(const DataFrame& frame) {
            switch (frame.fmt) {
            case DataFormat::CN_PIXEL_FORMAT_BGR24:
                return BGRToBGR(frame);
            case DataFormat::CN_PIXEL_FORMAT_RGB24:
                return RGBToBGR(frame);
            case DataFormat::CN_PIXEL_FORMAT_YUV420_NV12:
                return NV12ToBGR(frame);
            case DataFormat::CN_PIXEL_FORMAT_YUV420_NV21:
                return NV21ToBGR(frame);
            default:
                LOG(ERROR) << "[frame]" << "Unsupport pixel format. fmt[" << static_cast<int>(frame.fmt) << "]";
            }
            // never be here
            abort();
            return cv::Mat();
        }

    }  // namespace color_cvt
    */
    cv::Mat* DataFrame::ImageBGR() {
        std::lock_guard<std::mutex> lk(mtx);
        if (bgr_mat != nullptr) {
            return bgr_mat;
        }
        bgr_mat = new (std::nothrow) cv::Mat();
        if(nullptr == bgr_mat) LOG(ERROR) << "DataFrame::ImageBGR() failed to alloc cv::Mat";
        // *bgr_mat = color_cvt::FrameToImageBGR(*this);
        *bgr_mat = src_mat.clone();
        return bgr_mat;
    }

    size_t DataFrame::GetPlaneBytes(int plane_idx) const {
        if (plane_idx < 0 || plane_idx >= GetPlanes()) return 0;
        switch (fmt) {
        case PIXEL_FORMAT_BGR24:
        case PIXEL_FORMAT_RGB24:
            return height * stride[0] * 3;
        case PIXEL_FORMAT_YUV420_NV12:
        case PIXEL_FORMAT_YUV420_NV21:
            if (0 == plane_idx)
                return height * stride[0];
            else if (1 == plane_idx)
                return std::ceil(1.0 * height * stride[1] / 2);
            else
                LOG(ERROR) << "[frame]"<< "plane index wrong.";
        default:
            return 0;
        }
        return 0;
    }

    size_t DataFrame::GetBytes() const {
        size_t bytes = 0;
        for (int i = 0; i < GetPlanes(); ++i) {
            bytes += GetPlaneBytes(i);
        }
        return bytes;
    }
 

    bool InferObject::AddAttribute(const std::string& key, const InferAttr& value) {
        std::lock_guard<std::mutex> lk(attribute_mutex_);
        if (attributes_.find(key) != attributes_.end()) return false;

        attributes_.insert(std::make_pair(key, value));
        return true;
    }

    bool InferObject::AddAttribute(const std::pair<std::string, InferAttr>& attribute) {
        std::lock_guard<std::mutex> lk(attribute_mutex_);
        if (attributes_.find(attribute.first) != attributes_.end()) return false;

        attributes_.insert(attribute);
        return true;
    }

    InferAttr InferObject::GetAttribute(const std::string& key) {
        std::lock_guard<std::mutex> lk(attribute_mutex_);
        if (attributes_.find(key) != attributes_.end()) return attributes_[key];

        return InferAttr();
    }

    bool InferObject::AddExtraAttribute(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lk(attribute_mutex_);
        if (extra_attributes_.find(key) != extra_attributes_.end()) return false;

        extra_attributes_.insert(std::make_pair(key, value));
        return true;
    }

    bool InferObject::AddExtraAttributes(const std::vector<std::pair<std::string, std::string>>& attributes) {
        std::lock_guard<std::mutex> lk(attribute_mutex_);
        bool ret = true;
        for (auto& attribute : attributes) {
            ret &= AddExtraAttribute(attribute.first, attribute.second);
        }
        return ret;
    }

    std::string InferObject::GetExtraAttribute(const std::string& key) {
        std::lock_guard<std::mutex> lk(attribute_mutex_);
        if (extra_attributes_.find(key) != extra_attributes_.end()) {
            return extra_attributes_[key];
        }
        return "";
    }

    bool InferObject::RemoveExtraAttribute(const std::string& key) {
        std::lock_guard<std::mutex> lk(attribute_mutex_);
        if (extra_attributes_.find(key) != extra_attributes_.end()) {
            extra_attributes_.erase(key);
        }
        return true;
    }

    StringPairs InferObject::GetExtraAttributes() {
        std::lock_guard<std::mutex> lk(attribute_mutex_);
        return StringPairs(extra_attributes_.begin(), extra_attributes_.end());
    }

    bool InferObject::AddFeature(const std::string& key, const InferFeature& feature) {
        std::lock_guard<std::mutex> lk(feature_mutex_);
        if (features_.find(key) != features_.end()) {
            return false;
        }
        features_.insert(std::make_pair(key, feature));
        return true;
    }

    InferFeature InferObject::GetFeature(const std::string& key) {
        std::lock_guard<std::mutex> lk(feature_mutex_);
        if (features_.find(key) != features_.end()) {
            return features_[key];
        }
        return InferFeature();
    }

    InferFeatures InferObject::GetFeatures() {
        std::lock_guard<std::mutex> lk(feature_mutex_);
        return InferFeatures(features_.begin(), features_.end());
    }

}  // namespace easysa