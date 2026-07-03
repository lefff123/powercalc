add_subdirectory(core)

add_executable(powercalc main.cpp)
target_link_libraries(powercalc PRIVATE matrix)