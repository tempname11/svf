cmake_minimum_required(VERSION 3.10)
project(svf)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER clang)

# Assumptions: Windows, clang, x64

add_executable(svfc
  src/exe_svfc.cpp
  src/svf/parsing.cpp
  src/svf/typechecking.cpp
  src/svf/output_binary.cpp
  src/svf/output_cpp.cpp
  src/platform_windows.cpp
)

add_executable(test_a
  test/a.cpp
  src/svf/parsing.cpp
  src/svf/compatibility.cpp
  src/platform_windows.cpp
)

add_executable(embed
  src/exe_embed.cpp
  src/platform_windows.cpp
)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_definitions(svfc PRIVATE COMPILE_TIME_OPTION_BUILD_TYPE_DEVELOPMENT)
endif()

enable_testing()
add_test(NAME test_a COMMAND test_a)