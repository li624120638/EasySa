#ifndef PLUGINS_FER_INCLUDE_FER_HPP__
#define PLUGINS_FER_INCLUDE_FER_HPP__
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <opencv2/opencv.hpp>
#include "model_memory.hpp"
#include "model_loader.hpp"
#include "thread_pool.hpp"

using namespace easysa::components;
using NetOutputs = std::vector<std::vector<std::vector<float>>>;
/*
* @brief this class use to Facial expression recognition
* @parmeter model_path: the onnx model path
* @parmeter model_cache_path: the serialize model result to store
*/
class Fer {
public:
	Fer(std::string model_path, std::string model_cache_path);
	bool Init(uint32_t infer_engin_num, uint32_t model_memory_number = 0);
	bool PushMatToPool(cv::Mat* src);
	void PreProcess(void*);
	bool GetResultAsynchr(NetOutputs& results);
	std::vector<float> GetSoftMaxResult(const NetOutputs& results);
	std::vector<float> GetSoftMaxResult();
	virtual ~Fer();
private:
	// user not need to call this function
	void Infer();
private:
	ThreadPool* pre_pool_ = nullptr;
	std::shared_ptr<MemoryPool> pool_ = nullptr;
	ModelLoader* model_ = nullptr;
	std::string inputBolbName = "inputs:0";
	std::vector<std::string> outputsBlobName = { "output_1" };
private:
	uint32_t infer_engin_num_ = 1;
	std::vector<std::thread*> infer_thread_vec_;
	std::string model_cache_path_;
	std::string model_path_;
	std::atomic<bool> is_init_{ false };
	std::mutex mtx_;
	std::atomic<bool> running_{false};
}; // class Fer
#endif // !PLUGINS_FER_INCLUDE_FER_HPP__
