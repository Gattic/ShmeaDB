# Plotter headers include <ft2build.h>, but your Plotter target
# does not propagate FreeType's include dirs; add them here.
find_package(Freetype REQUIRED)

add_library(PlotterTests
  plotter-test.cpp
)

target_include_directories(PlotterTests PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
  ${FREETYPE_INCLUDE_DIRS}
)

target_link_libraries(PlotterTests PRIVATE
  shmea::Plotter
  shmea::DB
  ${FREETYPE_LIBRARIES}
)
