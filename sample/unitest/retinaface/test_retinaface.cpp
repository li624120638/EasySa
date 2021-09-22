#include "etnet.hpp"
#include "RetinaFace.h"
#include "face_position.hpp"
#include "make_eye_ds.hpp"
#include "fer.hpp"
#include <io.h>
#include <cstring>
#include <iostream>
#include <glog/logging.h>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

#include "timer.h"

using namespace std;
using NetOutputs = std::vector<std::vector<std::vector<float>>>;
// Get All Files by Dir
void GetAllFilesByDir(const char* dir_path, std::vector<std::string>& img_dir);
// Get File Name By file path
std::string GetFileNameByPath(const std::string& file_path);
// change file like "../../image.jpg" to "..\..\image.jpg"
std::string TranfromFilePresent(const std::string& file_path, const std::string& key,
    const std::string& replace_key);
void SaveImageRoi(const cv::Mat& src, const FaceDetectInfo& detect_info, const std::string& save_path);
/*
int main()
{
    printf("==================start============= \n");
    string path = "../../../data/models/retinaface";
    RetinaFace *rf = new RetinaFace(path, "net3", 0.4);
    cv::Mat img = cv::imread("D:\github\easy_sa\data\images\face_001.jpg");
    if (!img.empty()) {
        printf("open image failed \n");
    }
    vector<Mat> imgs;
    imgs.push_back(img.clone());

    //注：使用OPENCV计时和timer类计时有点偏差
    float time = 0;
    int count = 0;
    RK::Timer ti;

    while(1) {
        //ti.reset();
        rf->detect(img, 0.9, 1.0);

        time += ti.elapsedMilliSeconds();
        count ++;
        if(count % 1000 == 0) {
            printf("face detection average time = %f.\n", time / count);
        }
    }
    return 0;
}
*/

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

std::string TranfromFilePresent(const std::string& file_path, const std::string& key, const std::string& replace_key) {

    size_t pos = 0;
    std::string temp = file_path;
    while ((pos = temp.find(key, pos)) != string::npos)
    {
        temp.erase(pos, key.size());
        temp.insert(pos, replace_key);
        pos += replace_key.size();
    }
    // change jpeg to jpg
    while ((pos = temp.find("jpeg", 0)) != string::npos)
    {
        temp.erase(pos, key.size());
        temp.insert(pos, "jpg");
        //pos += replace_key.size();
    }
    return temp;
}

void SaveImageRoi(const cv::Mat& src, const FaceDetectInfo& detect_info, const std::string& save_path, cv::Size size) {
    float width = detect_info.rect.x2 - detect_info.rect.x1;
    float height = detect_info.rect.y2 - detect_info.rect.y1;
    float scale = detect_info.scale_ratio;
    cv::Mat roi = src(cv::Rect(detect_info.rect.x1 * scale, detect_info.rect.y1 * scale,
        width * scale, height * scale));
    cv::resize(roi, roi, size);
    cv::imwrite(save_path, roi);
}

/*
*
* head pose：-30、-15、0、15、30
* * vectical gaze direction: -10、0、10
* horizontal gaze direction: -15、-10、-5、0、5、10、15
*
*/
static int place_label[3][8] = { {-30, -15, 0, 15, 30, -1, -1, -1},
                                {-10, 0, 10, -1, -1, -1, -1, -1},
                                {-15, -10, -5, 0, 5, 10, 15, -1} };

std::string GetFileNameByPath(const std::string& file_path) {
    auto it = file_path.find_last_of("/");
    std::string file_name = file_path.substr(it + 1, file_path.size() - it);
    return file_name;
}

