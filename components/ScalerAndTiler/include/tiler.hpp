#ifndef COMPONENTS_TILER_HPP__
#define COMPONENTS_TILER_HPP__

#include <cstdint>
#include <atomic>
#include <mutex>
#include <vector>

#include "scaler.hpp"

namespace easysa {
namespace components {
	/*
	* @brief Tiler use to make image buffer to canvas
	*/
	class Tiler {
	public:
		using ColorFormat = Scaler::ColorFormat;
		using Buffer = Scaler::Buffer;
		using Rect = Scaler::Rect;

		explicit Tiler(uint32_t rows, uint32_t cols, ColorFormat color, uint32_t width, uint32_t height, uint32_t stride = 0);
		explicit Tiler(const std::vector<Rect>& grids, ColorFormat color, uint32_t width, uint32_t height, uint32_t stride = 0);
		~Tiler();

		bool Blit(const Buffer* buffer, int position);
		Buffer* GetCanvas(Buffer* buffer = nullptr);
		void ReleaseCanvas();

	private:
		void Init();
		void DumpCanvas();

		uint32_t rows_, cols_;
		std::vector<Rect> grids_;
		ColorFormat color_;
		uint32_t width_, height_, stride_;

		std::mutex mtx_;
		int last_position_ = 0;
		int grid_buffer_count_ = 0;
		std::mutex buf_mtx_;
		std::atomic<int> canvas_index_{ 0 };
		std::atomic<bool> canvas_locked_{ false };
		int canvas_diff_ = 0;
		Buffer canvas_buffers_[2];
	};
} // namespace components
}  // namespace easysa

#endif  // COMPONENTS_TILER_HPP__