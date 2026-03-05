cmake_minimum_required(VERSION 3.28)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Force Release for CommonLibF4" FORCE)
endif()

macro(set_from_environment VARIABLE)
  if(NOT DEFINED ${VARIABLE} AND DEFINED ENV{${VARIABLE}})
    set(${VARIABLE} $ENV{${VARIABLE}})
  endif()
endmacro()

set(PROJECT_DIR "${CMAKE_SOURCE_DIR}/Projects/${PROJECT_ID}")

set(COMMONLIBF4_DIR "${CMAKE_SOURCE_DIR}/extern/CommonLibF4")

project(${PROJECT_ID} VERSION ${PROJECT_VERSION} LANGUAGES CXX)

add_subdirectory(${COMMONLIBF4_DIR} CommonLibF4)

find_package(spdlog REQUIRED CONFIG)
find_package(fmt REQUIRED CONFIG)
find_package(eventpp CONFIG REQUIRED)

find_path(SIMPLEINI_INCLUDE_DIRS "ConvertUTF.c")

set(SOURCES
  ${CMAKE_SOURCE_DIR}/src/pch.cpp
  ${CMAKE_SOURCE_DIR}/src/main.cpp
)

source_group(
  TREE ${CMAKE_SOURCE_DIR}
  FILES ${SOURCES}
)

add_library(
  ${PROJECT_ID}
  SHARED
  ${SOURCES}
)

# Generate a configured header for in-code access to the project version
set(VERSION_HEADER_IN "${CMAKE_SOURCE_DIR}/cmake/project.h.in")
# generate project.h from project.h.in so code can include <project.h>
set(VERSION_HEADER "${CMAKE_BINARY_DIR}/project.h")
configure_file(${VERSION_HEADER_IN} ${VERSION_HEADER} @ONLY)
target_include_directories(${PROJECT_ID} PRIVATE ${CMAKE_BINARY_DIR})

set(VERSION_RC_IN "${CMAKE_SOURCE_DIR}/cmake/version.rc.in")
set(VERSION_RC "${CMAKE_BINARY_DIR}/${PROJECT_ID}_version.rc")
configure_file(${VERSION_RC_IN} ${VERSION_RC} @ONLY)
target_sources(${PROJECT_ID} PRIVATE ${VERSION_RC})

target_compile_definitions(
  ${PROJECT_ID}
  PRIVATE
  _UNICODE
)

target_compile_definitions(
  CommonLibF4
  PUBLIC
  F4SE_SUPPORT_XBYAK
  _SILENCE_CXX23_ALIGNED_STORAGE_DEPRECATION_WARNING
)

target_compile_features(
  ${PROJECT_ID}
  PRIVATE
  cxx_std_23
)

target_include_directories(${PROJECT_ID} PRIVATE ${SIMPLEINI_INCLUDE_DIRS})

target_include_directories(
  ${PROJECT_ID}
  PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${PROJECT_DIR}/include
)

target_precompile_headers(
  ${PROJECT_ID}
  PRIVATE
  "${CMAKE_SOURCE_DIR}/include/pch.h"
)

function(target_link_libraries_system target lib)
  # Get the include directories of the library
  get_target_property(_includes ${lib} INTERFACE_INCLUDE_DIRECTORIES)
  if(_includes)
    target_include_directories(${target} SYSTEM PRIVATE ${_includes})
  endif()

  # Link the library normally
  target_link_libraries(${target} PRIVATE ${lib})
endfunction()

target_link_libraries_system(${PROJECT_ID} CommonLibF4::CommonLibF4)
target_link_libraries_system(${PROJECT_ID} eventpp::eventpp)

set_target_properties(${PROJECT_ID} PROPERTIES OUTPUT_NAME "${PROJECT_ID}")

target_compile_definitions(
  ${PROJECT_ID}
  PRIVATE
  WIN32_LEAN_AND_MEAN
  NOMB
  NOMINMAX
  NOSERVICE
)

file(GLOB PROJECT_HEADERS CONFIGURE_DEPENDS "${PROJECT_DIR}/include/*.hpp")
set_source_files_properties(${PROJECT_HEADERS} PROPERTIES LANGUAGE CXX HEADER_FILE_ONLY OFF)

add_library(CLANGD_PROJECT_HEADERS OBJECT EXCLUDE_FROM_ALL ${PROJECT_HEADERS})
target_compile_definitions(CLANGD_PROJECT_HEADERS PRIVATE $<TARGET_PROPERTY:${PROJECT_ID},COMPILE_DEFINITIONS>)
target_include_directories(CLANGD_PROJECT_HEADERS PRIVATE $<TARGET_PROPERTY:${PROJECT_ID},INCLUDE_DIRECTORIES>)
target_compile_options(CLANGD_PROJECT_HEADERS PRIVATE $<TARGET_PROPERTY:${PROJECT_ID},COMPILE_OPTIONS>)
target_precompile_headers(CLANGD_PROJECT_HEADERS PRIVATE "${CMAKE_SOURCE_DIR}/include/pch.h")

set(OUTPUT_DIR_F4SE "${PROJECT_DIR}/F4SE/Plugins")

add_custom_command(TARGET ${PROJECT_ID} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR_F4SE}"
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_ID}> "${OUTPUT_DIR_F4SE}"
  COMMAND ${CMAKE_COMMAND} -E echo "Built ${PROJECT_ID}"
)
