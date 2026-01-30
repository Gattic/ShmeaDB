cmake_minimum_required(VERSION 3.21)

project(shmea VERSION 0.57 LANGUAGES CXX)

include(GNUInstallDirs)

# --------------------
# Project-wide settings (safe as subproject)
# --------------------
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)

# Default build type for single-config generators (Ninja, Makefiles)
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

# Warnings/flags (do NOT propagate to consumers)
if(MSVC)
  add_compile_options(/W3 /EHsc)
else()
  add_compile_options(-Wall -Wextra -Wno-unused-variable -Wno-unused-parameter)
endif()

# MSVC static runtime per-target (avoid forcing cache/global in parent projects)
set(SHMEA_MSVC_RUNTIME "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Default install prefix ONLY when ShmeaDB is the top-level project
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  if(WIN32)
    set(_default_prefix "$ENV{USERPROFILE}/.local")
  else()
    set(_default_prefix "$ENV{HOME}/.local")
  endif()
  if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${_default_prefix}" CACHE PATH "Install prefix" FORCE)
  endif()
endif()

# --------------------
# Sub-libraries
# --------------------
add_subdirectory(Backend/Core)
add_subdirectory(Backend/Database)
add_subdirectory(Backend/Plotter)
add_subdirectory(Backend/Networking)

# --------------------
# Umbrella library target: shmea
# (This is what Gattic should link to if it wants "everything".)
# --------------------
add_library(shmea STATIC
  # IMPORTANT:
  # Do NOT compile a file that contains `int main()` into a library.
  # If your current main.cpp has `int main()`, move it to Gattic.
  g_export.h
)

# Include dirs: allow consumers to include <Backend/...> and <include/...>
target_include_directories(shmea
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Link all internal libs PUBLIC so deps propagate to final exe
target_link_libraries(shmea
  PUBLIC
    Plotter
    GNet
    DB
    GCore
)

# MSVC static runtime on this target
if(MSVC)
  set_property(TARGET shmea PROPERTY MSVC_RUNTIME_LIBRARY "${SHMEA_MSVC_RUNTIME}")
endif()

# --------------------
# Install headers
# --------------------
if(EXISTS "${PROJECT_SOURCE_DIR}/include")
  install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/"
          DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
endif()

install(DIRECTORY "${PROJECT_SOURCE_DIR}/Backend/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/Backend"
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")

# --------------------
# Install targets (all export into one set)
# --------------------
install(TARGETS shmea
  EXPORT   shmeaTargets
  ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-w)
endif()

# --------------------
# Package config + export
# --------------------
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/shmeaConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY SameMajorVersion
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/shmeaConfig.cmake"
  "include(\"\${CMAKE_CURRENT_LIST_DIR}/shmeaTargets.cmake\")\n"
)

install(EXPORT shmeaTargets
  NAMESPACE shmea::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/shmea
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/shmeaConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/shmeaConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/shmea
)

# Optional: export from build tree (useful for find_package in a build folder)
export(EXPORT shmeaTargets
  NAMESPACE shmea::
  FILE "${CMAKE_CURRENT_BINARY_DIR}/shmeaTargets.cmake"
)

# --------------------
# Dev helpers (only meaningful when top-level)
# --------------------
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  add_custom_target(fmt
    COMMAND ${CMAKE_COMMAND} -E echo "Run ./clang-format-all"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )

  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -E echo "Uninstalling..."
    COMMAND xargs rm < ${CMAKE_BINARY_DIR}/install_manifest.txt
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )
endif()

option(SHMEA_BUILD_TESTS "Build unit tests" ON)

if(SHMEA_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(unit-tests)
endif()
