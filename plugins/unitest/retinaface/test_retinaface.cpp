#include "RetinaFace.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include "timer.h"

using namespace std;
using NetOutputs = std::vector<std::vector<std::vector<float>>>;

void DrawResult(cv::Mat& src, const std::vector<FaceDetectInfo>& detect_info, cv::Size rz, int pixel_sz = 8) {
    if (src.empty()) return;
    anchor_box rect;
    float scale_ratio;
    int width = src.cols;
    int height = src.rows;
    float x1, x2, x3, x4, y1, y2, y3, y4;
    std::vector<float> output_lab;
    for (auto& it : detect_info) {
        Mat roi;
        rect = it.rect;
        scale_ratio = it.scale_ratio;
        /*
````````*    p1(x1,y1)p2(x2,y1)
````````*    p3(x1,y2)p4(x2,y2)
````````*/
        x1 = it.rect.x1 * scale_ratio * (1.0 * width / rz.width);
        y1 = it.rect.y1 * scale_ratio * (1.0 * height / rz.height);
        x2 = it.rect.x2 * scale_ratio * (1.0 * width / rz.width);
        y2 = it.rect.y2 * scale_ratio * (1.0 * height / rz.height);
        cv::Point p1(x1, y1), p2(x2, y1), p3(x1, y2), p4(x2, y2);
        cv::line(src, p1, p2, cv::Scalar(0, 255, 0), pixel_sz, cv::LineTypes::LINE_AA);
        cv::line(src, p1, p3, cv::Scalar(0, 255, 0), pixel_sz, cv::LineTypes::LINE_AA);
        cv::line(src, p2, p4, cv::Scalar(0, 255, 0), pixel_sz, cv::LineTypes::LINE_AA);
        cv::line(src, p3, p4, cv::Scalar(0, 255, 0), pixel_sz, cv::LineTypes::LINE_AA);
        /*
        * draw ¹Ø¼üµã
        */
        for (size_t j = 0; j < 5; j++) {
            cv::Point2f pt = cv::Point2f(it.pts.x[j] * scale_ratio * (1.0 * width / rz.width),
                it.pts.y[j] * scale_ratio * (1.0 * height / rz.height));
            cv::circle(src, pt, pixel_sz, Scalar(0, 255, 0), cv::LineTypes::LINE_AA);
        }
    }
}

TEST(PLUGINS, RetinaFace) {
	std::string rtfm = "../../../data/models/retinaface";
	std::string test_img = "../../../data/images/face.jpg";
	std::string save_img = "../../../data/images/output/face.jpg";
	RetinaFace rf(rtfm, "net3", 0.4);
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
		"../../../data/images/ScalerAndTiler/rf/0019_2m_-30P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/rf/0019_2m_-15P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/rf/0019_2m_0P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/rf/0019_2m_15P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/rf/0019_2m_30P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/rf/0016_2m_-30P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/rf/0016_2m_-15P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/rf/0016_2m_0P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/rf/0016_2m_15P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/rf/0016_2m_30P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/rf/0044_2m_-30P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/rf/0044_2m_-15P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/rf/0044_2m_0P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/rf/0044_2m_15P_0V_0H.jpg",
		"../../../data/images/ScalerAndTiler/rf/0044_2m_30P_0V_0H.jpg"
	};
	std::vector<FaceDetectInfo> detect_info;
	size_t sz = img_lists.size();
	for (int i = 0; i < sz; ++i) {
		cv::Mat src = cv::imread(img_lists[i], cv::IMREAD_COLOR);
		assert(!test_img.empty());
		cv::Size rz(640, 640);
		cv::Mat img_rsz;
		cv::resize(src, img_rsz, rz);
		detect_info = rf.detect(img_rsz, 0.9, 1.0);
		DrawResult(src, detect_info, rz);
		cv::imwrite(save_img_lists[i], src);
	}
}

TEST(PLUGINS, OuluFaceDetection) {
	std::string rtfm = "../../../data/models/retinaface";
	std::string test_img = "../../../data/images/face.jpg";
	std::string save_img = "../../../data/images/output/face.jpg";
	RetinaFace rf(rtfm, "net3", 0.4);
	std::vector<std::string> img_lists = {
	"../../../data/images/ScalerAndTiler/face_detect/input/P003_Anger.jpeg",
	"../../../data/images/ScalerAndTiler/face_detect/input/P006_Happiness.jpeg",
	"../../../data/images/ScalerAndTiler/face_detect/input/P008_Surprise.jpeg",
	"../../../data/images/ScalerAndTiler/face_detect/input/P009_Anger.jpeg",
	"../../../data/images/ScalerAndTiler/face_detect/input/P037_Happiness.jpeg",
	"../../../data/images/ScalerAndTiler/face_detect/input/P041_Fear.jpeg",
	"../../../data/images/ScalerAndTiler/face_detect/input/P053_Happiness.jpeg",
	"../../../data/images/ScalerAndTiler/face_detect/input/P056_Disgust.jpeg",
	"../../../data/images/ScalerAndTiler/face_detect/input/P064_Happiness.jpeg",
	"../../../data/images/ScalerAndTiler/face_detect/input/P068_Happiness.jpeg"
	};
	std::vector<std::string> save_img_lists = {
	"../../../data/images/ScalerAndTiler/face_detect/output/P003_Anger.jpeg",
	"../../../data/images/ScalerAndTiler/face_detect/output/P006_Happiness.jpeg",
	"../../../data/images/ScalerAndTiler/face_detect/output/P008_Surprise.jpeg",
	"../../../data/images/ScalerAndTiler/face_detect/output/P009_Anger.jpeg",
	"../../../data/images/ScalerAndTiler/face_detect/output/P037_Happiness.jpeg",
	"../../../data/images/ScalerAndTiler/face_detect/output/P041_Fear.jpeg",
	"../../../data/images/ScalerAndTiler/face_detect/output/P053_Happiness.jpeg",
	"../../../data/images/ScalerAndTiler/face_detect/output/P056_Disgust.jpeg",
	"../../../data/images/ScalerAndTiler/face_detect/output/P064_Happiness.jpeg",
	"../../../data/images/ScalerAndTiler/face_detect/output/P068_Happiness.jpeg"
	};
	std::vector<FaceDetectInfo> detect_info;
	size_t sz = img_lists.size();
	for (int i = 0; i < sz; ++i) {
		cv::Mat src = cv::imread(img_lists[i], cv::IMREAD_COLOR);
		assert(!test_img.empty());
		cv::Size rz(640, 640);
		cv::Mat img_rsz;
		cv::resize(src, img_rsz, rz);
		detect_info = rf.detect(img_rsz, 0.9, 1.0);
		DrawResult(src, detect_info, rz, 2);
		cv::imwrite(save_img_lists[i], src);
	}
}

