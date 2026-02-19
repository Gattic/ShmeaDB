#include "ChartLayout.h"
#include <vector>
#include <algorithm>

namespace shmea {

ChartLayout::ChartLayout(unsigned int width, unsigned int height,
                         unsigned int margin_top, unsigned int margin_right,
                         unsigned int margin_bottom, unsigned int margin_left,
                         unsigned int ssaa_factor)
    : width(width),
      height(height),
      marginTop(margin_top),
      marginRight(margin_right),
      marginBottom(margin_bottom),
      marginLeft(margin_left),
      cornerRadius(12),
      showGrid(true),
      showAxes(true),
      dateLabel(false),
      legendLabel(true),
      showOriginAxes(false),
      ssaaFactor(ssaa_factor),
      logoWidth(0),
      logoHeight(0)
{
    // Initialize the supersampled image with the correct dimensions
    ssaaImage.Allocate(width * ssaaFactor, height * ssaaFactor);
    ssaaImage.SetAllPixels(RGBA(0, 0, 0, 0));
}

ChartLayout::~ChartLayout() {
    // Nothing to clean up
}

int ChartLayout::estimateTextWidth(const std::string& text, unsigned int fontSize) const {
    // Use a more accurate character width estimation
    // Different characters have different widths, so we'll use an average factor
    return static_cast<int>(text.length() * fontSize * 0.75) + 10; // Add padding for safety
}

int ChartLayout::calculateInfoBoxHeight(const std::vector<std::string>& labels, unsigned int fontSize) const {
    // For horizontal layout, height is primarily determined by a single row's height
    // plus padding, regardless of number of items - exactly matching plotter.cpp implementation
    int itemHeight = fontSize + 6;

    // Calculate total height including top and bottom padding
    return 24 + itemHeight;  // 12px padding top and bottom + single row height
}

void ChartLayout::setSsaaFactor(unsigned int factor) {
    if (factor < 1) factor = 1;
    ssaaFactor = factor;
}

} // namespace shmea
