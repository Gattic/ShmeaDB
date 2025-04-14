#ifndef PLOTTER_H
#define PLOTTER_H

#include "../Database/image.h"
#include "DataMapper.h"
#include <string>
#include <vector>

// Forward declarations to avoid circular dependencies
namespace shmea {
    class ColorManager;
    class SuperSamplingManager;
    class ChartLayout;
    class ShapeRenderer;
    class TextRenderer;
    class GridRenderer;
    class DataMapper;
    class ChartStyler;
}

namespace shmea {

// Chart types for margin calculation
enum ChartType {
    CHART_DEFAULT,
    CHART_HISTOGRAM, 
    CHART_LINE,
    CHART_SCATTER,
    CHART_CANDLESTICK,
    CHART_CLUSTER
};

// Struct for chart configuration
struct ChartConfig {
    std::string title;
    unsigned int titleFontSize;
    std::string xAxisLabel;
    std::string yAxisLabel;
    unsigned int axisFontSize;
    
    ChartConfig(const std::string& title_, unsigned int titleFontSize_,
                const std::string& xAxisLabel_, const std::string& yAxisLabel_,
                unsigned int axisFontSize_)
        : title(title_), titleFontSize(titleFontSize_),
          xAxisLabel(xAxisLabel_), yAxisLabel(yAxisLabel_),
          axisFontSize(axisFontSize_) {}
};

// Plotter class that handles visualization using the component classes
class Plotter {
public:
    // Data point structure for visualizations
    struct Point {
        double x;
        double y;
        
        Point() : x(0), y(0) {}
        Point(double x_, double y_) : x(x_), y(y_) {}
    };
    
    // Candlestick data structure
    struct CandleData {
        double timestamp;
        double open;
        double close;
        double high;
        double low;
        
        CandleData() : timestamp(0), open(0), close(0), high(0), low(0) {}
        CandleData(double t, double o, double c, double h, double l)
            : timestamp(t), open(o), close(c), high(h), low(l) {}
    };
    
    // Constructor and destructor
    Plotter(unsigned int width, unsigned int height,
            unsigned int ssaa_factor = 1);
    
    // Backwards compatibility constructor that allows explicit margin specification
    Plotter(unsigned int width, unsigned int height,
            unsigned int margin_top, unsigned int margin_right,
            unsigned int margin_bottom, unsigned int margin_left,
            unsigned int ssaa_factor = 1);
    
    ~Plotter();
    
    // Initialization
    void initialize();
    
    // Chart configuration methods
    void setShowGrid(bool show);
    void setShowAxes(bool show);
    void setCornerRadius(int radius);
    void setSuperSamplingFactor(unsigned int factor);
    void setMarginTop(unsigned int margin);
    void setMarginRight(unsigned int margin);
    void setMarginBottom(unsigned int margin);
    void setMarginLeft(unsigned int margin);
    
    // Auto margin calculation
    void calculateOptimalMargins(ChartType chartType = CHART_DEFAULT);
    
    // Custom color manager
    void setCustomColors(const std::vector<RGBA>& clusterColors);
    void use10ClusterColorScheme(); // Use the 10-cluster color scheme
    
    // Basic drawing methods
    void prepareCanvas();
    void addTitle(const std::string& text, unsigned int fontSize = 24);
    void addAxisLabels(const std::string& xLabel, const std::string& yLabel, unsigned int fontSize = 18);
    int addLegend(const std::vector<std::string>& labels, const std::vector<RGBA>& colors,
                  int x, int y, unsigned int fontSize = 18);
    void setYAxisLabel(const std::string& label);
    
    // Logo handling
    void loadLogo(const std::string& logoPath);
    void drawLogo();
    
    // Output methods
    void saveAsPNG(const std::string& filename, const std::string& folder);
    
    // Chart setup helper
    void setupChart(const ChartConfig& config, unsigned int* originalTopMargin = NULL,
                    unsigned int* originalRightMargin = NULL, unsigned int newRightMargin = 180,
                    int* titleY = NULL, int* legendY = NULL);
    