void CreateFaceKNNModel(std::string model_path, const std::string& dir_path) {
    std::vector<std::string> images_path;
    GetAllFilesByDir(dir_path.c_str(), images_path);

    std::ofstream fout(model_path, std::ios_base::out | std::ios_base::binary);
    if (!fout.good()) {
        LOG(ERROR) << "When create FPC model, open file failed";
        return;
    }
    std::string rtfm = "../../../data/models/retinaface";
    RetinaFace* rf = new RetinaFace(rtfm, "net3", 0.4);
    std::vector<FaceDetectInfo> detect_info;
    detect_info.clear();
    for (auto& it : images_path) {
        detect_info.clear();
        cv::Mat img = cv::imread(it.c_str(), cv::IMREAD_COLOR);
        if (img.empty()) {
            printf("open image failed \n");
            continue;
        }
        cv::Mat img_rsz;
        cv::resize(img, img_rsz, cv::Size(640, 640));
        detect_info = rf->detect(img_rsz, 0.9, 1.0);
        std::string fn = it;
        std::string file_name = GetFileNameByPath(fn);

        auto P_index = file_name.find_first_of('P', 8);
        std::string head_pose = file_name.substr(8, (P_index - 8));
        for (auto i = 0; i < detect_info.size(); i++) {
            auto& pts = detect_info[i].pts;
            float w_ratio = detect_info[i].rect.x2 - detect_info[i].rect.x1;
            float n_ration = pts.x[2] - detect_info[i].rect.x1;
            float l_ration = pts.x[3] - detect_info[i].rect.x1;
            float r_ration = pts.x[4] - detect_info[i].rect.x1;
            n_ration /= w_ratio;
            l_ration /= w_ratio;
            r_ration /= w_ratio;
            fout << n_ration << "," << l_ration << "," << r_ration << "," << head_pose <<"," << std::endl;
        }
    }
    fout.close();
    delete rf;
    LOG(INFO) << "create Face model success";
}
/*
int main(int argc, char** argv) {
    printf("==================start============= \n");
    std::string dir_path = "../../../data/images";
    std::vector<std::string> images_path;
    GetAllFilesByDir(dir_path.c_str(), images_path);

    string path = "../../../data/models/retinaface";
    RetinaFace* rf = new RetinaFace(path, "net3", 0.4);

    float time = 0;
    RK::Timer ti;
    std::vector<FaceDetectInfo> detect_info;

    detect_info.clear();
    */
    /*
    cv::Mat img = cv::imread("../../../data/images/face.jpg", cv::IMREAD_COLOR);
    cv::Mat img_rsz;
    cv::resize(img, img_rsz, cv::Size(640, 640));
    if (img.empty()) {
        printf("open image failed \n");
    }
    detect_info = rf->detect(img_rsz, 0.9, 1.0);
    float width = detect_info[0].rect.x2 - detect_info[0].rect.x1;
    float height = detect_info[0].rect.y2 - detect_info[0].rect.y1;
    float scale = detect_info[0].scale_ratio;
    cv::Mat roi = img_rsz(cv::Rect(detect_info[0].rect.x1 * scale, detect_info[0].rect.y1 * scale,
                          width *scale, height * scale));
    cv::namedWindow("roi_demo", WINDOW_FREERATIO);
    cv::imshow("roi_demo", roi);
    cv::namedWindow("src_demo", WINDOW_FREERATIO);
    cv::imshow("src_demo", img);
    waitKey(0);
    destroyAllWindows();
    */
    /*
        for (auto& it : images_path) {
            detect_info.clear();
            cv::Mat img = cv::imread(it.c_str(), cv::IMREAD_COLOR);
            if (img.empty()) {
                printf("open image failed \n");
                continue;
            }
            cv::Mat img_rsz;
            cv::resize(img, img_rsz, cv::Size(640, 640));
            detect_info = rf->detect(img_rsz, 0.9, 1.0);
            for (auto i = 0; i < detect_info.size();i++) {
                std::string save_path = TranfromFilePresent(it, "images", "images/output");
                //save_path = save_path + std::to_string(i);
                SaveImageRoi(img_rsz, detect_info[i], save_path, cv::Size(224,224));
            }
            time += ti.elapsedMilliSeconds();
        }
        float one_time = (time * 1.0 / images_path.size()) / 1000.0;
        LOG(INFO) << "one_time:" << one_time << " total:" << time << " total number:" << images_path.size();
        return 0;
    }
    */

