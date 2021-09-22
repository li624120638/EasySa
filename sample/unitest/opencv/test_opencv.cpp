#include <opencv2/opencv.hpp>
#include <glog/logging.h>

int main(int argc, char* argv[]) {
    google::InitGoogleLogging(argv[0]);
    google::SetStderrLogging(google::INFO);
    FLAGS_logtostderr = true;
    FLAGS_colorlogtostderr = true;
    FLAGS_log_prefix = true;
    cv::Mat src = cv::imread("../../../data/images.jpg", -1);
    //if (src.empty()) return 1;
    LOG(INFO) << "src success!";
    LOG(ERROR) << "src success!";
    LOG(INFO) << "src success!";
    google::ShutdownGoogleLogging();
    return 0;
}