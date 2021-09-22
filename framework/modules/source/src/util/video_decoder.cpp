/*************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *************************************************************************/

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <glog/logging.h>

#include "easysa_common.hpp"
#include "video_decoder.hpp"

namespace easysa {

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

    // FFMPEG use AVCodecParameters instead of AVCodecContext
    // since from version 3.1(libavformat/version:57.40.100)
#define FFMPEG_VERSION_3_1 AV_VERSION_INT(57, 40, 100)

    //----------------------------------------------------------------------------
    // CPU decoder
    bool FFmpegCpuDecoder::Create(VideoInfo* info, ExtraDecoderInfo* extra) {
        AVCodec* dec = avcodec_find_decoder(info->codec_id);
        if (!dec) {
            LOG(ERROR) << "[source]: " << "[" << stream_id_ << "]: "
                << "avcodec_find_decoder failed";
            return false;
        }
        // init decoder
        instance_ = avcodec_alloc_context3(dec);
        if (!instance_) {
            LOG(ERROR) << "[source]: " << "[" << stream_id_ << "]: "
                << "Failed to do avcodec_alloc_context3";
            return false;
        }
        // av_codec_set_pkt_timebase(instance_, st->time_base);

        if (!extra->extra_info.empty()) {
            instance_->extradata = extra->extra_info.data();
            instance_->extradata_size = extra->extra_info.size();
        }

        if (avcodec_open2(instance_, dec, NULL) < 0) {
            LOG(ERROR) << "[source]: " << "[" << stream_id_ << "]: "
                << "Failed to open codec";
            return false;
        }
        av_frame_ = av_frame_alloc();
        if (!av_frame_) {
            LOG(ERROR) << "[source]: " << "[" << stream_id_ << "]: "
                << "Could not alloc frame";
            return false;
        }
        eos_got_.store(0);
        eos_sent_.store(0);
        return true;
    }

    void FFmpegCpuDecoder::Destroy() {
        LOG(ERROR) << "[source]: " << "[" << stream_id_ << "]: Begin destroy decoder";
        if (instance_ != nullptr) {
            if (!eos_sent_.load()) {
                while (this->Process(nullptr, true)) {
                }
            }
            while (!eos_got_.load()) {
                std::this_thread::yield();
            }
            avcodec_close(instance_), av_free(instance_);
            instance_ = nullptr;
        }
        if (av_frame_) {
            av_frame_free(&av_frame_);
            av_frame_ = nullptr;
        }
        LOG(ERROR) << "[source]: " << "[" << stream_id_ << "]: Finish destroy decoder";
    }

    bool FFmpegCpuDecoder::Process(VideoEsPacket* pkt) {
        if (pkt && pkt->data && pkt->len) {
            AVPacket packet;
            av_init_packet(&packet);
            packet.data = pkt->data;
            packet.size = pkt->len;
            packet.pts = pkt->pts;
            return Process(&packet, false);
        }
        return Process(nullptr, true);
    }

    bool FFmpegCpuDecoder::Process(AVPacket* pkt, bool eos) {
        if (eos) {
            AVPacket packet;
            av_init_packet(&packet);
            packet.size = 0;
            packet.data = NULL;

            LOG(INFO) << "[source]: " << "[" << stream_id_ << "]: Sent EOS packet to decoder";
            eos_sent_.store(1);
            // flush all frames ...
            int got_frame = 0;
            do {
                avcodec_decode_video2(instance_, av_frame_, &got_frame, &packet);
                if (got_frame) ProcessFrame(av_frame_);
            } while (got_frame);

            if (result_) {
                result_->OnDecodeEos();
            }
            eos_got_.store(1);
            return false;
        }
        int got_frame = 0;
        int ret = avcodec_decode_video2(instance_, av_frame_, &got_frame, pkt);
        if (ret < 0) {
            LOG(ERROR) << "[source]: " << "[" << stream_id_ << "]: "
                << "avcodec_decode_video2 failed, data ptr, size:" << pkt->data << ", " << pkt->size;
            return true;
        }
#if LIBAVFORMAT_VERSION_INT <= FFMPEG_VERSION_3_1
        av_frame_->pts = pkt->pts;
#endif
        if (got_frame) {
            ProcessFrame(av_frame_);
        }
        return true;
    }


    bool FFmpegCpuDecoder::ProcessFrame(AVFrame* frame) {
        if (instance_->pix_fmt != AV_PIX_FMT_YUV420P && instance_->pix_fmt != AV_PIX_FMT_YUVJ420P &&
            instance_->pix_fmt != AV_PIX_FMT_YUYV422) {
            LOG(ERROR) << "[source]:" << "[" << stream_id_ << "]: "
                << "FFmpegCpuDecoder only supports AV_PIX_FMT_YUV420P , AV_PIX_FMT_YUVJ420P and AV_PIX_FMT_YUYV422";
            return false;
        }
        DecodeFrame cn_frame;
        cn_frame.valid = true;
        cn_frame.width = frame->width;
        cn_frame.height = frame->height;
        cn_frame.pts = frame->pts;
        switch (instance_->pix_fmt) {
        case AV_PIX_FMT_YUV420P:
        {
            cn_frame.fmt = DecodeFrame::FMT_I420;
            cn_frame.planeNum = 3;
            break;
        }
        case AV_PIX_FMT_YUVJ420P:
        {
            cn_frame.fmt = DecodeFrame::FMT_J420;
            cn_frame.planeNum = 3;
            break;
        }
        case AV_PIX_FMT_YUYV422:
        {
            cn_frame.fmt = DecodeFrame::FMT_YUYV;
            cn_frame.planeNum = 1;
            break;
        }
        default:
        {
            cn_frame.fmt = DecodeFrame::FMT_INVALID;
            cn_frame.planeNum = 0;
            break;
        }
        }
        cn_frame.cuda_addr = false;
        for (int i = 0; i < cn_frame.planeNum; i++) {
            cn_frame.stride[i] = frame->linesize[i];
            cn_frame.plane[i] = frame->data[i];
        }
        cn_frame.buf_ref = nullptr;
        if (result_) {
            result_->OnDecodeFrame(&cn_frame);
        }
        return true;
    }

}  // namespace easysa