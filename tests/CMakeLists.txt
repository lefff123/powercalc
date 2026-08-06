message(STATUS "Configuring tests...")

# Подключаем Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)

# Для Windows: не переопределять настройки компилятора
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# ВАЖНО: это запустит скачивание и сборку gtest
message(STATUS "Fetching Google Test (may take a while on first run)...")
FetchContent_MakeAvailable(googletest)
message(STATUS "Google Test fetched successfully")

# Исполняемый файл тестов
add_executable(matrix_tests 
    test_matrix.cpp
    test_network.cpp
	test_parser.cpp
	test_parser_solver_integration.cpp
	test_document_parser.cpp
	test_expr_evaluator.cpp
	test_document_perf.cpp
)
# Линкуем с gtest и нашей библиотекой
target_link_libraries(matrix_tests PRIVATE 
    matrix
	parser
    GTest::gtest 
    GTest::gtest_main
	Qt6::Core
	document
)

# Интеграция с CTest
include(GoogleTest)
gtest_discover_tests(matrix_tests)