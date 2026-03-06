# Add FreeType dependency like in the main Backend/Database/CMakeLists.txt
if(EMSCRIPTEN)
    message(STATUS "Emscripten detected, enabling Freetype port.")
    add_compile_definitions(USE_FREETYPE_PORT)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_FREETYPE=1")
else()
    include(FindPkgConfig)
    find_package(Freetype REQUIRED)
    include_directories(${FREETYPE_INCLUDE_DIRS})
endif()

# Set C++03 standard flag
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(DBTests_src_files
GType-test.cpp
GString-test.cpp
GPointer-test.cpp
GList-test.cpp
GList-edge-test.cpp
GTable-test.cpp
GObjects-test.cpp
GVector-test.cpp
image-test.cpp
ServiceData-test.cpp
binary-payload-test.cpp
)
add_library(DBTests ${DBTests_src_files})
target_include_directories(DBTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

# Link against FreeType
if(NOT EMSCRIPTEN)
    target_link_libraries(DBTests ${FREETYPE_LIBRARIES})
endif()
