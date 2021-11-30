if(NOT (${CMAKE_BINARY_DIR} STREQUAL "${CMAKE_SOURCE_DIR}/build"))
	message(FATAL_ERROR "Must be in \"build\" directory")
endif()

#Init
cmake_minimum_required(VERSION 3.5.1)
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

#Compiler Flags
set(CMAKE_BUILD_TYPE Debug) # -g
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-unused-variable -Wno-unused-parameter")
#set(CMAKE_CXX_FLAGS_RELEASE "-O3")

#Project
project(shmea-unit-tests)
set(G_VERSION_MAJOR 0)
set(G_VERSION_MINOR 56)

set(DEFAULT_BUILD_TYPE "Release")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

include(GNUInstallDirs)

#Import libs
find_package(shmea REQUIRED)

#Subdirectories
add_subdirectory("Backend/Database")
# add_subdirectory("Backend/Networking")

#Executable
set(MAIN_src_files
	main.cpp
	main.h
	unit-test.cpp
	unit-test.h
)
add_executable(${PROJECT_NAME} ${MAIN_src_files})

#Link libraries
target_link_libraries(${PROJECT_NAME}
	DBTests shmea)#custom libs

target_include_directories(${PROJECT_NAME} PRIVATE "Backend")

#make run
add_custom_target(run
	COMMAND ${PROJECT_NAME}
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
	COMMAND valgrind --leak-check=full --track-origins=yes --show-reachable=yes ./build/${PROJECT_NAME}
	DEPENDS ${PROJECT_NAME}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

#make profile
	add_custom_target(profile
	COMMAND valgrind --tool=callgrind ./build/${PROJECT_NAME}
	DEPENDS ${PROJECT_NAME}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
