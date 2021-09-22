/*************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *************************************************************************/

#ifndef FRAMEWORK_CORE_INCLUDE_EASYSA_FRAME_HPP
#define FRAMEWORK_CORE_INCLUDE_EASYSA_FRAME_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "easysa_common.hpp"
#include "util/easysa_any.hpp"
#include "util/easysa_spinlock.hpp"

 /**
  *  @file easysa_frame.hpp
  *
  *  This file contains a declaration of the FrameInfo struct and its substructure.
  */
namespace easysa {

    class Module;
    class Pipeline;

    /**
     * An enumerated type that specifies the mask of DataFrame.
     */
    enum FrameFlag {
        FRAME_FLAG_EOS = 1 << 0,      ///< Identifies the end of data stream.
        FRAME_FLAG_INVALID = 1 << 1,  ///< Identifies the invalid of frame.
        FRAME_FLAG_REMOVED = 2 << 1   ///< Identifies the stream has been removed.
    };

    /**
     *  A structure holding the information of a frame.
     */
    class FrameInfo : private NonCopyable {
    public:
        /**
         * Creates a FrameInfo instance.
         *
         * @param stream_id The data stream alias. Identifies which data stream the frame data comes from.
         * @param eos  Whether this is the end of the stream. This parameter is set to false by default to
         *             create a FrameInfo instance. If you set this parameter to true,
         *             DataFrame::flags will be set to ``FRAME_FLAG_EOS``. Then, the modules
         *            do not have permission to process this frame. This frame should be handed over to
         *            the pipeline for processing.
         *
         * @return Returns ``shared_ptr`` of ``FrameInfo`` if this function has run successfully. Otherwise, returns NULL.
         */
        static std::shared_ptr<FrameInfo> Create(const std::string& stream_id, bool eos = false,
            std::shared_ptr<FrameInfo> payload = nullptr);
        ~FrameInfo();
        /**
         * Whether DataFrame is end of stream (EOS) or not.
         *
         * @return Returns true if the frame is EOS. Returns false if the frame is not EOS.
         */
        bool IsEos() { return (flags & easysa::FRAME_FLAG_EOS) ? true : false; }
        bool IsRemoved() { return (flags & easysa::FRAME_FLAG_REMOVED) ? true : false; }

        /**
        * Whether DataFrame is availability or not.
        *
        * @return true: frame is invalid, false: frame is valid.
        */
        bool IsInvalid() { return (flags & easysa::FRAME_FLAG_INVALID) ? true : false; }
        /**
         * Sets index (usually the index is a number) to identify stream. This is only used for distributing each stream
         * data to the appropriate thread.
         * We do not recommend SDK users to use this API because it will be removed later.
         *
         * @param index Number to identify stream.
         *
         * @return Returns true if the frame is EOS. Returns false if the frame is not EOS.
         */
        void SetStreamIndex(uint32_t index) { channel_idx = index; }
        // GetStreamIndex() will be removed later
        uint32_t GetStreamIndex() const { return channel_idx; }

        std::string stream_id;   ///< The data stream aliases where this frame is located to.
        int64_t timestamp = -1;  ///< The time stamp of this frame.
        size_t flags = 0;        ///< The mask for this frame, ``FrameFlag``.

        // user-defined DataFrame£¬InferResult etc...
        std::unordered_map<int, easysa::any> datas;
        easysa::SpinLock datas_lock_;

        // FrameInfo instance of parent pipeine
        std::shared_ptr<easysa::FrameInfo> payload = nullptr;

    private:
        /**
         * The below methods and members are used by the framework.
         */
        friend class Pipeline;
        mutable uint32_t channel_idx = INVALID_STREAM_IDX;        ///< The index of the channel, stream_index
        void SetModulesMask(uint64_t mask);
        uint64_t MarkPassed(Module* current);  // return changed mask
        uint64_t GetModulesMask();

    private:
        SpinLock mask_lock_;
        /* Identifies which modules have processed this data */
        uint64_t modules_mask_ = 0;

    private:
        FrameInfo() {}
        static easysa::SpinLock spinlock_;
        static std::unordered_map<std::string, int> stream_count_map_;

    public:
        static int flow_depth_;
    };

}  // namespace easysa

#endif // !FRAMEWORK_CORE_INCLUDE_EASYSA_FRAME_HPP
