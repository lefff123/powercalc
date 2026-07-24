add_library(parser STATIC 
    csvparser.cpp
    iparser.h
)
target_include_directories(parser INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(parser PUBLIC matrix Qt6::Core Qt6::Core5Compat)