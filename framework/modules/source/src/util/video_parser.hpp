/*************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *************************************************************************/
#ifndef MODULES_SOURCE_SRC_UTIL_VIDEO_PARSER_HPP_
#define MODULES_SOURCE_SRC_UTIL_VIDEO_PARSER_HPP_

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#ifdef HAVE_FFMPEG_AVDEVICE
#include <libavdevice/avdevice.h>
#endif
#ifdef __cplusplus
}
#endif

#include <memory>
#include <string>
#include <vector>
#include <cstring>

/*
*
* 1. open camera
* 2. read packet to VideoInfo
* 3. deal VideoInfeo to frame
* 4. create FrameInfo
*/
namespace easysa {

	struct VideoInfo {
		AVCodecID codec_id;
		int progressive;
		std::vector<unsigned char> extra_data;
	};

	/* data == null && len == 0  indicates EOS
	*/
	struct VideoEsFrame {
		uint8_t* data = nullptr;
		size_t len = 0;
		int64_t pts = 0;
		uint32_t flags = 0;
		enum { FLAG_KEY_FRAME = 0x01 };
		bool IsEos();
	};

	inline
		bool VideoEsFrame::IsEos() {
		return data == nullptr && len == 0;
	}

	class IParserResult {
	public:
		virtual ~IParserResult() = default;
		virtual void OnParserInfo(VideoInfo* info) = 0;
		virtual void OnParserFrame(VideoEsFrame* frame) = 0;
	};

	struct VideoEsPacket {
		uint8_t* data = nullptr;
		size_t len = 0;
		int64_t pts = -1;
	};

	// FFmpeg demuxer and parser
	class FFParserImpl;
	class FFParser {
	public:
		explicit FFParser(const std::string& stream_id);
		~FFParser();
		int Open(const std::string& url, IParserResult* result);
		void Close();
		int Parse();

	private:
		FFParser(const FFParser&) = delete;
		FFParser(FFParser&&) = delete;
		FFParser& operator=(const FFParser&) = delete;
		FFParser& operator=(FFParser&&) = delete;
		FFParserImpl* impl_ = nullptr;
	};

}  // namespace easysa
#endif // !MODULES_SOURCE_SRC_UTIL_VIDEO_PARSER_HPP_