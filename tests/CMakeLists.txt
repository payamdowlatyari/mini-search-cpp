add_executable(mini_search_tests
    test_tokenizer.cpp
    test_inverted_index.cpp
    test_ranking.cpp
    test_search_engine.cpp
)

target_include_directories(mini_search_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(mini_search_tests PRIVATE
    search_engine_lib
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(mini_search_tests)
