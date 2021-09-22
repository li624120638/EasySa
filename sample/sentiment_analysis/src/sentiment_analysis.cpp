#include "sentiment_analysis.hpp"
#include "easysa_frame.hpp"
#include "easysa_frame_va.hpp"
#include <chrono>
#include <string>
#include "timer.h"
using FrameInfoPtr = std::shared_ptr<easysa::FrameInfo>;
using NetOutputs = std::vector<std::vector<std::vector<float>>>;

#define CALL_LNINK_MODULE(FUNC, ...)  {                \
  if (true != FUNC(__VA_ARGS__)) {                     \
	std::cerr << "Link two module failed" << std::endl;\
    exit(1);                                           \
  }                                                    \
}

bool SentimentAnalysis::LinkTwoModule(std::string pre_module, std::string post_module,
	const size_t conveyor_count, size_t conveyor_capacity) {
	std::string connector_name = pre_module + "_" + post_module;
	std::shared_ptr<Connector_cfg> pre_post = std::make_shared<Connector_cfg>(pre_module, post_module);
	if (pre_post == nullptr) return false;
	pre_post->Init(4U, 20U);
	pre_post->connector_->Start();
	connector_map_[connector_name] = pre_post;
	return true;
}

bool SentimentAnalysis::InitModuleConnector() {
	// init Videocapture and detector connector
	CALL_LNINK_MODULE(LinkTwoModule, videocapture_, detector_, 1, 25);
	CALL_LNINK_MODULE(LinkTwoModule, detector_, fer_, 1, 25);
	CALL_LNINK_MODULE(LinkTwoModule, fer_, etvh_, 1, 20);
	CALL_LNINK_MODULE(LinkTwoModule, etvh_, osd_, 1, 20);
	return true;
}

