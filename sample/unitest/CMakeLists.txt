#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/out/build/lib)

option(build_retinaface_test "build retinaface test" ON)
option(build_etnet_test "build eye tracking net test" ON)
option(build_frp_test "build face postion test" ON)

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

include_directories(${PROJECT_SOURCE_DIR}/plugins/fer/include)
file(GLOB_RECURSE test_fer_src ${PROJECT_SOURCE_DIR}/plugins/fer/src/*.cpp)

include_directories(${PROJECT_SOURCE_DIR}/plugins/fp/include)
file(GLOB_RECURSE test_fp_src ${PROJECT_SOURCE_DIR}/plugins/fp/src/*.cpp)

#add_executable(test_google_net ./tensorrt/sampleGoogleNet.cpp ${srcs})
#target_link_libraries(test_google_net ${CUDA_LIBRARIES} nvinfer nvinfer_plugin nvonnxparser nvparsers)

#add_executable(test_opencv ./opencv/test_opencv.cpp)
#target_link_libraries(test_opencv ${OpenCV_LIBS} glogd)


# build retina face test

if(build_retinaface_test)
  include_directories(${PROJECT_SOURCE_DIR}/sample/unitest/retinaface)
  include_directories(${PROJECT_SOURCE_DIR}/sample/unitest/retinaface/tensorrt)
  include_directories(${PROJECT_SOURCE_DIR}/sample/unitest/etnet)
  file(GLOB_RECURSE test_rf_src ${PROJECT_SOURCE_DIR}/sample/unitest/retinaface/*.cpp )
  message(STATUS "retinaface_src" ${test_rf_src})

  cuda_add_executable(test_retinaface ./retinaface/test_retinaface.cpp ${srcs} ${test_rf_src} ${PROJECT_SOURCE_DIR}/sample/unitest/etnet/etnet.cpp ${test_fer_src})
  target_link_libraries(test_retinaface glogd ${CUDA_LIBRARIES} ${OpenCV_LIBS} nvinfer nvinfer_plugin nvonnxparser nvparsers easysa_plugins easysa_components)
endif()

if(build_etnet_test)
	include_directories(${PROJECT_SOURCE_DIR}/sample/unitest/etnet)
	file(GLOB_RECURSE test_et_src ${PROJECT_SOURCE_DIR}/sample/unitest/etnet/*.cpp )
	message(STATUS "etnet_src" ${test_et_src})
	cuda_add_executable(test_etnet ./etnet/test_etnet.cpp ${srcs} ${test_et_src} )
    target_link_libraries(test_etnet glogd ${CUDA_LIBRARIES} ${OpenCV_LIBS} nvinfer nvinfer_plugin nvonnxparser nvparsers)
endif()

#if(build_frp_test)
#	include_directories(${PROJECT_SOURCE_DIR}/sample/unitest/frp)
#	include_directories(${PROJECT_SOURCE_DIR}/sample/unitest/retinaface)
#    include_directories(${PROJECT_SOURCE_DIR}/sample/unitest/retinaface/tensorrt)
#	include_directories(${PROJECT_SOURCE_DIR}/sample/unitest/etnet)
#	file(GLOB_RECURSE test_frp_src ${PROJECT_SOURCE_DIR}/sample/unitest/retinaface/*.cpp )
#	message(STATUS "frp_src" ${test_frp_src})
#	cuda_add_executable(test_frp ./frp/test_fr_position.cpp ${srcs} ${test_frp_src} ${PROJECT_SOURCE_DIR}/sample/unitest/etnet/etnet.cpp)
#    target_link_libraries(test_frp glogd ${CUDA_LIBRARIES} ${OpenCV_LIBS} nvinfer nvinfer_plugin nvonnxparser nvparsers)
#endif()
