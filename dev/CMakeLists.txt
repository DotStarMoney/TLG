cmake_minimum_required(VERSION 3.2)
project(TLG)

set(EX_PROJ_SOURCE_DIR ${CMAKE_BINARY_DIR}/deps/Source)
set(EX_PROJ_BUILD_DIR ${CMAKE_BINARY_DIR}/deps/Build)
set(TLG_SOURCE_DIR ${CMAKE_BINARY_DIR}/../src)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

configure_file(deps_CMakeLists.txt deps/CMakeLists.txt COPYONLY)
execute_process(COMMAND ${CMAKE_COMMAND} . WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/deps)
execute_process(COMMAND ${CMAKE_COMMAND} --build . WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/deps)

# From: https://github.com/google/googletest/blob/master/googletest/README.md {
#
# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# }

add_subdirectory(${EX_PROJ_SOURCE_DIR}/GoogleTest_EX
                 ${EX_PROJ_BUILD_DIR}/GoogleTest_EX
                 EXCLUDE_FROM_ALL)
set_target_properties(gtest gtest_main PROPERTIES FOLDER gtestTargets)
add_subdirectory(${EX_PROJ_SOURCE_DIR}/SDL_EX
                 ${EX_PROJ_BUILD_DIR}/SDL_EX
                 EXCLUDE_FROM_ALL)
include_directories(${EX_PROJ_SOURCE_DIR}/GLM_EX/glm
					${TLG_SOURCE_DIR})

enable_testing()
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
add_subdirectory(${TLG_SOURCE_DIR})