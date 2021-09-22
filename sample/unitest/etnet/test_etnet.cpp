#include "etnet.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <opencv2/opencv.hpp>
using namespace easysa::components;
//#include <cuda_runtime_api.h>
//#include "NvInfer.h"
//#include "NvOnnxParser.h"
//#include "common.h"
//#include "logging.h"
/*
#define CHECK(status)									\
{														\
    if (status != 0)									\
    {													\
        printf("Cuda failure: %d.\n", status); 		\
        abort();										\
    }													\
}
struct EtNetBlob
{
	std::string layer_name;
	int layer_index;
	int outputSize;
	std::vector<std::vector<float>> result;
	DimsCHW outputDims;
	int batchsize;
};

int main() {
	std::string etnet_cache_path = "./etnet.cache";
	constexpr char* etnet_path = "../../../data/models/etnet_onnx/etnet.onnx";
	std::string img_path = "../../../data/images/0001_2m_0P_0V_0H.jpg";
	ICudaEngine* engine = nullptr;
	IExecutionContext* context = nullptr;
	IRuntime* runtime = nullptr;
	float* cpuBuffers = nullptr;//cpu input buffer
	void** buffers = nullptr;//gpu buffer include input and outputs
	std::vector<float*> outputBuffers;//cpu output buffer
	std::vector<EtNetBlob> results;
	uint32_t max_batch_sz = 1;
	// deserialize model
	runtime = createInferRuntime(gLogger);
	std::ifstream fin(etnet_cache_path.c_str(), std::ios_base::in | std::ios_base::binary);
	if (fin.good()) {
		size_t sz = 0;
		size_t i = 0;
		fin.seekg(0, std::ios::end);
		sz = fin.tellg();
		fin.seekg(0, std::ios::beg);
		char* buff = new char[sz];
		fin.read(buff, sz);
		fin.close();
		engine = runtime->deserializeCudaEngine((void*)buff, sz, NULL);
		delete buff;
	} else {
		LOG(INFO) << "test eye tracking net start!";
		LOG(INFO) << "Load etnet onnx model";
		IBuilder* builder = createInferBuilder(gLogger);
		nvinfer1::INetworkDefinition* network = builder->createNetwork();
		nvonnxparser::IParser* parser = nvonnxparser::createParser(*network, gLogger);
		parser->parseFromFile(etnet_path, static_cast<int>(Logger::Severity::kWARNING));
		for (int i = 0; i < parser->getNbErrors(); i++) {
			LOG(INFO) << parser->getError(i)->desc();
		}
		LOG(INFO) << "Load etnet onnx model sucess!";

		LOG(INFO) << "etnet : build engine start";
		
		builder->setMaxBatchSize(max_batch_sz);
		IBuilderConfig* config = builder->createBuilderConfig();
		config->setMaxWorkspaceSize(1 << 20);
		engine = builder->buildEngineWithConfig(*network, *config);

		if (!engine) {
			LOG(ERROR) << "etnet : create engine failed";
		}
		LOG(INFO) << "etnet: create engine suceess! ";
		LOG(INFO) << "etnet: serialize Model start";
		IHostMemory* modelStream = engine->serialize();
		std::string serialize_str;
		serialize_str.resize(modelStream->size());
		memcpy((void*)serialize_str.data(), modelStream->data(), modelStream->size());
		std::ofstream srz_out_stream;
		srz_out_stream.open(etnet_cache_path.c_str(), std::ios_base::out | std::ios_base::binary);
		srz_out_stream << serialize_str;
		srz_out_stream.close();
		LOG(INFO) << "serizalize etnet model sucess!";
		network->destroy();
		parser->destroy();
		builder->destroy();
	}
	context = engine->createExecutionContext();
	if (!context) {
		LOG(ERROR) << "etnet : create execution context failed";
	}
	// get network info
	const ICudaEngine& cudaEngine = context->getEngine();
	int numBinding = cudaEngine.getNbBindings();//5
	buffers = new void* [numBinding];
	for (int i = 0; i < numBinding; i++)
	{
		buffers[i] = nullptr;
	}
	std::string inputBlobName = "inputs:0";
	int inputIndex = cudaEngine.getBindingIndex(inputBlobName.c_str());

	std::vector<int> outputIndexs;
	std::vector<std::string> outputs = {
		"output_1","output_2","output_3","output_4",
	};
	outputIndexs.resize(outputs.size());
	results.resize(outputs.size());
	for (size_t i = 0; i < outputs.size(); i++) {
		int idx = cudaEngine.getBindingIndex(outputs[i].c_str());
		outputIndexs[i] = idx;
		results[i].layer_index = idx;
	}
	for (size_t i = 0; i < outputs.size(); i++) {
		results[i].layer_name = outputs[i];
	}
	DimsCHW inputDims;
	std::vector<DimsCHW> outputDims;
	inputDims = static_cast<DimsCHW&&>(cudaEngine.getBindingDimensions(inputIndex));
	outputDims.resize(outputIndexs.size());
	for (size_t i = 0; i < outputIndexs.size(); i++) {
		DimsCHW dim = static_cast<DimsCHW&&>(cudaEngine.getBindingDimensions(outputIndexs[i]));
		outputDims[i] = dim;
		results[i].outputDims = dim;
	}
	size_t inputSize = max_batch_sz * inputDims.c() * inputDims.h() * inputDims.w() * sizeof(float);
	// preprocess 
	int batch_sz = 1;
	//这里有个错误
	//int netWidth = inputDims.w();//224
	//int netHight = inputDims.h();//224
	//int channel = inputDims.c();//3
	int netWidth = 224;
	int netHight = 224;
	int channel = 3;
	int output_sz = 4;
	for (size_t i = 0; i < output_sz; i++) {
		outputBuffers.push_back(nullptr);
	}
	std::vector<size_t> outputsizes;
	outputsizes.resize(outputDims.size());//5, 3, 7,401408(7,7,2048)
	for (size_t i = 0; i < outputDims.size(); i++) {
		size_t sz;
		if (i < 3) {
			sz = max_batch_sz * outputDims[i].c() * sizeof(float);
		}
		else {
			sz = max_batch_sz * outputDims[i].c() * outputDims[i].h() * outputDims[i].w() * sizeof(float);
		}
		outputsizes[i] = sz;
		results[i].outputSize = sz;
	}
	for (size_t i = 0; i < outputBuffers.size(); i++) {
		if (outputBuffers[i] == NULL) {
			outputBuffers[i] = (float*)malloc(outputsizes[i]);
			assert(outputBuffers[i] != NULL);
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
	cpuBuffers = new float[netWidth * netHight * channel * max_batch_sz];

	cv::Mat tst_img = cv::imread(img_path.c_str(),cv::IMREAD_COLOR);
	cv::Mat rz_img;
	cv::resize(tst_img,rz_img,cv::Size(netWidth, netHight));
	rz_img.convertTo(rz_img, CV_32FC3);
	cvtColor(rz_img, rz_img, cv::COLOR_BGR2RGB);

	std::vector<cv::Mat> input_channels;
	float* input_data = cpuBuffers;
	for (int i = 0; i < channel; ++i) {
		cv::Mat channel(netHight, netWidth, CV_32FC1, input_data);
		input_channels.push_back(channel);
		input_data += netWidth * netHight;
	}
	cv::split(rz_img, input_channels);
	float* inputData = (float*)buffers[0];
	cudaMemcpy(inputData, cpuBuffers, netWidth * netHight * 3 * sizeof(float), cudaMemcpyHostToDevice);

	// do inference
	cudaStream_t stream;
	CHECK(cudaStreamCreate(&stream));
	context->execute(max_batch_sz, buffers);
	for (size_t i = 0; i < outputDims.size(); i++) {
		CHECK(cudaMemcpyAsync(outputBuffers[i], buffers[outputIndexs[i]], outputsizes[i], cudaMemcpyDeviceToHost));
	}
	//output to vector
	for (size_t i = 0; i < outputBuffers.size(); i++) {
		int count = outputsizes[i] / (sizeof(float) * max_batch_sz);
		results[i].result.clear();
		for (int j = 0; j < max_batch_sz; j++) {
			const float* confidenceBegin = outputBuffers[i] + j * count;
			const float* confidenceEnd = confidenceBegin + count;
			std::vector<float> ret = std::vector<float>(confidenceBegin, confidenceEnd);
			results[i].result.push_back(ret);
		}
		results[i].batchsize = max_batch_sz;
	}

	// destory
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
		delete[] buffers;
		buffers = nullptr;
	}
	context->destroy();
	engine->destroy();
	runtime->destroy();
	return 0;
}
*/
int main() {
	std::string etnet_cache_path = "./etnet.cache";
	std::string etnet_path = "../../../data/models/etnet_onnx/etnet.onnx";
	std::string img_path = "../../../data/images/0001_2m_0P_0V_0H.jpg";
	EtNet etnet(etnet_path, etnet_cache_path);
	cv::Mat tst_img = cv::imread(img_path.c_str(), cv::IMREAD_COLOR);
	if (tst_img.empty()) {
		LOG(ERROR) << "etnet test open imges failed";
	}
	etnet.DoInference(tst_img);
	//std::vector<EtNetBlob> results = etnet.GetResult();
	std::vector<int> res = etnet.GetPredictResult();
	LOG(INFO) << res[0] << " " << res[1] << " " << res[2];
	return 0;
}