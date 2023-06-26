#Threading lib
include(FindPkgConfig)
find_package(Threads REQUIRED)

find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIRS})

set(DB_src_files
	png-helper.cpp
	image.cpp
	GType.cpp
	GType_helpers.cpp
	GType_operators.cpp
	GList.cpp
	GTable.cpp
	GObject.cpp
	GString.cpp
	GString_helpers.cpp
	GPointer.h
	GAnalysis.cpp
	maxid.cpp
	SaveTable.cpp
	SaveFolder.cpp
	Serializable.cpp
	ServiceData.cpp
	standardizable.cpp
)
add_library(DB ${DB_src_files})

#Link libraries
target_link_libraries(DB ${CMAKE_THREAD_LIBS_INIT} ${PNG_LIBRARIES})

install(TARGETS DB EXPORT shmeaConfig
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})

install(EXPORT shmeaConfig DESTINATION share/shmea/cmake)

export(TARGETS DB FILE shmeaConfig.cmake)
