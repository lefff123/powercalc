add_library(matrix STATIC
			types.h line.h matrix.h node.h solver.h 
			solver.cpp powersystem.h powersystem.cpp)
target_include_directories(matrix PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})