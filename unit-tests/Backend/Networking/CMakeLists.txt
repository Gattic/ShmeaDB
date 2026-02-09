set(GNetTests_src_files
crypt-test.cpp
udp-test.cpp
protocol-fuzz-test.cpp
connection-test.cpp
socket-integration-test.cpp
socket-advanced-test.cpp
)
add_library(GNetTests ${GNetTests_src_files})
