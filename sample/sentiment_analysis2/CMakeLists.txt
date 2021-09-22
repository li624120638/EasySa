include_directories(${PROJECT_SOURCE_DIR}/framework/core/include)
include_directories(${PROJECT_SOURCE_DIR}/framework/core/src)

include_directories(${PROJECT_SOURCE_DIR}/sample/sentiment_analysis2/include)
file(GLOB_RECURSE test_sa2_src ${PROJECT_SOURCE_DIR}/sample/sentiment_analysis2/src/*.cpp )

include_directories(${CUDA_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/3rdparty/TensorRT/include)
link_directories(${PROJECT_SOURCE_DIR}/3rdparty/TensorRT/lib)

include_directories(${PROJECT_SOURCE_DIR}/3rdparty/TensorRT/samples/common/)
include_directories(${PROJECT_SOURCE_DIR}/3rdparty/TensorRT/samples/common/windows/)

file(GLOB_RECURSE srcs ${PROJECT_SOURCE_DIR}/3rdparty/TensorRT/samples/common/*.cpp)
list(APPEND srcs ${PROJECT_SOURCE_DIR}/3rdparty/TensorRT/samples/common/windows/getopt.c)

include_directories(${3RDPARTY_INCLUDE_DIRS})
link_directories(${3RDPARTY_LIBS})
include_directories(${GLOG_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/3rdparty/glog/include)
link_directories(${PROJECT_SOURCE_DIR}/3rdparty/glog/lib)

include_directories(${PROJECT_SOURCE_DIR}/components/ModelLoader/include)
include_directories(${PROJECT_SOURCE_DIR}/components/FrameControl/include)
include_directories(${PROJECT_SOURCE_DIR}/plugins/retinaface)
include_directories(${PROJECT_SOURCE_DIR}/plugins/retinaface/tensorrt)
file(GLOB_RECURSE test_retinaface_src ${PROJECT_SOURCE_DIR}/plugins/retinaface/*.cpp )

include_directories(${PROJECT_SOURCE_DIR}/plugins/fp/include)
file(GLOB_RECURSE test_fp_src ${PROJECT_SOURCE_DIR}/plugins/fp/src/*.cpp)

include_directories(${PROJECT_SOURCE_DIR}/plugins/fer/include)
file(GLOB_RECURSE test_fer_src ${PROJECT_SOURCE_DIR}/plugins/fer/src/*.cpp )

include_directories(${PROJECT_SOURCE_DIR}/plugins/etvh/include)
file(GLOB_RECURSE test_etvh_src ${PROJECT_SOURCE_DIR}/plugins/etvh/src/*.cpp )

cuda_add_executable(test_sa2 ./test_sentiment_analysis2.cpp ${srcs} ${test_sa2_src}
				  ${test_retinaface_src} ${test_fp_src} ${test_fer_src} ${test_etvh_src})
target_link_libraries(test_sa2 easysa_plugins easysa_components easysa_core glogd ${OpenCV_LIBS} ${CUDA_LIBRARIES} nvinfer nvinfer_plugin nvonnxparser nvparsers)

