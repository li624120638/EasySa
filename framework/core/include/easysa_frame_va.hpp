/*************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *************************************************************************/

#ifndef MODULES_EASYSA_FRAME_VA_HPP_
#define MODULES_EASYSA_FRAME_VA_HPP_

 /**
  *  @file easysa_frame_va.hpp
  *
  *  This file contains a declaration of the FrameData & InferObject struct and its substructure.
  */
#include <opencv2/opencv.hpp>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "easysa_allocator.hpp"
#include "easysa_common.hpp"
#include "easysa_frame.hpp"
#include "util/easysa_any.hpp"


#ifndef MAX_PLANES
#define MAX_PLANES 6
#endif

#ifndef ROUND_UP
#define ROUND_UP(addr, boundary) (((uint64_t)(addr) + (boundary)-1) & ~((boundary)-1))
#endif

namespace easysa {
    /**
     * An enumerated type that is used to
     * identify the pixel format of the data in DataFrame.
     */
    typedef enum {
        INVALID = -1,                  ///< This frame is invalid.
        PIXEL_FORMAT_YUV420_NV21 = 0,  ///< This frame is in the YUV420SP(NV21) format.
        PIXEL_FORMAT_YUV420_NV12,      ///< This frame is in the YUV420sp(NV12) format.
        PIXEL_FORMAT_BGR24,            ///< This frame is in the BGR24 format.
        PIXEL_FORMAT_RGB24,            ///< This frame is in the RGB24 format.
        PIXEL_FORMAT_ARGB32,           ///< This frame is in the ARGB32 format.
        PIXEL_FORMAT_ABGR32,           ///< This frame is in the ABGR32 format.
        PIXEL_FORMAT_RGBA32,           ///< This frame is in the RGBA32 format.
        PIXEL_FORMAT_BGRA32            ///< This frame is in the BGRA32 format.
    } DataFormat;

    /**
     * Identifies if the DataFrame data is allocated by CPU or CUDA.
     */
    struct DevContext {
        enum DevType {
            INVALID = -1,        ///< Invalid device type.
            CPU = 0,             ///< The data is allocated by CPU.
            CUDA = 1,             ///< The data is allocated by CUDA.
        } dev_type = INVALID;  ///< Device type.
        int dev_id = 0;        ///< Ordinal device ID.
        int ddr_channel = 0;   ///< Ordinal channel ID for CUDA. The value should be in the range [0, 4).
    };

    /**
     * Identifies memory shared type for multi-process.
     */
    enum MemMapType {
        MEMMAP_INVALID = 0,  ///< Invalid memory shared type.
        MEMMAP_CPU = 1,      ///< CPU memory is shared.
        MEMMAP_CUDA = 2       ///< CUDA memory is shared.
    };

    /**
     * Gets image plane number by a specified image format.
     *
     * @param
     *   fmt The format of the image.
     *
     * @return
     * @retval 0: Unsupported image format.
     * @retval >0: Image plane number.
     */
    inline int GetPlanesByFormat(DataFormat fmt) {
        switch (fmt) {
        case PIXEL_FORMAT_BGR24:
        case PIXEL_FORMAT_RGB24:
            return 1;
        case PIXEL_FORMAT_YUV420_NV12:
        case PIXEL_FORMAT_YUV420_NV21:
            return 2;
        default:
            return 0;
        }
        return 0;
    }

    /**
     * Dedicated deallocator for the Decoder buffer.
     */
    class IDataDeallocator {
    public:
        virtual ~IDataDeallocator() {}
    };

    /**
     * The structure holding a data frame and the frame description.
     */
    class DataFrame : public NonCopyable {
    public:
        DataFrame() = default;
        ~DataFrame();
        std::vector<uint32_t> skip_status;
        uint64_t frame_id = -1;  ///< The frame index that incremented from 0.

        /**
         * The source data information. You need to set the information below before calling CopyToSyncMem().
         */
        DataFormat fmt;                                          ///< The format of the frame.
        int width;                                                 ///< The width of the frame.
        int height;                                                ///< The height of the frame.
        int stride[MAX_PLANES];                                 ///< The strides of the frame.
        DevContext ctx;                                            ///< The device context of SOURCE data (ptr_cuda/ptr_cpu).
        void* ptr_cuda[MAX_PLANES];                              ///< The CUDA data addresses for planes.
        void* ptr_cpu[MAX_PLANES];                              ///< The CPU data addresses for planes.
        std::unique_ptr<IDataDeallocator> deAllocator_ = nullptr;  ///< The dedicated deallocator for Decoder buffer.

        /* The 'dst_device_id' is for SyncedMemory.
        */
        std::atomic<int> dst_device_id{ -1 };                        ///< The device context of SyncMemory.

