cmake_minimum_required(VERSION 3.15)
project(PowerFlowCalc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Основной код
set(CMAKE_AUTOMOC on)
find_package(Qt6 REQUIRED COMPONENTS Core Core5Compat Widgets WebEngineWidgets Network)
find_package(QuaZip-Qt6 REQUIRED)
#внешняя зависимость
find_package(yaml-cpp QUIET)
if(NOT TARGET yaml-cpp::yaml-cpp)
  include(FetchContent)
  FetchContent_Declare(yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG 0.8.0)
  set(YAML_CPP_BUILD_TESTS OFF  CACHE BOOL "" FORCE)
  set(YAML_CPP_BUILD_TOOLS OFF  CACHE BOOL "" FORCE)
  set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
  set(YAML_CPP_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(yaml-cpp)
  if(NOT TARGET yaml-cpp::yaml-cpp AND TARGET yaml-cpp)
    add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)
  endif()
endif()

if(MSVC)
  add_compile_options(/utf-8)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/RELEASE)
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