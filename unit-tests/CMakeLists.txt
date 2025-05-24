if(NOT (${CMAKE_BINARY_DIR} STREQUAL "${CMAKE_SOURCE_DIR}/build"))
    message(FATAL_ERROR "Must be in \"build\" directory")
endif()

if(WIN32)
    set(CMAKE_INSTALL_PREFIX "$ENV{USERPROFILE}/.local" CACHE PATH "default install path" FORCE)
else()
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local" CACHE PATH "default install path" FORCE)
endif()

cmake_minimum_required(VERSION 3.5.1)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/include)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /EHsc")
elseif(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2")
endif()

project(shmea-unit-tests)
set(G_VERSION_MAJOR 0)
set(G_VERSION_MINOR 57)

set(DEFAULT_BUILD_TYPE "Release")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

include(GNUInstallDirs)

find_package(shmea REQUIRED)

add_subdirectory("Backend/Database")
add_subdirectory("Backend/Plotter")
add_subdirectory("Backend/Networking")

set(MAIN_src_files
    main.cpp
    main.h
    unit-test.cpp
    unit-test.h
)

add_executable(${PROJECT_NAME} ${MAIN_src_files})

if(WIN32)
    target_link_libraries(${PROJECT_NAME} ws2_32)
endif()

target_link_libraries(${PROJECT_NAME}
    PlotterTests DBTests GNetTests shmea
)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}/Backend"
    "${CMAKE_SOURCE_DIR}/Backend/Database"
    "${CMAKE_SOURCE_DIR}/Backend/Plotter"
    "${CMAKE_SOURCE_DIR}/Backend/Networking"
)

add_custom_target(run
    COMMAND ${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(debug
    COMMAND gdb ./build/${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

if(UNIX AND NOT APPLE)
    add_custom_target(mem
        COMMAND valgrind --leak-check=full --track-origins=yes --show-reachable=yes ./build/${PROJECT_NAME}
        DEPENDS ${PROJECT_NAME}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    add_custom_target(profile
        COMMAND valgrind --tool=callgrind ./build/${PROJECT_NAME}
        DEPENDS ${PROJECT_NAME}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()
