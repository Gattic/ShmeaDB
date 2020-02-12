#Threading lib
include(FindPkgConfig)
find_package(Threads REQUIRED)

set(DB_src_files
	image.cpp
	image.h
	oldjson.cpp
	oldjson.h
	gtype.cpp
	gtype.h
	GList.cpp
	GList.h
	gtable.cpp
	gtable.h
	GAnalysis.cpp
	GAnalysis.h
	maxid.cpp
	maxid.h
	saveitem.cpp
	saveitem.h
	savelist.cpp
	savelist.h
	Serializable.cpp
	Serializable.h
)
add_library(DB ${DB_src_files})

#Link libraries
target_link_libraries(DB ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS DB EXPORT shmeaConfig
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})

install(EXPORT shmeaConfig DESTINATION share/shmea/cmake)

export(TARGETS DB FILE shmeaConfig.cmake)
