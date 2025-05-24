if(NOT (CMAKE_BINARY_DIR STREQUAL "${CMAKE_SOURCE_DIR}/build"))
    message(FATAL_ERROR "Must be in \"build\" directory")
endif()

# Set install prefix (handles both Windows and UNIX)
if(WIN32)
    set(CMAKE_INSTALL_PREFIX "$ENV{USERPROFILE}/.local" CACHE PATH "default install path" FORCE)
else()
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local" CACHE PATH "default install path" FORCE)
endif()

cmake_minimum_required(VERSION 3.5.1)
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

project(shmea)

file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/include)

# Compiler Flags
if(MSVC)
    # For Visual Studio, adjust as needed
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /EHsc")
elseif(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2")
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Only enable sanitizers on UNIX
if(NOT WIN32)
    add_link_options(-fsanitize=address)
endif()

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the build type" FORCE)
endif()

include(GNUInstallDirs)

# Subdirectories (core libraries)
add_subdirectory("Backend/Database")
add_subdirectory("Backend/Plotter")
add_subdirectory("Backend/Networking")

set(MAIN_SRC_FILES
    main.cpp
    main.h
    g_export.h
)

# Build SHARED or STATIC library
if(EMSCRIPTEN)
    add_library(${PROJECT_NAME} STATIC ${MAIN_SRC_FILES})
else()
    add_library(${PROJECT_NAME} SHARED ${MAIN_SRC_FILES})
endif()

target_link_libraries(${PROJECT_NAME}
    Plotter GNet DB
)

# --- Add Windows system libs if building on Windows ---
if(WIN32)
    target_link_libraries(${PROJECT_NAME} ws2_32)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_SOURCE_DIR}
)

install(TARGETS ${PROJECT_NAME} EXPORT shmeaConfig
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(EXPORT shmeaConfig DESTINATION share/shmea/cmake)
export(TARGETS ${PROJECT_NAME} FILE shmeaConfig.cmake)

# ---- Dev targets (optional, not all work on Windows) ----

add_custom_target(fmt
    COMMAND ./clang-format-all
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(local
    COMMAND ${PROJECT_NAME} local
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Only add debug/mem/profile on UNIX
if(UNIX)
    add_custom_target(debug
        COMMAND gdb ./build/${PROJECT_NAME}
        DEPENDS ${PROJECT_NAME}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    add_custom_target(mem
        COMMAND valgrind --leak-check=full --track-origins=yes ./build/${PROJECT_NAME}
        DEPENDS ${PROJECT_NAME}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    add_custom_target(profile
        COMMAND valgrind --tool=callgrind ./build/${PROJECT_NAME}
        DEPENDS ${PROJECT_NAME}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()

add_custom_target(uninstall
    COMMAND xargs rm < ./build/install_manifest.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