        /**
         * Gets plane count for a specified frame.
         *
         * @return Returns the plane count of this frame.
         */
        int GetPlanes() const { return GetPlanesByFormat(fmt); }

        /**
         * Gets the number of bytes in a specified plane.
         *
         * @param plane_idx The index of the plane. The index increments from 0.
         *
         * @return Returns the number of bytes in the plane.
         */
        size_t GetPlaneBytes(int plane_idx) const;

        /**
         * Gets the number of bytes in a frame.
         *
         * @return Returns the number of bytes in a frame.
         */
        size_t GetBytes() const;
        const std::vector<uint32_t>& GetSkipStatus() const{
            return skip_status;
        }
    public:
        std::shared_ptr<void> cpu_data = nullptr;  ///< CPU data pointer.
        std::shared_ptr<void> cuda_data = nullptr;  ///< A pointer to the CUDA data.
        // std::unique_ptr<easysa::SyncedMemory> data[MAX_PLANES];  ///< Synchronizes data helper.
    public:
    cv::Mat src_mat;
  /**
   * Converts data from RGB to BGR. Called after CopyToSyncMem() is invoked.
   *
   * If data is not RGB image but BGR, YUV420NV12 or YUV420NV21 image, its color mode will not be converted.
   *
   * @return Returns data with opencv mat type.
   */
    cv::Mat* ImageBGR();
        bool HasBGRImage() {
            if (bgr_mat) return true;
            return false;
        }
    private:
        cv::Mat* bgr_mat = nullptr;
    private:
        std::mutex mtx;
    };                                 // struct DataFrame

    /**
     * A structure holding the bounding box for detection information of an object.
     * Normalized coordinates.
     */
    struct InferBoundingBox {
        float x;  ///< The x-axis coordinate in the upper left corner of the bounding box.
        float y;  ///< The y-axis coordinate in the upper left corner of the bounding box.
        float w;  ///< The width of the bounding box.
        float h;  ///< The height of the bounding box.
    };

    /**
     * A structure holding the classification properties of an object.
     */
    struct InferAttr {
        int id = -1;      ///< The unique ID of the classification. The value -1 is invalid.
        int value = -1;   ///< The label value of the classification.
        float score = 0;  ///< The label score of the classification.
    };

    /**
     * The feature value for one object.
     */
    using InferFeature = std::vector<float>;

    /**
     * All kinds of features for one object.
     */
    using InferFeatures = std::vector<std::pair<std::string, InferFeature>>;

    /**
     * String pairs for extra attributes.
     */
    using StringPairs = std::vector<std::pair<std::string, std::string>>;

    /**
     * A structure holding the information for an object.
     */
    struct InferObject {
    public:
        std::string id;           ///< The ID of the classification (label value).
        std::string track_id;     ///< The tracking result.
        float score;              ///< The label score.
        InferBoundingBox bbox;  ///< The object normalized coordinates.
        std::unordered_map<int, any> datas;  ///< user-defined structured information.

        /**
         * Adds the key of an attribute to a specified object.
         *
         * @param key The Key of the attribute you want to add to. See GetAttribute().
         * @param value The value of the attribute.
         *
         * @return Returns true if the attribute has been added successfully. Returns false if the attribute
         *         already existed.
         *
         * @note This is a thread-safe function.
         */
        bool AddAttribute(const std::string& key, const InferAttr& value);

        /**
         * Adds the key pairs of an attribute to a specified object.
         *
         * @param attribute The attribute pair (key, value) to be added.
         *
         * @return Returns true if the attribute has been added successfully. Returns false if the attribute
         *         has already existed.
         *
         * @note This is a thread-safe function.
         */
        bool AddAttribute(const std::pair<std::string, InferAttr>& attribute);

        /**
         * Gets an attribute by key.
         *
         * @param key The key of an attribute you want to query. See AddAttribute().
         *
         * @return Returns the attribute key. If the attribute
         *         does not exist, CNInferAttr::id will be set to -1.
         *
         * @note This is a thread-safe function.
         */
        InferAttr GetAttribute(const std::string& key);

        /**
         * Adds the key of the extended attribute to a specified object.
         *
         * @param key The key of an attribute. You can get this attribute by key. See GetExtraAttribute().
         * @param value The value of the attribute.
         *
         * @return Returns true if the attribute has been added successfully. Returns false if the attribute
         *         has already existed in the object.
         *
         * @note This is a thread-safe function.
         */
        bool AddExtraAttribute(const std::string& key, const std::string& value);

        /**
         * Adds the key pairs of the extended attributes to a specified object.
         *
         * @param attributes Attributes to be added.
         *
         * @return Returns true if the attribute has been added successfully. Returns false if the attribute
         *         has already existed.
         * @note This is a thread-safe function.
         */
        bool AddExtraAttributes(const std::vector<std::pair<std::string, std::string>>& attributes);

