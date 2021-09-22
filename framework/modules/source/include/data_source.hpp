/*************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *************************************************************************/

#ifndef MODULES_SOURCE_DATA_SOURCE_HPP_
#define MODULES_SOURCE_DATA_SOURCE_HPP_
#include <utility>
#include <string>
#include <memory>
#include <sstream>
#include "easysa_config.hpp"
#include "easysa_source.hpp"
#include "easysa_module.hpp"

namespace easysa {
	using ModuleParamSet = std::unordered_map<std::string, std::string>;

	enum OutputType {
	  OUTPUT_CPU = 0, // output decoded buffer to cpu
	  OUTPUT_CUDA, // output decoded buffer to cuda
	};
	enum DecoderType {
	  DECODER_CPU = 0, //use cpu decoder
	  DECODER_CUDA
	};
	struct DataSourceParam {
		OutputType output_type_ = OUTPUT_CPU;
		size_t interval_ = 1;
		DecoderType decoder_type_ = DECODER_CPU;
		uint32_t input_buf_number_ = 2;
		uint32_t output_buf_number_ = 3;
		int device_id_ = -1;
	};

	struct ESPacket {
		unsigned char* data = nullptr;
		int size = 0; // the size of data
		uint64_t pts = 0;
		uint32_t flags = 0; // the flags of the data
		/*
		* the flag of frame
		*/
		enum {
			FLAG_KEY_FRAME = 0x01,    // flag of key frame
			FLAG_EOS = 0x02,          // flag for eos frame
		};
	}; // struct ESPacket

	class DataSource : public SourceModule, public ModuleCreator<DataSource> {
	 public:
		 explicit DataSource(const std::string& module_name);
		 ~DataSource();
		 bool Open(ModuleParamSet param_set) override;
		 bool Close() override;
		 DataSourceParam GetParam() const { return param_; }
	private:
		DataSourceParam param_;
	}; // class DataSource

	/*
	* @brief source handler for USB_Camera, mp4,  flv
	*/
	class FileHandlerImpl;
	class FileHandler : public SourceHandler {
	public:
		static std::shared_ptr<SourceHandler> Create(DataSource* module, const std::string& stream_id, const std::string& file_name, int frame_rate, bool loop = false);
		~FileHandler();
		bool Open() override;
		void Close() override;
	private:
		explicit FileHandler(DataSource* module, const std::string& stream_id, const std::string& file_name, int frame_rate, bool loop = false);
	private:
		FileHandlerImpl* impl_ = nullptr;
	}; // class FileHander

} // namespace easysa

#endif // MODULES_SOURCE_DATA_SOURCE_HPP_