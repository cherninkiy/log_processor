add_library(parser_lib STATIC parser.cpp)
target_include_directories(parser_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(parser_lib PRIVATE common_lib)