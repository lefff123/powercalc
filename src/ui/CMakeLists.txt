add_library(ui STATIC mainwindow.cpp mainwindow.h
					  nodetablemodel.h nodetablemodel.cpp)
target_include_directories(ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ui PUBLIC Qt6::Widgets matrix)