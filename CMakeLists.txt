cmake_minimum_required(VERSION 3.0)

include(GenerateExportHeader)

project(librejson VERSION 1.0.0)

configure_file(rejson/version.hpp.cmake
               rejson/version.hpp)

include_directories(BEFORE "${PROJECT_BINARY_DIR}")
include_directories(BEFORE "${PROJECT_SOURCE_DIR}")

file(GLOB rejson_src_files "src/*.cpp")
add_library(rejson SHARED ${rejson_src_files})
set_property(TARGET rejson PROPERTY CXX_STANDARD 14)
generate_export_header(rejson EXPORT_FILE_NAME rejson/export.h)

enable_testing()
add_subdirectory(test)
