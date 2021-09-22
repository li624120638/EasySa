#include <fstream>
#include <string>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
using namespace rapidjson;
using namespace std;
TEST(COMPONENTS, LoadJson) {

	std::string json_path = "../../../data/documents/test.json";
	std::ifstream in;
	in.open(json_path, std::ifstream::in);
	if (!in.good()) {
		exit(0);
	}
	std::string str;
	std::string line;
	while (getline(in,line))
	{
		str.append(line);
	}
	Document doc;
	if (doc.Parse(str.c_str()).HasParseError()) {
		LOG(ERROR) << "[CoreParam]:" << "Parse profiler configuration failed. Error code [" << std::to_string(doc.GetParseError()) << "]"
			<< " Offset [" << std::to_string(doc.GetErrorOffset()) << "]. JSON:" << json_path;
		exit(1);
	}
	if (doc.HasMember("width")) {
		LOG(INFO) << doc["width"].GetInt();
	}
}