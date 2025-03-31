#include "GridDrawer.h"
#include <cmath>
#include <cstring>
#include <sstream>
#include <algorithm>

namespace shmea {

GridDrawer::GridDrawer(Image& image, unsigned int width, unsigned int height, 
                      int margin_top, int margin_right, int margin_bottom, int margin_left,
                      double min_price, double max_price)
    : BaseDrawer(image, width, height, margin_top, margin_right, margin_bottom, margin_left),
      min_price(min_price), max_price(max_price) {
}

void GridDrawer::drawFourQuadrants() {
    RGBA lineColor(0xC8, 0xC8, 0xC8, 0xC8); // Light gray for the quadrant lines

    // Calculate positions for the middle lines
    int midX = (width - margin_left - margin_right) / 2 + margin_left;
    int midY = (height - margin_top - margin_bottom) / 2 + margin_top;

    // Draw the vertical middle line
    drawLine(midX, margin_top, midX, height - margin_bottom, lineColor);

    // Draw the horizontal middle line
    drawLine(margin_left, midY, width - margin_right, midY, lineColor);
}

std::vector<float> GridDrawer::get_axis_ticks(float max_price, float min_price, int max_ticks) const {
    std::vector<float> ticks;

    // Calculate the range
    float range = max_price - min_price;

    // Handle edge cases
    if (range <= 0 || max_ticks <= 1) {
        ticks.push_back(min_price);
        ticks.push_back(max_price);
        return ticks;
    }

    // Calculate a "nice" step size
    float rough_step = range / (max_ticks - 1);
    float step_magnitude = std::pow(10, std::floor(std::log10(rough_step)));
    float nice_step;

    if (rough_step / step_magnitude < 2) {
        nice_step = step_magnitude;
    } else if (rough_step / step_magnitude < 5) {
        nice_step = 2 * step_magnitude;
    } else {
        nice_step = 5 * step_magnitude;
    }

    // Adjust the start and end to be within the given range
    float start = std::ceil(min_price / nice_step) * nice_step;
    float end = std::floor(max_price / nice_step) * nice_step;

    // Generate the ticks
    for (float tick = start; tick <= end; tick += nice_step) {
        if (tick <= max_price && tick >= min_price) {
            ticks.push_back(tick);
        }
    }

    return ticks;
}

std::string GridDrawer::dateToString(int64_t timestamp, const char* format) const {
    // Ensure the timestamp fits within the range of time_t
    std::time_t time = static_cast<std::time_t>(timestamp);

    // Create a buffer for the formatted date string
    char buffer[64];
    std::memset(buffer, 0, sizeof(buffer));

    // Format the timestamp into a human-readable string
    if (std::strftime(buffer, sizeof(buffer), format, std::localtime(&time))) {
        return std::string(buffer);
    } else {
        return "Invalid Date";
    }
}

std::vector<std::string> GridDrawer::get_date_labels(int64_t start, int64_t end, int total_candles, int max_ticks) const {
    std::vector<std::string> labels;

    // Handle edge cases
    if (total_candles <= 0 || max_ticks <= 1 || start >= end) {
        labels.push_back(dateToString(start, "%m-%d-%Y"));
        labels.push_back(dateToString(end, "%m-%d-%Y"));
        return labels;
    }

    // Calculate the number of candles per tick
    int candles_per_tick = total_candles / (max_ticks - 1);
    if (candles_per_tick < 1) candles_per_tick = 1;

    // Calculate the time interval per candle
    int64_t time_per_candle = (end - start) / total_candles;

    std::string last_month_label = ""; //Track last month label
    std::string last_year_label = ""; //Track last year label
    
    // Generate labels
    for (int tick = 0; tick < max_ticks; ++tick) {
        int candle_index = tick * candles_per_tick;
        if (candle_index >= total_candles) break;

        // Calculate the timestamp for this tick
        int64_t timestamp = start + candle_index * time_per_candle;

        // Add the appropriate label based on time range
        if ((end - start) < 86400) { // Less than a day
            labels.push_back(dateToString(timestamp, "%H:%M"));
        } else if ((end - start) < 30 * 86400) { // Less than a month
            labels.push_back(dateToString(timestamp, "%m-%d"));
        } else if ((end - start) < 365 * 86400) { // Less than a year
            std::string current_month = dateToString(timestamp, "%b");
            if(current_month == last_month_label) {
                labels.push_back(dateToString(timestamp, "%d")); //Use day if month repeats
            } else {
                labels.push_back(current_month);
                last_month_label = current_month;
            }
        } else { // Multiple years
            std::string current_year = dateToString(timestamp, "%Y");
            if(current_year == last_year_label) {
                labels.push_back(dateToString(timestamp, "%b"));
            } else {
                labels.push_back(current_year);
                last_year_label = current_year;
            } 
        }
    }

    return labels;
}

void GridDrawer::drawYGrid() {
    RGBA gridColor(200, 200, 200, 200); // Light gray for the grid lines

    std::vector<float> horizontalLines = get_axis_ticks(max_price, min_price);
    float adjusted_max = max_price - min_price;
    
    // Draw horizontal grid lines and y-axis labels
    for (size_t i = 0; i < horizontalLines.size(); ++i) {
        float adjusted_tick = horizontalLines[i] - min_price;
        int y = height - margin_bottom - static_cast<int>(adjusted_tick / adjusted_max * (height - margin_top - margin_bottom));
        y = clamp(y, margin_top, height - margin_bottom);

        drawLine(margin_left, y, width - margin_right, y, gridColor);
    }
}

void GridDrawer::drawXGrid(int64_t start, int64_t end, int graphSize) {
    RGBA gridColor(200, 200, 200, 200); // Light gray for the grid lines

    std::vector<std::string> verticalLines = get_date_labels(start, end, graphSize);
    
    // Calculate step size for x-axis grid
    int step = (width - margin_left - margin_right) / (verticalLines.size() - 1);

    // Draw vertical grid lines
    for (size_t i = 0; i < verticalLines.size(); ++i) {
        int x = margin_left + i * step;
        drawLine(x, margin_top, x, height - margin_bottom, gridColor);
    }
}

}  // namespace shmea
