set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/out/build/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/out/build/bin)

include_directories(${3RDPARTY_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/3rdparty/glog/include)
link_directories(${PROJECT_SOURCE_DIR}/3rdparty/glog/lib)

set(module_list core)

set(SOURCE_LINKER_LIBS pthread ${FFmpeg_LIBS} ${Opencv_LIBS} ${YUV_LIBS})
set(srcs "")
foreach(module ${module_list})
  include_directories(${PROJECT_SOURCE_DIR}/framework/${module}/include)
  file(GLOB_RECURSE module_src ${PROJECT_SOURCE_DIR}/framework/${module}/*.cpp )
  list(APPEND srcs ${module_src})
endforeach()
include_directories(${PROJECT_SOURCE_DIR}/framework/core/src)

add_library(easysa_core ${srcs})
target_link_libraries(easysa_core ${Opencv_LIBS} ${FFmpeg_LIBS} ${YUV_LIBS} glogd)

if(build_framework_test)
  add_subdirectory(unitest)
endif()