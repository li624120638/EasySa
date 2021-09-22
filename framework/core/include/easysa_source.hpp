
/*************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *************************************************************************/

#ifndef FRAMEWORK_CORE_INCLUDE_EASYSA_SOURCE_HPP_
#define FRAMEWORK_CORE_INCLUDE_EASYSA_SOURCE_HPP_

#include <memory>
#include <atomic>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <mutex>
#include <glog/logging.h>

#include "easysa_common.hpp"
#include "easysa_frame.hpp"
#include "easysa_module.hpp"

namespace easysa {
  /*
   *  easysa_source.hpp
   * 
   *  This file contains a declaration of the SourceModule class.
   */
	class SourceHandler;
	class SourceModule : public Module {
	public:
		explicit SourceModule(const std::string& name) : Module(name) { has_transmit_.store(1); }
		bool AddSource(std::shared_ptr<SourceHandler> handler);
		int RemoveSource(std::shared_ptr<SourceHandler> handler, bool force);
		int RemoveSource(const std::string& stream_id, bool force_remove = false);
		std::shared_ptr<SourceHandler> GetSourceHandler(const std::string& stream_id);
		int RemoveSources(bool force = false);
		bool Process(std::shared_ptr<FrameInfo> data) override {
			(void)data;
			LOG(INFO) << " source module Process() should not be invoked \n";
			return true;
		}
	protected:
		uint32_t GetStreamIndex(const std::string& stream_id);
		void ReturnStreamIndex(const std::string& stream_id);
		bool SendData(std::shared_ptr<FrameInfo> data);
	protected:
		friend class SourceHandler;
	private:
		uint64_t source_idx_ = 0;
		std::mutex mtx_;
		std::unordered_map<std::string /* stream_id*/, std::shared_ptr<SourceHandler>> source_map_;

	}; // class SourceModule

	class SourceHandler : public NonCopyable {
	public:
		explicit SourceHandler(SourceModule *module, const std::string& stream_id) : module_(module), stream_id_(stream_id){
			if (module_) {
				stream_index_ = module_->GetStreamIndex(stream_id_);
			}
		}
		virtual ~SourceHandler() {
			if (module_) module_->ReturnStreamIndex(stream_id_);  // for reuse stream_id
		}
		virtual bool Open() = 0;
		virtual void Close() = 0;
		std::string GetStreamId() const { return stream_id_; }
		void SetStreamUniqueIdx(uint64_t idx) { stream_unique_idx_ = idx; }
		uint64_t GetStreamUniqueIdx() const { return stream_unique_idx_; }
	
	public:
		std::shared_ptr<FrameInfo> CreateFrameInfo(bool eos = false, std::shared_ptr<FrameInfo> payload = nullptr) {
			std::shared_ptr<FrameInfo> data = FrameInfo::Create(stream_id_, eos, payload);
			if (data) data->SetStreamIndex(stream_index_);
			return data;
		}
		bool SendData(std::shared_ptr<FrameInfo> data) {
			if (module_) return module_->SendData(data);
			return false;
		}
	protected:
		SourceModule* module_ = nullptr;
		mutable std::string stream_id_;
		uint64_t stream_unique_idx_;
		uint32_t stream_index_;  // (string) stream_id -> (unit32_t)stream_idx_
	}; // class SourceHandler


} // namespace easysa

#endif // FRAMEWORK_CORE_INCLUDE_EASYSA_SOURCE_HPP_