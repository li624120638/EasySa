#include "make_eye_ds.hpp"

static void GetAllFilesByDir(const char* dir_path, std::vector<std::string>& img_dir) {
    intptr_t handle;
    _finddata_t find_data;
    std::string path_name;

    handle = _findfirst(path_name.assign(dir_path).append("\\*").c_str(), &find_data);
    if (handle == -1) {
        LOG(ERROR) << "[retinaface]:" << "when deal Columbia Gaze Data Set, get images by dir error ";
        return;
    }
    do {
        if (find_data.attrib & _A_SUBDIR) {

            if (strcmp(find_data.name, ".") != 0 && strcmp(find_data.name, "..") != 0) {
                std::string tmp(dir_path);
                GetAllFilesByDir(tmp.append("/").append(find_data.name).c_str(), img_dir);
            }
        }
        else {
            std::string tmp(dir_path);
            img_dir.push_back(tmp.append("/").append(find_data.name));
        }
    } while (_findnext(handle, &find_data) == 0); // find next images
    _findclose(handle);
}
std::string MakeEyeDs::GetFileNameByPath(const std::string& file_path) {
    auto it = file_path.find_last_of("/");
    std::string file_name = file_path.substr(it + 1, file_path.size() - it);
    return file_name;
}

MakeEyeDs::MakeEyeDs(std::string model_path, std::string img_dir, std::string save_dir):
    model_path_(model_path), img_dir_(img_dir),save_dir_(save_dir){
    std::vector<std::string> images_path;
    GetAllFilesByDir(img_dir.c_str(), images_path);
    images_path_ = std::move(images_path);
    rf = new RetinaFace(model_path_, "net3", 0.4);
}
void MakeEyeDs::SaveEyeRoi(const cv::Mat& src, FaceDetectInfo info, std::string file_full_path) {
    if (src.empty()) return;
    float ratio = info.scale_ratio;
    std::string file_name = GetFileNameByPath(file_full_path);
    std::string out_file_name = save_dir_ + "eye/" + file_name;
    auto& pts = info.pts;
    auto& rec = info.rect;
    cv::Point left_up_core, right_bottom_core;
    size_t width = src.cols;
    size_t height = src.rows;
    /*
    * 眼到头顶取中间作为上点y1, 鼻子到眼取中点作为y2
    * 左眼到脸左边界取中点作为x1, 右眼到脸的右边界作为x2
    */
    cv::Size rsz(640, 640);
    /*
    left_up_core.y = (height / rsz.height) * ratio * (rec.y1 + pts.y[0]) / 2;
    right_bottom_core.y = (height / rsz.height) * ratio * (pts.y[0] + pts.y[2]) / 2;
    left_up_core.x = (width / rsz.width) * ratio * (rec.x1 + pts.x[0]) / 2;
    right_bottom_core.x = (width / rsz.width) * ratio * (pts.x[1] + rec.x2) / 2;
    */
    left_up_core.y = (height / rsz.height) * ratio * (rec.y1 + (pts.y[0] - rec.y1) * 9 /10);
    right_bottom_core.y = (height / rsz.height) * ratio * (pts.y[0] + (pts.y[2] - pts .y[0]) * 5 / 6);
    left_up_core.x = (width / rsz.width) * ratio * (rec.x1 + pts.x[0]) / 2;
    //right_bottom_core.x = (width / rsz.width) * ratio * (pts.x[1] + rec.x2) / 2;
    right_bottom_core.x = (width / rsz.width) * ratio * (pts.x[0] + pts.x[1]) / 2; // 只取左眼
    cv::Mat roi = src(cv::Rect(left_up_core,right_bottom_core)).clone();
    if (roi.empty()) return;
    cv::resize(roi, roi, cv::Size(224, 224));
    cv::imwrite(out_file_name, roi);
}
bool MakeEyeDs::CreateEyeDs() {
    if (images_path_.size() == 0) {
        LOG(INFO) << "[MakeEyeDs]:No images to find.";
        return false;
    }
    cv::Size rz(640, 640);
    for (auto& it : images_path_) {
        cv::Mat src = cv::imread(it, cv::IMREAD_COLOR);
        if (src.empty()) {
            LOG(ERROR) << "[MakeEyeDs]:open image failed!";
            continue;
        }
        cv::Mat img_rsz;
        cv::resize(src, img_rsz, rz);
        std::vector<FaceDetectInfo> detect_info = rf->detect(img_rsz, 0.9, 1.0);
        for (auto& info : detect_info) {
            SaveEyeRoi(src, info, it);
        }
    }
}
MakeEyeDs::~MakeEyeDs() {
    if (rf != nullptr) {
        delete rf;
        rf = nullptr;
    }
}