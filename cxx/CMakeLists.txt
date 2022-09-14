cmake_minimum_required(VERSION 3.18)
project(fakex)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fPIC -fsanitize=address -g -Wall")
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_BUILD_TYPE Release)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/0.18.1/conan.cmake"
            "${CMAKE_BINARY_DIR}/conan.cmake"
            TLS_VERIFY ON)
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)
conan_cmake_run(CONANFILE conanfile.py BASIC_SETUP CMAKE_TARGETS BUILD missing)

enable_testing()
find_package(GTest REQUIRED)
include(GoogleTest)
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/exchange)

add_executable(main startup/main.cpp)
target_link_libraries(main exchange_lib ${Boost_LIBRARIES})

add_executable(fakex_tests 
tests/price_tests.cpp 
tests/account_tests.cpp)
target_include_directories(fakex_tests PRIVATE ${CMAKE_SOURCE_DIR}/exchange)
target_link_libraries(fakex_tests exchange_lib ${Boost_LIBRARIES} GTest::gtest_main)
gtest_discover_tests(fakex_tests PROPERTIES TIMEOUT 600)