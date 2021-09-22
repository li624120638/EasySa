set(LIBRARY_OUTPUT_PATH $${PROJECT_SOURCE_DIR}/build/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}bin/)

# include framework
include_directories(${PROJECT_SOURCE_DIR}/framework/core/include)
include_directories(${PROJECT_SOURCE_DIR}/3rdparty/googletest/include)
link_directories(${PROJECT_SOURCE_DIR}/3rdparty/googletest/lib)
include_directories(${PROJECT_SOURCE_DIR}/3rdparty/libyuv/include)
link_directories(${PROJECT_SOURCE_DIR}/3rdparty/libyuv/lib)

set(module_list "")

if(build_source)
  list(APPEND module_list source)
endif()

if(WITH_FFMPEG)
  include_directories(${PROJECT_SOURCE_DIR}/3rdparty/ffmpeg/include)
endif()

if(WITH_OPENCV)
  include_directories(${OpenCV_INCLUDE_DIRS})
endif()

set(SOURCE_LINKER_LIBS ${3RDPARTY_LIBS} pthread)

include_directories(${PROJECT_SOURCE_DIR}/modules)
file(GLOB srcs ${PROJECT_SOURCE_DIR}/modules/*.cpp)

foreach(module ${module_list})
  include_directories(${PROJECT_SOURCE_DIR}/modules/${module}/include)
  file(GLOB_RECURSE module_src ${PROJECT_SOURCE_DIR}/modules/${module}/*.cpp)
  list(APPEND srcs ${module_src})
endforeach()

set(SOURCE_LINKER_LIBS  ${Opencv_LIBS} glogd)
add_library(easysa_va SHARED ${srcs})
target_link_libraries(easysa_va ${SOURCE_LINKER_LIBS})

#if(build_tests)
# add_subdirectory(unitest)
#endif()