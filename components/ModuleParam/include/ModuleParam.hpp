#ifndef COMPONENTS_MODULE_PARAM_HPP_
#define COMPONENTS_MODULE_PARAM_HPP_
#include <glog/logging.h>
#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#define OFFSET(S, M) (size_t) & (((S*)0)->M)  // NOLINT
#define PARAM_MUST true
#define PARAM_NOT_MUST false

namespace easysa {
    namespace components {
/*
 *
 * @brief this file for Module to registed check paramter
 */
typedef struct ModuleParamDesc {
    std::string name;
    std::string default_value;
    std::string str_desc;
    bool optional;
    int offset;
    std::function<bool(const std::string&, const std::string&, void*)> parser;
} ModuleParamDesc;

/*
 *	@brief some inner paser
 */
template <typename T>
class ModuleParamParser {
public:
    static bool Parser(const std::string& param_name, const std::string& str, void* result) {
        LOG(ERROR) << "[ModuleParamPaser]: can`t find support Paser ";
        return false;
    }
};

template <>
class ModuleParamParser<bool> {
public:
    static bool Parser(const std::string& param_name, const std::string& str, void* result) {
        static std::vector<std::string> true_vec = { "True", "TRUE", "true", "1" };
        static std::vector<std::string> false_vec = { "False", "FALSE", "false", "0" };
        std::string not_const_str = str;
        std::remove_if(not_const_str.begin(), not_const_str.end(), ::isblank);
        for (auto& it : true_vec) {
            if (it == not_const_str) {
                *static_cast<bool*>(result) = true;
                return true;
            }
        }
        for (auto& it : false_vec) {
            if (it == not_const_str) {
                *static_cast<bool*>(result) = false;
                return true;
            }
        }
        LOG(ERROR) << "[ModuleParamPaser] : Bool Paser wrong param :" << param_name << ":" << not_const_str;
        return false;
    }
};

template <>
class ModuleParamParser<int> {
public:
    static bool Parser(const std::string& param_name, const std::string& str, void* result) {
        std::string not_const_str = str;
        try {
            std::remove_if(not_const_str.begin(), not_const_str.end(), ::isblank);
            int res = std::stoi(not_const_str);
            *static_cast<int*>(result) = res;
            return true;
        }
        catch (...) {
            LOG(ERROR) << "[ModuleParamPaser] : Int Paser wrong param :" << param_name << ":" << not_const_str;
            return false;
        }
        return false;
    }
};

template <>
class ModuleParamParser<float> {
public:
    static bool Parser(const std::string& param_name, const std::string& str, void* result) {
        std::string not_const_str = str;
        try {
            std::remove_if(not_const_str.begin(), not_const_str.end(), ::isblank);
            float res = std::stof(not_const_str);
            *static_cast<float*>(result) = res;
            return true;
        }
        catch (...) {
            LOG(ERROR) << "[ModuleParamPaser] : Float Paser wrong param :" << param_name << ":" << not_const_str;
            return false;
        }
        return false;
    }
};

template <>
class ModuleParamParser<double> {
public:
    static bool Parser(const std::string& param_name, const std::string& str, void* result) {
        std::string not_const_str = str;
        try {
            std::remove_if(not_const_str.begin(), not_const_str.end(), ::isblank);
            float res = std::stod(not_const_str);
            *static_cast<double*>(result) = res;
            return true;
        }
        catch (...) {
            LOG(ERROR) << "[ModuleParamPaser] : double Paser wrong param :" << param_name << ":" << not_const_str;
            return false;
        }
        return false;
    }
};

template <>
class ModuleParamParser<uint32_t> {
public:
    static bool Parser(const std::string& param_name, const std::string& str, void* result) {
        std::string not_const_str = str;
        try {
            std::remove_if(not_const_str.begin(), not_const_str.end(), ::isblank);
            uint32_t res = std::stoul(not_const_str);
            *static_cast<uint32_t*>(result) = res;
            return true;
        }
        catch (...) {
            LOG(ERROR) << "[ModuleParamPaser] : Uint32 Paser wrong param :" << param_name << ":" << not_const_str;
            return false;
        }
        return false;
    }
};

template <>
class ModuleParamParser<std::string> {
public:
    static bool Parser(const std::string& param_name, const std::string& str, void* result) {
        std::string not_const_str = str;
        try {
            std::remove_if(not_const_str.begin(), not_const_str.end(), ::isblank);
            *(std::string*)result = not_const_str;
            return true;
        }
        catch (...) {
            LOG(ERROR) << "[ModuleParamPaser] : Uint32 Paser wrong param :" << param_name << ":" << not_const_str;
            return false;
        }
        return false;
    }
};

template <class T>
class ModuleParamsHelper {
public:
    explicit ModuleParamsHelper(const std::string& name) : module_name_(name) {}
    ModuleParamsHelper(const ModuleParamsHelper&) = delete;
    ModuleParamsHelper& operator=(const ModuleParamsHelper&) = delete;
    T GetParams() {
        if (!init_) {
            LOG(ERROR) << "module param not init.";
        }
        return params_;
    }
    bool Register(const std::vector<ModuleParamDesc>& params_desc) {
        for (auto& it : params_desc) {
            if (!Register(it)) {
                LOG(ERROR) << "Param Regist failed. " << it.name;
                return false;
            }
        }
        return true;
    }
    bool Register(const ModuleParamDesc& param_desc) {
        try {
            std::shared_ptr<ModuleParamDesc> p_desc = std::make_shared<ModuleParamDesc>();
            std::string name = param_desc.name;
            std::remove_if(name.begin(), name.end(), ::isblank);
            p_desc->name = name;
            if ("" == p_desc->name) {
                throw 0;
            }
            p_desc->default_value = param_desc.default_value;
            p_desc->optional = param_desc.optional;
            p_desc->offset = param_desc.offset;
            if (nullptr == param_desc.parser) {
                LOG(ERROR) << "[ModuleParam] : register " << param_desc.name << " falied, "
                    << " you should set default paser or custom paser";
                return false;
            }
            else {
                p_desc->parser = param_desc.parser;
            }
            p_desc->str_desc = param_desc.str_desc;
            params_desc_[name] = p_desc;
            return true;
        }
        catch (...) {
            LOG(ERROR) << "[ModuleParam] : register falied, " << param_desc.name;
            return false;
        }
        return false;
    }

