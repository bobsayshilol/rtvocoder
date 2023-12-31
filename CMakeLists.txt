cmake_minimum_required(VERSION 3.26)

project(
  pipewire_vocoder
  VERSION 0.0
  LANGUAGES CXX
)
enable_testing()

# Modern C++
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)
add_compile_options(-Wall -Wextra -pedantic -Werror)

# Always enable debug symbols.
add_compile_options(-g)

# Speedy.
add_compile_options(-march=native)

# Dynamic libs require PIC and shouldn't export anything by default.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# Bring in pipewire
find_package(PkgConfig REQUIRED)
pkg_check_modules(PIPEWIRE REQUIRED IMPORTED_TARGET libpipewire-0.3)

# Bring in the parts
add_subdirectory(cmdline)
add_subdirectory(lib)
add_subdirectory(libs)
add_subdirectory(module)
add_subdirectory(pipewire)
add_subdirectory(tests)
