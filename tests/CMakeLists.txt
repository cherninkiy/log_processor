# tests/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# Подавление предупреждения CMP0135 для FetchContent
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# Сохраняем оригинальные флаги
set(CMAKE_CXX_FLAGS_BACKUP ${CMAKE_CXX_FLAGS})

# Отключаем санитайзеры для Google Test
if(CMAKE_CXX_FLAGS MATCHES "-fsanitize=")
    message(STATUS "⚠️  Temporarily disabling sanitizers for Google Test")
    
    # Убираем все санитайзеры из флагов компиляции
    string(REGEX REPLACE "-fsanitize=[^ ]*" "" CMAKE_CXX_FLAGS_TEMP ${CMAKE_CXX_FLAGS})
    string(REGEX REPLACE "-fsanitize-address-use-after-scope" "" CMAKE_CXX_FLAGS_TEMP ${CMAKE_CXX_FLAGS_TEMP})
    string(REGEX REPLACE "-fno-omit-frame-pointer" "" CMAKE_CXX_FLAGS_TEMP ${CMAKE_CXX_FLAGS_TEMP})
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS_TEMP})
endif()

# Скачиваем Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

# Отключаем установку GTest
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Восстанавливаем оригинальные флаги
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS_BACKUP})

# Функция для создания теста с правильными флагами
function(add_gtest TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    
    target_link_libraries(${TEST_NAME}
        PRIVATE
        common_lib
        reader_lib
        parser_lib
        analyzer_lib
        GTest::gtest_main
        GTest::gmock
    )
    
    # Если санитайзеры были включены, добавляем их к тесту
    if(CMAKE_CXX_FLAGS_BACKUP MATCHES "-fsanitize=")
        separate_arguments(SANITIZER_FLAGS UNIX_COMMAND "${CMAKE_CXX_FLAGS_BACKUP}")
        target_compile_options(${TEST_NAME} PRIVATE ${SANITIZER_FLAGS})
        target_link_options(${TEST_NAME} PRIVATE ${SANITIZER_FLAGS})
    endif()
    
    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    
    set_target_properties(${TEST_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    )
    
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

# Создаем тесты
add_gtest(test_parser test_parser.cpp)
add_gtest(test_analyzer test_analyzer.cpp)
add_gtest(test_reader test_reader.cpp)
add_gtest(test_stats test_stats.cpp)
add_gtest(test_threading test_threading.cpp)
add_gtest(test_atomics test_atomics.cpp)

# Если нужно больше тестов, добавляйте их так же
# add_gtest(test_something test_something.cpp)