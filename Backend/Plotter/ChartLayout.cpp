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
      showOriginAxes(false),
      ssaaFactor(ssaa_factor),
      logoWidth(0),
      logoHeight(0)
{
    // Initialize the supersampled image with the correct dimensions
    ssaaImage.Allocate(width * ssaaFactor, height * ssaaFactor);
    
    // Fill with transparent black to start
    for (unsigned int y = 0; y < height * ssaaFactor; ++y) {
        for (unsigned int x = 0; x < width * ssaaFactor; ++x) {
            ssaaImage.SetPixel(x, y, RGBA(0, 0, 0, 0));
        }
    }
}

ChartLayout::~ChartLayout() {
    // Nothing to clean up
}

unsigned int ChartLayout::getWidth() const {
    return width;
}

unsigned int ChartLayout::getHeight() const {
    return height;
}

unsigned int ChartLayout::getMarginTop() const {
    return marginTop;
}

unsigned int ChartLayout::getMarginRight() const {
    return marginRight;
}

unsigned int ChartLayout::getMarginBottom() const {
    return marginBottom;
}

unsigned int ChartLayout::getMarginLeft() const {
    return marginLeft;
}

void ChartLayout::setMarginTop(unsigned int margin) {
    marginTop = margin;
}

void ChartLayout::setMarginRight(unsigned int margin) {
    marginRight = margin;
}

void ChartLayout::setMarginBottom(unsigned int margin) {
    marginBottom = margin;
}

void ChartLayout::setMarginLeft(unsigned int margin) {
    marginLeft = margin;
}

unsigned int ChartLayout::getPlotWidth() const {
    // Calculate exact plot width to match plotter.cpp
    // This is width minus the left and right margins, exactly as in plotter.cpp
    return width - marginLeft - marginRight;
}

unsigned int ChartLayout::getPlotHeight() const {
    // Calculate exact plot height to match plotter.cpp
    // This is height minus the top and bottom margins, exactly as in plotter.cpp
    return height - marginTop - marginBottom;
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

void ChartLayout::setCornerRadius(int radius) {
    cornerRadius = radius;
}

int ChartLayout::getCornerRadius() const {
    return cornerRadius;
}

void ChartLayout::setShowGrid(bool show) {
    showGrid = show;
}

void ChartLayout::setShowAxes(bool show) {
    showAxes = show;
}

bool ChartLayout::isGridVisible() const {
    return showGrid;
}

bool ChartLayout::areAxesVisible() const {
    return showAxes;
}

int ChartLayout::clamp(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

unsigned int ChartLayout::getSsaaFactor() const {
    return ssaaFactor;
}

void ChartLayout::setSsaaFactor(unsigned int factor) {
    if (factor < 1) factor = 1;
    ssaaFactor = factor;
}

// Access methods for supersampled image
Image& ChartLayout::getSsaaImage() {
    return ssaaImage;
}

const Image& ChartLayout::getSsaaImage() const {
    return ssaaImage;
}

// Logo dimension getters and setters
int ChartLayout::getLogoWidth() const {
    return logoWidth;
}

int ChartLayout::getLogoHeight() const {
    return logoHeight;
}

void ChartLayout::setLogoWidth(int width) {
    logoWidth = width;
}

void ChartLayout::setLogoHeight(int height) {
    logoHeight = height;
}

void ChartLayout::setShowOriginAxes(bool show) {
    showOriginAxes = show;
}

bool ChartLayout::areOriginAxesVisible() const {
    return showOriginAxes;
}

} // namespace shmea 