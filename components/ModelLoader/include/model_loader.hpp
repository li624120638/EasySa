#ifndef COMPONENTS_MODEL_LOADER_HPP_
#define COMPONENTS_MODEL_LOADER_HPP_
#include <glog/logging.h>
#include <atomic>
#include <string>
#include <vector>
#include <mutex>

#include <opencv2/opencv.hpp>
#include <cuda_runtime_api.h>
#include <cuda_runtime_api.h>
#include "NvInfer.h"
#include "NvOnnxParser.h"
#include "common.h"
#include "logging.h"

namespace easysa {
	namespace components {
/*
* @brief this class for load model by trt, you should transfrom your model
*  to onnx
* @model_path:the path of model, this path relative to the execution
* @model_cache_path: a path for serialize the model, or load a serialize model
*/
class ModelLoader {
public:
	// just set parmeter, and you should call Init() after call constructor function
	explicit ModelLoader(std::string model_path, std::string model_cache_path, uint32_t max_batch_sz = 1);
	bool Init();
	const std::atomic<bool>& IsInit() const { return is_init_; }
	virtual ~ModelLoader();
	// there should Dependency Inversion
	// bool CreateModelMemory(ModelMemory* model_memory);
	const ICudaEngine& GetCudaEngine() const;
	IExecutionContext* GetContext() const {
		return context_;
	}
private:
	std::atomic<bool> is_init_{ false };
	std::string model_cache_path_;
	std::string model_path_;
	uint32_t max_batch_sz_ = 1;
	ICudaEngine* engine_ = nullptr;
	IExecutionContext* context_ = nullptr;
	IRuntime* runtime_ = nullptr;
}; // class ModelLoader
	} // namespace components
} // namespace easysa
#endif // !COMPONENTS_MODEL_LOADER_HPP_
