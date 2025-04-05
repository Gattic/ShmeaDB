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

// Forward declarations
class ThemeManager;
class GridRenderer;
class TextRenderer;

class Cluster10 {
public:
    // Data structures
    struct Point {
        double x;
        double y;
        Point() : x(0.0), y(0.0) {}
        Point(double x_, double y_) : x(x_), y(y_) {}
    };

    struct CandleData {
        double timestamp;  // Unix timestamp for the candle
        double open;
        double close;
        double high;
        double low;
        
        CandleData() : timestamp(0.0), open(0.0), close(0.0), high(0.0), low(0.0) {}
        CandleData(double t, double o, double c, double h, double l) 
            : timestamp(t), open(o), close(c), high(h), low(l) {}
    };

    struct AxisRange {
        double min;
        double max;
        double padding;
        
        AxisRange() : min(0.0), max(1.0), padding(0.05) {}
        AxisRange(double min_, double max_, double padding_) 
            : min(min_), max(max_), padding(padding_) {}
    };

    // Chart configuration structure
    struct ChartConfig {
        std::string title;
        unsigned int titleFontSize;
        std::string xAxisLabel;
        std::string yAxisLabel;
        unsigned int axisFontSize;
        
        ChartConfig() 
            : title(""), titleFontSize(36), xAxisLabel(""), yAxisLabel(""), axisFontSize(24) {}
        
        ChartConfig(const std::string& title_, unsigned int titleSize, 
                   const std::string& xLabel, const std::string& yLabel, unsigned int axisSize = 24)
            : title(title_), titleFontSize(titleSize), xAxisLabel(xLabel), yAxisLabel(yLabel), axisFontSize(axisSize) {}
    };

    // Constants for standard sizes
    static const int DEFAULT_WIDTH = 2400;
    static const int DEFAULT_HEIGHT = 1200;
    
    // Constructor/Destructor
    Cluster10(unsigned int width = DEFAULT_WIDTH, unsigned int height = DEFAULT_HEIGHT, 
              unsigned int margin_top = 60, unsigned int margin_right = 60, 
              unsigned int margin_bottom = 60, unsigned int margin_left = 60);
    ~Cluster10();
    
    // Basic controls
    void setShowGrid(bool show);
    void setShowAxes(bool show);
    void setCornerRadius(int radius);
    
    // Chart visualization methods
    void plotPoints(const std::vector<Point>& points, const RGBA& color, int pointSize = 8);
    void plotLine(const std::vector<Point>& points, const RGBA& color, int lineWidth = 2);
    void plotHistogram(const std::vector<int>& bins, const RGBA& color);
    void plotCandlestickChart(const std::vector<CandleData>& candles, 
                             const RGBA& bullishColor = RGBA(0x03, 0xC0, 0x3C, 0xFF),
                             const RGBA& bearishColor = RGBA(0xFF, 0x47, 0x45, 0xFF));
    void plotClusters(const std::vector<std::vector<double> >& data, const std::vector<int>& labels, 
                     const std::vector<std::vector<double> >& centroids = std::vector<std::vector<double> >());
    
    // Text and label methods
    void addTitle(const std::string& text, unsigned int fontSize = 36);
    void addAxisLabels(const std::string& xLabel, const std::string& yLabel, unsigned int fontSize = 24);
    void addLegend(const std::vector<std::string>& labels, const std::vector<RGBA>& colors, 
                  int x, int y, unsigned int fontSize = 18);
    
    // Save methods
    void saveAsPNG(const std::string& filename, const std::string& folder = ".");

private:
    //==================== MEMBER VARIABLES ====================//
    
    // Image and dimensions
    Image image;
    unsigned int width;
    unsigned int height;
    unsigned int margin_top;
    unsigned int margin_right;
    unsigned int margin_bottom;
    unsigned int margin_left;
    
    // Theme and color management
    std::vector<RGBA> themeColors;
    std::map<std::string, RGBA> elementColors;
    
    // Font handling
    FT_Library ft;
    FT_Face face;
    
    // Design elements settings
    bool showGrid;
    bool showAxes;
    int cornerRadius;
    
    //==================== INITIALIZATION METHODS ====================//
    
    // Common initialization for constructor
    void initialize();
    
    // Initialize color palette
    void initialize_colors();
    
    // Initialize font system
    void initialize_font(const std::string fontPath = "fonts/font.ttf");
    
    //==================== BASIC DRAWING METHODS ====================//
    
    // Draw the chart background gradient
    void drawBackground();
    
    // Draw grid lines
    void drawGrid();
    
    // Draw x and y axes
    void drawAxes();
    
    // Initialize a chart with background, grid, title, etc.
    void initializeChart(const std::string& title = "", unsigned int titleFontSize = 36);
    
    //==================== TEXT RENDERING METHODS ====================//
    
    // Draw text at specified position
    void drawText(int x, int y, const std::string& text, const RGBA& color, 
                 unsigned int fontSize = 18, bool centerAligned = false);
    
    // Draw text vertically (for y-axis labels)
    void drawVerticalText(const std::string& text, int x, int y, int fontSize, const RGBA& color);
    
    //==================== PRIMITIVE DRAWING METHODS ====================//
    
    // Draw a point (filled circle)
    void drawPoint(int x, int y, int size, const RGBA& color);
    
    // Draw a line between two points
    void drawLine(int x1, int y1, int x2, int y2, const RGBA& color, int width = 2);
    