std::vector<int> GetFacePosition(const std::vector<FaceDetectInfo>& detect_info) {
    std::vector<int> res;
    for (auto& it : detect_info) {
        
    }
    return res;
}
void DrawFace(cv::Mat& src, const std::vector<FaceDetectInfo>& detect_info,
    cv::Size rz, EtNet* etnet, FacePositionClassifier* fpc) {
    if (src.empty()) return;
    anchor_box rect;
    float scale_ratio;
    int width = src.cols;
    int height = src.rows;
    float x1, x2, x3, x4, y1, y2, y3, y4;
    std::vector<float> output_lab;
    for (auto& it : detect_info) {
        Mat roi;
        rect = it.rect;
        scale_ratio = it.scale_ratio;
        /*
        *    p1(x1,y1)p2(x2,y1)
        *    p3(x1,y2)p4(x2,y2)
        */
        x1 = it.rect.x1 * scale_ratio * (1.0 * width / rz.width);
        y1 = it.rect.y1 * scale_ratio * (1.0 * height / rz.height);
        x2 = it.rect.x2 * scale_ratio * (1.0 * width / rz.width);
        y2 = it.rect.y2 * scale_ratio * (1.0 * height / rz.height);
        roi = src(cv::Rect(x1, y1, x2 - x1, y2 - y1));
        etnet->DoInference(roi);
        std::vector<int> et_ers = etnet->GetPredictResult();
        std::string txt_v = "vertical_gaze_directions:" + std::to_string(et_ers[1]);
        std::string txt_h = "horizontal_gaze_directions:" + std::to_string(et_ers[2]);
        
        cv::putText(src, txt_v, cv::Point(x1, y1 + 20), 1, FONT_HERSHEY_PLAIN,
            Scalar(255, 255, 255), 1, LINE_8);
        cv::putText(src, txt_h, cv::Point(x1, y1 + 30), 1, FONT_HERSHEY_PLAIN,
            Scalar(255, 255, 255), 1, LINE_8);
        cv::Point p1(x1, y1), p2(x2, y1), p3(x1, y2), p4(x2, y2);
        cv::line(src, p1, p2, cv::Scalar(255, 255, 255), 1, cv::LineTypes::LINE_8);
        cv::line(src, p1, p3, cv::Scalar(255, 255, 255), 1, cv::LineTypes::LINE_8);
        cv::line(src, p2, p4, cv::Scalar(255, 255, 255), 1, cv::LineTypes::LINE_8);
        cv::line(src, p3, p4, cv::Scalar(255, 255, 255), 1, cv::LineTypes::LINE_8);
        /*
        * draw 关键点
        */
        for(size_t j = 0; j < 5; j++) {
            //if (j < 2) continue;
            cv::Point2f pt = cv::Point2f(it.pts.x[j] * scale_ratio * (1.0 * width / rz.width),
                                    it.pts.y[j] * scale_ratio * (1.0 * height / rz.height));
            cv::circle(src, pt, 1, Scalar(0, 255, 0), 2);
        }
        output_lab.clear();
        auto& pts = it.pts;
        float w_ratio = it.rect.x2 - it.rect.x1;
        float n_ration = pts.x[2] - it.rect.x1;
        float l_ration = pts.x[3] - it.rect.x1;
        float r_ration = pts.x[4] - it.rect.x1;
        n_ration /= w_ratio;
        l_ration /= w_ratio;
        r_ration /= w_ratio;
        output_lab.push_back(n_ration);
        output_lab.push_back(l_ration);
        output_lab.push_back(r_ration);
        int label = fpc->Predict(output_lab, 50, 10);
        std::string txt_p = "poses:" + std::to_string(label);
        cv::putText(src, txt_p, cv::Point(x1, y1 + 10), 1, FONT_HERSHEY_PLAIN,
            Scalar(255, 255, 255), 1, LINE_8);
    }
}
/*
void GetRoiImage(cv::Mat& src, cv::Mat& roi, FaceDetectInfo info) {
    // 这里src是原图resize后的那张图
    float scale_ratio = info.scale_ratio;
    float width = info.rect.x2 - info.rect.x1;
    float height = info.rect.y2 - info.rect.y1;
    float scale = info.scale_ratio;
    cv::Mat roi = src(cv::Rect(info.rect.x1 * scale, info.rect.y1 * scale,
        width * scale, height * scale));
}
*/
/*
int main() {
    // create retina face
    string path = "../../../data/models/retinaface";
    RetinaFace* rf = new RetinaFace(path, "net3", 0.4);
    std::vector<FaceDetectInfo> detect_info;
    VideoCapture capture(0);
    if (!capture.isOpened()) {
        cout << "can`t opeen this camera" << endl;
    }
    namedWindow("easy_sa", WINDOW_AUTOSIZE);
    Mat frame;
    Mat img_rsz;
    cv::Size rz(640, 640);
    while (true)
    {
        detect_info.clear();
        bool ret = capture.read(frame);
        if (!ret)
        {
            break;
        }
        cv::resize(frame, img_rsz, rz);
        detect_info = rf->detect(img_rsz, 0.9, 1.0);
        DrawFace(frame, detect_info, rz);
        imshow("easy_sa", frame);
        char c = waitKey(23);
        if (c == 27)
        {
            break;
        }
    }
    capture.release();
    return 0;
}
*/

