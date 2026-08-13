message(STATUS "Configuring tests...")

add_executable(matrix_tests 
    test_matrix.cpp
    test_network.cpp
    test_parser.cpp
    test_parser_solver_integration.cpp
    test_document_parser.cpp
    test_expr_evaluator.cpp
    test_document_perf.cpp
    test_number_format.cpp
    test_substitution.cpp
    test_html_generator.cpp
)

target_link_libraries(matrix_tests PRIVATE 
    matrix
    parser
    GTest::gtest 
    GTest::gtest_main
    Qt6::Core
    document
)

include(GoogleTest)
gtest_discover_tests(matrix_tests)