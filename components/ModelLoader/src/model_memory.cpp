#include "model_memory.hpp"
#include <glog/logging.h>
#define CHECK(status)									\
{														\
    if (status != 0)									\
    {													\
        printf("Cuda failure: %d.\n", status); 		    \
        abort();										\
    }													\
}
namespace easysa {
	namespace components {
ModelMemory::ModelMemory(std::string inputBlobName,
	std::vector<std::string> outputsBlobName) :inputBlobName_(inputBlobName),
	outputsBlobName_(outputsBlobName){
}
bool ModelMemory::AlloctorMemory(ModelLoader* model) {
	if (init_) {
		LOG(WARNING) << "ModelMemory has inited";
		return false;
	}
	
	const ICudaEngine& cudaEngine = model->GetCudaEngine();
	numBinding = cudaEngine.getNbBindings();//5
	buffers = new void* [numBinding];
	for (int i = 0; i < numBinding; i++)
	{
		buffers[i] = nullptr;
	}
	inputIndex = cudaEngine.getBindingIndex(inputBlobName_.c_str());
	outputIndexs.resize(outputsBlobName_.size());
	for (size_t i = 0; i < outputsBlobName_.size(); i++) {
		int idx = cudaEngine.getBindingIndex(outputsBlobName_[i].c_str());
		outputIndexs[i] = idx;
	}
	inputDims = static_cast<DimsCHW&&>(cudaEngine.getBindingDimensions(inputIndex));
	outputDims.resize(outputIndexs.size());
	for (size_t i = 0; i < outputIndexs.size(); i++) {
		DimsCHW dim = static_cast<DimsCHW&&>(cudaEngine.getBindingDimensions(outputIndexs[i]));
		outputDims[i] = dim;
	}
	inputSize = max_batch_sz_ * inputDims.c() * inputDims.h() * inputDims.w() * sizeof(float);
	outputsizes.resize(outputDims.size());
	for (size_t i = 0; i < outputDims.size(); i++) {
		size_t sz;
		size_t c_sz = (outputDims[i].c() == 0) ? 1 : outputDims[i].c();
		size_t h_sz = (outputDims[i].h() == 0) ? 1 : outputDims[i].h();
		size_t w_sz = (outputDims[i].w() == 0) ? 1 : outputDims[i].w();
		sz = static_cast<size_t>(max_batch_sz_) * c_sz * h_sz * w_sz * sizeof(float);
		outputsizes[i] = sz;
	}
	/*
	* 这里好像有问题,我模型转换的时候是224 * 224 * 3
	*/  
	netWidth = inputDims.c();
	netHight = inputDims.h();
	channel = inputDims.w();
	cpuBuffers = new float[netWidth * netHight * channel * max_batch_sz_];
	for (size_t i = 0; i < outputsizes.size(); i++) {
		outputBuffers.push_back(nullptr);
	}
	for (size_t i = 0; i < outputBuffers.size(); i++) {
		if (outputBuffers[i] == nullptr) {
			outputBuffers[i] = (float*)malloc(outputsizes[i]);
			assert(outputBuffers[i] != nullptr);
		}
	}
	// create GPU buffers and a stream
	if (buffers[inputIndex] == nullptr) {
		CHECK(cudaMalloc(&buffers[inputIndex], inputSize));
	}
	for (size_t i = 0; i < outputIndexs.size(); i++) {
		if (buffers[outputIndexs[i]] == nullptr) {
			CHECK(cudaMalloc(&buffers[outputIndexs[i]], outputsizes[i]));
		}
	}
	init_ = true;
	return true;
}
ModelMemory::~ModelMemory() {
	// free cpu buffer
	if (cpuBuffers != nullptr) {
		delete cpuBuffers;
		cpuBuffers = nullptr;
	}
	for (size_t i = 0; i < outputBuffers.size(); i++) {
		if (outputBuffers[i] == nullptr) {
			free(outputBuffers[i]);
			outputBuffers[i] = nullptr;
		}
	}
	// free gpu buffer
	for (int i = 0; i < numBinding; i++) {
		if (buffers[i] != nullptr) {
			cudaFree(buffers[i]);
			buffers[i] = nullptr;
		}
	}
	if (buffers != nullptr) {
		delete[] buffers;
		buffers = nullptr;
	}
}
/****************************************************
*******************  MemoryPool ***********************
**************************************************/

bool MemoryPool::PushNewModelMemory(std::shared_ptr<ModelMemory> mm) {
	if (mm == nullptr) {
		LOG(ERROR) << "Push New Model Memory failed";
		return false;
	}
	pre_queue_.Push(mm);
	num_mm++;
	return true;
}
std::shared_ptr<ModelMemory> MemoryPool::PopOnePreModelMemory() {
	std::shared_ptr<ModelMemory> mm = nullptr;
	pre_queue_.WaitAndTryPop(mm, (std::chrono::microseconds)20);
	return mm;
}

std::shared_ptr<ModelMemory> MemoryPool::PopOneInferModelMemory() {
	std::shared_ptr<ModelMemory> mm = nullptr;
	infer_queue_.WaitAndTryPop(mm, (std::chrono::microseconds)20);
	return mm;
}

std::shared_ptr<ModelMemory> MemoryPool::PopOnePostModelMemory() {
	std::shared_ptr<ModelMemory> mm = nullptr;
	post_queue_.WaitAndTryPop(mm, (std::chrono::microseconds)20);
	return mm;
}

bool MemoryPool::PushOnePreModelMemory(std::shared_ptr<ModelMemory> mm) {
	if (mm == nullptr) {
		LOG(ERROR) << "PushOnePreModelMemory, the model memory is NULL";
		return false;
	}
	pre_queue_.Push(mm);
	return true;
}
bool MemoryPool::PushOneInferModelMemory(std::shared_ptr<ModelMemory> mm) {
	if (mm == nullptr) {
		LOG(ERROR) << "PushOnePreModelMemory, the model memory is NULL";
		return false;
	}
	infer_queue_.Push(mm);
	return true;
}

bool MemoryPool::PushOnePostModelMemory(std::shared_ptr<ModelMemory> mm) {
	if (mm == nullptr) {
		LOG(ERROR) << "PushOnePreModelMemory, the model memory is NULL";
		return false;
	}
	post_queue_.Push(mm);
	return true;
}

MemoryPool::~MemoryPool() {
	running_.store(false);
}

	} // namespace components
} // namespace easysa