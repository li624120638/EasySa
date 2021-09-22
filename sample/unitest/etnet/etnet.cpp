#include "etnet.hpp"

#define CHECK_(status)									\
{														\
    if (status != 0)									\
    {													\
        printf("Cuda failure: %d.\n", status); 		    \
        abort();										\
    }													\
}
EtNet::EtNet(std::string etnet_path, std::string etnet_cache_path) {
	etnet_path_ = etnet_path;
	etnet_cache_path_ = etnet_cache_path;
	runtime_ = createInferRuntime(gLogger);
	std::ifstream fin(etnet_cache_path_.c_str(), std::ios_base::in | std::ios_base::binary);
	if (fin.good()) {
		size_t sz = 0;
		size_t i = 0;
		fin.seekg(0, std::ios::end);
		sz = fin.tellg();
		fin.seekg(0, std::ios::beg);
		char* buff = new char[sz];
		fin.read(buff, sz);
		fin.close();
		engine_ = runtime_->deserializeCudaEngine((void*)buff, sz, NULL);
		delete buff;
	}
	else {
		LOG(INFO) << "test eye tracking net start!";
		LOG(INFO) << "Load etnet onnx model";
		IBuilder* builder = createInferBuilder(gLogger);
		nvinfer1::INetworkDefinition* network = builder->createNetwork();
		nvonnxparser::IParser* parser = nvonnxparser::createParser(*network, gLogger);
		parser->parseFromFile(etnet_path_.c_str(), static_cast<int>(Logger::Severity::kWARNING));
		for (int i = 0; i < parser->getNbErrors(); i++) {
			LOG(INFO) << parser->getError(i)->desc();
		}
		LOG(INFO) << "Load etnet onnx model sucess!";
		LOG(INFO) << "etnet : build engine start";

		builder->setMaxBatchSize(max_batch_sz);
		IBuilderConfig* config = builder->createBuilderConfig();
		config->setMaxWorkspaceSize(1 << 20);
		engine_ = builder->buildEngineWithConfig(*network, *config);
		if (!engine_) {
			LOG(ERROR) << "etnet : create engine failed";
		}
		LOG(INFO) << "etnet: create engine suceess! ";
		LOG(INFO) << "etnet: serialize Model start";
		IHostMemory* modelStream = engine_->serialize();
		std::string serialize_str;
		serialize_str.resize(modelStream->size());
		memcpy((void*)serialize_str.data(), modelStream->data(), modelStream->size());
		std::ofstream srz_out_stream;
		srz_out_stream.open(etnet_cache_path_.c_str(), std::ios_base::out | std::ios_base::binary);
		srz_out_stream << serialize_str;
		srz_out_stream.close();
		LOG(INFO) << "serizalize etnet model sucess!";
		network->destroy();
		parser->destroy();
		builder->destroy();
	}
	context_ = engine_->createExecutionContext();
	if (!context_) {
		LOG(ERROR) << "etnet : create execution context failed";
	}
	const ICudaEngine& cudaEngine = context_->getEngine();
	numBinding = cudaEngine.getNbBindings();//5
	buffers = new void* [numBinding];
	for (int i = 0; i < numBinding; i++)
	{
		buffers[i] = nullptr;
	}
	inputIndex = cudaEngine.getBindingIndex(inputBlobName.c_str());
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
	inputDims = static_cast<DimsCHW&&>(cudaEngine.getBindingDimensions(inputIndex));
	outputDims.resize(outputIndexs.size());
	for (size_t i = 0; i < outputIndexs.size(); i++) {
		DimsCHW dim = static_cast<DimsCHW&&>(cudaEngine.getBindingDimensions(outputIndexs[i]));
		outputDims[i] = dim;
		results[i].outputDims = dim;
	}
	inputSize = max_batch_sz * inputDims.c() * inputDims.h() * inputDims.w() * sizeof(float);
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
	cpuBuffers = new float[netWidth * netHight * channel * max_batch_sz];
	for (size_t i = 0; i < output_sz; i++) {
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
		CHECK_(cudaMalloc(&buffers[inputIndex], inputSize));
	}
	for (size_t i = 0; i < outputIndexs.size(); i++) {
		if (buffers[outputIndexs[i]] == nullptr) {
			CHECK_(cudaMalloc(&buffers[outputIndexs[i]], outputsizes[i]));
		}
	}
}

EtNet::~EtNet() {
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
	context_->destroy();
	engine_->destroy();
	runtime_->destroy();
}

void EtNet::DoInference(const cv::Mat& img) {
	std::lock_guard<std::mutex> guard(mtx_);
	cv::Mat rz_img;
	cv::resize(img, rz_img, cv::Size(netWidth, netHight));
	rz_img.convertTo(rz_img, CV_32FC3);
	cvtColor(rz_img, rz_img, cv::COLOR_BGR2RGB);
	// r g b mean 123.68, 116.78, 103.94
	rz_img = rz_img - cv::Scalar(123.68,116.78,103.94);
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
	//do inference
	cudaStream_t stream;
	CHECK_(cudaStreamCreate(&stream));
	context_->execute(max_batch_sz, buffers);
	for (size_t i = 0; i < outputDims.size(); i++) {
		CHECK_(cudaMemcpyAsync(outputBuffers[i], buffers[outputIndexs[i]], outputsizes[i], cudaMemcpyDeviceToHost));
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
}

std::vector<EtNetBlob> EtNet::GetResult() {
	std::lock_guard<std::mutex> guard(mtx_);
	return results;
}

const static std::vector<std::vector<int>> labels = { {-30, -15, 0, 15, 30},
													 {-10, 0, 10},
													{-15, -10, -5, 0, 5, 10, 15}};
std::vector<int>  EtNet::GetPredictResult() {
	std::lock_guard<std::mutex> guard(mtx_);
	std::vector<int> res;
	int sz = 3;
	for (int i = 0; i < sz; i++) {
		float max = results[i].result[0][0];
		int index = 0;
		for (int j = 0; j < results[i].result[0].size(); j++) {
			if (results[i].result[0][j] > max) {
				max = results[i].result[0][j];
				index = j;
			}
		}
		res.push_back(labels[i][index]);
	}
	return res;
}