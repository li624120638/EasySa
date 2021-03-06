/*************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *************************************************************************/
#include <iostream>
#include <mutex>
#include <vector>
#if defined(__linux) || defined(__unix)
#include <ctime>
#elif defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif
#include <glog/logging.h>
#include "video_parser.hpp"

namespace easysa {

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

    /**
     * FFMPEG use FF_AV_INPUT_BUFFER_PADDING_SIZE instead of
     * FF_INPUT_BUFFER_PADDING_SIZE since from version 2.8
     * (avcodec.h/version:56.56.100)
     * */

#define FFMPEG_VERSION_2_8 AV_VERSION_INT(56, 56, 100)

     /**
      * FFMPEG use AVCodecParameters instead of AVCodecContext
      * since from version 3.1(libavformat/version:57.40.100)
      **/

#define FFMPEG_VERSION_3_1 AV_VERSION_INT(57, 40, 100)

    struct local_ffmpeg_init {
        local_ffmpeg_init() {
            avcodec_register_all();
            av_register_all();
            avformat_network_init();
        }
    };
    static local_ffmpeg_init init_ffmpeg;

#if defined(__linux) || defined(__unix)
    static uint64_t GetTickCount() {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
    }
#endif

    class FFParserImpl {
    public:
        explicit FFParserImpl(const std::string& stream_id) : stream_id_(stream_id) {}
        ~FFParserImpl() = default;

        static int InterruptCallBack(void* ctx) {
            FFParserImpl* demux = reinterpret_cast<FFParserImpl*>(ctx);
            if (demux->CheckTimeOut(GetTickCount())) {
                return 1;
            }
            return 0;
        }

        bool CheckTimeOut(uint64_t ul_current_time) {
            if ((ul_current_time - last_receive_frame_time_) / 1000 > max_receive_time_out_) {
                return true;
            }
            return false;
        }

