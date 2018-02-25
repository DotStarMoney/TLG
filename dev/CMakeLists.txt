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

define_property(
    TARGET
    PROPERTY FOLDER
    INHERITED
    BRIEF_DOCS "Set the folder name."
    FULL_DOCS  "Use to organize targets in an IDE."
)

function(add_subdirectory_with_folder _folder_name _folder)
    add_subdirectory(${_folder} ${ARGN})
    set_property(DIRECTORY "${_folder}" PROPERTY FOLDER "${_folder_name}")
endfunction()

set(gtest_force_shared_crt ON CACHE BOOL "Prevent overriding the parent project's compiler/linker settings on Windows" FORCE)
add_subdirectory_with_folder("gtest" ${EX_PROJ_SOURCE_DIR}/GoogleTest_EX)

set(GFLAGS_BUILD_gflags_LIB ON CACHE BOOL "")
set(GFLAGS_BUILD_STATIC_LIBS ON CACHE BOOL "")
set(GFLAGS_BUILD_gflags_nothreads_LIB OFF CACHE BOOL "")
add_subdirectory_with_folder("gflags" ${EX_PROJ_SOURCE_DIR}/GFlags_EX)

set(BUILD_TESTING OFF CACHE BOOL "")
set(BUILD_EXAMPLES OFF CACHE BOOL "")
add_subdirectory_with_folder("cctz" ${EX_PROJ_SOURCE_DIR}/CCTZ_EX)

set(BUILD_TESTING OFF CACHE BOOL "")
add_subdirectory_with_folder("abseil" ${EX_PROJ_SOURCE_DIR}/Abseil_EX)

add_subdirectory_with_folder("glog" ${EX_PROJ_SOURCE_DIR}/GLOG_EX)

set(SDL_STATIC ON CACHE BOOL "")
set(SDL_SHARED OFF CACHE BOOL "")
add_subdirectory_with_folder("SDL" ${EX_PROJ_SOURCE_DIR}/SDL_EX)
include_directories(${EX_PROJ_SOURCE_DIR}/SDL_EX/include)

add_subdirectory_with_folder("glm" ${EX_PROJ_SOURCE_DIR}/GLM_EX)

enable_testing()
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
include_directories(${TLG_SOURCE_DIR})
add_subdirectory(${TLG_SOURCE_DIR})