#include "RetinaFace.h"
#include "face_position.hpp"
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

static void DrawFpResult(FacePositionClassifier* fpc, cv::Mat& src, const std::vector<FaceDetectInfo>& detect_info, cv::Size rz, int pixel_sz = 8) {
	if (src.empty() || !fpc) return;
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

		std::vector<float> output_lab;
		output_lab.clear();
		auto& pts = it.pts;
		float w_ratio = it.rect.x2 - it.rect.x1;
		float n_ration = pts.x[2] - it.rect.x1;
		float l_ration = pts.x[3] - it.rect.x1;
		float r_ration = pts.x[4] - it.rect.x1;
		n_ration /= w_ratio;
		l_ration /= w_ratio;
		r_ration /= w_ratio;
		output_lab.push_back(n_ration);
		output_lab.push_back(l_ration);
		output_lab.push_back(r_ration);
		int label = fpc->Predict(output_lab, 100, 20);
		cv::putText(src, std::to_string(label), cv::Point(x1, y1+180), 2, 6/*font size*/,
			Scalar(0, 0, 255), 8, cv::LineTypes::LINE_4);
	}
}

TEST(PLUGINS, FacePostion) {
	std::string rtfm = "../../../data/models/retinaface";
	RetinaFace rf(rtfm, "net3", 0.4);
	std::vector<std::string> img_lists = {
	"../../../data/images/test_face_pose/input/0018_2m_0P_0V_0H.jpg",
	"../../../data/images/test_face_pose/input/0018_2m_15P_0V_0H.jpg",
	"../../../data/images/test_face_pose/input/0018_2m_-15P_0V_0H.jpg",
	"../../../data/images/test_face_pose/input/0018_2m_30P_0V_0H.jpg",
	"../../../data/images/test_face_pose/input/0018_2m_-30P_0V_0H.jpg",
	"../../../data/images/test_face_pose/input/0029_2m_0P_0V_0H.jpg",
	"../../../data/images/test_face_pose/input/0029_2m_15P_0V_0H.jpg",
	"../../../data/images/test_face_pose/input/0029_2m_-15P_0V_0H.jpg",
	"../../../data/images/test_face_pose/input/0029_2m_30P_0V_0H.jpg",
	"../../../data/images/test_face_pose/input/0029_2m_-30P_0V_0H.jpg"
	};
	std::vector<std::string> save_img_lists = {
	"../../../data/images/test_face_pose/output/0018_2m_0P_0V_0H.jpg",
	"../../../data/images/test_face_pose/output/0018_2m_15P_0V_0H.jpg",
	"../../../data/images/test_face_pose/output/0018_2m_-15P_0V_0H.jpg",
	"../../../data/images/test_face_pose/output/0018_2m_30P_0V_0H.jpg",
	"../../../data/images/test_face_pose/output/0018_2m_-30P_0V_0H.jpg",
	"../../../data/images/test_face_pose/output/0029_2m_0P_0V_0H.jpg",
	"../../../data/images/test_face_pose/output/0029_2m_15P_0V_0H.jpg",
	"../../../data/images/test_face_pose/output/0029_2m_-15P_0V_0H.jpg",
	"../../../data/images/test_face_pose/output/0029_2m_30P_0V_0H.jpg",
	"../../../data/images/test_face_pose/output/0029_2m_-30P_0V_0H.jpg"
	};
	// create face pose classfier
	std::string face_position_model_path = "../../../data/models/frp/FacePosition.txt";
	FacePositionClassifier fpc(face_position_model_path);
	size_t sz = img_lists.size();
	for (int i = 0; i < sz; ++i) {
		cv::Mat src = cv::imread(img_lists[i], cv::IMREAD_COLOR);
		assert(!src.empty());
		std::vector<FaceDetectInfo> detect_info;
		cv::Size rz(640, 640);
		cv::Mat img_rsz;
		cv::resize(src, img_rsz, rz);
		detect_info = rf.detect(img_rsz, 0.9, 1.0);
		DrawFpResult(&fpc, src, detect_info, rz, 2);
		cv::imwrite(save_img_lists[i], src);
	}
}
