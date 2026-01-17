add_executable(main main.c)
add_library(CALC STATIC calc.c)
add_library(RENDER STATIC render.c)

target_link_libraries(RENDER PUBLIC SDL3::SDL3)
target_link_libraries(main CALC RENDER)