// add etnet to retinaface and adapte face position
/*
int main() {
    // create retina face 
    string path = "../../../data/models/retinaface";
    std::string etnet_cache_path = "./etnet.cache";
    std::string etnet_path = "../../../data/models/etnet_onnx/etnet.onnx";
    std::string fpc_path = "../../../data/models/frp/FacePosition.txt";
    FacePositionClassifier* fpc = new FacePositionClassifier(fpc_path);
    EtNet* etnet = new EtNet(etnet_path, etnet_cache_path);
    RetinaFace* rf = new RetinaFace(path, "net3", 0.4);
    std::vector<FaceDetectInfo> detect_info;
    VideoCapture capture(0);
    if (!capture.isOpened()) {
        cout << "can`t opeen this camera" << endl;
    }
    namedWindow("easy_sa", WINDOW_AUTOSIZE);
    Mat frame;
    Mat img_rsz;

    cv::Size rz(640, 640);
    while (true)
    {
        detect_info.clear();
        bool ret = capture.read(frame);
        if (!ret)
        {
            break;
        }
        cv::resize(frame, img_rsz, rz);
        detect_info = rf->detect(img_rsz, 0.9, 1.0);
        DrawFace(frame, detect_info, rz, etnet, fpc);
        imshow("easy_sa", frame);
        char c = waitKey(23);
        if (c == 27)
        {
            break;
        }
    }
    capture.release();
    delete rf;
    delete etnet;
    delete fpc;
    return 0;
}*/


 // test FacePositionClassifier
/*
int main() {
    std::string fpc_path = "../../../data/models/frp/FacePosition.txt";
    FacePositionClassifier* fpc = new FacePositionClassifier(fpc_path);
    std::vector<FaceDetectInfo> detect_info;

    detect_info.clear();
    cv::Mat img = cv::imread("../../../data/images/0001_2m_0P_0V_0H.jpg", cv::IMREAD_COLOR);
    if (img.empty()) {
        printf("open image failed \n");
        return -1;
    }
    cv::Mat img_rsz;
    cv::resize(img, img_rsz, cv::Size(640, 640));
    std::string rtfm = "../../../data/models/retinaface";
    RetinaFace* rf = new RetinaFace(rtfm, "net3", 0.4);
    detect_info = rf->detect(img_rsz, 0.9, 1.0);
    std::vector<float> output_lab;
    for (auto i = 0; i < detect_info.size(); i++) {
        output_lab.clear();
        auto& pts = detect_info[i].pts;
        float w_ratio = detect_info[i].rect.x2 - detect_info[i].rect.x1;
        float n_ration = pts.x[2] - detect_info[i].rect.x1;
        float l_ration = pts.x[3] - detect_info[i].rect.x1;
        float r_ration = pts.x[4] - detect_info[i].rect.x1;
        n_ration /= w_ratio;
        l_ration /= w_ratio;
        r_ration /= w_ratio;
        output_lab.push_back(n_ration);
        output_lab.push_back(l_ration);
        output_lab.push_back(r_ration);
        int label = fpc->Predict(output_lab, 500, 10);
        LOG(INFO) << label;
    }

    delete fpc;
    delete rf;
    return 0;
}
*/
/*
* // create Face position KNN model
int main() {
    std::string fpc_path = "../../../data/models/frp/FacePosition.txt";
    std::string dir_path = "../../../data/images/Columbia Gaze Data Set";
    CreateFaceKNNModel(fpc_path, dir_path);
    return 0;
}
*/
/*
// make left eye roi data set
int main() {
    std::string model_path = "../../../data/models/retinaface";
    std::string dir_path = "../../../data/images";
    std::string save_dir = "../../../data/images/output/";
    MakeEyeDs med(model_path, dir_path, save_dir);
    med.CreateEyeDs();
    return 0;
}
*/
/*
int main() {
    std::string dir_path = "../../../data/images/Oulu/";
    std::string save_dir = "../../../data/images/output/";
    std::vector<std::string> images_path;
    GetAllFilesByDir(dir_path.c_str(), images_path);

    string path = "../../../data/models/retinaface";
    RetinaFace* rf = new RetinaFace(path, "net3", 0.4);
    std::vector<FaceDetectInfo> detect_info;
    detect_info.clear();

    for (auto& it : images_path) {
        detect_info.clear();
        cv::Mat img = cv::imread(it.c_str(), cv::IMREAD_COLOR);
        if (img.empty()) {
            printf("open image failed \n");
            continue;
        }
        cv::Mat img_rsz;
        cv::resize(img, img_rsz, cv::Size(640, 640));
        detect_info = rf->detect(img_rsz, 0.9, 1.0);
        for (auto i = 0; i < detect_info.size(); i++) {
            std::string save_path = TranfromFilePresent(it, "images", "images/output");
            std::cout << save_path << std::endl;
            SaveImageRoi(img_rsz, detect_info[i], save_path, cv::Size(224, 224));
        }
    }
    delete rf;
    return 0;
}
*/

