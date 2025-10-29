add_library(GNetTests
  crypt-test.cpp
)

target_include_directories(GNetTests PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(GNetTests PRIVATE
  shmea::GNet
  shmea::DB
  shmea::GCore
)

# If any test directly calls winsock (rare), you could also:
# if(WIN32)
#   target_link_libraries(GNetTests PRIVATE ws2_32)
# endif()
