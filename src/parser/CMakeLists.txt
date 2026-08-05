add_library(parser STATIC 
    csvparser.h
	csvparser.cpp
	csvwriter.h
	csvwriter.cpp
    iparser.h
)
target_include_directories(parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(parser PUBLIC matrix Qt6::Core Qt6::Core5Compat)