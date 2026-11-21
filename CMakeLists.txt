cmake_minimum_required(VERSION 3.20)
project(cpp_embedded_refresh CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # for clangd and clang-tidy

# -DSANITIZER=address (default), thread (Project 5), or none
set(SANITIZER "address" CACHE STRING "address | thread | none")

# Flags for every target.
add_library(common_flags INTERFACE)
target_compile_options(common_flags INTERFACE -Wall -Wextra -Wpedantic -Wconversion -g)
if(SANITIZER STREQUAL "address")
    target_compile_options(common_flags INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(common_flags INTERFACE -fsanitize=address,undefined)
elseif(SANITIZER STREQUAL "thread")
    target_compile_options(common_flags INTERFACE -fsanitize=thread)
    target_link_options(common_flags INTERFACE -fsanitize=thread)
endif()

# Extra flags for "firmware" code in src/: link your libraries to this too.
add_library(firmware_flags INTERFACE)
target_compile_options(firmware_flags INTERFACE -Werror -fno-exceptions -fno-rtti)

# GoogleTest: use the installed copy, or download it.
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz)
    FetchContent_MakeAvailable(googletest)
endif()
include(GoogleTest)
enable_testing()

# One test executable per project: every .cpp in projects/<name>/tests/
# plus every .cpp in projects/<name>/src/.
file(GLOB project_dirs LIST_DIRECTORIES true "${CMAKE_SOURCE_DIR}/projects/*")
foreach(dir ${project_dirs})
    if(IS_DIRECTORY ${dir})
        get_filename_component(name ${dir} NAME)
        file(GLOB test_sources CONFIGURE_DEPENDS "${dir}/tests/*.cpp")
        file(GLOB src_sources CONFIGURE_DEPENDS "${dir}/src/*.cpp")
        if(test_sources)
            add_executable(test_${name} ${test_sources} ${src_sources})
            target_include_directories(test_${name} PRIVATE ${dir} ${dir}/src)
            target_link_libraries(test_${name} PRIVATE common_flags GTest::gtest_main)
            gtest_discover_tests(test_${name})
        endif()
    endif()
endforeach()
