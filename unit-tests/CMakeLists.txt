cmake_minimum_required(VERSION 3.21)

# Collect all test translation units under unit-tests/backend/**
file(GLOB_RECURSE SHMEA_TEST_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_LIST_DIR}/Backend/*-test.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/Backend/*test.cpp"
)

# Your unit test runner sources (these exist at unit-tests root per screenshot)
set(SHMEA_TEST_RUNNER
  "${CMAKE_CURRENT_LIST_DIR}/main.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/unit-test.cpp"
)

add_executable(shmea_unit_tests
  ${SHMEA_TEST_RUNNER}
  ${SHMEA_TEST_SOURCES}
)

target_compile_features(shmea_unit_tests PRIVATE cxx_std_11)

# Make includes work regardless of test folder depth
target_include_directories(shmea_unit_tests PRIVATE
  ${CMAKE_SOURCE_DIR}                 # ShmeaDB/
  ${CMAKE_SOURCE_DIR}/Backend         # ShmeaDB/Backend
  ${CMAKE_CURRENT_LIST_DIR}           # ShmeaDB/unit-tests
  ${CMAKE_CURRENT_LIST_DIR}/Backend   # ShmeaDB/unit-tests/Backend
)


list(LENGTH SHMEA_TEST_SOURCES _n)
message(STATUS "FOUND ${_n} test sources")

# Link whatever your tests need (safe to link all)
target_link_libraries(shmea_unit_tests PRIVATE
  DB
  GNet
  Plotter
)

# Register a CTest entry
add_test(NAME shmea_unit_tests COMMAND shmea_unit_tests)

target_compile_definitions(shmea_unit_tests PRIVATE
  SHMEA_TEST_DATA_DIR="${CMAKE_CURRENT_LIST_DIR}"
)

# Run tests from unit-tests so relative paths like datasets/, fonts/, test-output/ work
set_tests_properties(shmea_unit_tests PROPERTIES
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
