add_library(ui STATIC mainwindow.cpp mainwindow.h
					  nodetablemodel.h nodetablemodel.cpp
					  tabledelegate.h
					  linetablemodel.h linetablemodel.cpp)
target_include_directories(ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(ui PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(ui PUBLIC Qt6::Widgets matrix parser QuaZip::QuaZip)