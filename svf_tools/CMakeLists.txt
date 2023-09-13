cmake_minimum_required(VERSION 3.20)
project(svf_tools)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 99)

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Windows" AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  # Temporary assumption.
  message(FATAL_ERROR "Target OS must be Windows or Linux.")
endif()

if (NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64" AND NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  # Temporary assumption.
  message(FATAL_ERROR "Target architecture must be x64.")
endif()

if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # Only support Clang for now.
  message(FATAL_ERROR "Compiler must be Clang.")
endif()

#
# Force default compiler options.
#
# set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE Release)
set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON")

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

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
  # Note: UBSan stopped working on Windows after some changes in this file, why?
  # lld-link: error /failifmismatch: mismatch detected for '_ITERATOR_DEBUG_LEVEL'

  set(OUR_FLAGS
    -fsanitize=undefined
    -fsanitize=implicit-conversion
    -fsanitize=local-bounds
    # -fno-sanitize=alignment
    -fsanitize-trap=undefined
    -fno-sanitize-recover=undefined
  )
  list(JOIN OUR_FLAGS " " OUR_FLAGS)

  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${OUR_FLAGS}")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${OUR_FLAGS}")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(OUR_FLAGS
    -Xclang
    --dependent-lib=msvcrt
  )
  list(JOIN OUR_FLAGS " " OUR_FLAGS)

  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${OUR_FLAGS}")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${OUR_FLAGS}")

  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Xclang --dependent-lib=msvcrt")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Xclang --dependent-lib=msvcrt")
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
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_sources(platform
    PRIVATE
    ../library/src/platform/linux/implementation.cpp
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
    DEPENDS svfc ${TXT_NAME} ${HPP_DIR}
  )
  add_custom_command(
    OUTPUT ${H_NAME}
    COMMAND svfc c ${TXT_NAME} ${H_NAME}
    DEPENDS svfc ${TXT_NAME} ${H_DIR}
  )
  if (${SCHEMA_NAME} STREQUAL meta)
    add_custom_target(${SCHEMA_NAME}_hpp ALL DEPENDS ${HPP_NAME})
    add_custom_target(${SCHEMA_NAME}_h ALL DEPENDS ${H_NAME})
  else()
    add_custom_target(${SCHEMA_NAME}_hpp DEPENDS ${HPP_NAME})
    add_custom_target(${SCHEMA_NAME}_h DEPENDS ${H_NAME})
  endif()
endfunction(generate_schema_files)

generate_schema_files(a0)
generate_schema_files(a1)
generate_schema_files(b0)
generate_schema_files(b1)
generate_schema_files(meta)

#
# `test_a`
#
add_executable(test_a
  src/test/a.cpp
  src/test/simple_write_read.cpp
)
target_link_libraries(test_a PRIVATE svf_runtime platform)
add_dependencies(test_a a0_hpp)
add_dependencies(test_a a1_hpp)

#
# `test_b`
#
add_executable(test_b
  src/test/b.cpp
  src/test/simple_write_read.cpp
)
target_link_libraries(test_b PRIVATE svf_runtime platform)
add_dependencies(test_b b0_hpp)
add_dependencies(test_b b1_hpp)

#
# `test_c`
#
add_executable(test_c
  src/test/c.c
)
target_link_libraries(test_c PRIVATE svf_runtime)
add_dependencies(test_b b0_hpp)
add_dependencies(test_c a0_h)
add_dependencies(test_c a1_h)

#
# Tests.
#
enable_testing()
add_test(NAME meta_hpp_consistency COMMAND
  ${CMAKE_COMMAND} -E compare_files
  ${CMAKE_CURRENT_BINARY_DIR}/custom/generated/hpp/meta.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../svf_runtime/src/svf_meta.hpp
)
add_test(NAME meta_h_consistency COMMAND
  ${CMAKE_COMMAND} -E compare_files
  ${CMAKE_CURRENT_BINARY_DIR}/custom/generated/h/meta.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../svf_runtime/src/svf_meta.h
)
add_test(NAME test_a COMMAND test_a)
add_test(NAME test_b COMMAND test_b)
add_test(NAME test_c COMMAND test_c)
