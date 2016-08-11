include(ExternalProject)

find_package(Threads REQUIRED)

ExternalProject_Add(googletest
	GIT_REPOSITORY https://github.com/google/googletest.git
	CMAKE_ARGS -DBUILD_GTEST=ON INSTALL_COMMAND ""
)

ExternalProject_Get_Property(googletest SOURCE_DIR)
ExternalProject_Get_Property(googletest BINARY_DIR)

include_directories("${SOURCE_DIR}/googletest/include")
include_directories("${SOURCE_DIR}/googlemock/include")

add_library(gtest UNKNOWN IMPORTED)
set(gtest_path "${BINARY_DIR}/googlemock/gtest/libgtest.a")
set_target_properties(gtest PROPERTIES IMPORTED_LOCATION "${gtest_path}")
add_dependencies(gtest googletest)

add_library(gmock UNKNOWN IMPORTED)
set(gmock_path "${BINARY_DIR}/googlemock/libgmock.a")
set_target_properties(gmock PROPERTIES IMPORTED_LOCATION "${gmock_path}")
add_dependencies(gmock googletest)

add_library(gtest_main UNKNOWN IMPORTED)
set(gtest_main_path "${BINARY_DIR}/googlemock/gtest/libgtest_main.a")
set_target_properties(gtest_main PROPERTIES IMPORTED_LOCATION "${gtest_main_path}")
add_dependencies(gtest_main googletest)

add_executable(value_tests value.cpp)
set_target_properties(value_tests PROPERTIES CXX_STANDARD 14)
set_target_properties(value_tests PROPERTIES OUTPUT_NAME value-tests)
target_link_libraries(value_tests ${CMAKE_THREAD_LIBS_INIT} rejson gtest_main gtest gmock)
add_test(NAME value-tests COMMAND $<TARGET_FILE:value_tests>)

add_executable(parse_tests parse.cpp)
set_target_properties(parse_tests PROPERTIES CXX_STANDARD 14)
set_target_properties(parse_tests PROPERTIES OUTPUT_NAME parse-tests)
target_link_libraries(parse_tests ${CMAKE_THREAD_LIBS_INIT} rejson gtest_main gtest gmock)
add_test(NAME parse-tests COMMAND $<TARGET_FILE:parse_tests>)
