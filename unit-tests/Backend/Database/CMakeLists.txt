# DB-related unit-test objects
add_library(DBTests
  GType-test.cpp
  GString-test.cpp
  GPointer-test.cpp
  GList-test.cpp
  GTable-test.cpp
  GObjects-test.cpp
  GVector-test.cpp
  image-test.cpp
)

# Tests include their own headers under unit-tests/Backend/...
target_include_directories(DBTests PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

# Link to installed libraries
target_link_libraries(DBTests PRIVATE
  shmea::DB
  shmea::GCore
)

# Quiet Windows macro noise (optional)
if(WIN32)
  target_compile_definitions(DBTests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()
