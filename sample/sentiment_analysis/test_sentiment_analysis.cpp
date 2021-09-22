#include "sentiment_analysis.hpp"
#include <opencv2/opencv.hpp>
#include <thread>

int main() {
	SentimentAnalysis sa(30);
	sa.InitFrameSkip();
	sa.InitModuleConnector();
	sa.InitModule();
	sa.Run();
	sa.OsdModule();
	return 0;
}