cmake_minimum_required(VERSION 3.10)
project(Sync-VDC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(include)

# Add WT13106 connection library
add_library(WT13106Connection STATIC
    src/WT13106Connection.cpp
    include/WT13106Connection.h
)

# Example usage executable
add_executable(example_usage
    src/example_usage.cpp
)

target_link_libraries(example_usage WT13106Connection)

# Platform-specific libraries
if(WIN32)
    # Windows serial communication uses standard Windows APIs
    # SetupAPI for USB device enumeration
    target_link_libraries(WT13106Connection setupapi)
else()
    # Linux/macOS: Link libusb for USB support
    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(LIBUSB libusb-1.0)
        if(LIBUSB_FOUND)
            target_link_libraries(WT13106Connection ${LIBUSB_LIBRARIES})
            target_include_directories(WT13106Connection PRIVATE ${LIBUSB_INCLUDE_DIRS})
        endif()
    endif()
endif()

# Installation
install(TARGETS WT13106Connection example_usage
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# Copy DLLs to output directory (Windows)
if(WIN32)
    file(GLOB DLL_FILES "${CMAKE_SOURCE_DIR}/*.dll")
    foreach(DLL_FILE ${DLL_FILES})
        configure_file(${DLL_FILE} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COPYONLY)
    endforeach()
endif()

