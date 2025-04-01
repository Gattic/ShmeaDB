#ifndef GRAPH_BOUNDS_H
#define GRAPH_BOUNDS_H

namespace shmea {

class GraphBounds {
private:
    double min_price;
    double max_price;

public:
    GraphBounds(double min = 0.0, double max = 100.0) 
        : min_price(min), max_price(max) {}
    
    // Getters
    double getMinPrice() const { return min_price; }
    double getMaxPrice() const { return max_price; }
    
    // Setters
    void setMinPrice(double min) { min_price = min; }
    void setMaxPrice(double max) { max_price = max; }
    
    // Utility methods
    double getRange() const { return max_price - min_price; }
    double normalize(double value) const { return value - min_price; }
    double getNormalizedRange() const { return getRange(); }
};

} // namespace shmea
#endif
