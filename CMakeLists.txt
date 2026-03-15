if(NOT (${CMAKE_BINARY_DIR} STREQUAL "${CMAKE_SOURCE_DIR}/build"))
	message(FATAL_ERROR "Must be in \"build\" directory")
endif()

if(WIN32)
    set(CMAKE_INSTALL_PREFIX "$ENV{USERPROFILE}/shmea" CACHE PATH "default install path" FORCE)
else()
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local" CACHE PATH "default install path" FORCE)
endif()
file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/include)

#Init
cmake_minimum_required(VERSION 3.5.1)
cmake_policy(SET CMP0091 NEW)
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

#Compiler Flags
#set(DEFAULT_BUILD_TYPE "Release")
set(CMAKE_BUILD_TYPE Release) # -O3 -g
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

#Project
project(shmea)

# Compiler flags must come after project() so MSVC variable is available
if(MSVC)
    add_compile_options(/W3 /wd4101 /wd4100)
    add_compile_definitions(NOMINMAX _CRT_SECURE_NO_WARNINGS)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
elseif(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g")
else()
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-unused-variable -Wno-unused-parameter")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g")
endif()

if(EMSCRIPTEN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE=4 -s PROXY_TO_PTHREAD -s INITIAL_MEMORY=64MB -s EXIT_RUNTIME=1")
endif()
set(G_VERSION_MAJOR 0)
set(G_VERSION_MINOR 57)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

#add_link_options(-fsanitize=address)

#Import libs
include(GNUInstallDirs)

#Subdirectories
add_subdirectory("Backend/Core")
add_subdirectory("Backend/Database")
add_subdirectory("Backend/Plotter")
add_subdirectory("Backend/Networking")

set(MAIN_SRC_FILES
	main.cpp
	main.h
	g_export.h
)

if(EMSCRIPTEN)
    add_library(${PROJECT_NAME} STATIC ${MAIN_SRC_FILES})
else()
    add_library(${PROJECT_NAME} SHARED ${MAIN_SRC_FILES})
endif()

#Link libraries
target_link_libraries(${PROJECT_NAME}
	Plotter GNet DB) #custom libs

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_SOURCE_DIR})

install(TARGETS ${PROJECT_NAME} EXPORT shmeaConfig
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})

# Install headers preserving Backend/ directory structure
install(DIRECTORY Backend/Core/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Backend/Core FILES_MATCHING PATTERN "*.h")
install(DIRECTORY Backend/Database/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Backend/Database FILES_MATCHING PATTERN "*.h")
install(DIRECTORY Backend/Networking/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Backend/Networking FILES_MATCHING PATTERN "*.h")
install(DIRECTORY Backend/Plotter/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Backend/Plotter FILES_MATCHING PATTERN "*.h")
install(FILES main.h g_export.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT shmeaConfig DESTINATION share/shmea/cmake)

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

if(NOT WIN32)
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
endif()

if(NOT WIN32)
    #make uninstall
    add_custom_target(uninstall
        COMMAND xargs rm < ./build/install_manifest.txt
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()
