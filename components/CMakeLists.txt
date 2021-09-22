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

include_directories(${PROJECT_SOURCE_DIR}/3rdparty/libyuv/include)
link_directories(${PROJECT_SOURCE_DIR}/3rdparty/libyuv/lib)

include_directories(${PROJECT_SOURCE_DIR}/3rdparty/rapidjson/include)

set(component_list ModelLoader FrameControl ModuleParam Common FrameSkipControl ScalerAndTiler)
foreach(component ${component_list})
  include_directories(${PROJECT_SOURCE_DIR}/components/${component}/include)
  file(GLOB_RECURSE component_src ${PROJECT_SOURCE_DIR}/components/${component}/*.cpp)
  message(STATUS "component: " ${component_src})
  list(APPEND srcs ${component_src})
endforeach()

set(SOURCE_LINKER_LIBS glogd ${CUDA_LIBRARIES} ${OpenCV_LIBS} nvinfer nvinfer_plugin nvonnxparser nvparsers)
add_library(easysa_components STATIC ${srcs})
target_link_libraries(easysa_components ${SOURCE_LINKER_LIBS})

if(build_components_test)
add_subdirectory(unitest)
endif()