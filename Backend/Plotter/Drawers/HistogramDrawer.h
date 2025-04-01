#ifndef HISTOGRAM_DRAWER_H
#define HISTOGRAM_DRAWER_H

#include "BaseDrawer.h"
#include "../GraphBounds.h"
#include <vector>

namespace shmea {

class HistogramDrawer : public BaseDrawer {
private:
    GraphBounds bounds;
    int graphSize;

public:
    HistogramDrawer(Image& image, unsigned int width, unsigned int height,
                   const GraphBounds& bounds, int graphSize);

    void drawBar(int x_start, int y_start, int bar_width, const RGBA& barColor);
    void addHistogram(const std::vector<int>& bins, const RGBA& barColor);
};

}  // namespace shmea
#endif
