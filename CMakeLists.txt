cmake_minimum_required(VERSION 3.15)
project(PowerFlowCalc VERSION 1.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Conan toolchain должен быть загружен ДО всего
if(EXISTS "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
    include("${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
endif()

set(CMAKE_AUTOMOC ON)

# Qt6 и все зависимости теперь из Conan
find_package(Qt6 REQUIRED COMPONENTS Core Core5Compat Widgets WebEngineWidgets Network)
find_package(QuaZip-Qt6 REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(GTest REQUIRED)

if(MSVC)
    add_compile_options(/utf-8)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/RELEASE)
add_subdirectory(src)

option(BUILD_TESTS "Build unit tests with Google Test" ON)
if(BUILD_TESTS)
    message(STATUS "Tests are ENABLED")
    enable_testing()
    add_subdirectory(tests)
else()
    message(STATUS "Tests are DISABLED")
endif()