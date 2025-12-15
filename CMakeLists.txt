cmake_minimum_required(VERSION 3.14)
project(rstd VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)

add_subdirectory(examples)

add_library(rstd INTERFACE)
add_library(rstd::rstd ALIAS rstd)

target_include_directories(rstd INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(DIRECTORY include/rstd DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS rstd EXPORT rstdTargets)

install(EXPORT rstdTargets
    FILE rstdConfig.cmake
    NAMESPACE rstd::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rstd
)

message(STATUS "")
message(STATUS "rstd ${PROJECT_VERSION} configured")
message(STATUS "Installation paths:")
message(STATUS "  Headers:      ${CMAKE_INSTALL_INCLUDEDIR}/rstd")
message(STATUS "  CMake config: ${CMAKE_INSTALL_LIBDIR}/cmake/rstd")
message(STATUS "")
message(STATUS "To build examples:      cmake --build .")
message(STATUS "To install:             cmake --install . --prefix /path/to/install")
message(STATUS "")
