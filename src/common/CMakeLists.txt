add_library(common_lib STATIC stats.cpp)

target_include_directories(common_lib PUBLIC ${CMAKE_SOURCE_DIR}/src)
