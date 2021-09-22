/*************************************************************************
 * This source code is licensed under the Apache-2.0 license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * A part of this source code is referenced from Nebula project.
 * https://github.com/Bwar/Nebula/blob/master/src/actor/DynamicCreator.hpp
 * https://github.com/Bwar/Nebula/blob/master/src/actor/ActorFactory.hpp
 *
 *************************************************************************/

#ifndef FRAMEWORK_CORE_INCLUDE_EASYSA_MODULE_HPP_
#define FRAMEWORK_CORE_INCLUDE_EASYSA_MODULE_HPP_
 /*
 *
 * this file contains a declaration of the Module class and ModuleFactory class
 *
 */
#include <atomic>
#include <functional>
#include <list>
#include <memory>
#include <shared_mutex>
#include <mutex>
#include <typeinfo>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include <glog/logging.h>
#include "easysa_common.hpp"
#include "easysa_eventbus.hpp"
#include "easysa_frame.hpp"
#include "profiler/module_profiler.hpp"

namespace easysa {
	/*
	* @brief Notify "data" after processed by this module
	*/
	class IModuleObserver {
	public:
		virtual void Notify(std::shared_ptr<FrameInfo> data) = 0;
		virtual ~IModuleObserver() {}
	}; // class IModuleObserver

	class Pipeline;
	/*
	* @brief Module virtual base class.
	*/
	class Module : private NonCopyable {
	public:
		explicit Module(const std::string& name) : name_(name) {}
		virtual ~Module();
		void SetObserver(IModuleObserver* observer) {
			std::unique_lock<std::shared_mutex> guard(observer_lock_);
			observer_ = observer;
		}
		virtual bool Open(ModuleParamSet param_set) = 0;
		virtual void Close() = 0;
		virtual bool Process(std::shared_ptr<FrameInfo> data) = 0;
		virtual void OnEos(const std::string& stream_id) {}
		inline std::string GetName() const { return name_; }
		/*
		* return ture if this function has run sucessfully, return false if this module has not been added to pipeline
		*/
		bool PostEvent(EventType type, const std::string& msg);
		bool PostEvent(Event e);
		/*
		* @brief Transmits data to the following stages
		*/
		bool TransmitData(std::shared_ptr<FrameInfo> data);
		Pipeline* GetContainer() const { return container_; }
		size_t GetId();
		ModuleProfiler* GetProfiler();
	protected:
		friend class Pipeline;
		friend class FrameInfo;
		void SetContainer(Pipeline* container);
		std::vector<size_t> GetParentIds() const { return parent_ids_; }
		void SetParentId(size_t id) {
			parent_ids_.push_back(id);
			mask_ = 0;
			for (auto& it : parent_ids_) mask_ |= (uint64_t)1 << it;
		}
		uint64_t GetModulesMask() const { return mask_; }

	public:
		bool HasTransmit() const { return has_transmit_.load(); }
	protected:
		/*
		*@brief this function is called by pipeline 
		*/
		int DoProcess(std::shared_ptr<FrameInfo> data);
	private:
		void NotifyObserver(std::shared_ptr<FrameInfo> data) {
			std::shared_lock<std::shared_mutex> guard(observer_lock_);
			if (observer_) observer_->Notify(data);
		}
		int DoTransmitData(std::shared_ptr<FrameInfo> data);
	protected:
		Pipeline* container_ = nullptr;
		mutable std::shared_mutex container_lock_;

		std::string name_;
		std::atomic<bool> has_transmit_{ false };
	private:
		size_t id_; // support no more than 64 modules
		static std::mutex module_id_lock_;
		static uint64_t module_id_mask_;

		std::vector<size_t> parent_ids_;
		uint64_t mask_ = 0;

		IModuleObserver* observer_ = nullptr;
		mutable std::shared_mutex observer_lock_;
	};