    bool ParseParamsAndInspect(const std::unordered_map<std::string, std::string>& params) {
        std::unordered_map<std::string, std::string> map = params;
        for (auto& it : params_desc_) {
            auto iter = map.find(it.first);
            if (map.end() == iter && it.second->optional == PARAM_MUST) {
                LOG(ERROR) << "[ModuleParam]:not find " << it.first << " , parser failed."
                    << "you must set this param!";
                return false;
            }
            std::string str_value;
            std::string str_param = it.first;
            if (it.second->optional == PARAM_NOT_MUST && map.end() == iter) {
                str_value = it.second->default_value;
            }
            else {
                str_value = iter->second;
            }
            it.second->parser(str_param, str_value, ((char*)&params_ + it.second->offset));  // NOLINT
        }
        for (auto& it : params_desc_) {
            auto iter = map.find(it.first);
            if (iter != map.end()) {
                map.erase(it.first);
            }
        }
        bool flag = true;
        for (auto& it : map) {
            if ("easysa_dir" != it.first) {
                LOG(ERROR) << "[ModuleParam]: not registed this param:[" << it.first << "]:[" << it.second << "]";
                flag = false;
            }
        }
        init_ = true;
        return flag;
    }

private:
    std::atomic<bool> init_{ false };
    std::string module_name_;
    using ParamsDesc = std::unordered_map<std::string, std::shared_ptr<ModuleParamDesc>>;
    ParamsDesc params_desc_;
    T params_;
};  // class ModuleParamHelper

    } // namespace easysa
} // namespace easysa
#endif // !COMPONENTS_MODULE_PARAM_HPP_
