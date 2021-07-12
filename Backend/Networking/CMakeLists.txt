#ssl and crypto libs
#find_library(OpenSSL REQUIRED ssl crypto)

#find_package(OpenSSL)
#include_directories(${OPENSSL_INCLUDE_DIRS})

#Threading lib
include(FindPkgConfig)
find_package(Threads REQUIRED)

set(GNet_src_files
	main.cpp
	main.h
	crypt.cpp
	crypt.h
	connection.cpp
	connection.h
	service.cpp
	service.h
	socket.cpp
	socket.h
)
add_library(GNet ${GNet_src_files})

#Link libraries
target_link_libraries(GNet ${OPENSSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS GNet EXPORT shmeaConfig
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})

install(EXPORT shmeaConfig DESTINATION share/shmea/cmake)

export(TARGETS GNet FILE shmeaConfig.cmake)
