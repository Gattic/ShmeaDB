if(NOT (${CMAKE_BINARY_DIR} STREQUAL "${CMAKE_SOURCE_DIR}/build"))
	message(FATAL_ERROR "Must be in \"build\" directory")
endif()

set (CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local" CACHE PATH "default install path" FORCE)
file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/include)

#Init
cmake_minimum_required(VERSION 3.5.1)
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

#Compiler Flags
set(DEFAULT_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-unused-variable -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_link_options(-fsanitize=address)

#Project
project(shmea)
set(G_VERSION_MAJOR 0)
set(G_VERSION_MINOR 57)

set(CMAKE_BUILD_TYPE Debug) # -g

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

add_link_options(-fsanitize=address)

#Import libs
include(GNUInstallDirs)

#Subdirectories
add_subdirectory("Backend/Database")
add_subdirectory("Backend/Networking")

set(MAIN_SRC_FILES
	main.cpp
	main.h
)

add_library(${PROJECT_NAME} SHARED ${MAIN_SRC_FILES})

#Link libraries
target_link_libraries(${PROJECT_NAME}
	GNet DB) #custom libs

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_SOURCE_DIR})

install(TARGETS ${PROJECT_NAME} EXPORT shmeaConfig
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})

install(EXPORT shmeaConfig DESTINATION share/shmea/cmake)

export(TARGETS ${PROJECT_NAME} FILE shmeaConfig.cmake)

#make fmt
add_custom_target(fmt
	COMMAND ./clang-format-all
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

#make local
add_custom_target(local
	COMMAND ${PROJECT_NAME} local
	DEPENDS ${PROJECT_NAME}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

#make debug
add_custom_target(debug
	COMMAND gdb ./build/${PROJECT_NAME}
	DEPENDS ${PROJECT_NAME}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

#make mem
	add_custom_target(mem
	COMMAND valgrind --leak-check=full --track-origins=yes ./build/${PROJECT_NAME}
	DEPENDS ${PROJECT_NAME}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

#make profile
	add_custom_target(profile
	COMMAND valgrind --tool=callgrind ./build/${PROJECT_NAME}
	DEPENDS ${PROJECT_NAME}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

#make uninstall
	add_custom_target(uninstall
	COMMAND xargs rm < ./build/install_manifest.txt
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)