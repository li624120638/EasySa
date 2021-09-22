#include "model_loader.hpp"
namespace easysa {
	namespace components {
ModelLoader::ModelLoader(std::string model_path, std::string model_cache_path, uint32_t max_batch_sz) {
	this->model_path_ = model_path;
	this->model_cache_path_ = model_cache_path;
	this->max_batch_sz_ = max_batch_sz;
}

bool ModelLoader::Init() {
	// check parmeter,just for model_path
	if (model_path_.find(".onnx") == std::string::npos) {
		LOG(ERROR) << "[ModerLoader]:init failed, this version just for onnx model";
		return false;
	}
	runtime_ = createInferRuntime(gLogger);
	std::ifstream fin(model_cache_path_.c_str(), std::ios_base::in | std::ios_base::binary);

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
	} else {
		LOG(INFO) << "Load onnx model start ";
		IBuilder* builder = createInferBuilder(gLogger);
		nvinfer1::INetworkDefinition* network = builder->createNetwork();
		nvonnxparser::IParser* parser = nvonnxparser::createParser(*network, gLogger);
		parser->parseFromFile(model_path_.c_str(), static_cast<int>(Logger::Severity::kWARNING));
		for (int i = 0; i < parser->getNbErrors(); i++) {
			LOG(INFO) << parser->getError(i)->desc();
		}
		LOG(INFO) << "Load  onnx model sucess!";
		LOG(INFO) << "[ModelLoader] : build engine start";
		builder->setMaxBatchSize(max_batch_sz_);
		IBuilderConfig* config = builder->createBuilderConfig();
		config->setMaxWorkspaceSize(1 << 20);
		engine_ = builder->buildEngineWithConfig(*network, *config);
		if (!engine_) {
			LOG(ERROR) << "[ModelLoader] : create engine failed";
			return false;
		}
		LOG(INFO) << "[ModelLoader] : create engine suceess! ";
		LOG(INFO) << "[ModelLoader] : serialize Model start";
		IHostMemory* modelStream = engine_->serialize();
		std::string serialize_str;
		serialize_str.resize(modelStream->size());
		memcpy((void*)serialize_str.data(), modelStream->data(), modelStream->size());
		std::ofstream srz_out_stream;
		srz_out_stream.open(model_cache_path_.c_str(), std::ios_base::out | std::ios_base::binary);
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
		return false;
	}
	is_init_ = true;
	// const ICudaEngine& cudaEngine = context_->getEngine();
	return true;
}

const ICudaEngine& ModelLoader::GetCudaEngine() const{
	return context_->getEngine();
}
ModelLoader::~ModelLoader() {
	if (context_) {
		context_->destroy();
		context_ = nullptr;
	}
	if (engine_) {
		engine_->destroy();
		engine_ = nullptr;
	}
	if (runtime_) {
		runtime_->destroy();
		runtime_ = nullptr;
	}
}

	} // namespace components
} // namespace easysa
