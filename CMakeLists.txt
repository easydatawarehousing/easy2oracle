# Make sure we can find dependancies
cmake_minimum_required (VERSION 2.6)

# Projectname
project(ETO C)

# Set build type
set(CMAKE_BUILD_TYPE Release)

# Add our own find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# Set target dir
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Targets
add_subdirectory(EasyMTOra)
add_subdirectory(EasyPTOra)
add_subdirectory(EasySTOra)
add_subdirectory(EasyTTOra)
