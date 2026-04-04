set(GNetTests_src_files
crypt-test.cpp
udp-test.cpp
protocol-fuzz-test.cpp
connection-test.cpp
socket-integration-test.cpp
socket-advanced-test.cpp
binary-socket-test.cpp
)
add_library(GNetTests ${GNetTests_src_files})
target_include_directories(GNetTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
