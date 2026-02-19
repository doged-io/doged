# Copyright (c) 2026 Tobias Ruck and Alexandre Guillioud

cmake_minimum_required(VERSION 3.16)
project(doged-miner LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- OpenCL (optional) ---
find_package(OpenCL QUIET)

set(MINER_SOURCES
    main.cpp
    stratum_client.cpp
    mining_engine.cpp
)

add_executable(doged-miner ${MINER_SOURCES})

target_include_directories(doged-miner PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(OpenCL_FOUND)
    message(STATUS "OpenCL found — GPU mining enabled")
    target_compile_definitions(doged-miner PRIVATE HAVE_OPENCL)
    target_link_libraries(doged-miner PRIVATE OpenCL::OpenCL)
else()
    message(STATUS "OpenCL NOT found — CPU-only mining")
endif()

target_link_libraries(doged-miner PRIVATE pthread)

# Copy kernel file next to binary for convenience
if(OpenCL_FOUND)
    add_custom_command(TARGET doged-miner POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/scrypt.cl
            $<TARGET_FILE_DIR:doged-miner>/scrypt.cl
    )
endif()

install(TARGETS doged-miner DESTINATION bin)
