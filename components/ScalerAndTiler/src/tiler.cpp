#include <memory>

#include <glog/logging.h>
#include <opencv2/opencv.hpp>

#include "tiler.hpp"

namespace easysa {
    namespace components {
    static thread_local std::unique_ptr<uint8_t> tl_grid_buffer = nullptr;
    static thread_local uint32_t tl_grid_buffer_size = 0;

    Tiler::Tiler(uint32_t rows, uint32_t cols, ColorFormat color, uint32_t width, uint32_t height, uint32_t stride)
        : rows_(rows), cols_(cols), color_(color), width_(width), height_(height), stride_(stride) {
        grids_.clear();
        Init();
    }

    Tiler::Tiler(const std::vector<Rect>& grids, ColorFormat color, uint32_t width, uint32_t height, uint32_t stride)
        : grids_(grids), color_(color), width_(width), height_(height), stride_(stride) {
        Init();
    }

    void Tiler::Init() {
        uint32_t buffer_size;
        uint32_t stride = stride_ < width_ ? width_ : stride_;
        if (color_ <= ColorFormat::YUV_NV21) {
            buffer_size = stride * height_ * 3 / 2;
        }
        else if (color_ <= ColorFormat::RGB) {
            buffer_size = stride * height_ * 3;
        }
        else if (color_ <= ColorFormat::ARGB) {
            buffer_size = stride * height_ * 4;
        }
        else {
            LOG(WARNING) << "Tiler::Init() unsupported color format, set to BGR default";
            color_ = ColorFormat::BGR;
            buffer_size = stride * height_ * 3;
        }
        for (int i = 0; i < 2; i++) {
            memset(canvas_buffers_ + i, 0, sizeof(Buffer));
            uint8_t* buffer = new uint8_t[buffer_size];
            memset(buffer, 0, buffer_size);
            if (color_ == ColorFormat::YUV_I420) {
                canvas_buffers_[i].data[0] = buffer;
                canvas_buffers_[i].data[1] = buffer + stride * height_;
                canvas_buffers_[i].data[2] = buffer + stride * height_ * 5 / 4;
                canvas_buffers_[i].stride[0] = stride;
                canvas_buffers_[i].stride[1] = stride / 2;
                canvas_buffers_[i].stride[2] = stride / 2;
                memset(canvas_buffers_[i].data[1], 0x80, buffer_size / 3);
            }
            else if (color_ <= ColorFormat::YUV_NV21) {
                canvas_buffers_[i].data[0] = buffer;
                canvas_buffers_[i].data[1] = buffer + stride * height_;
                canvas_buffers_[i].stride[0] = stride;
                canvas_buffers_[i].stride[1] = stride;
                memset(canvas_buffers_[i].data[1], 0x80, buffer_size / 3);
            }
            else if (color_ <= ColorFormat::RGB) {
                canvas_buffers_[i].data[0] = buffer;
                canvas_buffers_[i].stride[0] = stride * 3;
            }
            else {
                canvas_buffers_[i].data[0] = buffer;
                canvas_buffers_[i].stride[0] = stride * 4;
            }
            canvas_buffers_[i].width = width_;
            canvas_buffers_[i].height = height_;
            canvas_buffers_[i].color = color_;
            canvas_buffers_[i].device_id = -1;
        }

        if (grids_.empty()) {
            if (cols_ < 1) cols_ = 1;
            if (rows_ < 1) rows_ = 1;
            uint32_t grid_number = cols_ * rows_;
            uint32_t grid_x = 0, grid_y = 0, grid_w, grid_h;
            grid_w = width_ / cols_;
            grid_h = height_ / rows_;
            uint32_t last_w_minus_1 = 0, last_h_minus_1 = 0;
            if (color_ <= ColorFormat::YUV_NV21) {
                for (uint32_t i = 0; i < grid_number; i++) {
                    Rect rect;
                    rect.x = grid_x;
                    rect.y = grid_y;
                    rect.w = (i % cols_ < width_% cols_) ? (grid_w + 1) : grid_w;
                    rect.h = (i / cols_ < height_% rows_) ? (grid_h + 1) : grid_h;
                    grids_.push_back(rect);
                    rect.w += last_w_minus_1;
                    rect.h += last_h_minus_1;
                    last_w_minus_1 = (rect.w % 2) ? (rect.w--, 1) : 0;
                    last_h_minus_1 = (rect.h % 2) ? (rect.h--, 1) : 0;
                    if ((grid_x + rect.w) >= width_) {
                        grid_x = 0;
                        last_w_minus_1 = 0;
                        grid_y += rect.h;
                    }
                    else {
                        grid_x += rect.w;
                    }
                }
            }
            else {
                for (uint32_t i = 0; i < grid_number; i++) {
                    Rect rect;
                    rect.x = grid_x;
                    rect.y = grid_y;
                    rect.w = (i % cols_ < width_% cols_) ? (grid_w + 1) : grid_w;
                    rect.h = (i / cols_ < height_% rows_) ? (grid_h + 1) : grid_h;
                    grids_.push_back(rect);
                    if ((grid_x + rect.w) >= width_) {
                        grid_x = 0;
                        grid_y += rect.h;
                    }
                    else {
                        grid_x += rect.w;
                    }
                }
            }
        }
        else {
            for (auto& grid : grids_) {
                if ((grid.x + grid.w) > static_cast<int>(width_)) grid.w = width_ - grid.x;
                if ((grid.y + grid.h) > static_cast<int>(height_)) grid.h = height_ - grid.y;
            }
        }

        Scaler::SetCarrier(Scaler::OPENCV);
    }