    // Draw a rectangle
    void drawRect(int x, int y, int width, int height, const RGBA& color, bool filled = true, int borderWidth = 1);
    
    // Draw a circle
    void drawCircle(int x, int y, int radius, const RGBA& color, bool filled = true, int borderWidth = 1);
    
    // Draw rounded corners for grid border (matches CSS styling)
    void drawRoundedCorners(int left, int top, int right, int bottom, int radius, const RGBA& color);
    
    // Draw rounded corners
    void drawCornerRadius(int x, int y, int radius, bool topLeft, bool topRight, bool bottomRight, bool bottomLeft);
    
    //==================== COLOR AND PIXEL METHODS ====================//
    
    // Clamp a value between min and max
    inline int clamp(int value, int min, int max);
    
    // Blend two colors with alpha
    RGBA blendColors(const RGBA& baseColor, const RGBA& overlayColor, float alpha);
    
    // Proper alpha blending that preserves the background
    RGBA blendRGBA(const RGBA& base, const RGBA& over);
    
    // Blend a pixel with bounds checking
    void blendPixel(int x, int y, const RGBA& color, float alpha);
    
    // Get theme color by index with proper bounds checking
    RGBA getThemeColor(int index);
    
    //==================== COMMON UI COMPONENT METHODS ====================//
    
    // Draw an info box with gradient background and rounded corners
    void drawInfoBox(int x, int y, int width, int height, const std::string& text, unsigned int fontSize = 16);
    
    //==================== AXIS RENDERING METHODS ====================//
    
    // Draw Y-axis ticks and labels
    void drawYAxisTicks(double minValue, double maxValue, int numTicks, bool isInteger = false, 
                       int precision = 1, int labelOffset = 25);
    
    // Draw X-axis ticks with text labels
    void drawXAxisTicks(const std::vector<std::string>& labels, int numTicks);
    
    // Draw X-axis ticks with numeric values
    void drawXAxisTicks(double minValue, double maxValue, int numTicks, int precision = 1);
    
    //==================== CHART-SPECIFIC COMPONENTS ====================//
    
    // Draw a candlestick for financial charts
    void drawCandlestick(int x, int y_open, int y_close, int y_high, int y_low, const RGBA& color);
    
    // Draw histogram Y-axis
    void drawHistogramYAxis(int maxValue, int numTicks);
    
    // Draw histogram bars
    void drawHistogramBars(const std::vector<int>& bins, int maxBinValue, int totalBars, 
                          int barWidth, int barSpacing, const RGBA& color);
    
    // Add 3D effect to histogram bars
    void drawHistogramBarHighlights(int x, int y, int barWidth, int barHeight);
    
    // Draw statistics box for histogram
    void drawHistogramStats(const std::vector<int>& bins, int maxBinValue);
    
    // Draw candlestick chart X-axis with dates
    void drawCandlestickXAxis(const std::vector<CandleData>& candles, int maxVisibleCandles, 
                             int totalCandles, double firstTimestamp, double lastTimestamp);
    
    // Draw candlestick chart Y-axis with prices
    void drawCandlestickYAxis(double minPrice, double maxPrice, int numTicks);
    
    // Draw price info box for candlestick chart
    void drawCandlestickPriceInfo(const std::vector<CandleData>& candles,
                                const RGBA& bullishColor, const RGBA& bearishColor);
    
    // Create cluster visualization legend
    void createClusterLegend(const std::vector<RGBA>& clusterColors, int numClusters, int x, int y);
    
    // Calculate cluster boundaries for visualization
    void calculateClusterBounds(
        const std::vector<std::vector<double> >& data,
        const std::vector<int>& labels,
        int cluster,
        std::vector<Point>& clusterCenters,
        std::vector<int>& clusterRadii,
        const AxisRange& xRange,
        const AxisRange& yRange);
    
    // Draw cluster circles with proper transparency
    void drawClusterCircles(
        const std::vector<Point>& clusterCenters,
        const std::vector<int>& clusterRadii,
        const std::vector<RGBA>& clusterColors);
    
    // Draw centroids for cluster visualization
    void drawCentroids(
        const std::vector<std::vector<double> >& centroids,
        const std::vector<RGBA>& clusterColors,
        const AxisRange& xRange,
        const AxisRange& yRange);
    
    // Draw cluster labels
    void drawClusterLabels(
        const std::vector<Point>& clusterCenters,
        const std::vector<std::vector<std::pair<int, int> > >& clusterPoints,
        const std::vector<RGBA>& clusterColors);
    
    //==================== DATA SCALING METHODS ====================//
    
    // Calculate X range for points
    AxisRange calculateXRange(const std::vector<Point>& points);
    
    // Calculate Y range for points
    AxisRange calculateYRange(const std::vector<Point>& points);
    
    // Calculate X range for data matrix
    AxisRange calculateXRange(const std::vector<std::vector<double> >& data);
    
    // Calculate Y range for data matrix
    AxisRange calculateYRange(const std::vector<std::vector<double> >& data);
    
    // Map data coordinates to screen coordinates
    Point mapDataToScreen(double x, double y, const AxisRange& xRange, const AxisRange& yRange);
    
    //==================== LAYOUT HELPERS ====================//
    
    // Get the width of the plotting area
    int getPlotWidth() const { return width - margin_left - margin_right; }
    
    // Get the height of the plotting area
    int getPlotHeight() const { return height - margin_top - margin_bottom; }
};

} // namespace shmea

#endif // CLUSTER10_H 