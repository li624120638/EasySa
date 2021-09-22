#include <opencv2/opencv.hpp>
#include <gtest/gtest.h>
#include <glog/logging.h>
#include <vector>
#include <string>

#include "scaler.hpp"
#include "tiler.hpp"

/*
* don`t have enough time, there just test opencv bgr resize,
*/
using namespace easysa::components;
TEST(COMPONENTS, Tiler) {
	std::vector<std::string> img_lists = {
		"../../../data/images/ScalerAndTiler/input/0019_2m_-30P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/input/0019_2m_-15P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/input/0019_2m_0P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/input/0019_2m_15P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/input/0019_2m_30P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/input/0016_2m_-30P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/input/0016_2m_-15P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/input/0016_2m_0P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/input/0016_2m_15P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/input/0016_2m_30P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/input/0044_2m_-30P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/input/0044_2m_-15P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/input/0044_2m_0P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/input/0044_2m_15P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/input/0044_2m_30P_0V_0H.jpg"
	};
	std::vector<std::string> save_img_lists = {
		"../../../data/images/ScalerAndTiler/output/0019_2m_-30P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/output/0019_2m_-15P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/output/0019_2m_0P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/output/0019_2m_15P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/output/0019_2m_30P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/output/0016_2m_-30P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/output/0016_2m_-15P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/output/0016_2m_0P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/output/0016_2m_15P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/output/0016_2m_30P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/output/0044_2m_-30P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/output/0044_2m_-15P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/output/0044_2m_0P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/output/0044_2m_15P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/output/0044_2m_30P_0V_0H.jpg"
	};
	int row = 3;
	int col = 5;
	int width = 1920;
	int height = 1080;
	Tiler tiler(row, col, Scaler::ColorFormat::BGR, width, height, width);
	size_t sz = img_lists.size();
	for (size_t i = 0; i < sz; ++i) {
		cv::Mat src = cv::imread(img_lists[i].c_str(), cv::IMREAD_ANYCOLOR);
		if (src.empty()) {
			LOG(ERROR) << "read image failed." << img_lists[i];
			continue;
		}
		int src_w = src.cols;
		int src_h = src.rows;
		//LOG(ERROR) << src_w << " " << src_h;
		Scaler::Buffer buf;
		buf.width = src_w;
		buf.height = src_h;
		buf.stride[0] = src_w * 3;
		buf.color = Scaler::ColorFormat::BGR;
		buf.data[0] = new uint8_t[src_w * src_h * 3];
		memcpy(buf.data[0], src.data, src_w * src_h * 3);
		buf.device_id = -1;
		//buf.stride[0] = src_w;
		tiler.Blit(&buf, i);
		delete[] buf.data[0];
	}
	// dump tiler
	Scaler::Buffer* canvas = tiler.GetCanvas();
	cv::Mat result(height, width, CV_8UC3, canvas->data[0]);
	std::string save_path = "../../../data/images/ScalerAndTiler/output/tiler_result.jpg";
	cv::imwrite(save_path.c_str(), result);
	tiler.ReleaseCanvas();
}

