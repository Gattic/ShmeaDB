#ifndef GRID_DRAWER_H
#define GRID_DRAWER_H

#include "BaseDrawer.h"
#include "LabelsDrawer.h" // Add this include
#include "../GraphBounds.h"
#include <string>
#include <vector>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace shmea {

class GridDrawer : public BaseDrawer {
private:
    GraphBounds bounds;
    LabelsDrawer* labelsDrawer; // Add this pointer
    
    std::vector<float> get_axis_ticks(float max_price, float min_price, int max_ticks = 8) const;
    std::vector<std::string> get_date_labels(int64_t start, int64_t end, int total_candles, int max_ticks = 8) const;
    std::string dateToString(int64_t timestamp, const char* format = "%m-%d-%Y") const;
    std::string numberToString(float number) const; // New method
    
public:
    GridDrawer(Image& image, unsigned int width, unsigned int height,
               const GraphBounds& bounds);
    ~GridDrawer();
    
    // Set the LabelsDrawer reference
    void setLabelsDrawer(LabelsDrawer* drawer);
    
    void drawYGrid();
    void drawXGrid(int64_t start, int64_t end, int graphSize);
    void drawFourQuadrants();
    
    // Update the method signature to accept font size
    void drawYAxisLabels(unsigned int fontSize = 500);
    void drawXAxisLabels(int64_t start, int64_t end, int graphSize, unsigned int fontSize = 500);
};

}  // namespace shmea
#endif
