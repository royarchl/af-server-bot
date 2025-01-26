cmake_minimum_required(VERSION 3.10)
project(SteamServerQuery VERSION 1.0)

# Compilation options
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Enable CMake to export the compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Specify C++ standard to use
set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the Steamworks SDK include directory
include_directories(external/steamworks_sdk/public)

# Automatically grab all .cc files from the src/ directory
file(GLOB_RECURSE SOURCES "src/*.cc")

# Define target executable
add_executable(SteamServerQuery ${SOURCES})

# Add directory containing libsteam_api.so to linker search path
# link_directories(${CMAKE_SOURCE_DIR}/external/steamworks_sdk/redistributable_bin/linux64/)

# Link to the Steamworks SDK library
target_link_libraries(SteamServerQuery
  ${CMAKE_SOURCE_DIR}/external/steamworks_sdk/redistributable_bin/linux64/libsteam_api.so
  # steam_api
  pthread
  dl
)
