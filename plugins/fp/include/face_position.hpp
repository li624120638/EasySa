#ifndef UNITEST_FACE_POSITION_CLASSIFIER
#define UNITEST_FACE_POSITION_CLASSIFIER
#include <string>
#include <vector>
#include <random>
#include <mutex>

/*
*	this plugin use to predict face poses
*   algorithm:KNN and data[in]: face landmark
*/
class FacePositionClassifier {
public:
	FacePositionClassifier(std::string model_path);
	/* m[in] : select how much dataset size to participate cal
	*  k[in] : k tickts
	*/
	int Predict(std::vector<float> input, int m, int k);
	~FacePositionClassifier();
private:
	void SplitLineDataTofloat(std::string str, char separtor);
	float CalDistance(std::vector<float> ori, std::vector<float> dst);
	int Vote(int k);
private:
	std::vector<std::vector<float>> dataset;
	std::vector<std::vector<float>> distances;
	std::uniform_int_distribution<> randomer_;
	std::default_random_engine* e_;
	std::mutex mtx;
}; // class FacePositionClassifier
#endif // !UNITEST_FACE_POSITION_CLASSIFIER