	/*
	* @brief ModuleEx class
	*/
	class Modulex : public Module {
	public:
		explicit Modulex(const std::string& name) : Module(name) { has_transmit_.store(true); }
	}; // class ModuleEx

/**
 * @brief ModuleCreator, ModuleFactory, and ModuleCreatorWorker:
 *   Implements reflection mechanism to create a module instance dynamically with the ``ModuleClassName`` and
 *    ``moduleName`` parameters.
 *   See ActorFactory&DynamicCreator in https://github.com/Bwar/Nebula (under Apache2.0 license)
 */
	/*
	* @brief ModuleFactory
	*/
	class ModuleFactory {
	public:
		static ModuleFactory* Instance() {
			factory_ = new (std::nothrow) ModuleFactory();
			if (nullptr == factory_) LOG(ERROR) << "ModuleFactory::Instance() new ModuleFactory failed.";
			return (factory_);
		}
		virtual ~ModuleFactory() {};
/**
 * Registers ``ModuleClassName`` and ``CreateFunction``.
 *
 * @param strTypeName The module class name.
 * @param pFunc The ``CreateFunction`` of a Module object that has a parameter ``moduleName``.
 *
 * @return Returns true if this function has run successfully.
 */
		bool Regist(const std::string& strTypeName, std::function<Module* (const std::string&)> pFunc) {
			if (nullptr == pFunc) {
				return (false);
			}
			bool ret = map_.insert(std::make_pair(strTypeName, pFunc)).second;
			return ret;
		}
		/**
		 * Creates a module instance with ``ModuleClassName`` and ``moduleName``.
		 *
		 * @param strTypeName The module class name.
		 * @param name The ``CreateFunction`` of a Module object that has a parameter ``moduleName``.
		 *
		 * @return Returns the module instance if this function has run successfully. Otherwise, returns nullptr if failed.
		 */
		Module* Create(const std::string& strTypeName, const std::string& name) {
			auto iter = map_.find(strTypeName);
			if (iter == map_.end()) {
				return (nullptr);
			}
			else {
				return (iter->second(name));
			}
		}
		/**
 * Gets all registered modules.
 *
 * @return All registered module class names.
 */
		std::vector<std::string> GetRegisted() {
			std::vector<std::string> registed_modules;
			for (auto& it : map_) {
				registed_modules.push_back(it.first);
			}
			return registed_modules;
		}
	private:
		ModuleFactory() {}
		static ModuleFactory* factory_;
		std::unordered_map<std::string, std::function<Module* (const std::string&)>> map_;
	};

/**
 * @brief ModuleCreator
 *   A concrete ModuleClass needs to inherit ModuleCreator to enable reflection mechanism.
 *   ModuleCreator provides ``CreateFunction``, and registers ``ModuleClassName`` and ``CreateFunction`` to
 * ModuleFactory().
 */
	template <typename T>
	class ModuleCreator {
	public:
		struct Register {
			Register() {
				char* szDemangleName = nullptr;
				std::string strTypeName;
#ifdef __GNUC__
				szDemangleName = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
#else
				// in this format?:     szDemangleName =  typeid(T).name();
				szDemangleName = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
#endif
				if (nullptr != szDemangleName) {
					strTypeName = szDemangleName;
					free(szDemangleName);
				}
				ModuleFactory::Instance()->Regist(strTypeName, CreateObject);
			}
			inline void do_nothing() const {}
		};
		ModuleCreator() { register_.do_nothing(); }
		virtual ~ModuleCreator() { register_.do_nothing(); }
		/**
		 * @brief Creates an instance of template (T) with specified instance name.
		 *
		 * This is a template function.
		 *
		 * @param name The name of the instance.
		 *
		 * @return Returns the instance of template (T).
		 */
		static T* CreateObject(const std::string& name) { return new (std::nothrow) T(name); }
		static Register register_;
	};

	template <typename T>
	typename ModuleCreator<T>::Register ModuleCreator<T>::register_;

	/**
	 * @brief ModuleCreatorWorker, a dynamic-creator helper.
	 */
	class ModuleCreatorWorker {
	public:
		/**
		 * @brief Creates a module instance with ``ModuleClassName`` and ``moduleName``.
		 *
		 * @param strTypeName The module class name.
		 * @param name The module name.
		 *
		 * @return Returns the module instance if the module instance is created successfully. Returns nullptr if failed.
		 * @see ModuleFactory::Create
		 */
		Module* Create(const std::string& strTypeName, const std::string& name) {
			Module* p = ModuleFactory::Instance()->Create(strTypeName, name);
			return (p);
		}
	};
} // namespace easysa
#endif // !FRAMEWORK_CORE_INCLUDE_EASYSA_MODULE_HPP_
