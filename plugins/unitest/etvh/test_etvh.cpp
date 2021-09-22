#include <gtest/gtest.h>
#include "etvh.hpp"
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
using namespace easysa::components;
using namespace cv;
using NetOutputs = std::vector<std::vector<std::vector<float>>>;

static const std::vector<std::string> v_status = { "-10","0","10" };
static const std::vector<std::string> h_status = {
	"-15","-10","-5","0","5", "10","15"
};
static void DrawEtvhResult(cv::Mat& src, std::vector<float> etv, std::vector<float> eth,int pixel_sz = 8) {
	if (src.empty()) return;
	size_t index = 0;
	float max = etv[0];
	size_t sz = etv.size();
	for (size_t i = 1; i < sz; ++i) {
		if (etv[i] > max) {
			index = i;
			max = etv[i];
		}
	}
	std::string lable = "V: " + v_status[index];
	cv::putText(src, lable, cv::Point(0, 40), 1, 4/*font size*/,
			Scalar(0, 0, 255), 2, cv::LineTypes::LINE_4);

	index = 0;
	max = eth[0];
	sz = eth.size();
	for (size_t i = 1; i < sz; ++i) {
		if (eth[i] > max) {
			index = i;
			max = eth[i];
		}
	}
	lable = "H: " + h_status[index];
	cv::putText(src, lable, cv::Point(0, 80), 1, 4/*font size*/,
		Scalar(0, 0, 255), 2, cv::LineTypes::LINE_4);
}

TEST(PLUGNS, ETVH) {
	/*
	* @Notice
	* In this test case,the image contains one eye
	*/
	std::string etvh_cache_path = "../../../data/models/etvh/etvh.cache";
	std::string etvh_path = "../../../data/models/etvh/etvh.onnx";
	Etvh etvh(etvh_path, etvh_cache_path);
	uint32_t mm_num = 6;
	etvh.Init(1, mm_num);
	std::vector<std::string> test_img = {
		"../../../data/images/0001_2m_0P_0V_10H.jpg",
		"../../../data/images/0006_2m_0P_10V_15H.jpg",
		"../../../data/images/0041_2m_15P_0V_15H.jpg",
		"../../../data/images/0019_2m_0P_0V_0H.jpg"
	};
	std::vector<std::string> save_test_img = {
		"../../../data/images/test_etvh/0001_2m_0P_0V_10H.jpg",
		"../../../data/images/test_etvh/0006_2m_0P_10V_15H.jpg",
		"../../../data/images/test_etvh/0041_2m_15P_0V_15H.jpg",
		"../../../data/images/test_etvh/0019_2m_0P_0V_0H.jpg"
	};
	size_t sz = test_img.size();
	for (int i = 0; i < sz; ++i) {
		cv::Mat src = cv::imread(test_img[i].c_str(), cv::IMREAD_COLOR);
		EXPECT_TRUE(etvh.PushMatToPool(&src));
		NetOutputs results;
		EXPECT_TRUE(etvh.GetResultAsynchr(results));
		DrawEtvhResult(src, results[0][0], results[1][0]);
		cv::imwrite(save_test_img[i].c_str(), src);
	}
}
