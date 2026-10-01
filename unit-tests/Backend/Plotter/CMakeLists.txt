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
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PlotterTests_src_files
	plotter-test.cpp
)
add_library(PlotterTests ${PlotterTests_src_files})

# Link to main Plotter library which now contains all the refactored components
target_link_libraries(PlotterTests Plotter)

# Link against FreeType
if(NOT EMSCRIPTEN)
    target_link_libraries(PlotterTests ${FREETYPE_LIBRARIES})
endif()
