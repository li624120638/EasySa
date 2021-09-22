#ifndef SAMPLE_SENTIMENT_ANALYSIS2_HPP__
#define SAMPLE_SENTIMENT_ANALYSIS2_HPP__
#include <opencv2/opencv.hpp>
#include <glog/logging.h>
#include <atomic>
#include <thread>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include "connector.hpp"
#include "easysa_frame_va.hpp"
#include "etvh.hpp"
#include "RetinaFace.h"
#include "face_position.hpp"
#include "fer.hpp"
#include "frame_control.hpp"
using namespace easysa::components;
/*
* @brief this struct use to manage the data buffer between two module
*/
struct Connector_cfg2 {
	std::string pre_module_;
	std::string post_module_;
	easysa::Connector* connector_ = nullptr;
	Connector_cfg2(std::string pre_module, std::string post_module_) :
		pre_module_(pre_module),post_module_(post_module_) {}
	bool Init(size_t conveyor_count, size_t conveyor_capacity = 20) {
		connector_ = new easysa::Connector(conveyor_count, conveyor_capacity);
		if (connector_ == nullptr) {
			return false;
		}
		return true;
	}
	~Connector_cfg2() {
		if (connector_) {
			connector_->Stop();
			delete connector_;
			connector_ = nullptr;
		}
	}
}; // struct Connector_cfg

/*
* @brief this file,i merge fer and etvh to one module
*/
class SentimentAnalysis2 {
	enum LABEL_TYPE
	{
		FER = 0,
		V_TYPE = 1,
		H_TYPE = 2
	};
public:
	SentimentAnalysis2(uint32_t frame_rate) :
				frame_rate_(frame_rate){ running_ = true; }
	virtual ~SentimentAnalysis2();
	bool InitModule();
	bool InitModuleConnector();
	void Run();
	void OsdModule();
private:
	void ProvideSource();
	bool LinkTwoModule(std::string pre_module, std::string post_module,
		const size_t conveyor_count, size_t conveyor_capacity = 10);
	void DetectModule();
	void FerEtvhModule();
private:
	int PredictFacePose(const FaceDetectInfo& info);
	void DrawByObj(cv::Mat& src, std::shared_ptr<easysa::InferObject> obj);
	std::string GetLabel(const std::vector<float>& result, LABEL_TYPE type);
private:
	std::string rtfm_ = "../../../data/models/retinaface";
	std::string fer_cache_path_ = "../../../data/models/fer/fer.cache";
	std::string fer_path_ = "../../../data/models/fer/fer.onnx";
	std::string etvh_cache_path_ = "../../../data/models/etvh/etvh.cache";
	std::string etvh_path_ = "../../../data/models/etvh/etvh.onnx";
	std::string fpc_path_ = "../../../data/models/frp/FacePosition.txt";
private:
	RetinaFace* rf_ = nullptr;
	FacePositionClassifier* fpc_ = nullptr;
	Fer* fer_model_ = nullptr;
	Etvh* etvh_model_ = nullptr;
	FrameController* frame_controller_ = nullptr;
private:
	std::thread* source_thread_ = nullptr;
	std::thread* detect_thread_ = nullptr;
	std::thread* fer_etvh_thread_ = nullptr;
	std::thread* osd_thread_ = nullptr;
private:
	int camera_id_ = 0;
	uint32_t frame_rate_{ 25 };
	std::string stream_id_ = "0";
	std::string videocapture_ = "source";
	std::string detector_ = "detect";
	std::string fer_etvh_ = "fer_etvh";
	std::string osd_ = "osd";
private:
	std::unordered_map<std::string, std::shared_ptr<Connector_cfg2>> connector_map_;
private:
	std::atomic<bool> running_{ false };
private:
	std::vector<std::string> fer_status_ = {
	"Anger","Disgust","Fear","Happiness","Neutral", "Sadness","Surprise"
	};
	std::vector<std::string> v_status_ = { "-10","0","10" };
	std::vector<std::string> h_status_ = {
	"-15","-10","-5","0","5", "10","15"
	};
}; // class SentimentAnalysis2

#endif // !SAMPLE_SENTIMENT_ANALYSIS2_HPP__


