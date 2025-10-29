# ShmeaDB / top-level CMakeLists.txt (static library package)

cmake_minimum_required(VERSION 3.21)
project(shmea VERSION 0.57 LANGUAGES CXX)

# ---------- Global policy: all static, MSVC static runtime ----------
set(BUILD_SHARED_LIBS OFF)
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)

# ---------- Default install prefix (~/.local or %USERPROFILE%/.local) ----------
if(WIN32)
  set(_default_prefix "$ENV{USERPROFILE}/.local")
else()
  set(_default_prefix "$ENV{HOME}/.local")
endif()
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${_default_prefix}" CACHE PATH "Install prefix" FORCE)
endif()

# ---------- Tooling / warnings ----------
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()
if(MSVC)
  add_compile_options(/W3 /EHsc)
else()
  add_compile_options(-Wall -Wextra -Wno-unused-variable -Wno-unused-parameter)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
endif()

include(GNUInstallDirs)

# ---------- Subdirectories: define STATIC libs & install into ONE export set ----------
# Each subdir must: add_library(<name> STATIC ...); install(TARGETS <name> EXPORT shmeaTargets ...)
add_subdirectory("Backend/Core")
add_subdirectory("Backend/Database")
add_subdirectory("Backend/Plotter")
add_subdirectory("Backend/Networking")

# ---------- Main library (static) ----------
set(SHMEA_MAIN_SRC
  main.cpp
  main.h
  g_export.h
)
add_library(shmea STATIC ${SHMEA_MAIN_SRC})
target_include_directories(shmea
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# Link to your internal libs
target_link_libraries(shmea
  PRIVATE
    Plotter GNet DB
)

# Windows system libs if needed
if(WIN32)
  target_link_libraries(shmea PRIVATE ws2_32)
  add_definitions(-D_WIN32_WINNT=0x0601)
endif()

# ---------- Install headers ----------
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include")
  install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
install(DIRECTORY Backend/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Backend FILES_MATCHING PATTERN "*.h")

# ---------- Install main lib into same export set ----------
install(TARGETS shmea
  EXPORT   shmeaTargets
  RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# ---------- Package config (single export set emitted only here) ----------
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

# Optional: export from build-tree
export(EXPORT shmeaTargets
  NAMESPACE shmea::
  FILE "${CMAKE_CURRENT_BINARY_DIR}/shmeaTargets.cmake"
)

# ---- Dev helpers ----
add_custom_target(fmt
    COMMAND ${CMAKE_COMMAND} -E echo "Run ./clang-format-all"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(local
    COMMAND ${PROJECT_NAME} local
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

if(UNIX)
    add_custom_target(debug
        COMMAND gdb $<TARGET_FILE:${PROJECT_NAME}>
        DEPENDS ${PROJECT_NAME}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    add_custom_target(mem
        COMMAND valgrind --leak-check=full --track-origins=yes $<TARGET_FILE:${PROJECT_NAME}>
        DEPENDS ${PROJECT_NAME}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    add_custom_target(profile
        COMMAND valgrind --tool=callgrind $<TARGET_FILE:${PROJECT_NAME}>
        DEPENDS ${PROJECT_NAME}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()

add_custom_target(uninstall
    COMMAND xargs rm < ${CMAKE_BINARY_DIR}/install_manifest.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
