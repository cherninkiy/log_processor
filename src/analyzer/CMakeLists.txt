add_library(analyzer_lib STATIC analyzer.cpp)
target_include_directories(analyzer_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(analyzer_lib PRIVATE common_lib)