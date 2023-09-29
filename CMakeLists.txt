# -----------------------------------------------------------------------------
# @brief  : Root cmake file.
# @author : Enrico Fraccaroli
# @create : 23/03/2021
# @update : 23/03/2021
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Set the minimum CMake version, the project name and default build type.
# -----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.1...3.18)

# Set the project name.
project(robsec)

# Set the default build type to Debug.
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
endif()

# -----------------------------------------------------------------------------
# OPTIONS
# -----------------------------------------------------------------------------

# option(ROBSEC_BUILD_EXAMPLES "Build examples" OFF)
# option(ROBSEC_BUILD_TESTS "Build tests" OFF)
option(ROBSEC_STRICT_WARNINGS "Enable strict compiler warnings" ON)
option(ROBSEC_WARNINGS_AS_ERRORS "Treat all warnings as errors" OFF)

# -----------------------------------------------------------------------------
# DEPENDENCIES
# -----------------------------------------------------------------------------

# Find curses for traces compression.
find_package(Curses REQUIRED)

# -----------------------------------------------------------------------------
# COMPILATION FLAGS
# -----------------------------------------------------------------------------

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # Disable warnings that suggest using MSVC-specific safe functions
    set(ROBSEC_COMPILE_OPTIONS ${ROBSEC_COMPILE_OPTIONS} -D_CRT_SECURE_NO_WARNINGS)

    if(ROBSEC_WARNINGS_AS_ERRORS)
        set(ROBSEC_COMPILE_OPTIONS ${ROBSEC_COMPILE_OPTIONS} /WX)
    endif(ROBSEC_WARNINGS_AS_ERRORS)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(ROBSEC_WARNINGS_AS_ERRORS)
        set(ROBSEC_COMPILE_OPTIONS ${ROBSEC_COMPILE_OPTIONS} -Werror)
    endif(ROBSEC_WARNINGS_AS_ERRORS)
endif()

if(ROBSEC_STRICT_WARNINGS)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        # Mark system headers as external for MSVC explicitly
        # https://devblogs.microsoft.com/cppblog/broken-warnings-theory
        set(ROBSEC_COMPILE_OPTIONS ${ROBSEC_COMPILE_OPTIONS} /experimental:external)
        set(ROBSEC_COMPILE_OPTIONS ${ROBSEC_COMPILE_OPTIONS} /external:I ${CMAKE_BINARY_DIR})
        set(ROBSEC_COMPILE_OPTIONS ${ROBSEC_COMPILE_OPTIONS} /external:anglebrackets)
        set(ROBSEC_COMPILE_OPTIONS ${ROBSEC_COMPILE_OPTIONS} /external:W0)
        set(ROBSEC_COMPILE_OPTIONS ${ROBSEC_COMPILE_OPTIONS} /W4)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(ROBSEC_COMPILE_OPTIONS ${ROBSEC_COMPILE_OPTIONS} -Wall -Wextra -Wconversion -pedantic)
    endif()
endif(ROBSEC_STRICT_WARNINGS)

# -----------------------------------------------------------------------------
# DIPENDENCIES
# -----------------------------------------------------------------------------

# We need this in order to import external projects.
# include(FetchContent)

# -----------------------------------------------------------------------------
# EXECUTABLE
# -----------------------------------------------------------------------------
# Add the C++ library.
add_executable(${PROJECT_NAME} src/main.cpp src/robsec/game.cpp)
# Inlcude header directories.
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include ${CURSES_INCLUDE_DIR})
# Set compilation flags.
target_compile_options(${PROJECT_NAME} PUBLIC ${ROBSEC_COMPILE_OPTIONS})
# Set compiler flags.
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
# Link curses.
target_link_libraries(${PROJECT_NAME} PUBLIC ${CURSES_LIBRARIES})