bool SentimentAnalysis::InitFrameSkip(/*const std::vector<std::pair<std::string, uint32_t>>& map*/) {
	// now [param] map not use
	if (nullptr == fsc_) {
		fsc_ = new (std::nothrow) FrameSkipControl<5>();
	}
	std::vector<std::pair<std::string, uint32_t>> cfg;
	cfg.resize(N);
	for (size_t i = 0; i < N; i++) {
		cfg[i] = std::make_pair(modules_name_[i], modules_skip_[i]);
	}
	fsc_->Init(cfg);
	return true;
}
void SentimentAnalysis::ProvideSource() {
	cv::VideoCapture capture(camera_id_);
	if (!capture.isOpened()) {
		LOG(ERROR) << "can`t opeen this camera" << camera_id_;
	}
	std::string connector_name = videocapture_ + "_" + detector_;
	auto it = connector_map_.find(connector_name);
	if (it == connector_map_.end()) {
		LOG(ERROR) << "can`t find" << connector_name << "connector";
		return;
	}
	auto& connector = it->second;
	size_t conveyor_idx = std::stoi(stream_id_) % connector->connector_->GetConveyorCount();
	cv::Mat src;
	while (running_) {
		frame_controller_->Start();
		auto data = easysa::FrameInfo::Create(stream_id_,false);
		auto frame = std::make_shared<easysa::DataFrame>();
		frame->skip_status = fsc_->GetSkipVectorNow();
		bool ret = capture.read(frame->src_mat);
		if (!ret) break;
		/*
		bool ret = capture.read(src);
		if (!ret) break;
		frame->ptr_cpu[0] = src.data;
		frame->width = src.cols;
		frame->height = src.rows;
		frame->fmt = easysa::PIXEL_FORMAT_BGR24;
		frame->ctx.dev_type = easysa::DevContext::DevType::CPU;
		frame->src_mat = src.clone();
		*/
		data->datas[easysa::DataFramePtrKey] = frame;
		std::shared_ptr<easysa::InferObjs> objs_ptr = std::make_shared<easysa::InferObjs>();
		data->datas[easysa::InferObjsPtrKey] = objs_ptr;

		while (running_ && connector->connector_->PushDataBufferToConveyor(conveyor_idx, data) == false) {
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
		frame_controller_->Control();
	}
	capture.release();
}

void SentimentAnalysis::DetectModule() {
	std::string connector_name = videocapture_ + "_" + detector_;
	auto it = connector_map_.find(connector_name);
	if (it == connector_map_.end()) {
		LOG(ERROR) << "can`t find" << connector_name << "connector";
		return;
	}
	auto& connector = it->second;
	size_t conveyor_idx = std::stoi(stream_id_) % connector->connector_->GetConveyorCount();

	// next connector
	std::string next_connector_name = detector_ + "_" + fer_;
	auto nit = connector_map_.find(next_connector_name);
	if (nit == connector_map_.end()) {
		LOG(ERROR) << "can`t find" << connector_name << "connector";
		return;
	}
	auto& next_connector = nit->second;

	std::vector<FaceDetectInfo> detect_info;
	cv::Size rz(640, 640);
	while (running_) {
		FrameInfoPtr data = nullptr;
		while (!connector->connector_->IsStopped() && data == nullptr){
			data = connector->connector_->PopDataBufferFromConveyor(conveyor_idx);
		}
		if (connector->connector_->IsStopped()) {
			break;
		}
		if (data == nullptr) {
			continue;
		}
		// Process 
		auto frame = easysa::GetDataFramePtr(data);
		auto objs_ptr = easysa::GetInferObjsPtr(data);
		cv::Mat img_rsz;
		cv::resize(frame->src_mat, img_rsz, rz);
		detect_info.clear();
		detect_info = rf_->detect(img_rsz, 0.9, 1.0);
		for (auto& it : detect_info) {
			std::vector<float> face_pts;
			auto obj = std::make_shared<easysa::InferObject>();
			obj->bbox.x = it.rect.x1;
			obj->bbox.y = it.rect.y1;
			obj->bbox.w = it.rect.x2 - it.rect.x1;
			obj->bbox.h = it.rect.y2 - it.rect.y1;
			obj->score = it.score;
			if (it.rect.x1 < 0 && it.rect.x1 > 1) continue;
			if (it.rect.x2 < 0 && it.rect.x2 > 1) continue;
			if (it.rect.y1 < 0 && it.rect.y1 > 1) continue;
			if (it.rect.y2 < 0 && it.rect.y2 > 1) continue;
			// put x first, put y next
			// x0, x1, x2, x3, x4, y0, y1, y2, y3, y4
			for (auto& pts : it.pts.x) {
				face_pts.push_back(pts);
			}
			for (auto& pts : it.pts.y) {
				face_pts.push_back(pts);
			}
			obj->AddFeature("face_pts", std::move(face_pts));
			int label = PredictFacePose(it);
			obj->AddExtraAttribute("face_pose", std::to_string(label));
			obj->AddExtraAttribute("scaler_ratio", std::to_string(it.scale_ratio));
			objs_ptr->objs_.push_back(obj);
		}
		while (running_ &&
			next_connector->connector_->PushDataBufferToConveyor(conveyor_idx, data) == false) {
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
	}
}

void SentimentAnalysis::FerModule() {
	std::string connector_name = detector_ + "_" + fer_;
	auto it = connector_map_.find(connector_name);
	if (it == connector_map_.end()) {
		LOG(ERROR) << "can`t find" << connector_name << "connector";
		return;
	}
	auto& connector = it->second;
	size_t conveyor_idx = std::stoi(stream_id_) % connector->connector_->GetConveyorCount();
	// next connector
	std::string next_connector_name = fer_ + "_" + etvh_;
	auto nit = connector_map_.find(next_connector_name);
	if (nit == connector_map_.end()) {
		LOG(ERROR) << "can`t find" << connector_name << "connector";
		return;
	}
	auto& next_connector = nit->second;
	cv::Size rz(640, 640);
	while (running_) {
		FrameInfoPtr data = nullptr;
		while (!connector->connector_->IsStopped() && data == nullptr) {
			data = connector->connector_->PopDataBufferFromConveyor(conveyor_idx);
		}
		if (connector->connector_->IsStopped()) {
			break;
		}
		if (data == nullptr) {
			continue;
		}
		// process
		auto frame = easysa::GetDataFramePtr(data);
		auto objs_ptr = easysa::GetInferObjsPtr(data);
		// whether skip
		auto skip_status = frame->GetSkipStatus();
		if (1 != skip_status[2]) {
			cv::Mat src = frame->src_mat;
			int width = src.cols;
			int height = src.rows;
			for (auto& it : objs_ptr->objs_) {
				float x1, y1, w, h, scale_ratio;
				scale_ratio = std::stoi(it->GetExtraAttribute("scaler_ratio"));
				x1 = it->bbox.x * scale_ratio * (1.0 * width / rz.width);
				y1 = it->bbox.y * scale_ratio * (1.0 * height / rz.height);
				w = it->bbox.w * scale_ratio * (1.0 * width / rz.width);
				h = it->bbox.h * scale_ratio * (1.0 * height / rz.height);
				//cv::Mat roi = src(cv::Rect(x1, y1, w, h));
				cv::Mat* roi = new cv::Mat(src, cv::Rect(x1, y1, w, h));
				fer_model_->PushMatToPool(roi);
				std::vector<float> result = fer_model_->GetSoftMaxResult();
				it->AddFeature("fer", result);
			}
			// Asynchronous processing
			//for (auto& it : objs_ptr->objs_) {
				//std::vector<float> result = fer_model_->GetSoftMaxResult();
				//it->AddFeature("fer", result);
			//}
		}

		while (running_ &&
			next_connector->connector_->PushDataBufferToConveyor(conveyor_idx, data) == false) {
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
	}
}

void  SentimentAnalysis::EtvhModule() {
	std::string connector_name = fer_ + "_" + etvh_;
	auto it = connector_map_.find(connector_name);
	if (it == connector_map_.end()) {
		LOG(ERROR) << "can`t find" << connector_name << "connector";
		return;
	}
	auto& connector = it->second;
	size_t conveyor_idx = std::stoi(stream_id_) % connector->connector_->GetConveyorCount();
	// next connector
	std::string next_connector_name = etvh_ + "_" + osd_;
	auto nit = connector_map_.find(next_connector_name);
	if (nit == connector_map_.end()) {
		LOG(ERROR) << "can`t find" << connector_name << "connector";
		return;
	}
	auto& next_connector = nit->second;
	cv::Size rz(640, 640);
	while (running_) {
		FrameInfoPtr data = nullptr;
		while (!connector->connector_->IsStopped() && data == nullptr) {
			data = connector->connector_->PopDataBufferFromConveyor(conveyor_idx);
		}
		if (connector->connector_->IsStopped()) {
			break;
		}
		if (data == nullptr) {
			continue;
		}
		// process
		auto frame = easysa::GetDataFramePtr(data);
		auto objs_ptr = easysa::GetInferObjsPtr(data);
		auto skip_status = frame->GetSkipStatus();
		if (1 != skip_status[3]) {
			cv::Mat src = frame->src_mat;
			int width = src.cols;
			int height = src.rows;
			float x1, y1, w, h, scale_ratio;
			scale_ratio = 2;
			for (auto& it : objs_ptr->objs_) {
				w = scale_ratio * (1.0 * width / rz.width);
				h = scale_ratio * (1.0 * height / rz.height);
				x1 = it->bbox.x * w;
				y1 = it->bbox.y * h;
				w *= it->bbox.w;
				h *= it->bbox.h;
				/*
				scale_ratio = std::stoi(it->GetExtraAttribute("scaler_ratio"));
				x1 = it->bbox.x * scale_ratio * (1.0 * width / rz.width);
				y1 = it->bbox.y * scale_ratio * (1.0 * height / rz.height);
				w = it->bbox.w * scale_ratio * (1.0 * width / rz.width);
				h = it->bbox.h * scale_ratio * (1.0 * height / rz.height);
				*/
				cv::Point left_up_core, right_bottom_core;
				/*
				* 眼到头顶取中间作为上点y1, 鼻子到眼取中点作为y2
				* 左眼到脸左边界取中点作为x1, 右眼到脸的右边界作为x2
				*/
				std::vector<float> pts = it->GetFeature("face_pts");// pts: x0,x1,x2,x3,x4,y0,y1,y2,y3,y4
				if (pts.size() == 0) continue;
				left_up_core.y = (1.0 * height / rz.height) * scale_ratio * (it->bbox.y + pts[5]) * (1.0 / 2.0);
				right_bottom_core.y = (1.0 * height / rz.height) * scale_ratio * (pts[5] + (pts[7] - pts[5]) * ( 1.0 / 2.0));
				left_up_core.x = (1.0 * width / rz.width) * scale_ratio * (it->bbox.x + pts[0]) * (1.0 / 2.0);
				right_bottom_core.x = (1.0 * width / rz.width) * scale_ratio * (pts[1] + it->bbox.x + it->bbox.w) * (1.0 / 2.0);
				//cv::Mat roi = src(cv::Rect(left_up_core, right_bottom_core));
				//cv::imwrite("../../../data/images/output/face.jpg", roi);
				cv::Mat* roi = new Mat(src, cv::Rect(left_up_core, right_bottom_core));
				etvh_model_->PushMatToPool(roi);
				NetOutputs result;
				etvh_model_->GetResultAsynchr(result);
				it->AddFeature("etv", result[0][0]);
				it->AddFeature("eth", result[1][0]);
			}
			// Asynchronous processing
			// 测试了下多人的时候异步处理会出现错误
			/*
			for (auto& it : objs_ptr->objs_) {
				NetOutputs result;
				etvh_model_->GetResultAsynchr(result);
				it->AddFeature("etv", result[0][0]);
				it->AddFeature("eth", result[1][0]);
			}*/
		}
		while (running_ &&
			next_connector->connector_->PushDataBufferToConveyor(conveyor_idx, data) == false) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
}

void SentimentAnalysis::DrawByObj(cv::Mat& src, std::shared_ptr<easysa::InferObject> obj,
									bool skip_fer, bool skip_etvh) {
	float scale_ratio = std::stoi(obj->GetExtraAttribute("scaler_ratio"));
	int width = src.cols;
	int height = src.rows;
	cv::Size rz(640, 640);
	float x1, x2,y1, y2, w1, h1;
	w1 = scale_ratio * (1.0 * width / rz.width);
	h1 = scale_ratio * (1.0 * height / rz.height);
	x1 = obj->bbox.x * w1;
	y1 = obj->bbox.y * h1;
	x2 = x1 + obj->bbox.w * w1;
	y2 = y1 + obj->bbox.h * h1;
	//clip
	x1 = (x1 < width) ? x1 : width;
	x2 = (x2 < width) ? x2 : width;
	y1 = (y1 < height) ? y1 : height;
	y2 = (y2 < height) ? y2 : height;
	cv::Point p1(x1, y1), p2(x2, y1), p3(x1, y2), p4(x2, y2);
	cv::line(src, p1, p2, cv::Scalar(255, 255, 255), 2, cv::LineTypes::LINE_8);
	cv::line(src, p1, p3, cv::Scalar(255, 255, 255), 2, cv::LineTypes::LINE_8);
	cv::line(src, p2, p4, cv::Scalar(255, 255, 255), 2, cv::LineTypes::LINE_8);
	cv::line(src, p3, p4, cv::Scalar(255, 255, 255), 2, cv::LineTypes::LINE_8);
	std::vector<float> pts = obj->GetFeature("face_pts");
	/*
	for (size_t j = 0; j < 5; j++) {
		//if (j < 2) continue;
		cv::Point2f pt = cv::Point2f(pts[j] * w1,
			pts[5 + j] * h1);
		cv::circle(src, pt, 1, Scalar(0, 255, 0), 2);
	}*/
	std::string face_pose = "face_pose:" + obj->GetExtraAttribute("face_pose");
	if (!skip_fer) {
		std::vector<float> fer_result = obj->GetFeature("fer");
		fer_s = "fer:" + GetLabel(fer_result, LABEL_TYPE::FER);
	}
	if (!skip_etvh) {
		std::vector<float> etv = obj->GetFeature("etv");
		std::vector<float> eth = obj->GetFeature("eth");
		v_direction = "V:" + GetLabel(etv, LABEL_TYPE::V_TYPE);
		h_direction = "H:" + GetLabel(eth, LABEL_TYPE::H_TYPE);
	}
	cv::putText(src, fer_s, cv::Point(x1, y1 + 40), 1, FONT_HERSHEY_PLAIN,
		Scalar(0, 255, 255), 1, LINE_8);
	cv::putText(src, v_direction, cv::Point(x1, y1 + 20), 1, FONT_HERSHEY_PLAIN,
		Scalar(0, 255, 255), 1, LINE_8);
	cv::putText(src, h_direction, cv::Point(x1, y1 + 30), 1, FONT_HERSHEY_PLAIN,
		Scalar(0, 255, 255), 1, LINE_8);
	cv::putText(src, face_pose, cv::Point(x1, y1 + 10), 1, FONT_HERSHEY_PLAIN,
		Scalar(0, 255, 255), 1, LINE_8);
}
std::string SentimentAnalysis::GetLabel(const std::vector<float>& result, LABEL_TYPE type) {
	/*
	assert(result.size() > 0);
	switch (type)
	{
	case SentimentAnalysis::FER:
		assert(result.size() == 7);
		break;
	case SentimentAnalysis::V_TYPE:
		assert(result.size() == 3);
		break;
	case SentimentAnalysis::H_TYPE:
		assert(result.size() == 7);
		break;
	default:
		break;
	}
	*/
	size_t index = 0;
	float max = result[0];
	size_t sz = result.size();
	for (size_t i = 1; i < sz; ++i) {
		if (result[i] > max) {
			index = i;
			max = result[i];
		}
	}
	switch (type)
	{
	case SentimentAnalysis::FER:
		return fer_status_[index];
		break;
	case SentimentAnalysis::V_TYPE:
		return v_status_[index];
		break;
	case SentimentAnalysis::H_TYPE:
		return h_status_[index];
		break;
	default:
		return "";
		break;
	}
	return "";
}
void SentimentAnalysis::OsdModule() {
	std::string connector_name = etvh_ + "_" + osd_;
	auto it = connector_map_.find(connector_name);
	if (it == connector_map_.end()) {
		LOG(ERROR) << "can`t find" << connector_name << "connector";
		return;
	}
	auto& connector = it->second;
	size_t conveyor_idx = std::stoi(stream_id_) % connector->connector_->GetConveyorCount();
	cv::Size rz(640, 640);
	namedWindow("sentiment_analysis", WINDOW_AUTOSIZE);
	// 计时
	float time = 0;
	int count = 0;
	RK::Timer ti;
	while (running_) {
		FrameInfoPtr data = nullptr;
		while (!connector->connector_->IsStopped() && data == nullptr) {
			data = connector->connector_->PopDataBufferFromConveyor(conveyor_idx);
		}
		if (connector->connector_->IsStopped()) {
			break;
		}
		if (data == nullptr) {
			continue;
		}

		// process
		auto frame = easysa::GetDataFramePtr(data);
		auto objs_ptr = easysa::GetInferObjsPtr(data);
		cv::Mat src = frame->src_mat;
		auto skip_status = frame->GetSkipStatus();
		bool skip_fer = false;
		bool skip_etvh = false;
		if (1 == skip_status[2]){
			skip_fer = true;
		}
		if (1 == skip_status[3]) {
			skip_etvh = true;
		}
		for (auto& it : objs_ptr->objs_) {
			DrawByObj(src, it, skip_fer, skip_etvh);
		}
		
		cv::imshow("sentiment_analysis", src);
		char c = waitKey(1);
		if (c == 27)
		{
			break;
		}
		time += ti.elapsedMilliSeconds();
		count++;
		if (count % 25 == 0) {
			printf("face detection average time = %f.\n", time / (1000 * count));
		}
	}
	cv::destroyAllWindows();
}

int SentimentAnalysis::PredictFacePose(const FaceDetectInfo& info) {
	std::vector<float> output_lab;
	output_lab.clear();
	auto& pts = info.pts;
	float w_ratio = info.rect.x2 - info.rect.x1;
	float n_ration = pts.x[2] - info.rect.x1;
	float l_ration = pts.x[3] - info.rect.x1;
	float r_ration = pts.x[4] - info.rect.x1;
	n_ration /= w_ratio;
	l_ration /= w_ratio;
	r_ration /= w_ratio;
	output_lab.push_back(n_ration);
	output_lab.push_back(l_ration);
	output_lab.push_back(r_ration);
	return fpc_->Predict(output_lab, 20, 5);
}
bool SentimentAnalysis::InitModule() {
	do {
		// init retinaface
		rf_ = new RetinaFace(rtfm_, "net3", 0.4);
		if (rf_ == nullptr) {
			LOG(ERROR) << "[sa]: init retinaface failed";
			break;
		}
		// init face poses classfier model
		fpc_ = new FacePositionClassifier(fpc_path_);
		if (fpc_ == nullptr) {
			LOG(ERROR) << "[sa]: init face pose classifer failed";
			break;
		}
		// init fer model
		fer_model_ = new Fer(fer_path_, fer_cache_path_);
		if (fer_model_->Init(2, 12) == false) {
			LOG(ERROR) << "[sa]: init face emotion recongiztion model failed";
			break;
		}
		// init etvh model
		etvh_model_ = new Etvh(etvh_path_, etvh_cache_path_);
		if (etvh_model_->Init(2, 12) == false) {
			LOG(ERROR) << "[sa]: init etvh model failed";
			break;
		}
		return true;
	} while (false);

	if (rf_) {
		delete rf_;
		rf_ = nullptr;
	}
	if (fpc_) {
		delete fpc_;
		fpc_ = nullptr;
	}
	if (fer_model_) {
		delete fer_model_;
		fer_model_ = nullptr;
	}
	if (etvh_model_) {
		delete etvh_model_;
		etvh_model_ = nullptr;
	}
	return false;
}

void SentimentAnalysis::Run() {
	do {
		frame_controller_ = new FrameController(frame_rate_);
		if (frame_controller_ == nullptr) break;
		fer_fp_thread_ = new thread(&SentimentAnalysis::FerModule, this);
		if (fer_fp_thread_ == nullptr) break;
		etvh_thread_ = new thread(&SentimentAnalysis::EtvhModule, this);
		if (etvh_thread_ == nullptr) break;
		detect_thread_ = new thread(&SentimentAnalysis::DetectModule, this);
		if (detect_thread_ == nullptr) break;
		source_thread_ = new thread(&SentimentAnalysis::ProvideSource, this);
		if (source_thread_ == nullptr) break;
		return;
	} while (false);
	if (frame_controller_) {
		delete frame_controller_;
		frame_controller_ = nullptr;
	}
	if (fer_fp_thread_) {
		delete fer_fp_thread_;
		fer_fp_thread_ = nullptr;
	}
	if (etvh_thread_) {
		delete etvh_thread_;
		etvh_thread_ = nullptr;
	}
	if (detect_thread_) {
		delete detect_thread_;
		detect_thread_ = nullptr;
	}
	if (source_thread_) {
		delete source_thread_;
		source_thread_ = nullptr;
	}
	//osd_thread_ = new thread(&SentimentAnalysis::OsdModule, this);
}
SentimentAnalysis::~SentimentAnalysis() {
	running_ = false;
	if (rf_ != nullptr) {
		delete rf_;
		rf_ = nullptr;
	}
	if (fpc_ != nullptr) {
		delete fpc_;
		fpc_ = nullptr;
	}
	if (fer_model_ != nullptr) {
		delete fer_model_;
		fer_model_ = nullptr;
	}
	if (etvh_model_ != nullptr) {
		delete etvh_model_;
		etvh_model_ = nullptr;
	}
	if (source_thread_) {
		source_thread_->join();
	}
	if (detect_thread_) {
		detect_thread_->join();
	}
	if (fer_fp_thread_) {
		fer_fp_thread_->join();
	}
	if (etvh_thread_) {
		etvh_thread_->join();
	}
	if (frame_controller_) {
		delete frame_controller_;
		frame_controller_ = nullptr;
	}
	if (fsc_) {
		delete fsc_;
		fsc_ = nullptr;
	}
	/*
	if (osd_thread_) {
		osd_thread_->join();
	}*/
}