add_library(document STATIC
	document_ast.h
	utf8.h
	document_parser.h
	document_parser.cpp
	expr_evaluator.h
	expr_evaluator.cpp
	symbol_table.h
	document_evaluator.h
	document_evaluator.cpp
)
target_include_directories(document PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(document PUBLIC yaml-cpp::yaml-cpp)
target_compile_features(document PUBLIC cxx_std_17)