#ifndef GRID_DRAWER_H
#define GRID_DRAWER_H

#include "BaseDrawer.h"
#include <string>
#include <vector>
#include <ctime>

namespace shmea {

class GridDrawer : public BaseDrawer {
private:
    double min_price;
    double max_price;
    
    std::vector<float> get_axis_ticks(float max_price, float min_price, int max_ticks = 8) const;
    std::vector<std::string> get_date_labels(int64_t start, int64_t end, int total_candles, int max_ticks = 8) const;
    std::string dateToString(int64_t timestamp, const char* format = "%m-%d-%Y") const;
    
public:
    GridDrawer(Image& image, unsigned int width, unsigned int height, 
               int margin_top, int margin_right, int margin_bottom, int margin_left,
               double min_price, double max_price);
    
    void drawYGrid();
    void drawXGrid(int64_t start, int64_t end, int graphSize);
    void drawFourQuadrants();
};

}  // namespace shmea
#endif
