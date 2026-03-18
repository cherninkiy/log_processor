cmake_minimum_required(VERSION 3.16)

function(try_env_compiler CACHE_VAR DESCRIPTION)
    if(NOT DEFINED ${CACHE_VAR} OR ${CACHE_VAR} STREQUAL "")
        if(DEFINED ENV{${CACHE_VAR}})
            set(${CACHE_VAR} $ENV{${CACHE_VAR}} CACHE FILEPATH "${DESCRIPTION}" FORCE)
            message(STATUS "${CACHE_VAR} set from environment: $ENV{${CACHE_VAR}}")
        endif()
    endif()
endfunction()

try_env_compiler(CMAKE_C_COMPILER "C compiler")
try_env_compiler(CMAKE_CXX_COMPILER "C++ compiler")
try_env_compiler(CMAKE_MAKE_PROGRAM "Make program")

project(LogProcessor VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Опции сборки
option(BUILD_TESTS "Build tests" OFF)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)
option(USE_TSAN "Enable ThreadSanitizer" OFF)
option(USE_ASAN "Enable AddressSanitizer" OFF)
option(USE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

# Определяем платформу
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(LINUX TRUE)
    add_definitions(-D_LINUX)
endif()

# Настройки компилятора
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Базовые флаги для GCC/Clang
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    
    # Для Debug добавляем больше предупреждений и отладочную информацию
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -DDEBUG")
    
    # Для Release включаем оптимизацию
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
endif()

# Проверка наличия корутин (важно для C++20)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # Добавляем флаг корутин для GCC (для Clang не используется)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines")
    message(STATUS "Added -fcoroutines flag for ${CMAKE_CXX_COMPILER_ID}")
endif()

# Санитайзеры (только для Debug)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(USE_ASAN)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
        message(STATUS "AddressSanitizer enabled")
    endif()
    
    if(USE_TSAN)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -fno-omit-frame-pointer")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")
        message(STATUS "ThreadSanitizer enabled")
    endif()

    if(USE_UBSAN)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -fno-omit-frame-pointer")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined")
        message(STATUS "UndefinedBehaviorSanitizer enabled")
    endif()
endif()

# Поиск необходимых библиотек
find_package(Threads REQUIRED)

# Опционально: TBB для параллельных алгоритмов
find_package(TBB QUIET)
if(TBB_FOUND)
    # В WSL cmake деривирует prefix-пути из PATH, поэтому может подхватить
    # MSYS2/ucrt64 TBB по пути /mnt/... Этот TBB содержит Windows-только заголовки
    # (_CRTIMP, _WIN32, io.h) и ломает компиляцию с Linux gcc.
    if(LINUX AND TBB_DIR MATCHES "^/mnt/")
        message(STATUS "Intel TBB skipped: detected Windows path (${TBB_DIR}) — not usable on Linux")
        set(TBB_FOUND FALSE)
        unset(TBB_DIR CACHE)
    else()
        message(STATUS "Intel TBB found - parallel algorithms will be available")
    endif()
else()
    message(STATUS "Intel TBB not found - using fallback implementations")
endif()

# Подключаем дочерние CMakeLists
add_subdirectory(src)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
    
    # Автоматический запуск тестов после сборки (опционально)
    add_custom_target(check
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS tests
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running tests..."
    )
endif()

if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
    
    # Цель для запуска бенчмарков
    add_custom_target(run_benchmark
        COMMAND benchmark/log_benchmark
        DEPENDS benchmark
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running benchmarks..."
    )
endif()

# Информация о сборке
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "Target system: ${CMAKE_SYSTEM_NAME}")