        int Open(const std::string& url, IParserResult* result) {
            std::unique_lock<std::mutex> guard(mutex_);
            if (!result) return -1;
            result_ = result;
            // 1. open camera
            fmt_ctx_ = avformat_alloc_context();
            if (!fmt_ctx_) return -1;
            url_name_ = url;
            AVInputFormat* ifmt = NULL;
#if HAVE_FFMPEG_AVDEVICE
            // open v4l2 input
#if defined(__linux) || defined(__unix)
            ifmt = av_find_input_format("video4linux2");
            if (!ifmt) {
                LOG(ERROR) << "[source]: " << "[" << stream_id_ << "]: Could not find v4l2 format.";
                return false;
            }
#elif defined(_WIN32) || defined(_WIN64)
            ifmt = av_find_input_format("dshow");
            if (!ifmt) {
                LOG(ERROR) << "[source]: " << "[" << stream_id_ << "]: Could not find dshow.";
                return false;
            }
#else
            LOG(ERROR) << "[source]: " << "[" << stream_id_ << "]: Unsupported Platform";
            return false;
#endif
#else
            int ret_code;
            av_dict_set(&options_, "buffer_size", "1024000", 0);
            av_dict_set(&options_, "max_delay", "500000", 0);

#endif
            // open input
            ret_code = avformat_open_input(&fmt_ctx_, url_name_.c_str(), ifmt, &options_);
            if (0 != ret_code) {
                LOG(INFO) << "[source]: " << "[" << stream_id_ << "]: Couldn't open input stream -- " << url_name_;
                return -1;
            }
            // find video stream information
            ret_code = avformat_find_stream_info(fmt_ctx_, NULL);
            if (ret_code < 0) {
                LOG(ERROR) << "[source]: " << "[" << stream_id_ << "]: Couldn't find stream information -- " << url_name_;
                return -1;
            }
            // 2. init read packet handler
            // fill info (get packet data)
            VideoInfo video_info;
            VideoInfo* info = &video_info;
            int video_index = -1;
            AVStream* st = nullptr;
            // find video stream
            for (uint32_t loop_i = 0; loop_i < fmt_ctx_->nb_streams; loop_i++) {
                st = fmt_ctx_->streams[loop_i];
#if LIBAVFORMAT_VERSION_INT >= FFMPEG_VERSION_3_1
                if (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
#else
                if (st->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
#endif
                    video_index = loop_i;
                    break;
                }
            }
            if (video_index == -1) {
                LOG(INFO) << "[source]: " << "[" << stream_id_ << "]: Couldn't find a video stream -- " << url_name_;
                return -1;
            }
            video_index_ = video_index;

#if LIBAVFORMAT_VERSION_INT >= FFMPEG_VERSION_3_1
            info->codec_id = st->codecpar->codec_id;
            int field_order = st->codecpar->field_order;
#else
            info->codec_id = st->codec->codec_id;
            int field_order = st->codec->field_order;
#endif

            /*At this moment, if the demuxer does not set this value (avctx->field_order == UNKNOWN),
            *   the input stream will be assumed as progressive one.
            */
            switch (field_order) {
            case AV_FIELD_TT:
            case AV_FIELD_BB:
            case AV_FIELD_TB:
            case AV_FIELD_BT:
                info->progressive = 0;
                break;
            case AV_FIELD_PROGRESSIVE:  // fall through
            default:
                info->progressive = 1;
                break;
            }

#if LIBAVFORMAT_VERSION_INT >= FFMPEG_VERSION_3_1
            uint8_t* extradata = st->codecpar->extradata;
            int extradata_size = st->codecpar->extradata_size;
#else
            unsigned char* extradata = st->codec->extradata;
            int extradata_size = st->codec->extradata_size;
#endif
            if (extradata && extradata_size) {
                info->extra_data.resize(extradata_size);
                memcpy(info->extra_data.data(), extradata, extradata_size);
            }
            // bitstream filter
            bsf_ctx_ = nullptr;
            if (strstr(fmt_ctx_->iformat->name, "mp4") || strstr(fmt_ctx_->iformat->name, "flv") ||
                strstr(fmt_ctx_->iformat->name, "matroska")) {
                if (AV_CODEC_ID_H264 == info->codec_id) {
                    bsf_ctx_ = av_bitstream_filter_init("h264_mp4toannexb");
                }
                else if (AV_CODEC_ID_HEVC == info->codec_id) {
                    bsf_ctx_ = av_bitstream_filter_init("hevc_mp4toannexb");
                }
                else {
                    bsf_ctx_ = nullptr;
                }
            }
            if (result_) {
                result_->OnParserInfo(info);
            }
            av_init_packet(&packet_);
            first_frame_ = true;
            eos_reached_ = false;
            open_success_ = true;
            return 0;
            }

        void Close() {
            std::unique_lock<std::mutex> guard(mutex_);
            if (fmt_ctx_) {
                avformat_close_input(&fmt_ctx_);
                avformat_free_context(fmt_ctx_);
                av_dict_free(&options_);
                if (bsf_ctx_) {
                    av_bitstream_filter_close(bsf_ctx_);
                    bsf_ctx_ = nullptr;
                }
                options_ = nullptr;
                fmt_ctx_ = nullptr;
            }
            first_frame_ = true;
            eos_reached_ = false;
            open_success_ = false;
        }

        int Parse() {
            std::unique_lock<std::mutex> guard(mutex_);
            if (eos_reached_ || !open_success_) {
                return -1;
            }
            while (true) {
                last_receive_frame_time_ = GetTickCount();
                if (av_read_frame(fmt_ctx_, &packet_) < 0) {
                    if (result_) {
                        result_->OnParserFrame(nullptr);
                    }
                    eos_reached_ = true;
                    return -1;
                }

                if (packet_.stream_index != video_index_) {
                    av_packet_unref(&packet_);
                    continue;
                }

                AVStream* vstream = fmt_ctx_->streams[video_index_];
                if (first_frame_) {
                    if (packet_.flags & AV_PKT_FLAG_KEY) {
                        first_frame_ = false;
                    }
                    else {
                        av_packet_unref(&packet_);
                        continue;
                    }
                }

                if (bsf_ctx_) {
                    av_bitstream_filter_filter(bsf_ctx_, vstream->codec, NULL, &packet_.data, &packet_.size,
                        packet_.data, packet_.size, 0);
                }
                // find pts information
                if (AV_NOPTS_VALUE == packet_.pts && find_pts_) {
                    find_pts_ = false;
                    // LOGW(SOURCE) << "Didn't find pts informations, "
                    //              << "use ordered numbers instead. "
                    //              << "stream url: " << url_name_.c_str();
                }
                else if (AV_NOPTS_VALUE != packet_.pts) {
                    find_pts_ = true;
                    packet_.pts = av_rescale_q(packet_.pts, vstream->time_base, { 1, 90000 });
                }
                if (!find_pts_) {
                    packet_.pts = pts_++;  // FIXME
                }

                if (result_) {
                    VideoEsFrame frame;
                    frame.flags = packet_.flags;
                    frame.data = packet_.data;
                    frame.len = packet_.size;
                    frame.pts = packet_.pts;
                    result_->OnParserFrame(&frame);
                }
                if (bsf_ctx_) {
                    av_freep(&packet_.data);
                }
                av_packet_unref(&packet_);
                return 0;
            }
        }

    private:
        AVFormatContext* fmt_ctx_ = nullptr;
        AVBitStreamFilterContext* bsf_ctx_ = nullptr;
        AVDictionary* options_ = NULL;
        bool first_frame_ = true;
        int video_index_ = -1;
        uint64_t last_receive_frame_time_ = 0;
        uint8_t max_receive_time_out_ = 3;
        bool find_pts_ = false;
        uint64_t pts_ = 0;
        std::string stream_id_ = "";
        std::string url_name_;
        IParserResult* result_ = nullptr;
        AVPacket packet_;
        bool eos_reached_ = false;
        bool open_success_ = false;
        std::mutex mutex_;
        };  // class FFmpegDemuxerImpl  // NOLINT

    FFParser::FFParser(const std::string& stream_id) {
        impl_ = new FFParserImpl(stream_id);
    }

    FFParser::~FFParser() {
        if (impl_) delete impl_, impl_ = nullptr;
    }

    int FFParser::Open(const std::string& url, IParserResult* result) {
        if (impl_) {
            return impl_->Open(url, result);
        }
        return -1;
    }

    void FFParser::Close() {
        if (impl_) {
            impl_->Close();
        }
    }

    int FFParser::Parse() {
        if (impl_) {
            impl_->Parse();
        }
        return -1;
    }

    }  // namespace easysa