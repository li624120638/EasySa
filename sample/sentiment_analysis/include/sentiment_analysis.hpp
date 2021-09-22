#ifndef SAMPLE_SENTIMENT_ANALYSIS_HPP__
#define SAMPLE_SENTIMENT_ANALYSIS_HPP__
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
#include "frame_skip_control.hpp"
using namespace easysa::components;
/*
* @brief this struct use to manage the data buffer between two module
*/
struct Connector_cfg {
	std::string pre_module_;
	std::string post_module_;
	easysa::Connector* connector_ = nullptr;
	Connector_cfg(std::string pre_module, std::string post_module_) :
		pre_module_(pre_module),post_module_(post_module_) {}
	bool Init(size_t conveyor_count, size_t conveyor_capacity = 20) {
		connector_ = new easysa::Connector(conveyor_count, conveyor_capacity);
		if (connector_ == nullptr) {
			return false;
		}
		return true;
	}
	~Connector_cfg() {
		if (connector_) {
			connector_->Stop();
			delete connector_;
			connector_ = nullptr;
		}
	}
}; // struct Connector_cfg
class SentimentAnalysis {
	enum LABEL_TYPE
	{
		FER = 0,
		V_TYPE = 1,
		H_TYPE = 2
	};
public:
	SentimentAnalysis(uint32_t frame_rate) :
				frame_rate_(frame_rate){ running_ = true; }
	virtual ~SentimentAnalysis();
	/*
	* param[in] Now paramter not use, in future, map will use to
	* create FrameSkipControl instance
	* in pair first string is module_name, second represent how
	* much to skip one frame
	*/
	bool InitFrameSkip(/*const std::vector<std::pair<std::string, uint32_t>>& map*/);
	bool InitModule();
	bool InitModuleConnector();
	void Run();
	void OsdModule();
private:
	void ProvideSource();
	bool LinkTwoModule(std::string pre_module, std::string post_module,
		const size_t conveyor_count, size_t conveyor_capacity = 10);
	void DetectModule();
	void FerModule();
	void EtvhModule();
private:
	int PredictFacePose(const FaceDetectInfo& info);
	void DrawByObj(cv::Mat& src, std::shared_ptr<easysa::InferObject> obj,
		bool skip_fer = false, bool skip_etvh = false);
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
	std::thread* fer_fp_thread_ = nullptr;
	std::thread* etvh_thread_ = nullptr;
	std::thread* osd_thread_ = nullptr;
private:
	int camera_id_ = 0;
	uint32_t frame_rate_{ 60 };
	std::string stream_id_ = "0";
	std::string videocapture_ = "source";
	std::string detector_ = "detect";
	std::string fer_ = "fer";
	std::string etvh_ = "etvh";
	std::string osd_ = "osd";
private:
	std::unordered_map<std::string, std::shared_ptr<Connector_cfg>> connector_map_;
	std::atomic<bool> running_{ false };
	std::vector<std::string> fer_status_ = {
	"Anger","Disgust","Fear","Happiness","Neutral", "Sadness","Surprise"
	};
	std::vector<std::string> v_status_ = { "-10","0","10" };
	std::vector<std::string> h_status_ = {
	"-15","-10","-5","0","5", "10","15"
	};
	std::vector<std::string> modules_name_ = {
	"source", "detection", "fer", "etvh", "osd"
	};
	std::vector<uint32_t> modules_skip_ = {
	0, 0, 1, 1, 0
	};
	int N = 5;
	FrameSkipControl<5>* fsc_ = nullptr;
	std::string fer_s;
	std::string v_direction;
	std::string h_direction;
}; // class SentimentAnalysis

#endif // !SAMPLE_SENTIMENT_ANALYSIS_HPP__


