#ifndef COMPONENTS_SCALER_TILER_HPP_
#define COMPONENTS_SCALER_TILER_HPP_
#include <cstdint>
/*
* @brief this file for resize convert and crop
* in future will supoort libyuv¡¢npp, now only support opencv
*/
namespace easysa {
namespace components {
    class Scaler {
    public:
        enum ColorFormat {
            YUV_I420 = 0,
            YUV_NV12,
            YUV_NV21,
            BGR,
            RGB,
            BGRA,
            RGBA,
            ABGR,
            ARGB,
            COLOR_MAX,
        };

        struct Buffer {
            uint32_t width;
            uint32_t height;
            uint8_t* data[3];
            uint32_t stride[3];
            ColorFormat color;
            int device_id = -1;
        };

        struct Rect {
            int x, y, w, h;

            Rect() { x = 0; y = 0; w = 0; h = 0; }
            Rect(int rx, int ry, int rw, int rh) { x = rx; y = ry; w = rw; h = rh; }
            Rect(const Rect& r) { x = r.x; y = r.y; w = r.w; h = r.h; }
            Rect& operator=(const Rect& r) {
                x = r.x; y = r.y; w = r.w; h = r.h;
                return *this;
            }
            bool operator==(const Rect& r) const {
                return (x == r.x && y == r.y && w == r.w && h == r.h);
            }
            bool operator!=(const Rect& r) const {
                return !(x == r.x && y == r.y && w == r.w && h == r.h);
            }
        };

        static const Rect NullRect;

        enum Carrier {
            DEFAULT = -2,
            AUTO = -1,
            OPENCV = 0,
            // LIBYUV,
            CARRIER_MAX,
        };

        static void SetCarrier(int carrier) { carrier_ = carrier; }
        static int GetCarrier() { return carrier_; }
        static bool Process(const Buffer* src, Buffer* dst, const Rect* src_crop = nullptr, const Rect* dst_crop = nullptr,
            int carrier = DEFAULT);

    private:
        static int carrier_;
    };  // class Scaler


} // namespace components
} // namespace easysa
#endif // !COMPONENTS_SCALER_TILER_HPP_
