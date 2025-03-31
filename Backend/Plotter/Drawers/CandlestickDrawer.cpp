#include "CandlestickDrawer.h"
#include <algorithm>

namespace shmea {

CandlestickDrawer::CandlestickDrawer(Image& image, unsigned int width, unsigned int height, 
                                     int margin_top, int margin_right, int margin_bottom, int margin_left,
                                     int candle_width, const RGBA& bullish_color, const RGBA& bearish_color)
    : BaseDrawer(image, width, height, margin_top, margin_right, margin_bottom, margin_left),
      candle_width(candle_width),
      color_bullish(bullish_color),
      color_bearish(bearish_color) {
}

void CandlestickDrawer::drawCandleStick(int x, int y_open, int y_close, int y_high, int y_low, const RGBA& color) {
    const int body_width = candle_width;
    const int half_body_width = body_width / 2;
    const int wick_thickness = 20;

    // Draw wick (line between high and low)
    int wick_top = std::min(y_low, y_high);
    int wick_bottom = std::max(y_low, y_high);
    for (int y = wick_top; y <= wick_bottom; ++y) {
        for(int i = -wick_thickness; i <= wick_thickness; ++i) {
            if (x + i >= margin_left && x + i < width - margin_right && 
                y >= margin_top && y < height - margin_bottom) {
                image.SetPixel(x + i, y, color);
            }
        }
    }

    // Draw Body (rectangle between open and close)
    int body_top = std::min(y_open, y_close);
    int body_bottom = std::max(y_open, y_close);
    for (int y = body_top; y <= body_bottom; ++y) {
        for (int dx = -half_body_width; dx <= half_body_width; ++dx) {
            if (x + dx >= margin_left && x + dx < width - margin_right && 
                y >= margin_top && y < height - margin_bottom) {
                image.SetPixel(x + dx, y, color);
            }
        }
    }
}

}  // namespace shmea
