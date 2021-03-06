#include "etvh.hpp"
#include <cassert>
#include <cmath>
Etvh::Etvh(std::string model_path, std::string model_cache_path) :
	model_path_(model_path), model_cache_path_(model_cache_path) {
	running_ = true;
}
bool Etvh::Init(uint32_t infer_engin_num, uint32_t model_memory_number) {
	pool_ = std::make_shared<MemoryPool>(model_path_);
	model_ = new ModelLoader(model_path_, model_cache_path_, 1);
	if (!model_->IsInit()) {
		bool ret = model_->Init();
	}
	// you can visited https://netron.app/ to get your model blob name
	for (size_t i = 0; i < model_memory_number; i++) {
		std::shared_ptr<ModelMemory> mm = std::make_shared<ModelMemory>(inputBolbName, outputsBlobName);
		if (mm->AlloctorMemory(model_)) {
			pool_->PushNewModelMemory(mm);
		}
	}
	// create inference thread
	infer_engin_num_ = infer_engin_num;
	infer_thread_vec_.resize(infer_engin_num_);
	for (size_t i = 0; i < infer_engin_num_; ++i) {
		infer_thread_vec_[i] = new std::thread(&Etvh::Infer, this);
	}
	pre_pool_ = new ThreadPool(3, 10, 30);
	pre_pool_->Init();
	return true;
}
bool Etvh::PushMatToPool(cv::Mat* src) {
	if (src == nullptr) return false;
	std::shared_ptr<Task> task =
		std::make_shared<Task>();
	task->function = std::bind(&Etvh::PreProcess, this, std::placeholders::_1);
	task->arg = src;
	pre_pool_->AddTaskToQueue(task);
	return true;
}

void Etvh::Infer() {
	cudaStream_t stream;
	CHECK(cudaStreamCreate(&stream));
	auto context_ = model_->GetContext();
	while (running_) {
		std::shared_ptr<ModelMemory> mm = nullptr;
		while (running_ && mm == nullptr)
		{
			mm = pool_->PopOneInferModelMemory();
		}
		if (nullptr == mm) {
			break;
		}
		assert(mm != nullptr);
		context_->execute(1, mm->buffers);
		for (size_t i = 0; i < mm->outputDims.size(); i++) {
			CHECK(cudaMemcpyAsync(mm->outputBuffers[i],
				mm->buffers[mm->outputIndexs[i]], mm->outputsizes[i], cudaMemcpyDeviceToHost));
		}
		// put model memory into need postprocess queue
		pool_->PushOnePostModelMemory(mm);
	}
}
bool Etvh::GetResultAsynchr(NetOutputs& results) {
	/*
	* Now just support batch_size = 1 case
	*/
	results.clear();
	std::shared_ptr<ModelMemory> mm = nullptr;
	while (running_) {
		mm = pool_->PopOnePostModelMemory();
		if (mm != nullptr) break;
	}
	assert(mm != nullptr);
	uint32_t max_batch_sz = 1;
	for (size_t i = 0; i < mm->outputBuffers.size(); i++) {
		int count = mm->outputsizes[i] / (sizeof(float) * max_batch_sz);
		std::vector <std::vector<float> > result;
		for (int j = 0; j < max_batch_sz; j++) {
			const float* confidenceBegin = mm->outputBuffers[i] + j * count;
			const float* confidenceEnd = confidenceBegin + count;
			std::vector<float> ret = std::vector<float>(confidenceBegin, confidenceEnd);
			result.push_back(ret);
		}
		results.push_back(result);
	}
	// input to need new preprocess data
	return pool_->PushOnePreModelMemory(mm);
}

void Etvh::PreProcess(void* arg) {
	auto src = (cv::Mat*)arg;
	std::shared_ptr<ModelMemory> mm = nullptr;
	while (running_) {
		mm = pool_->PopOnePreModelMemory();
		if (mm != nullptr) break;
	}
	assert(mm != nullptr);
	cv::Mat rz_img;
	// cv::resize(src, rz_img, cv::Size(mm->netWidth, mm->netHight));
	cv::resize(*src, rz_img, cv::Size(mm->netHight, mm->netWidth));
	// cv::imwrite("../../../data/images/resize.jpg", rz_img);
	cvtColor(rz_img, rz_img, cv::COLOR_BGR2RGB);
	rz_img.convertTo(rz_img, CV_32FC3);
	// r g b mean 123.68, 116.78, 103.94
	rz_img = rz_img - cv::Scalar(123.68, 116.78, 103.94);

	float* inputData = (float*)mm->buffers[0];
	cudaMemcpy(inputData, rz_img.data,
		mm->netWidth * mm->netHight * mm->channel * sizeof(float), cudaMemcpyHostToDevice);
	// push model memory into need infer pool
	pool_->PushOneInferModelMemory(mm);
	if (src) {
		// ??????????????????????????????????
		//delete src;
		//src = nullptr;
	}
}
Etvh::~Etvh() {
	running_ = false;
	for (auto& it : infer_thread_vec_) {
		if (it) {
			it->join();
			it = nullptr;
		}
	}
	if (pre_pool_) {
		delete pre_pool_;
		pre_pool_ = nullptr;
	}
	if (model_ != nullptr) {
		delete model_;
		model_ = nullptr;
	}
}