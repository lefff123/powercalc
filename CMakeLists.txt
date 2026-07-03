cmake_minimum_required(VERSION 3.15)
project(PowerFlowCalc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Основной код
add_subdirectory(src)

# Тесты — включены по умолчанию
option(BUILD_TESTS "Build unit tests with Google Test" ON)

if(BUILD_TESTS)
    message(STATUS "Tests are ENABLED")
    enable_testing()
    add_subdirectory(tests)
else()
    message(STATUS "Tests are DISABLED")
endif()