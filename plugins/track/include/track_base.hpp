#ifndef PLUGINS_TRACK_BASE_HPP_
#define PLUGINS_TRACK_BASE_HPP_
#include "components_common.hpp"
/*
*	@file track_base.hpp
*	@brief this file use to construct track plugins
*/
namespace easysa {
	namespace plugins {
		using namespace easysa::components;
		using Objects = std::vector<DetectObject>;
		class TrackFrame {
			/*
			* the raw data of a frame
			*/
			void* data;
			uint32_t width;
			uint32_t height;
			uint64_t track_id;
			int device_id;
			enum class ColorSpace{
				GRAY, BGR24, RGB24
			}format;
			enum class DevType {
				CPU, CUDA
			} dev_type;
		}; // class TrackFrame

		class TrackBase {
		public:
			virtual ~TrackBase() {}
			virtual void UpdateTrackStatus(const TrackFrame& frame, Objects& detects) noexcept(false) = 0;
		}; // class TrackBase
	} // namespace plugins
} // namespace easysa

#endif // !PLUGINS_TRACK_BASE_HPP_
