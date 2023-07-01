cmake_minimum_required(VERSION 3.10)
project(svf)
set(CMAKE_CXX_STANDARD 20)

# Assumption: Windows
# Assumption: clang
# Assumption: x64

#
# `embed`: embeds binary data into C++ source code.
#
add_executable(embed
  src/exe/embed.cpp
)

#
# Generated files.
#
function(generate_schema_files SCHEMA_NAME)
  set (TXT_NAME ${CMAKE_CURRENT_SOURCE_DIR}/schema/${SCHEMA_NAME}.txt)
  set (HPP_NAME ${CMAKE_CURRENT_BINARY_DIR}/custom/generated/hpp/${SCHEMA_NAME}.hpp)
  set (BLOB_NAME ${CMAKE_CURRENT_BINARY_DIR}/custom/generated/blob/${SCHEMA_NAME}.blob)
  set (INC_NAME ${CMAKE_CURRENT_BINARY_DIR}/custom/generated/inc/${SCHEMA_NAME}.inc)
  add_custom_command(
    OUTPUT ${HPP_NAME}
    COMMAND svfc cpp ${TXT_NAME} > ${HPP_NAME}
    DEPENDS svfc ${TXT_NAME}
  )
  add_custom_command(
    OUTPUT ${BLOB_NAME}
    COMMAND svfc binary ${TXT_NAME} > ${BLOB_NAME}
    DEPENDS svfc ${TXT_NAME}
  )
  add_custom_command(
    OUTPUT ${INC_NAME}
    COMMAND embed ${BLOB_NAME} ${INC_NAME}
    DEPENDS embed ${BLOB_NAME}
  )
  add_custom_target(${SCHEMA_NAME}_hpp DEPENDS ${HPP_NAME})
  add_custom_target(${SCHEMA_NAME}_inc DEPENDS ${INC_NAME})
endfunction(generate_schema_files)

generate_schema_files(a0)
generate_schema_files(a1)
generate_schema_files(meta)
add_custom_command(
  TARGET meta_hpp
  COMMAND ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_SOURCE_DIR}/src/meta.hpp ${CMAKE_CURRENT_BINARY_DIR}/custom/generated/hpp/meta.hpp
)

#
# Common compiler options.
#
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_definitions(COMPILE_TIME_OPTION_BUILD_TYPE_DEVELOPMENT)
endif()
include_directories(${CMAKE_CURRENT_BINARY_DIR}/custom)

#
# `svfc`
#
add_executable(svfc
  src/exe/svfc.cpp
  src/core/parsing.cpp
  src/core/typechecking.cpp
  src/core/output_binary.cpp
  src/core/output_cpp.cpp
  src/platform/windows/implementation.cpp
)

#
# `test_a`
#
add_executable(test_a
  src/test/a.cpp
  src/core/parsing.cpp
  src/core/compatibility.cpp
  src/platform/windows/implementation.cpp
)
add_dependencies(test_a meta_hpp)
add_dependencies(test_a a0_hpp)
add_dependencies(test_a a1_hpp)
add_dependencies(test_a a0_inc)
add_dependencies(test_a a1_inc)

#
# Tests.
#
enable_testing()
add_test(NAME test_a COMMAND test_a)