    // Visualization methods
    void plotPoints(const std::vector<Point>& points, const RGBA& color, int pointSize = 8, bool redrawBackground = true);
    void plotLine(const std::vector<Point>& points, const RGBA& color, int lineWidth = 2, bool redrawBackground = true);
    void plotHistogram(const std::vector<int>& bins, const RGBA& color = RGBA(), bool showXAxisLabels = true);
    void plotClusters(const std::vector<std::vector<double> >& data,
                      const std::vector<int>& labels,
                      const std::vector<std::vector<double> >& centroids);
    void plotCandlestickChart(const std::vector<CandleData>& candles,
                              const RGBA& bullishColor = RGBA(0x03, 0xC0, 0x3C, 0xFF),
                              const RGBA& bearishColor = RGBA(0xFF, 0x47, 0x45, 0xFF));
    
    // Combines multiple series (lines and scatter points) in a single chart
    void plotMultiSeries(const std::vector<std::vector<Point> >& seriesData,
                        const std::vector<std::string>& seriesLabels,
                        const std::vector<RGBA>& seriesColors,
                        const std::vector<bool>& isLineStyleSeries,
                        const std::string& title = "Multi-Series Visualization",
                        const std::string& xAxisLabel = "X Value",
                        const std::string& yAxisLabel = "Y Value");
    
    // Direct access to GridRenderer for Y-axis ticks
    GridRenderer& getGridRenderer() { return *gridRenderer; }
    
private:
    // Component pointers
    ColorManager* colorManager;
    SuperSamplingManager* ssaaManager;
    ChartLayout* chartLayout;
    ShapeRenderer* shapeRenderer;
    TextRenderer* textRenderer;
    GridRenderer* gridRenderer;
    DataMapper* dataMapper;
    ChartStyler* chartStyler;
    
    // Output image
    Image image;
    
    // Logo handling
    bool hasLogo;
    Image logoImage;
    
    // Margin calculation helpers
    unsigned int calculateTopMargin(ChartType chartType, unsigned int width, unsigned int height);
    unsigned int calculateRightMargin(ChartType chartType, unsigned int width, unsigned int height);
    unsigned int calculateBottomMargin(ChartType chartType, unsigned int width, unsigned int height);
    unsigned int calculateLeftMargin(ChartType chartType, unsigned int width, unsigned int height);
    
    // Helper methods - general
    std::vector<DataMapper::Point> convertToDataPoints(const std::vector<Point>& points);
    void calculateDataRanges(const std::vector<DataMapper::Point>& dataPoints, 
                           DataMapper::AxisRange& xRange, 
                           DataMapper::AxisRange& yRange);
    void setupStandardAxisTicks(const DataMapper::AxisRange& xRange, 
                              const DataMapper::AxisRange& yRange, 
                              int yLabelOffset);
    bool isCoordinateValid(int x, int y);
    int clampToViewport(int value, int max);
    void drawLineSegment(int x1, int y1, int x2, int y2, const RGBA& color, int width, size_t segmentIndex);
    void prepareStandardChart(const std::string& title, 
                            const std::string& xAxisLabel, 
                            const std::string& yAxisLabel,
                            unsigned int titleFontSize,
                            unsigned int axisFontSize,
                            unsigned int rightMargin,
                            unsigned int* originalTopMargin,
                            unsigned int* originalRightMargin,
                            int* titleY,
                            int* legendY);
    
    // Helper methods - histogram specific
    void calculateHistogramBarDimensions(int totalBars, int& barWidth, int& barSpacing, int& startX);
    
    // Helper methods - candlestick specific
    std::vector<DataMapper::CandleData> convertToCandleData(const std::vector<CandleData>& candles);
    void calculateCandlestickRanges(const std::vector<DataMapper::CandleData>& candles,
                                  DataMapper::AxisRange& timeRange,
                                  DataMapper::AxisRange& priceRange);
    std::vector<std::string> createTimeLabels(double minTime, double maxTime, int numLabels);
    
    // Helper methods - cluster specific
    std::vector<RGBA> prepareClusterColors(int numClusters);
    std::vector<std::string> createClusterLegendLabels(int numClusters);
};

} // namespace shmea

#endif // PLOTTER_H 