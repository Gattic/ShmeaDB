#include "HistogramDrawer.h"
#include <algorithm>

namespace shmea {

HistogramDrawer::HistogramDrawer(Image& image, unsigned int width, unsigned int height,
                               double min_price, double max_price, int graphSize)
    : BaseDrawer(image, width, height),
      min_price(min_price), max_price(max_price), graphSize(graphSize) {
}

void HistogramDrawer::drawBar(int x_start, int y_start, int bar_width, const RGBA& barColor) {
    // Calculate margins for context
    int margin_left = width * 0.15;
    int margin_right = width * 0.1;
    int margin_top = height * 0.1;
    int margin_bottom = height * 0.15;
    
    for(int x = x_start; x < x_start + bar_width; ++x) {
        for(int y = y_start; y < height - margin_bottom; ++y) {
            if (x >= margin_left && x < width - margin_right &&
                y >= margin_top && y < height - margin_bottom) {
                image.SetPixel(x, y, barColor);
            }
        }
    }
}

void HistogramDrawer::addHistogram(const std::vector<int>& bins, const RGBA& barColor) {
    // Calculate margins for context
    int margin_left = width * 0.15;
    int margin_right = width * 0.1;
    int margin_top = height * 0.1;
    int margin_bottom = height * 0.15;
    
    int max_count = *std::max_element(bins.begin(), bins.end());
    if (max_count == 0) return; // Avoid division by zero
    
    int bar_spacing = 25;
    
    int plot_width = width - margin_left - margin_right;
    int plot_height = height - margin_top - margin_bottom;

    int total_spacing = (graphSize - 1) * bar_spacing;
    int bar_total_width = plot_width - total_spacing;

    int bar_width = bar_total_width / graphSize;

    for(int i = 0; i < graphSize && i < static_cast<int>(bins.size()); ++i) {
        int bar_height = static_cast<int>((static_cast<double>(bins[i]) / max_count) * plot_height);
        int x_start = margin_left + i * (bar_width + bar_spacing);
        int y_start = height - margin_bottom - bar_height;

        drawBar(x_start, y_start, bar_width, barColor);
    }
}

}  // namespace shmea
