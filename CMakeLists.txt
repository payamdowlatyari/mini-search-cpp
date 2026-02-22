cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(mini-search-cpp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(BUILD_TESTS "Build unit tests" ON)

include(FetchContent)

# ---------------------------------------------------------------------------
# Standalone Asio (no Boost)
# ---------------------------------------------------------------------------
FetchContent_Declare(asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        asio-1-28-0
    GIT_SHALLOW    TRUE
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
)
FetchContent_MakeAvailable(asio)

# Expose asio include dir so Crow's Findasio.cmake can find it.
set(ASIO_INCLUDE_DIR "${asio_SOURCE_DIR}/asio/include" CACHE PATH "" FORCE)

# ---------------------------------------------------------------------------
# Crow HTTP framework (header-only)
# ---------------------------------------------------------------------------
FetchContent_Declare(Crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG        v1.3.1
    GIT_SHALLOW    TRUE
)
set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CROW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(CROW_INSTALL        OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(Crow)

# ---------------------------------------------------------------------------
# search_engine_lib – core logic (no Crow dependency)
# ---------------------------------------------------------------------------
add_library(search_engine_lib STATIC
    src/tokenizer.cpp
    src/inverted_index.cpp
    src/ranking.cpp
    src/storage.cpp
    src/search_engine.cpp
    src/thread_pool.cpp
)

target_include_directories(search_engine_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(search_engine_lib PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(search_engine_lib PUBLIC pthread)

# ---------------------------------------------------------------------------
# mini-search – HTTP server binary
# ---------------------------------------------------------------------------
add_executable(mini-search src/main.cpp)

target_link_libraries(mini-search PRIVATE
    search_engine_lib
    Crow::Crow
)

target_compile_options(mini-search PRIVATE -Wall -Wextra)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(BUILD_TESTS)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
        GIT_SHALLOW    TRUE
    )
    set(BUILD_GMOCK   OFF CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_subdirectory(tests)
endif()
