#Threading lib

if(EMSCRIPTEN)
    message(STATUS "Emscripten detected, enabling Freetype port.")
    add_compile_definitions(USE_FREETYPE_PORT) # Optional, for your app logic
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_FREETYPE=1")
else()
    include(FindPkgConfig)
    find_package(Freetype REQUIRED)
    include_directories(${FREETYPE_INCLUDE_DIRS})
endif()

set(Plotter_src_files
	PNGPlotter.cpp
)
add_library(Plotter ${Plotter_src_files})

#Link libraries
if(EMSCRIPTEN)
else()
    target_link_libraries(Plotter ${FREETYPE_LIBRARIES})
endif()

install(TARGETS Plotter EXPORT shmeaConfig
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})

install(EXPORT shmeaConfig DESTINATION share/shmea/cmake)

export(TARGETS Plotter FILE shmeaConfig.cmake)
