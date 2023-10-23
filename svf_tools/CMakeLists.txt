cmake_minimum_required(VERSION 3.20)
project(svf_tools)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 99)

if (
  NOT CMAKE_SYSTEM_NAME STREQUAL "Windows"
  AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux"
  AND NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin"
)
  message(FATAL_ERROR "Target OS must be Windows, Linux, macOS.")
endif()

if (
  NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64"
  AND NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64"
  AND NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64"
)
  message(FATAL_ERROR "Target architecture must be x64 or arm64.")
endif()

if (
  NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
  AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"
)
  # Only support Clang for now.
  message(FATAL_ERROR "Compiler must be Clang.")
endif()

#
# Customize build variants.
#

# Disable these.
set(CMAKE_CXX_FLAGS_MINSIZEREL "--no-such-flag")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "--no-such-flag")
set(CMAKE_C_FLAGS_MINSIZEREL "--no-such-flag")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "--no-such-flag")

# Release will be similar to MinSizeRel + RelWithDebInfo.
set(OUR_FLAGS
  -Oz
  -DNDEBUG
  -g
)
list(JOIN OUR_FLAGS " " OUR_FLAGS)
set(CMAKE_CXX_FLAGS_RELEASE ${OUR_FLAGS})
set(CMAKE_C_FLAGS_RELEASE ${OUR_FLAGS})

# Debug. All checks are go here.
set(OUR_FLAGS
  -O0
  -D_DEBUG
  -g3
)
list(JOIN OUR_FLAGS " " OUR_FLAGS)
set(CMAKE_CXX_FLAGS_DEBUG ${OUR_FLAGS})
set(CMAKE_C_FLAGS_DEBUG ${OUR_FLAGS})

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Windows" AND NOT CMAKE_CROSSCOMPILING)
  # Note: UBSan stopped working on Windows after some changes in this file, why?
  # lld-link: error /failifmismatch: mismatch detected for '_ITERATOR_DEBUG_LEVEL'

  set(OUR_FLAGS
    -fsanitize=undefined
    -fsanitize=implicit-conversion
    -fsanitize=local-bounds
    -fsanitize-trap=undefined
    -fno-sanitize-recover=undefined
  )
  list(JOIN OUR_FLAGS " " OUR_FLAGS)

  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${OUR_FLAGS}")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${OUR_FLAGS}")
endif()

# Note: this does not work properly, as these flags are duplicated twice.
# Once from these commands, and once from some CMake magic.
# Need to figure it out...
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(OUR_FLAGS
    -Xclang
    --dependent-lib=msvcrt
    -fms-runtime-lib=dll
  )
  list(JOIN OUR_FLAGS " " OUR_FLAGS)

  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${OUR_FLAGS}")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${OUR_FLAGS}")

  set(OUR_FLAGS
    -Xclang
    --dependent-lib=msvcrtd
    -fms-runtime-lib=dll
  )
  list(JOIN OUR_FLAGS " " OUR_FLAGS)

  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${OUR_FLAGS}")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${OUR_FLAGS}")
endif()