TEST(COMPONENTS, OuluTiler) {
	std::vector<std::string> img_lists = {
		"../../../data/images/ScalerAndTiler/oulu_input/strong/Neutral.jpeg",
		"../../../data/images/ScalerAndTiler/oulu_input/strong/Anger.jpeg",
		"../../../data/images/ScalerAndTiler/oulu_input/strong/Disgust.jpeg",
		"../../../data/images/ScalerAndTiler/oulu_input/strong/Happiness.jpeg",
		"../../../data/images/ScalerAndTiler/oulu_input/strong/Fear.jpeg",
		"../../../data/images/ScalerAndTiler/oulu_input/strong/Sadness.jpeg",
		"../../../data/images/ScalerAndTiler/oulu_input/strong/Surprise.jpeg",
		"../../../data/images/ScalerAndTiler/oulu_input/weak/Neutral.jpeg",
		"../../../data/images/ScalerAndTiler/oulu_input/weak/Anger.jpeg",
		"../../../data/images/ScalerAndTiler/oulu_input/weak/Disgust.jpeg",
		"../../../data/images/ScalerAndTiler/oulu_input/weak/Happiness.jpeg",
		"../../../data/images/ScalerAndTiler/oulu_input/weak/Fear.jpeg",
		"../../../data/images/ScalerAndTiler/oulu_input/weak/Sadness.jpeg",
		"../../../data/images/ScalerAndTiler/oulu_input/weak/Surprise.jpeg",
		"../../../data/images/ScalerAndTiler/oulu_input/dark/Neutral.jpeg",
		"../../../data/images/ScalerAndTiler/oulu_input/dark/Anger.jpeg",
		"../../../data/images/ScalerAndTiler/oulu_input/dark/Disgust.jpeg",
		"../../../data/images/ScalerAndTiler/oulu_input/dark/Happiness.jpeg",
		"../../../data/images/ScalerAndTiler/oulu_input/dark/Fear.jpeg",
		"../../../data/images/ScalerAndTiler/oulu_input/dark/Sadness.jpeg",
		"../../../data/images/ScalerAndTiler/oulu_input/dark/Surprise.jpeg",
	};
	int row = 3;
	int col = 7;
	int width = 1920;
	int height = 1080;
	Tiler tiler(row, col, Scaler::ColorFormat::BGR, width, height, width);
	size_t sz = img_lists.size();
	for (size_t i = 0; i < sz; ++i) {
		cv::Mat src = cv::imread(img_lists[i].c_str(), cv::IMREAD_ANYCOLOR);
		if (src.empty()) {
			LOG(ERROR) << "read image failed." << img_lists[i];
			continue;
		}
		int src_w = src.cols;
		int src_h = src.rows;
		//LOG(ERROR) << src_w << " " << src_h;
		Scaler::Buffer buf;
		buf.width = src_w;
		buf.height = src_h;
		buf.stride[0] = src_w * 3;
		buf.color = Scaler::ColorFormat::BGR;
		buf.data[0] = new uint8_t[src_w * src_h * 3];
		memcpy(buf.data[0], src.data, src_w * src_h * 3);
		buf.device_id = -1;
		//buf.stride[0] = src_w;
		tiler.Blit(&buf, i);
		delete[] buf.data[0];
	}
	// dump tiler
	Scaler::Buffer* canvas = tiler.GetCanvas();
	cv::Mat result(height, width, CV_8UC3, canvas->data[0]);
	std::string save_path = "../../../data/images/ScalerAndTiler/output/tiler_oulu_result.jpg";
	cv::imwrite(save_path.c_str(), result);
	tiler.ReleaseCanvas();
}

TEST(COMPONENTS, OuluRoiTiler) {
	std::vector<std::string> img_lists = {
		"../../../data/images/ScalerAndTiler/oulu_roi_input/Neutral.jpg",
		"../../../data/images/ScalerAndTiler/oulu_roi_input/Anger.jpg",
		"../../../data/images/ScalerAndTiler/oulu_roi_input/Disgust.jpg",
		"../../../data/images/ScalerAndTiler/oulu_roi_input/Happiness.jpg",
		"../../../data/images/ScalerAndTiler/oulu_roi_input/Fear.jpg",
		"../../../data/images/ScalerAndTiler/oulu_roi_input/Sadness.jpg",
		"../../../data/images/ScalerAndTiler/oulu_roi_input/Surprise.jpg",
	};
	int row = 1;
	int col = 7;
	int width = 1080;
	int height = 240;
	Tiler tiler(row, col, Scaler::ColorFormat::BGR, width, height, width);
	size_t sz = img_lists.size();
	for (size_t i = 0; i < sz; ++i) {
		cv::Mat src = cv::imread(img_lists[i].c_str(), cv::IMREAD_ANYCOLOR);
		if (src.empty()) {
			LOG(ERROR) << "read image failed." << img_lists[i];
			continue;
		}
		int src_w = src.cols;
		int src_h = src.rows;
		//LOG(ERROR) << src_w << " " << src_h;
		Scaler::Buffer buf;
		buf.width = src_w;
		buf.height = src_h;
		buf.stride[0] = src_w * 3;
		buf.color = Scaler::ColorFormat::BGR;
		buf.data[0] = new uint8_t[src_w * src_h * 3];
		memcpy(buf.data[0], src.data, src_w * src_h * 3);
		buf.device_id = -1;
		//buf.stride[0] = src_w;
		tiler.Blit(&buf, i);
		delete[] buf.data[0];
	}
	// dump tiler
	Scaler::Buffer* canvas = tiler.GetCanvas();
	cv::Mat result(height, width, CV_8UC3, canvas->data[0]);
	std::string save_path = "../../../data/images/ScalerAndTiler/output/tiler_oulu_roi_result.jpg";
	cv::imwrite(save_path.c_str(), result);
	tiler.ReleaseCanvas();
}