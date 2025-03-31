#Threading lib

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
)
add_library(DB ${DB_src_files})

install(TARGETS DB EXPORT shmeaConfig
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})

install(EXPORT shmeaConfig DESTINATION share/shmea/cmake)

export(TARGETS DB FILE shmeaConfig.cmake)
