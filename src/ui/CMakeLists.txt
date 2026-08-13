add_library(ui STATIC mainwindow.cpp mainwindow.h
					  nodetablemodel.h nodetablemodel.cpp
					  tabledelegate.h
					  linetablemodel.h linetablemodel.cpp
					  document_highlighter.h document_highlighter.cpp
					  document_editor.h
					  document_editor.cpp
					  document_view.h
					  image_scheme_handler.h image_scheme_handler.cpp
					  pdf_pagenum.h pdf_pagenum.cpp
					  aboutdialog.h aboutdialog.cpp)
target_include_directories(ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(ui PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_sources(ui PRIVATE ${CMAKE_SOURCE_DIR}/res/katex.qrc)
set_target_properties(ui PROPERTIES AUTOMOC ON AUTORCC ON)
target_link_libraries(ui PUBLIC Qt6::Widgets Qt6::WebEngineWidgets matrix parser 
					  document QuaZip::QuaZip)