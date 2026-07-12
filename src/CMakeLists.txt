add_subdirectory(core)
add_subdirectory(parser)

add_executable(powercalc main.cpp)
target_link_libraries(powercalc PRIVATE matrix)
target_link_libraries(powercalc PRIVATE parser)