add_subdirectory(reader)
add_subdirectory(parser)
add_subdirectory(analyzer)
add_subdirectory(common)

add_executable(log_processor main.cpp)

target_link_libraries(log_processor
    PRIVATE
        reader_lib
        parser_lib
        analyzer_lib
        common_lib
)