if (0)
  message("CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
  message("CMAKE_CXX_FLAGS_DEBUG is ${CMAKE_CXX_FLAGS_DEBUG}")
  message("CMAKE_CXX_FLAGS_RELEASE is ${CMAKE_CXX_FLAGS_RELEASE}")
  message("CMAKE_CXX_FLAGS_RELWITHDEBINFO is ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
  message("CMAKE_CXX_FLAGS_MINSIZEREL is ${CMAKE_CXX_FLAGS_MINSIZEREL}")
  message("CMAKE_CXX_FLAGS_CUSTOM is ${CMAKE_CXX_FLAGS_CUSTOM}")
  message("CMAKE_C_FLAGS_DEBUG is ${CMAKE_C_FLAGS_DEBUG}")
  message("CMAKE_C_FLAGS_RELEASE is ${CMAKE_C_FLAGS_RELEASE}")
  message("CMAKE_C_FLAGS_RELWITHDEBINFO is ${CMAKE_C_FLAGS_RELWITHDEBINFO}")
  message("CMAKE_C_FLAGS_MINSIZEREL is ${CMAKE_C_FLAGS_MINSIZEREL}")
  message("CMAKE_C_FLAGS_CUSTOM is ${CMAKE_C_FLAGS_CUSTOM}")
  message("CMAKE_MSVC_RUNTIME_LIBRARY is ${CMAKE_MSVC_RUNTIME_LIBRARY}")
  message("CMAKE_C_STANDARD_LIBRARIES is ${CMAKE_C_STANDARD_LIBRARIES}")
  message("CMAKE_CXX_STANDARD_LIBRARIES is ${CMAKE_CXX_STANDARD_LIBRARIES}")
endif()

#
# Common compiler options.
#
add_compile_options(-Wall -Wextra -Wpedantic)
add_compile_options(-fno-exceptions -fno-rtti -nostdlib++)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/custom)
include_directories(../library)
include_directories(../svf_runtime)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
  # Prevent additional flags being added automatically.
  set(CMAKE_MSVC_RUNTIME_LIBRARY "")
  set(CMAKE_C_STANDARD_LIBRARIES "")
  set(CMAKE_CXX_STANDARD_LIBRARIES "")

  add_compile_options(
    -D_DLL
    -D_MT
    -Xclang -gcodeview
  )

  add_link_options(
    -Xclang -gcodeview
  )

  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

#
# `platform`
#
add_library(platform)
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
  target_sources(platform
    PRIVATE
    ../library/src/platform/windows/implementation.cpp
  )
  target_link_libraries(platform PRIVATE user32)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_sources(platform
    PRIVATE
    ../library/src/platform/unix/implementation.cpp
  )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  target_sources(platform
    PRIVATE
    ../library/src/platform/unix/implementation.cpp
  )
endif()

#
# `svfc`
#
add_executable(svfc
  src/exe/svfc.cpp
  src/core/parsing.cpp
  src/core/generation.cpp
  src/core/validation.cpp
  src/core/output_c.cpp
  src/core/output_cpp.cpp
  src/core/common.cpp
)
target_link_libraries(svfc PRIVATE platform)

#
# `svf_runtime`, compiled as C.
#
add_library(svf_runtime
  ../svf_runtime/src/svf_internal.c
  ../svf_runtime/src/svf_runtime.c
  ../svf_runtime/src/svf_compatibility.c
  ../svf_runtime/src/svf_conversion.c
)

#
# Everything below will only be built when compiling for the same platform as
# the host, as it involves running freshly-built executables.
#
if (CMAKE_CROSSCOMPILING)
  return()
endif()

#
# `amalgamate_runtime`
#
add_executable(amalgamate_runtime
  src/exe/amalgamate_runtime.cpp
)

#
# Generated files.
#

set (GENERATED_H_DIR ${CMAKE_CURRENT_BINARY_DIR}/custom/generated/h)
set (GENERATED_HPP_DIR ${CMAKE_CURRENT_BINARY_DIR}/custom/generated/hpp)
file(MAKE_DIRECTORY ${GENERATED_H_DIR})
file(MAKE_DIRECTORY ${GENERATED_HPP_DIR})

function(generate_schema_files SCHEMA_NAME)
  set (TXT_NAME ${CMAKE_CURRENT_SOURCE_DIR}/schema/${SCHEMA_NAME}.txt)
  set (H_NAME ${GENERATED_H_DIR}/${SCHEMA_NAME}.h)
  set (HPP_NAME ${GENERATED_HPP_DIR}/${SCHEMA_NAME}.hpp)
  add_custom_command(
    OUTPUT ${HPP_NAME}
    COMMAND svfc cpp ${TXT_NAME} ${HPP_NAME}
    DEPENDS svfc ${TXT_NAME} ${GENERATED_HPP_DIR}
  )
  add_custom_command(
    OUTPUT ${H_NAME}
    COMMAND svfc c ${TXT_NAME} ${H_NAME}
    DEPENDS svfc ${TXT_NAME} ${GENERATED_H_DIR}
  )
  add_custom_target(schema_${SCHEMA_NAME}_hpp ALL DEPENDS ${HPP_NAME})
  add_custom_target(schema_${SCHEMA_NAME}_h ALL DEPENDS ${H_NAME})
endfunction(generate_schema_files)

set (SINGLE_FILE_H_NAME ${CMAKE_CURRENT_BINARY_DIR}/custom/generated/svf.h)
add_custom_command(
  OUTPUT ${SINGLE_FILE_H_NAME}
  COMMAND amalgamate_runtime ${SINGLE_FILE_H_NAME}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../svf_runtime/src/
  DEPENDS
    amalgamate_runtime
    ${TXT_NAME}
    ../svf_runtime/src/svf_runtime.h
    ../svf_runtime/src/svf_runtime.hpp
    ../svf_runtime/src/svf_meta.h
    ../svf_runtime/src/svf_internal.h
    ../svf_runtime/src/svf_stdio.h
    ../svf_runtime/src/svf_compatibility.c
    ../svf_runtime/src/svf_conversion.c
    ../svf_runtime/src/svf_internal.c
    ../svf_runtime/src/svf_runtime.c
)
add_custom_target(single_file_h ALL DEPENDS ${SINGLE_FILE_H_NAME})

generate_schema_files(A0)
generate_schema_files(A1)
generate_schema_files(B0)
generate_schema_files(B1)
generate_schema_files(META)
generate_schema_files(JSON)
generate_schema_files(Hello)

#
# `test_simple_a`
#
add_executable(test_simple_a
  src/test/simple/a.cpp
  src/test/simple/simple_write_read.cpp
)
target_link_libraries(test_simple_a PRIVATE svf_runtime platform)
add_dependencies(test_simple_a schema_A0_hpp)
add_dependencies(test_simple_a schema_A1_hpp)

#
# `test_simple_b`
#
add_executable(test_simple_b
  src/test/simple/b.cpp
  src/test/simple/simple_write_read.cpp
)
target_link_libraries(test_simple_b PRIVATE svf_runtime platform)
add_dependencies(test_simple_b schema_B0_hpp)
add_dependencies(test_simple_b schema_B1_hpp)

#
# `test_simple_c`
#
add_executable(test_simple_c
  src/test/simple/c.c
)
target_compile_options(test_simple_c PRIVATE)
add_dependencies(test_simple_c single_file_h)
add_dependencies(test_simple_c schema_A0_h)
add_dependencies(test_simple_c schema_A1_h)

#
# `example_json`
#
add_executable(example_json
  src/example/json_glue.cpp
  src/example/json_write.cpp
  src/example/json_read.c
)
target_link_libraries(example_json PRIVATE svf_runtime platform)
add_dependencies(example_json schema_JSON_hpp)
add_dependencies(example_json schema_JSON_h)

#
# `example_hello`
#
add_executable(example_hello
  src/example/hello_glue.cpp
  src/example/hello_write.c
  src/example/hello_read.cpp
)
target_link_libraries(example_hello PRIVATE svf_runtime platform)
add_dependencies(example_hello schema_Hello_hpp)
add_dependencies(example_hello schema_Hello_h)

#
# CTest tests.
#
enable_testing()

# Check consistency of generated file for the metaschema.
add_test(NAME meta_hpp_consistency COMMAND
  ${CMAKE_COMMAND} -E compare_files
  ${CMAKE_CURRENT_BINARY_DIR}/custom/generated/hpp/META.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../svf_runtime/src/svf_meta.hpp
)
add_test(NAME meta_h_consistency COMMAND
  ${CMAKE_COMMAND} -E compare_files
  ${CMAKE_CURRENT_BINARY_DIR}/custom/generated/h/META.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../svf_runtime/src/svf_meta.h
)

# Simple tests.
add_test(NAME test_simple_a COMMAND test_simple_a)
add_test(NAME test_simple_b COMMAND test_simple_b)
add_test(NAME test_simple_c COMMAND test_simple_c)

# Examples.
add_test(NAME example_json COMMAND example_json)
add_test(NAME example_hello COMMAND example_hello)