        /**
         * Gets an extended attribute by key.
         *
         * @param key The key of an identified attribute. See AddExtraAttribute().
         *
         * @return Returns the attribute that is identified by the key. If the attribute
         *         does not exist, returns NULL.
         *
         * @note This is a thread-safe function.
         */
        std::string GetExtraAttribute(const std::string& key);

        /**
         * Removes an attribute by key.
         *
         * @param key The key of an attribute you want to remove. See AddAttribute.
         *
         * @return Return true.
         *
         * @note This is a thread-safe function.
         */
        bool RemoveExtraAttribute(const std::string& key);

        /**
         * Gets all extended attributes of an object.
         *
         * @return Returns all extended attributes.
         *
         * @note This is a thread-safe function.
         */
        StringPairs GetExtraAttributes();

        /**
         * Adds the key of feature to a specified object.
         *
         * @param key The Key of feature you want to add the feature to. See GetFeature.
         * @param value The value of the feature.
         *
         * @return Returns true if the feature is added successfully. Returns false if the feature
         *         identified by the key already exists.
         *
         * @note This is a thread-safe function.
         */
        bool AddFeature(const std::string& key, const InferFeature& feature);

        /**
         * Gets an feature by key.
         *
         * @param key The key of an feature you want to query. See AddFeature.
         *
         * @return Return the feature of the key. If the feature identified by the key
         *         is not exists, CNInferFeature will be empty.
         *
         * @note This is a thread-safe function.
         */
        InferFeature GetFeature(const std::string& key);

        /**
         * Gets the features of an object.
         *
         * @return Returns the features of an object.
         *
         * @note This is a thread-safe function.
         */
        InferFeatures GetFeatures();

        void* user_data_ = nullptr;  ///< User data. You can store your own data in this parameter.

    private:
        std::unordered_map<std::string, InferAttr> attributes_;
        std::unordered_map<std::string, std::string> extra_attributes_;
        std::unordered_map<std::string, InferFeature> features_;
        std::mutex attribute_mutex_;
        std::mutex feature_mutex_;
    };

    struct InferObjs : public NonCopyable {
        std::vector<std::shared_ptr<InferObject>> objs_;  /// the objects storing inference results
        std::mutex mutex_;   /// mutex of CNInferObjs
    };

    /**
     * A structure holding the information for inference input & outputs(raw).
     */
    struct InferData {
        // infer input
        DataFormat input_fmt_;
        int input_width_;
        int input_height_;
        std::shared_ptr<void> input_cpu_addr_;  //< this pointer means one input, a frame may has many inputs for a model
        size_t input_size_;

        // infer output
        std::vector<std::shared_ptr<void>> output_cpu_addr_;  //< many outputs for one input
        size_t output_size_;
        size_t output_num_;
    };

    struct InferDatas : public NonCopyable {
        std::unordered_map<std::string, std::vector<std::shared_ptr<InferData>>> datas_map_;
        std::mutex mutex_;
    };

    /*
     * user-defined data structure: Key-value
     *   key type-- int
     *   value type -- cnstream::any, since we store it in an map, std::share_ptr<T> should be used
     */
    static constexpr int DataFramePtrKey = 0;
    using DataFramePtr = std::shared_ptr<DataFrame>;

    static constexpr int InferObjsPtrKey = 1;
    using InferObjsPtr = std::shared_ptr<InferObjs>;
    using ObjsVec = std::vector<std::shared_ptr<InferObject>>;

    static constexpr int InferDatasPtrKey = 2;
    using InferDatasPtr = std::shared_ptr<InferDatas>;


    // helpers
    static inline
        DataFramePtr GetDataFramePtr(std::shared_ptr<FrameInfo> frameInfo) {
        SpinLockGuard guard(frameInfo->datas_lock_);
        return easysa::any_cast<DataFramePtr>(frameInfo->datas[DataFramePtrKey]);
    }

    static inline
        InferObjsPtr GetInferObjsPtr(std::shared_ptr<FrameInfo> frameInfo) {
        SpinLockGuard guard(frameInfo->datas_lock_);
        return easysa::any_cast<InferObjsPtr>(frameInfo->datas[InferObjsPtrKey]);
    }

    static inline
        InferDatasPtr GetInferDatasPtr(std::shared_ptr<FrameInfo> frameInfo) {
        SpinLockGuard guard(frameInfo->datas_lock_);
        return easysa::any_cast<InferDatasPtr>(frameInfo->datas[InferDatasPtrKey]);
    }

}  // namespace easysa
#endif // !MODULES_EASYSA_FRAME_VA_HPP_
