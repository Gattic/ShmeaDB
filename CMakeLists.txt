cmake_minimum_required(VERSION 3.5.1)
project(shmeadb)

# Version info
set(G_VERSION_MAJOR 0)
set(G_VERSION_MINOR 57)

# Enforce build from /build
if(NOT (CMAKE_BINARY_DIR STREQUAL "${CMAKE_SOURCE_DIR}/build"))
    message(FATAL_ERROR "Must be in \"build\" directory")
endif()

# Install path
if(WIN32)
    set(CMAKE_INSTALL_PREFIX "$ENV{USERPROFILE}/.local" CACHE PATH "default install path" FORCE)
else()
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local" CACHE PATH "default install path" FORCE)
endif()
file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/include)

# Output dir
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Language standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the build type" FORCE)
endif()

# General flags
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-sanitize=all")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fno-sanitize=all")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fno-sanitize=all")

# Platform-specific flags
if(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
elseif(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter")
endif()

# Emscripten (WASM) flags
if(EMSCRIPTEN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE=4 -s PROXY_TO_PTHREAD -s INITIAL_MEMORY=64MB -s EXIT_RUNTIME=1")
endif()

# Dependencies
include(GNUInstallDirs)

# Subdirectories
add_subdirectory("Backend/Database")
add_subdirectory("Backend/Plotter")
add_subdirectory("Backend/Networking")

# Sources
set(MAIN_SRC_FILES
    main.cpp
    main.h
    g_export.h
)

# Build target: use executable, not shared lib
add_executable(${PROJECT_NAME} ${MAIN_SRC_FILES})

# Link with internal components
target_link_libraries(${PROJECT_NAME} PUBLIC
    Plotter GNet DB
)

# Include headers
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_SOURCE_DIR}
)

# Install
install(TARGETS ${PROJECT_NAME} EXPORT shmeaConfig
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(EXPORT shmeaConfig DESTINATION share/shmea/cmake)
export(TARGETS ${PROJECT_NAME} FILE shmeaConfig.cmake)

# Format
add_custom_target(fmt
    COMMAND ./clang-format-all
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# CLI modes
add_custom_target(local
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME} local
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Linux-specific tools
if(UNIX)
    add_custom_target(debug
        COMMAND gdb ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}
        DEPENDS ${PROJECT_NAME}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    add_custom_target(mem
        COMMAND valgrind --leak-check=full --track-origins=yes ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}
        DEPENDS ${PROJECT_NAME}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    add_custom_target(profile
        COMMAND valgrind --tool=callgrind ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}
        DEPENDS ${PROJECT_NAME}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()

# Uninstall
add_custom_target(uninstall
    COMMAND xargs rm < ${CMAKE_BINARY_DIR}/install_manifest.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