static std::vector<std::string> fer_status = {
    "Anger","Disgust","Fear","Happiness","Neutral", "Sadness","Surprise"
};
/*
static std::vector<std::string> fer_status = {
    "Anger","Disgust","Fear","Happiness", "Sadness","Surprise"
};*/
void PredictFer(cv::Mat& src, const std::vector<FaceDetectInfo>& detect_info, cv::Size rz,
    Fer* fer) {
    if (src.empty()) return;
    anchor_box rect;
    float scale_ratio;
    int width = src.cols;
    int height = src.rows;
    float x1, x2, x3, x4, y1, y2, y3, y4;
    std::vector<float> output_lab;
    for (auto& it : detect_info) {
        Mat roi;
        rect = it.rect;
        scale_ratio = it.scale_ratio;
        /*
        *    p1(x1,y1)p2(x2,y1)
        *    p3(x1,y2)p4(x2,y2)
        */
        x1 = it.rect.x1 * scale_ratio * (1.0 * width / rz.width);
        y1 = it.rect.y1 * scale_ratio * (1.0 * height / rz.height);
        x2 = it.rect.x2 * scale_ratio * (1.0 * width / rz.width);
        y2 = it.rect.y2 * scale_ratio * (1.0 * height / rz.height);
        roi = src(cv::Rect(x1, y1, x2 - x1, y2 - y1));
        fer->PushMatToPool(roi);
        NetOutputs results;
        fer->GetResultAsynchr(results);
        /*
        std::vector<float> softmax_res = fer->GetSoftMaxResult(results);
        for (size_t i = 0; i < softmax_res.size(); i++) {
            std::string fer = "";
            fer = fer_status[i] + ":" + std::to_string(softmax_res[i]);
            cv::putText(src, fer, cv::Point(x1, y1 + i*15), 1, FONT_HERSHEY_PLAIN,
                Scalar(255, 255, 255), 1, LINE_4);
        }
        */
     
        float max = results[0][0][0];
        size_t index = 0;
        for (size_t i = 0; i < results[0][0].size(); i++) {
            if (results[0][0][i] > max) {
                max = results[0][0][i];
                index = i;
            }
        }
        std::string fer_s = "fer:" + fer_status[index];

        cv::putText(src, fer_s, cv::Point(x1, y1 + 20), 1, FONT_HERSHEY_PLAIN,
            Scalar(255, 255, 255), 1, LINE_8);
  
        
        cv::Point p1(x1, y1), p2(x2, y1), p3(x1, y2), p4(x2, y2);
        cv::line(src, p1, p2, cv::Scalar(255, 255, 255), 1, cv::LineTypes::LINE_8);
        cv::line(src, p1, p3, cv::Scalar(255, 255, 255), 1, cv::LineTypes::LINE_8);
        cv::line(src, p2, p4, cv::Scalar(255, 255, 255), 1, cv::LineTypes::LINE_8);
        cv::line(src, p3, p4, cv::Scalar(255, 255, 255), 1, cv::LineTypes::LINE_8);
        /*
        * draw 关键点
        */
        /*
        for (size_t j = 0; j < 5; j++) {
            //if (j < 2) continue;
            cv::Point2f pt = cv::Point2f(it.pts.x[j] * scale_ratio * (1.0 * width / rz.width),
                it.pts.y[j] * scale_ratio * (1.0 * height / rz.height));
            cv::circle(src, pt, 1, Scalar(0, 255, 0), 2);
        }
        */
    }
}

int main(int argc, char** argv) {
    string path = "../../../data/models/retinaface";
    RetinaFace rf(path, "net3", 0.4);
    std::string fer_cache_path = "../../../data/models/fer/fer.cache";
    std::string fer_path = "../../../data/models/fer/fer.onnx";
    Fer fer(fer_path, fer_cache_path);
    uint32_t mm_num = 8;
    fer.Init(mm_num);
    std::vector<FaceDetectInfo> detect_info;
    VideoCapture capture(0);
    if (!capture.isOpened()) {
        cout << "can`t opeen this camera" << endl;
    }
    namedWindow("easy_sa", WINDOW_AUTOSIZE);
    Mat frame;
    Mat img_rsz;
    cv::Size rz(640, 640);
    while (true)
    {
        detect_info.clear();
        bool ret = capture.read(frame);
        if (!ret)
        {
            break;
        }
        cv::resize(frame, img_rsz, rz);
        detect_info = rf.detect(img_rsz, 0.9, 1.0);
        PredictFer(frame, detect_info, rz, &fer);
        imshow("easy_sa", frame);
        /*
        char c = waitKey(23);
        if (c == 27)
        {
            break;
        }*/
    }
    capture.release();
    return 0;
}
