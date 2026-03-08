#Threading lib

if(EMSCRIPTEN)
    message(STATUS "Emscripten detected, enabling Freetype port.")
    add_compile_definitions(USE_FREETYPE_PORT) # Optional, for your app logic
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_FREETYPE=1")
else()
    if(NOT WIN32)
        include(FindPkgConfig)
    endif()
    find_package(Freetype REQUIRED)
    include_directories(${FREETYPE_INCLUDE_DIRS})
endif()

# Set C++03 standard flag
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(DB_src_files
	lodepng.cpp
	png-helper.cpp
	image.cpp
	GVector.h
	GPointer.h
	GType.cpp
	GType_helpers.cpp
	GType_operators.cpp
	GString.cpp
	GString_helpers.cpp
	GList.cpp
	GLogger.cpp
	GTable.cpp
	GObject.cpp
	maxid.cpp
	SaveTable.cpp
	SaveFolder.cpp
	Serializable.cpp
	ServiceData.cpp
	standardizable.cpp
	PNGPlotter.cpp
)
add_library(DB ${DB_src_files})

#Link libraries
if(EMSCRIPTEN)
else()
    target_link_libraries(DB Core ${FREETYPE_LIBRARIES})
endif()

install(TARGETS DB EXPORT shmeaConfig
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})

install(EXPORT shmeaConfig DESTINATION share/shmea/cmake)

export(TARGETS DB Core FILE shmeaConfig.cmake)
