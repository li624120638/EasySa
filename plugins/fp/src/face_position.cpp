#include "face_position.hpp"
#include <glog/logging.h>
#include <cassert>
#include <algorithm>
#include <fstream>
#include <string>
#include <ctime>
#include <cmath>
#include <unordered_map>
FacePositionClassifier::FacePositionClassifier(std::string model_path) {
	std::ifstream fin(model_path.c_str(), std::ios_base::in | std::ios_base::binary);
	if (!fin.good()) {
		LOG(ERROR) << "Face Position Classifier load model failed";
	} else {
		std::string str;
		while (std::getline(fin, str))
		{
			SplitLineDataTofloat(str, ',');
		}
	}
	int sz = dataset.size() - 1;
	randomer_.param(std::uniform_int_distribution<>::param_type{0, sz});
	e_ = new std::default_random_engine(time(NULL));
}

float FacePositionClassifier::CalDistance(std::vector<float> ori, std::vector<float> dst) {
	// ori是带label的，dst是input的，ori会比dst多一个float
	float foo = 0;
	for (int i = 0; i < dst.size(); i++) {
		foo += std::powf(ori[i] - dst[i], 2);
	}
	return foo;
}
int FacePositionClassifier::Vote(int k) {
	// -30, -15, 0, 15, 30
	std::unordered_map<int, size_t> map;
	for (int i = 0; i < k; i++) {
		auto iter = map.find(distances[i][1]);
		if (iter != map.end()) {
			size_t num = iter->second;
			map[iter->first] = num + 1;
		}
		else {
			map[distances[i][1]] = 1;
		}
	}

	int max_label = map.begin()->first;
	int max_num = map.begin()->second;
	for (auto& it : map) {
		if (it.second > max_num) {
			max_num = it.second;
			max_label = it.first;
		}
	}
	return max_label;
}
int FacePositionClassifier::Predict(std::vector<float> input, int m, int k) {
	assert(dataset.size() >= k && k <= m && dataset.size() >= m);
	std::lock_guard<std::mutex> guard(mtx);
	distances.clear();
	int rd_value = -1;
	for (int i = 0; i < m; i++) {
		std::vector<float> tmp;
		rd_value = randomer_(*e_);
		tmp.push_back(CalDistance(dataset[rd_value], input));
		// 放入label
		tmp.push_back(dataset[rd_value][3]);
		distances.push_back(tmp);
	}
	sort(distances.begin(), distances.end(), [](std::vector<float> A, std::vector<float> B)->bool {
		return A[0] < B[0];
		});
	int label = Vote(k);
	return label;
}
FacePositionClassifier::~FacePositionClassifier() {
	if (e_) {
		delete e_;
		e_ = nullptr;
	}
}

void FacePositionClassifier::SplitLineDataTofloat(std::string str, char separtor) {
	assert(str.size() > 0);
	int index = 0;
	bool flag = true;
	float out;
	std::vector<float> oneData;
	while (flag) {
		if (index >= str.size()) break;
		auto i = str.find(separtor, index);
		if (i != std::string::npos) {
			std::string foo = str.substr(index, i - index);
			index = i + 1;
			out = std::stof(foo);
			oneData.push_back(out);
		}
		else {
			flag = false;
		}
	}
	assert(oneData.size() == 4);
	dataset.push_back(oneData);
}