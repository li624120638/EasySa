#include "ModuleParam.hpp"
#include <glog/logging.h>
#include <gtest/gtest.h>
using namespace easysa::components;

typedef struct TestParam {
    bool dump_image;
    int device_id;
    int width;
    int height;
    float ratio;
    double threshold;
    std::string padding_type;
} TestParam;

TEST(COMPONENTS, RegistParams) {
    std::string module_name = "test_module";
    ModuleParamsHelper<TestParam> params_helper(module_name);
    std::vector<ModuleParamDesc> regist_params = {
        {"device_id", "0", "device id", PARAM_NOT_MUST, OFFSET(TestParam, device_id), ModuleParamParser<int>::Parser},
        {"dump_image", "false", "dump image", PARAM_NOT_MUST, OFFSET(TestParam, dump_image),
         ModuleParamParser<bool>::Parser},
        {"width", "110", "width of input image", PARAM_NOT_MUST, OFFSET(TestParam, width),
         ModuleParamParser<int>::Parser},
        {"height", "120", "height of input image", PARAM_NOT_MUST, OFFSET(TestParam, height),
         ModuleParamParser<int>::Parser},
        {"ratio", "1.0", "resize ratrio of input image", PARAM_NOT_MUST, OFFSET(TestParam, ratio),
         ModuleParamParser<float>::Parser},
        {"threshold", "0.6", "threshold for obj score", PARAM_NOT_MUST, OFFSET(TestParam, threshold),
         ModuleParamParser<double>::Parser},
        {"padding_type", "middle", "input image padding method", PARAM_NOT_MUST, OFFSET(TestParam, padding_type),
         ModuleParamParser<std::string>::Parser} };
    EXPECT_TRUE(params_helper.Register(regist_params));

    std::unordered_map<std::string, std::string> params_map;
    params_map["device_id"] = "1";
    params_map["dump_image"] = "False";
    params_map["height"] = "120";
    params_map["width"] = "240";
    params_map["ratio"] = "0.56";
    params_map["padding_type"] = "middle";
    params_map["threshold"] = "0.88";
    //params_map["json_file_dir"] = "../../";
    EXPECT_TRUE(params_helper.ParseParamsAndInspect(params_map));

    TestParam params = params_helper.GetParams();
    LOG(INFO) << params.device_id << " " << params.dump_image << " " << params.height << " " << params.width << " "
        << params.ratio << " " << params.threshold << " " << params.padding_type;
}

TEST(COMPONENTS, ParamsParser) {
    std::string module_name = "test_module";
    ModuleParamsHelper<TestParam> params_helper(module_name);
    ModuleParamDesc desc;
    desc.name = "device_id";
    desc.optional = false;
    desc.default_value = "0";
    desc.str_desc = "which device to run inference";
    desc.offset = OFFSET(TestParam, device_id);
    desc.parser = ModuleParamParser<int>::Parser;
    EXPECT_TRUE(params_helper.Register(desc));

    desc.name = "dump_image";
    desc.optional = false;
    desc.default_value = "false";
    desc.str_desc = "wheater to dump image";
    desc.offset = OFFSET(TestParam, dump_image);
    desc.parser = ModuleParamParser<bool>::Parser;
    EXPECT_TRUE(params_helper.Register(desc));

    desc.name = "height";
    desc.optional = true;
    desc.default_value = "0";
    desc.str_desc = "which the height of input image";
    desc.offset = OFFSET(TestParam, height);
    desc.parser = ModuleParamParser<int>::Parser;
    EXPECT_TRUE(params_helper.Register(desc));

    desc.name = "width";
    desc.optional = true;
    desc.default_value = "0";
    desc.str_desc = "which the width of input image";
    desc.offset = OFFSET(TestParam, width);
    desc.parser = ModuleParamParser<int>::Parser;
    EXPECT_TRUE(params_helper.Register(desc));

    desc.name = "ratio";
    desc.optional = false;
    desc.default_value = "1";
    desc.str_desc = "which ratio to resize image";
    desc.offset = OFFSET(TestParam, ratio);
    desc.parser = ModuleParamParser<float>::Parser;
    EXPECT_TRUE(params_helper.Register(desc));

    desc.name = "padding_type";
    desc.optional = false;
    desc.default_value = "center";
    desc.str_desc = "which method to padding image";
    desc.offset = OFFSET(TestParam, padding_type);
    desc.parser = ModuleParamParser<std::string>::Parser;

    EXPECT_TRUE(params_helper.Register(desc));

    desc.name = "threshold";
    desc.optional = false;
    desc.default_value = "0.66";
    desc.str_desc = "threshold for score";
    desc.offset = OFFSET(TestParam, threshold);
    desc.parser = ModuleParamParser<double>::Parser;
    EXPECT_TRUE(params_helper.Register(desc));

    std::unordered_map<std::string, std::string> params_map;
    params_map["device_id"] = "1";
    params_map["dump_image"] = "False";
    params_map["height"] = "120";
    params_map["width"] = "240";
    params_map["ratio"] = "0.56";
    params_map["padding_type"] = "middle";
    params_map["threshold"] = "0.88";
    //params_map["easysa_dir"] = "../../";
    EXPECT_TRUE(params_helper.ParseParamsAndInspect(params_map));
}

TEST(CoreParam, ValidParam) {
    std::string module_name = "test_module";
    ModuleParamsHelper<TestParam> params_helper(module_name);
    std::vector<ModuleParamDesc> regist_params = {
        {"device_id", "0", "device id", PARAM_NOT_MUST, OFFSET(TestParam, device_id), ModuleParamParser<int>::Parser},
        {"dump_image", "false", "dump image", PARAM_NOT_MUST, OFFSET(TestParam, dump_image),
         ModuleParamParser<bool>::Parser},
        {"width", "110", "width of input image", PARAM_NOT_MUST, OFFSET(TestParam, width),
         ModuleParamParser<int>::Parser},
        {"height", "120", "height of input image", PARAM_NOT_MUST, OFFSET(TestParam, height),
         ModuleParamParser<int>::Parser},
        {"ratio", "1.0", "resize ratrio of input image", PARAM_NOT_MUST, OFFSET(TestParam, ratio),
         ModuleParamParser<float>::Parser},
        {"threshold", "0.6", "threshold for obj score", PARAM_NOT_MUST, OFFSET(TestParam, threshold),
         ModuleParamParser<double>::Parser},
        {"padding_type", "middle", "input image padding method", PARAM_NOT_MUST, OFFSET(TestParam, padding_type),
         ModuleParamParser<std::string>::Parser} };
    EXPECT_TRUE(params_helper.Register(regist_params));
    std::unordered_map<std::string, std::string> params_map;
    params_map["device_id"] = "1";
    params_map["dump_image"] = "False";
    params_map["height"] = "120";
    params_map["width"] = "240";
    params_map["ratio"] = "0.56";
    params_map["padding_type"] = "middle";
    params_map["threshold"] = "0.88";
    params_map["WRONG_DATADIR"] = "../../";
    params_map["wrong_param"] = "99999";
    EXPECT_FALSE(params_helper.ParseParamsAndInspect(params_map));
}
