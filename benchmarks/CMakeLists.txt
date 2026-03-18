# benchmarks/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# Подавление предупреждения CMP0135 для FetchContent
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# Сохраняем оригинальные флаги
set(CMAKE_CXX_FLAGS_BACKUP ${CMAKE_CXX_FLAGS})

# Отключаем санитайзеры для Google Benchmark
if(CMAKE_CXX_FLAGS MATCHES "-fsanitize=")
    message(STATUS "⚠️  Temporarily disabling sanitizers for Google Benchmark")
    
    # Убираем все санитайзеры из флагов компиляции
    string(REGEX REPLACE "-fsanitize=[^ ]*" "" CMAKE_CXX_FLAGS_TEMP ${CMAKE_CXX_FLAGS})
    string(REGEX REPLACE "-fsanitize-address-use-after-scope" "" CMAKE_CXX_FLAGS_TEMP ${CMAKE_CXX_FLAGS_TEMP})
    string(REGEX REPLACE "-fno-omit-frame-pointer" "" CMAKE_CXX_FLAGS_TEMP ${CMAKE_CXX_FLAGS_TEMP})
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS_TEMP})
endif()

# Отключаем Werror для Google Benchmark (опционально)
set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

# Скачиваем Google Benchmark
include(FetchContent)
FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.3
)

FetchContent_MakeAvailable(googlebenchmark)

# Восстанавливаем оригинальные флаги
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS_BACKUP})

# Создаем наш бенчмарк
add_executable(log_benchmark benchmark.cpp)

# Линкуем с Google Benchmark
target_link_libraries(log_benchmark 
    PRIVATE
    benchmark::benchmark
    common_lib
    reader_lib
    parser_lib
    analyzer_lib
)

# Если TBB найден — линкуем и включаем бенчмарк с concurrent_hash_map
if(TBB_FOUND)
    target_link_libraries(log_benchmark PRIVATE TBB::tbb)
    target_compile_definitions(log_benchmark PRIVATE HAVE_TBB)
    message(STATUS "log_benchmark: TBB enabled (BM_ProcessFileConcurrentTBB active)")
else()
    message(STATUS "log_benchmark: TBB not found (BM_ProcessFileConcurrentTBB skipped)")
endif()

# Если санитайзеры были включены, добавляем их к нашему бенчмарку
if(CMAKE_CXX_FLAGS_BACKUP MATCHES "-fsanitize=")
    # Разбиваем строку флагов на отдельные аргументы
    separate_arguments(SANITIZER_FLAGS UNIX_COMMAND "${CMAKE_CXX_FLAGS_BACKUP}")
    target_compile_options(log_benchmark PRIVATE ${SANITIZER_FLAGS})
    target_link_options(log_benchmark PRIVATE ${SANITIZER_FLAGS})
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(benchmark PRIVATE -Wno-switch)
    target_compile_options(benchmark_main PRIVATE -Wno-switch)
endif()

set_target_properties(log_benchmark PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/benchmarks"
)