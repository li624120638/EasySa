#ifndef COMPONENTS_INCLUDE_MODEL_MEMORY__
#define COMPONENTS_INCLUDE_MODEL_MEMORY__
#include <atomic>
#include <functional>
#include <vector>
#include <memory>
#include <model_loader.hpp>
#include "memory_queue.hpp"

namespace easysa {
	namespace components {
/*
* @brief this class use to alloc trt Model memory.
*/
class ModelMemory {
public:
	ModelMemory(std::string inputBlobName, std::vector<std::string> outputsBlobName);
	virtual ~ModelMemory();
	bool AlloctorMemory(ModelLoader* model = nullptr);
public:
	std::atomic<bool> init_{ false };
	// status_ use to flag whether this memory is in using status 
	std::atomic<bool> status_{ false };
	int numBinding = 0;
	float* cpuBuffers = nullptr;//cpu input buffer
	void** buffers = nullptr;//gpu buffer include input and outputs
	std::vector<float*> outputBuffers;//cpu output buffer
public:
	uint32_t max_batch_sz_ = 1;
	std::string inputBlobName_;
	std::vector<std::string> outputsBlobName_;
	int inputIndex;
	std::vector<int> outputIndexs;
	// input and outputs Dims
	DimsCHW inputDims;
	std::vector<DimsCHW> outputDims;
	// input and outputs sz
	size_t inputSize;
	std::vector<size_t> outputsizes;
public:
	int netWidth = 0;
	int netHight = 0;
	int channel = 0;
}; // class ModelMemory

/*
* this class use manage alloc ModelMemory
* 
*/
class MemoryPool {
public:
	explicit MemoryPool(std::string model_name) :
						model_name_(model_name) { running_ = true; }
	MemoryPool(const MemoryPool&) = delete;
	MemoryPool& operator = (const MemoryPool&) = delete;
	virtual ~MemoryPool();
public:
	// input new ModelMemory into memory pool
	bool PushNewModelMemory(std::shared_ptr<ModelMemory> mm);
	// get one pre model memory for store prepocess result
	std::shared_ptr<ModelMemory> PopOnePreModelMemory();
	// get one infer model memory for inference
	std::shared_ptr<ModelMemory> PopOneInferModelMemory();
	// get one post model memory for postpocess
	std::shared_ptr<ModelMemory> PopOnePostModelMemory();
	// after postprocess,you should put the invalid model memory in pre model memory
	bool PushOnePreModelMemory(std::shared_ptr<ModelMemory> mm);
	// after prepocess, put the model memory in infer model memory queue
	bool PushOneInferModelMemory(std::shared_ptr<ModelMemory> mm);
	// after inference, put the model in posr model memory queue
	bool PushOnePostModelMemory(std::shared_ptr<ModelMemory> mm);
public:
	int GetMMNumberHaveInput() { return num_mm; }
	// this function not thread safe
	int GetMMNumberInPool() {
		return pre_queue_.Size() + post_queue_.Size() + infer_queue_.Size();
	}
private:
	std::string model_name_;
	std::atomic<bool> running_{ false };
	// num_mm may not equal the number of model memory in pool now; 
	std::atomic<uint32_t> num_mm{ 0 };
private:
	// the use shared_ptr just because if model memory has not in pool, but i
	// want to deconstruct pool. avoid memory leak
	// this queue use to preprocess function
	MemorySafeQueue<std::shared_ptr<ModelMemory>> pre_queue_;
	// this queue use to store Model memory which need inference
	MemorySafeQueue<std::shared_ptr<ModelMemory>> infer_queue_;
	// this queue use to store inference result
	MemorySafeQueue<std::shared_ptr<ModelMemory>> post_queue_;

}; // class MemoryPool

	} // namespace components
} // namespace easysa
#endif // !COMPONENTS_INCLUDE_MODEL_MEMORY__
