option(build_plugins_test "build plugins unitest" ON)

include_directories(${CUDA_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/3rdparty/TensorRT/include)
link_directories(${PROJECT_SOURCE_DIR}/3rdparty/TensorRT/lib)

include_directories(${PROJECT_SOURCE_DIR}/3rdparty/TensorRT/samples/common/)
include_directories(${PROJECT_SOURCE_DIR}/3rdparty/TensorRT/samples/common/windows/)

file(GLOB_RECURSE srcs ${PROJECT_SOURCE_DIR}/3rdparty/TensorRT/samples/common/*.cpp)
list(APPEND srcs ${PROJECT_SOURCE_DIR}/3rdparty/TensorRT/samples/common/windows/getopt.c)

include_directories(${3RDPARTY_INCLUDE_DIRS})
link_directories(${3RDPARTY_LIBS})
include_directories(${PROJECT_SOURCE_DIR}/components/ModelLoader/include)
include_directories(${GLOG_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/3rdparty/glog/include)
link_directories(${PROJECT_SOURCE_DIR}/3rdparty/glog/lib)

include_directories(${PROJECT_SOURCE_DIR}/3rdparty/googletest/include)
link_directories(${PROJECT_SOURCE_DIR}/3rdparty/googletest/lib)

set(test_plugins_srcs "")
list(APPEND test_plugins_srcs ${PROJECT_SOURCE_DIR}/plugins/unitest/test_plugins.cpp)

option(build_etvh_test "build eye track v&h plugin unitest" ON)
option(build_fer_test "build facial expression recognition plugin unitest" ON)
option(build_retinaface_test "build face detection plugin unitest" ON)
option(build_fp_test "build face pose plugin unitest" ON)
option(build_track_test "build face track plugin unitest" ON)

# plugins fer
if(build_fer_test)
	#include_directories(${PROJECT_SOURCE_DIR}/plugins/fer/include)
	#file(GLOB_RECURSE test_fer_src ${PROJECT_SOURCE_DIR}/plugins/fer/src/*.cpp )
	file(GLOB_RECURSE test_fer_src ${PROJECT_SOURCE_DIR}/plugins/unitest/fer/*.cpp )
	#message(STATUS "fer_src" ${test_fer_src})
	list(APPEND test_plugins_srcs ${test_fer_src})
endif()

#include_directories(${PROJECT_SOURCE_DIR}/plugins/fer/include)
#file(GLOB_RECURSE test_fer_src ${PROJECT_SOURCE_DIR}/plugins/fer/src/*.cpp )
#message(STATUS "fer_src" ${test_fer_src})
#cuda_add_executable(test_fer ./fer/test_fer.cpp ${srcs} ${test_fer_src})
#target_link_libraries(test_fer easysa_plugins easysa_components glogd ${OpenCV_LIBS})

# plugin etvh
if(build_etvh_test)
	#include_directories(${PROJECT_SOURCE_DIR}/plugins/etvh/include)
	#file(GLOB_RECURSE test_fer_src ${PROJECT_SOURCE_DIR}/plugins/etvh/src/*.cpp )
	file(GLOB_RECURSE test_etvh_src ${PROJECT_SOURCE_DIR}/plugins/unitest/etvh/*.cpp )
	#message(STATUS "etvh_src" ${test_etvh_src})
	list(APPEND test_plugins_srcs ${test_etvh_src})
endif()

#include_directories(${PROJECT_SOURCE_DIR}/plugins/etvh/include)
#file(GLOB_RECURSE test_etvh_src ${PROJECT_SOURCE_DIR}/plugins/etvh/src/*.cpp )
#message(STATUS "etvh_src" ${test_etvh_src})
#cuda_add_executable(test_etvh ./etvh/test_etvh.cpp ${srcs} ${test_etvh_src})
#target_link_libraries(test_etvh easysa_plugins easysa_components glogd ${OpenCV_LIBS})

# plugin retinaface
if(build_retinaface_test)
	include_directories(${PROJECT_SOURCE_DIR}/plugins/retinaface)
	include_directories(${PROJECT_SOURCE_DIR}/plugins/retinaface/tensorrt)
	#file(GLOB_RECURSE test_retinaface_src ${PROJECT_SOURCE_DIR}/plugins/retinaface/*.cpp )
	file(GLOB_RECURSE test_retinaface_src ${PROJECT_SOURCE_DIR}/plugins/unitest/retinaface/*.cpp )
	#message(STATUS "retinaface_src" ${test_retinaface_src})
	list(APPEND test_plugins_srcs ${test_retinaface_src})
endif()
#include_directories(${PROJECT_SOURCE_DIR}/plugins/retinaface)
#include_directories(${PROJECT_SOURCE_DIR}/plugins/retinaface/tensorrt)
#file(GLOB_RECURSE test_retinaface_src ${PROJECT_SOURCE_DIR}/plugins/retinaface/*.cpp )
#message(STATUS "retinaface_src" ${test_retinaface_src})
#cuda_add_executable(test_retinaface1 ./retinaface/test_retinaface.cpp ${srcs} ${test_retinaface_src})
#target_link_libraries(test_retinaface1 easysa_plugins easysa_components glogd ${OpenCV_LIBS})

if(build_retinaface_test)
	include_directories(${PROJECT_SOURCE_DIR}/plugins/retinaface)
	include_directories(${PROJECT_SOURCE_DIR}/plugins/retinaface/tensorrt)
	file(GLOB_RECURSE test_fp_src ${PROJECT_SOURCE_DIR}/plugins/unitest/fp/*.cpp )
	#message(STATUS "retinaface_src" ${test_retinaface_src})
	list(APPEND test_plugins_srcs ${test_fp_src})
endif()

if(build_track_test)
	file(GLOB_RECURSE test_track_src ${PROJECT_SOURCE_DIR}/plugins/unitest/track/*.cpp )
	list(APPEND test_plugins_srcs ${test_track_src})
endif()

cuda_add_executable(test_plugins ${srcs} ${test_plugins_srcs})
target_link_libraries(test_plugins easysa_plugins easysa_components gtestd glogd ${OpenCV_LIBS})
