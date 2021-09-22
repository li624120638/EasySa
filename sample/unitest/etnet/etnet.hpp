#ifndef UNITEST_ETNET_HPP
#define UNITEST_ETNET_HPP
#include <glog/logging.h>
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
using namespace easysa::components;
struct EtNetBlob
{
	std::string layer_name;
	int layer_index;
	int outputSize;
	std::vector<std::vector<float>> result;
	DimsCHW outputDims;
	int batchsize;
};

class EtNet {
public:
	EtNet(std::string etnet_path, std::string etnet_cache_path);
	~EtNet();
	void DoInference(const cv::Mat& img);
	std::vector<EtNetBlob> GetResult();
	std::vector<int> GetPredictResult();//top 1
private:
	std::string etnet_cache_path_;
	std::string etnet_path_;
	ICudaEngine* engine_ = nullptr;
	IExecutionContext* context_ = nullptr;
	IRuntime* runtime_ = nullptr;
	float* cpuBuffers = nullptr;//cpu input buffer
	void** buffers = nullptr;//gpu buffer include input and outputs
	std::vector<float*> outputBuffers;//cpu output buffer
	std::vector<EtNetBlob> results;
	// input and outputs Dims
	DimsCHW inputDims;
	std::vector<DimsCHW> outputDims;
	// input and outputs sz
	size_t inputSize;
	std::vector<size_t> outputsizes;
	// input or outputs`s blob and its index
	std::string inputBlobName = "inputs:0";
	std::vector<std::string> outputs = {
	"output_1","output_2","output_3","output_4",
	};
	int inputIndex;
	std::vector<int> outputIndexs;

	uint32_t max_batch_sz = 1;
	int numBinding; // input and outputs bind number
	int netWidth = 224;
	int netHight = 224;
	int channel = 3;
	int output_sz = 4;
	std::mutex mtx_;
}; // class EtNet
#endif //UNITEST_ETNET_HPP