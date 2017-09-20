cmake_minimum_required( VERSION 2.8.3 )

set(ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(BUILD_PATH "${CMAKE_BINARY_DIR}")
set(CMAKE_INSTALL_PREFIX "${ROOT_DIR}/output")

set(APP_INSTALL_DIR "./")
set(LIB_INSTALL_DIR "lib")

find_package(OpenSSL REQUIRED)

set(CMAKE_INSTALL_RPATH ${LIB_INSTALL_DIR})

include( "cmake/default.cmake" )

add_custom_target("uninstall"
                  COMMAND rm -rf ${CMAKE_INSTALL_PREFIX}/*
                  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
                  COMMENT "[TACO] Clean ${CMAKE_INSTALL_PREFIX}")

add_custom_target("distclean"
                  ${CMAKE_MAKE_PROGRAM} clean
                  COMMAND rm -rf *
                  WORKING_DIRECTORY ${BUILD_PATH}
                  COMMENT "[TACO] Uninstall, make clean, and clean CMake generated files"
                  DEPENDS "uninstall")

add_subdirectory(source)
add_subdirectory(examples)
