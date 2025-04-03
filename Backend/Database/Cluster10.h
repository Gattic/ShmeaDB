// Cluster10.h
#ifndef CLUSTER10_H
#define CLUSTER10_H

#include "image.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <string>
#include <vector>
#include <map>
#include <cmath>

namespace shmea {

class Cluster10 {
private:
    Image image;
    unsigned int width;
    unsigned int height;
    unsigned int margin_top;
    unsigned int margin_right;
    unsigned int margin_bottom;
    unsigned int margin_left;
    
    // Modern color palette from cluster10.fig
    std::vector<RGBA> themeColors;
    std::map<std::string, RGBA> elementColors;
    
    // Font handling
    FT_Library ft;
    FT_Face face;
    
    // Design elements
    bool showGrid;
    bool showAxes;
    int cornerRadius;
    
    // Initialize methods
    void initialize_colors();
    void initialize_font(const std::string fontPath = "fonts/font.ttf");
    
    // Drawing methods
    void drawBackground();
    void drawGrid();
    void drawAxes();
    void drawCornerRadius(int x, int y, int radius, bool topLeft, bool topRight, bool bottomRight, bool bottomLeft);
    
    // Helper method to clamp values
    inline int clamp(int value, int min, int max);

public:
    // Pair struct to replace std::pair from C++11
    struct Point {
        double x;
        double y;
        Point() : x(0.0), y(0.0) {}
        Point(double x_, double y_) : x(x_), y(y_) {}
    };

    // Constants for standard sizes
    static const int DEFAULT_WIDTH = 2400;
    static const int DEFAULT_HEIGHT = 1200;
    
    Cluster10(unsigned int width = DEFAULT_WIDTH, unsigned int height = DEFAULT_HEIGHT, 
              unsigned int margin_top = 60, unsigned int margin_right = 60, 
              unsigned int margin_bottom = 60, unsigned int margin_left = 60);
    ~Cluster10();
    
    // Basic controls
    void setShowGrid(bool show);
    void setShowAxes(bool show);
    void setCornerRadius(int radius);
    
    // Plot data methods
    void plotPoints(const std::vector<Point>& points, const RGBA& color, int pointSize = 8);
    void plotLine(const std::vector<Point>& points, const RGBA& color, int lineWidth = 2);
    void plotBars(const std::vector<double>& values, const RGBA& color, int barWidth = 20);
    void plotHistogram(const std::vector<int>& bins, const RGBA& color);
    void plotPieChart(const std::vector<double>& values, const std::vector<RGBA>& colors);
    void plotHeatmap(const std::vector<std::vector<double> >& data, bool useColorGradient = true);
    
    // Cluster visualization
    void plotClusters(const std::vector<std::vector<double> >& data, const std::vector<int>& labels, 
                     const std::vector<std::vector<double> >& centroids = std::vector<std::vector<double> >());
    
    // Text and label methods
    void addTitle(const std::string& text, unsigned int fontSize = 36);
    void addAxisLabels(const std::string& xLabel, const std::string& yLabel, unsigned int fontSize = 24);
    void addLegend(const std::vector<std::string>& labels, const std::vector<RGBA>& colors, 
                  int x, int y, unsigned int fontSize = 18);
    
    // Helper methods
    void drawPoint(int x, int y, int size, const RGBA& color);
    void drawLine(int x1, int y1, int x2, int y2, const RGBA& color, int width = 2);
    void drawText(int x, int y, const std::string& text, const RGBA& color, unsigned int fontSize = 18, 
                 bool centerAligned = false);
    void drawRect(int x, int y, int width, int height, const RGBA& color, bool filled = true, int borderWidth = 1);
    void drawCircle(int x, int y, int radius, const RGBA& color, bool filled = true, int borderWidth = 1);
    
    // Save methods
    void saveAsPNG(const std::string& filename, const std::string& folder = ".");
};

} // namespace shmea

#endif // CLUSTER10_H 