#include "fer.hpp"
#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
using namespace easysa::components;
using NetOutputs = std::vector<std::vector<std::vector<float>>>;

static const std::vector<std::string> fer_status = {
"Anger","Disgust","Fear","Happiness","Neutral", "Sadness","Surprise"
};

static void DrawFerResult(cv::Mat& src, std::vector<float> result, int pixel_sz = 8) {
    if (src.empty()) return;
	size_t index = 0;
	float max = result[0];
	size_t sz = result.size();
	for (size_t i = 1; i < sz; ++i) {
		if (result[i] > max) {
			index = i;
			max = result[i];
		}
	}
	cv::putText(src, fer_status[index], cv::Point(20, 20), pixel_sz, cv::FONT_HERSHEY_PLAIN,
		cv::Scalar(0, 255, 0), 1, cv::LINE_8);
}

TEST(PLUGINS, FER) {
	std::string fer_cache_path = "../../../data/models/fer/fer.cache";
	std::string fer_path = "../../../data/models/fer/fer.onnx";
	Fer fer(fer_path, fer_cache_path);
	uint32_t mm_num = 4;
	fer.Init(1, mm_num);
	std::vector<std::string> test_img = {
		"../../../data/images/Anger.jpg",
		"../../../data/images/Disgust.jpg",
		"../../../data/images/Fear.jpg",
		"../../../data/images/Happiness.jpg",
		"../../../data/images/Sadness.jpg",
		"../../../data/images/Surprise.jpg",
	};
	std::vector<std::string> save_test_img = {
	"../../../data/images/output/test_fer/Anger.jpg",
	"../../../data/images/output/test_fer/Disgust.jpg",
	"../../../data/images/output/test_fer/Fear.jpg",
	"../../../data/images/output/test_fer/Happiness.jpg",
	"../../../data/images/output/test_fer/Sadness.jpg",
	"../../../data/images/output/test_fer/Surprise.jpg",
	};
	//cv::namedWindow("src", cv::WINDOW_FREERATIO);
    for (int i = 0; i < test_img.size(); ++i) {
        cv::Mat roi = cv::imread(test_img[i].c_str(), cv::IMREAD_COLOR);
        EXPECT_TRUE(fer.PushMatToPool(&roi));
        //NetOutputs results;
        //EXPECT_TRUE(fer.GetResultAsynchr(results));
		std::vector<float> result = fer.GetSoftMaxResult();
		DrawFerResult(roi, result, 2);
		cv::imwrite(save_test_img[i].c_str(), roi);
    }
	// 这里有个问题就是，将资源放入PushMatToPool里面的函数会释放这个资源
}