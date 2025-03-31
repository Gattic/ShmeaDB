if(EMSCRIPTEN)
    message(STATUS "Emscripten detected, enabling Freetype port.")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_FREETYPE=1")
else()
    include(FindPkgConfig)
    find_package(Freetype REQUIRED)
    
    if(FREETYPE_FOUND)
        message(STATUS "FreeType found: ${FREETYPE_INCLUDE_DIRS}")
        include_directories(${FREETYPE_INCLUDE_DIRS})
    else()
        message(FATAL_ERROR "FreeType is required but was not found")
    endif()
endif()

set(Drawers_src_files
    ArrowDrawer.cpp
    BaseDrawer.cpp
    CandlestickDrawer.cpp
    DataDrawer.cpp
    GridDrawer.cpp
    HistogramDrawer.cpp
    LabelsDrawer.cpp
    LegendDrawer.cpp
)
add_library(Drawers ${Drawers_src_files})

#Link libraries
if(EMSCRIPTEN)
    # Emscripten linking handled by compiler flag
else()
    target_link_libraries(Drawers ${FREETYPE_LIBRARIES})
endif()

install(TARGETS Drawers EXPORT shmeaConfig
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})

install(EXPORT shmeaConfig DESTINATION share/shmea/cmake)

export(TARGETS Drawers FILE shmeaConfig.cmake)
