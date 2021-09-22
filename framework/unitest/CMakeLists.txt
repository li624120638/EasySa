set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/out/build/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/out/build/bin)

if(build_framework_test)
  include_directories(${PROJECT_SOURCE_DIR}/framework/unitest/include)
  include_directories(${PROJECT_SOURCE_DIR}/framework/core/src)

  file(GLOB_RECURSE test_core_srcs ${PROJECT_SOURCE_DIR}/framework/unitest/core/*.cpp)
  set(test_srcs ${test_core_srcs})
  list(APPEND test_srcs ${PROJECT_SOURCE_DIR}/framework/unitest/test_main.cpp)

  add_executable(easysa_core_test ${test_srcs})
  target_link_libraries(easysa_core_test gtestd  glogd easysa_core ${3RDPARTY_LIBS})
  add_test(easysa_core_test ${EXECUTABLE_OUTPUT_PATH}/easysa_core_test)
endif()