    Tiler::~Tiler() {
        std::lock_guard<std::mutex> lk(buf_mtx_);
        for (int i = 0; i < 2; i++) {
            if (canvas_buffers_[i].data[0]) delete[] canvas_buffers_[i].data[0];
        }
        grids_.clear();
        grid_buffer_count_ = 0;
        canvas_index_ = 0;
        canvas_diff_ = 0;
    }

    bool Tiler::Blit(const Buffer* buffer, int position) {
        if (static_cast<size_t>(position) >= grids_.size()) return false;
        mtx_.lock();
        if (position < 0) position = (last_position_ + 1) % grids_.size();
        last_position_ = position;

        Rect* grid = &grids_[position];
        Buffer grid_buffer;
        memset(&grid_buffer, 0, sizeof(Buffer));
        grid_buffer.width = grid->w;
        grid_buffer.height = grid->h;
        grid_buffer.color = color_;
        grid_buffer.device_id = -1;

        uint32_t grid_buffer_size;
        if (color_ <= ColorFormat::YUV_NV21) {
            grid_buffer_size = grid->w * grid->h * 3 / 2;
        }
        else if (color_ <= ColorFormat::RGB) {
            grid_buffer_size = grid->w * grid->h * 3;
        }
        else {
            grid_buffer_size = grid->w * grid->h * 4;
        }
        if (tl_grid_buffer_size < grid_buffer_size) {
            // LOG(INFO) << "Tiler::Blit() update grid_buffer_size to " << grid_buffer_size;
            if (!tl_grid_buffer && ++grid_buffer_count_ >= 64) {
                LOG(ERROR) << "Tiler::Blit() only support no more than 64 threads to blit canvas";
                mtx_.unlock();
                return false;
            }
            tl_grid_buffer.reset(new uint8_t[grid_buffer_size]);
            tl_grid_buffer_size = grid_buffer_size;
        }

        if (color_ <= ColorFormat::YUV_NV21) {
            grid_buffer.data[0] = tl_grid_buffer.get();
            grid_buffer.data[1] = tl_grid_buffer.get() + grid->w * grid->h;
            if (color_ == ColorFormat::YUV_I420) {
                grid_buffer.data[2] = tl_grid_buffer.get() + grid->w * grid->h * 5 / 4;
            }
        }
        else {
            grid_buffer.data[0] = tl_grid_buffer.get();
        }
        mtx_.unlock();

        if (!Scaler::Process(buffer, &grid_buffer)) {
            LOG(ERROR) << "Tiler::Blit() scaler process src to grid failed";
            return false;
        }

        std::lock_guard<std::mutex> lk(buf_mtx_);
        if (!Scaler::Process(&grid_buffer, &canvas_buffers_[canvas_index_], nullptr, grid)) {
            LOG(ERROR) << "Tiler::Blit() scaler process grid to canvas failed";
            return false;
        }
        if (!canvas_locked_) canvas_diff_++;

        return true;
    }

    Tiler::Buffer* Tiler::GetCanvas(Buffer* buffer) {
        std::lock_guard<std::mutex> lk(buf_mtx_);
        if (!buffer) {
            int canvas_index = (canvas_index_ + 1) % 2;
            if (!canvas_locked_) {
                Buffer* canvas_buffer = &canvas_buffers_[canvas_index_];
                if (canvas_diff_ > 0) {
                    // LOG(INFO) << "Tiler::GetCanvas() canvas_diff=" << canvas_diff_ << ", need update canvas";
                    // TODO: only copy diff grids
                    if (!Scaler::Process(canvas_buffer, &canvas_buffers_[canvas_index])) {
                        LOG(ERROR) << "Tiler::GetCanvas() scaler process canvas_buffers_ failed";
                    }
                    canvas_diff_ = 0;
                }
                canvas_index_ = canvas_index;
                canvas_locked_ = true;
                return canvas_buffer;
            }
            else {
                return &canvas_buffers_[canvas_index];
            }
        }

        if (!Scaler::Process(&canvas_buffers_[canvas_index_], buffer)) {
            LOG(ERROR) << "Tiler::GetCanvas() scaler process canvas_buffer to output buffer failed";
            return nullptr;
        }
        return buffer;
    }

    void Tiler::ReleaseCanvas() {
        std::lock_guard<std::mutex> lk(buf_mtx_);
        if (canvas_locked_) canvas_locked_ = false;
    }

    void Tiler::DumpCanvas() {
        static const int yuv_to_bgr_color_map[3] = { cv::COLOR_YUV2BGR_I420, cv::COLOR_YUV2BGR_NV21, cv::COLOR_YUV2BGR_NV12, };
        static int index = 0;
        for (int i = 0; i < 2; i++) {
            cv::Mat mat;
            Buffer* buffer = &canvas_buffers_[i];
            if (buffer->color <= Scaler::ColorFormat::YUV_NV21) {
                mat = cv::Mat(buffer->height * 3 / 2, buffer->stride[0], CV_8UC1, buffer->data[0]);
                mat.cols = buffer->width;
                mat.step = buffer->stride[0];
                cv::cvtColor(mat, mat, yuv_to_bgr_color_map[buffer->color]);
            }
            else if (buffer->color <= Scaler::ColorFormat::RGB) {
                mat = cv::Mat(buffer->height, buffer->stride[0] / 3, CV_8UC3, buffer->data[0]);
                mat.cols = buffer->width;
                mat.step = buffer->stride[0];
            }
            else {
                mat = cv::Mat(buffer->height, buffer->stride[0] / 4, CV_8UC4, buffer->data[0]);
                mat.cols = buffer->width;
                mat.step = buffer->stride[0];
            }
            cv::imwrite("canvas" + std::to_string(i) + "_" + std::to_string(index) + ".jpg", mat);
        }
        index++;
    }
}// namespace components
}  // namespace easysa
