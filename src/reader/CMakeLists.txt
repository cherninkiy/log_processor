add_library(reader_lib STATIC reader.cpp)
target_include_directories(reader_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})