#include "sentiment_analysis2.hpp"
#include <opencv2/opencv.hpp>
#include <thread>

int main() {
	SentimentAnalysis2 sa(25);
	sa.InitModuleConnector();
	sa.InitModule();
	sa.Run();
	sa.OsdModule();
	return 0;
}