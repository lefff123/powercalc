add_subdirectory(core)
add_subdirectory(parser)
add_subdirectory(ui)
add_subdirectory(document)

add_executable(powercalc main.cpp)
target_link_libraries(powercalc PRIVATE matrix parser ui Qt6::Widgets Qt6::Core)