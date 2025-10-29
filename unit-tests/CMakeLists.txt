cmake_minimum_required(VERSION 3.16)  # safer on Windows; 3.5.1 is too old for many niceties
project(shmea-unit-tests LANGUAGES CXX)

# Force out-of-source build in a 'build' dir
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR OR NOT (CMAKE_BINARY_DIR STREQUAL "${CMAKE_SOURCE_DIR}/build"))
  message(FATAL_ERROR "Must configure in a separate 'build' directory (i.e., run from unit-tests/build).")
endif()

# C++ standard for tests
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default build type for single-config generators
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

# Where the exe lands
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Warnings
if(MSVC)
  add_compile_options(/W3 /EHsc)
else()
  add_compile_options(-Wall -Wextra -Wno-unused-variable -Wno-unused-parameter)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
endif()

# Help CMake find your installed 'shmea' package under ~/.local or %USERPROFILE%/.local
if(DEFINED ENV{USERPROFILE})
  file(TO_CMAKE_PATH "$ENV{USERPROFILE}/.local" _user_local)
  list(APPEND CMAKE_PREFIX_PATH "${_user_local}")
endif()
if(DEFINED ENV{HOME})
  file(TO_CMAKE_PATH "$ENV{HOME}/.local" _home_local)
  list(APPEND CMAKE_PREFIX_PATH "${_home_local}")
endif()

# Load the installed package (exported by your main ShmeaDB install)
# Expect imported targets: shmea::shmea, shmea::DB, shmea::GNet, shmea::Plotter, shmea::GCore
find_package(shmea CONFIG REQUIRED)

# Subdirs with the three small test libs
add_subdirectory(Backend)

# Test runner
set(MAIN_src_files
  main.cpp
  main.h
  unit-test.cpp
  unit-test.h
)
add_executable(${PROJECT_NAME} ${MAIN_src_files})

# Make sure it's a console app (show output)
set_target_properties(shmea-unit-tests PROPERTIES WIN32_EXECUTABLE OFF)

# Copy ALL dependent DLLs beside the exe (CMake >= 3.21)
if(WIN32)
  add_custom_command(TARGET shmea-unit-tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:shmea-unit-tests>
            $<TARGET_FILE_DIR:shmea-unit-tests>
    COMMAND_EXPAND_LISTS
  )
endif()


# Ensure console subsystem on Windows (so we actually see stdout/stderr)
set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE OFF)

# Test code includes its own headers under unit-tests/Backend/...
target_include_directories(${PROJECT_NAME} PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/Backend"
)

# Link the three helper libs + the installed shmea library
target_link_libraries(${PROJECT_NAME} PRIVATE
  DBTests
  GNetTests
  PlotterTests
  shmea::shmea
)

# Convenience targets
add_custom_target(run
  COMMAND $<TARGET_FILE:${PROJECT_NAME}>
  DEPENDS ${PROJECT_NAME}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(debug
  COMMAND gdb ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}
  DEPENDS ${PROJECT_NAME}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

if(UNIX AND NOT APPLE)
  add_custom_target(mem
    COMMAND valgrind --leak-check=full --track-origins=yes --show-reachable=yes ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
  add_custom_target(profile
    COMMAND valgrind --tool=callgrind ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
endif()

# Optional: ctest integration
include(CTest)
if(BUILD_TESTING)
  add_test(NAME shmea_unit_tests COMMAND $<TARGET_FILE:${PROJECT_NAME}>)
endif()
