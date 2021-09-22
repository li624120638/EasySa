#ifndef SAMPLE_UNITEST_MAKE_EYE_DS_HPP
#define SAMPLE_UNITEST_MAKE_EYE_DS_HPP
#include "RetinaFace.h"
#include <glog/logging.h>
#include <io.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>

/*
* this class use retinaface to create eye roi dataset
*/
class MakeEyeDs {
public:
	explicit MakeEyeDs(std::string model_path, std::string img_dir, std::string save_dir);
	~MakeEyeDs();
	bool CreateEyeDs();
private:
	std::string GetFileNameByPath(const std::string& file_path);
	// for the same picture, just save one image
	void SaveEyeRoi(const cv::Mat& src, FaceDetectInfo info, std::string file_full_path);
private:
	RetinaFace* rf = nullptr;
	std::string model_path_;
	std::string img_dir_;
	std::string save_dir_;
	std::vector<std::string> images_path_;
}; // class MakeEyeDs
#endif // !SAMPLE_UNITEST_MAKE_EYE_DS_HPP
