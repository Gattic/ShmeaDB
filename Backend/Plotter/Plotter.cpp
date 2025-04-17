#include "Plotter.h"
#include "ColorManager.h"
#include "SuperSamplingManager.h"
#include "ChartLayout.h"
#include "ShapeRenderer.h"
#include "TextRenderer.h"
#include "GridRenderer.h"
#include "DataMapper.h"
#include "ChartStyler.h"
#include "../Database/png-helper.h"
#include <algorithm>
#include <vector>
#include <cstdio>

namespace shmea {

//
// ChartBuilder Implementation
//

ChartBuilder::ChartBuilder(Plotter& plotter)
    : plotter(plotter), 
      chartType(CHART_DEFAULT),
      hasHistogramData(false),
      histogramShowXAxisLabels(true),
      hasCandlestickData(false),
      bullishColor(0x03, 0xC0, 0x3C, 0xFF),
      bearishColor(0xFF, 0x47, 0x45, 0xFF),
      hasClusterData(false)
{
}

ChartBuilder& ChartBuilder::title(const std::string& title, unsigned int fontSize) {
    plotter.addTitle(title, fontSize);
    return *this;
}

ChartBuilder& ChartBuilder::size(unsigned int width, unsigned int height, unsigned int ssaaFactor) {
    // Size can't be changed after Plotter is created, so this is a no-op
    // Could be implemented if we allocated a new Image and re-initialized components
    printf("Warning: Cannot change chart size after creation. Use a new Plotter instance instead.\n");
    return *this;
}

ChartBuilder& ChartBuilder::margins(unsigned int top, unsigned int right, 
                                   unsigned int bottom, unsigned int left) {
    plotter.setMarginTop(top);
    plotter.setMarginRight(right);
    plotter.setMarginBottom(bottom);
    plotter.setMarginLeft(left);
    return *this;
}

ChartBuilder& ChartBuilder::autoMargins(ChartType chartType) {
    this->chartType = chartType;
    plotter.calculateOptimalMargins(chartType);
    return *this;
}

ChartBuilder& ChartBuilder::axisLabels(const std::string& xLabel, const std::string& yLabel,
                                      unsigned int fontSize) {
    plotter.addAxisLabels(xLabel, yLabel, fontSize);
    return *this;
}

ChartBuilder& ChartBuilder::grid(bool show) {
    plotter.setShowGrid(show);
    return *this;
}

ChartBuilder& ChartBuilder::axes(bool show) {
    plotter.setShowAxes(show);
    return *this;
}

ChartBuilder& ChartBuilder::cornerRadius(int radius) {
    plotter.setCornerRadius(radius);
    return *this;
}

ChartBuilder& ChartBuilder::logo(const std::string& logoPath) {
    plotter.loadLogo(logoPath);
    return *this;
}

ChartBuilder& ChartBuilder::colors(const std::vector<RGBA>& colors) {
    plotter.setCustomColors(colors);
    return *this;
}

ChartBuilder& ChartBuilder::addSeries(const Series& series) {
    this->series.push_back(series);
    
    // If origin axes are enabled, update the data ranges to include this series
    if (plotter.chartLayout->areOriginAxesVisible() && !series.data.empty()) {
        // Calculate min/max values to determine if data spans multiple quadrants
        double minX = series.data[0].x;
        double maxX = series.data[0].x;
        double minY = series.data[0].y;
        double maxY = series.data[0].y;
        
        for (size_t i = 1; i < series.data.size(); i++) {
            minX = std::min(minX, series.data[i].x);
            maxX = std::max(maxX, series.data[i].x);
            minY = std::min(minY, series.data[i].y);
            maxY = std::max(maxY, series.data[i].y);
        }
        
        // Check if data spans multiple quadrants or includes the origin
        bool spansQuadrants = (minX < 0 && maxX > 0) || (minY < 0 && maxY > 0);
        
        if (spansQuadrants) {
            // Ensure a good view of all quadrants by making the ranges balanced
            double xRange = std::max(std::abs(minX), std::abs(maxX)) * 1.2;
            double yRange = std::max(std::abs(minY), std::abs(maxY)) * 1.2;
            
            // Set the origin axes ranges to ensure good visualization
            double xMin = -xRange;
            double xMax = xRange;
            double yMin = -yRange;
            double yMax = yRange;
            
            // Update the data mapper ranges
            plotter.drawOriginAxes(xMin, xMax, yMin, yMax);
        }
    }
    
    return *this;
}

ChartBuilder& ChartBuilder::addSeries(const std::string& name, const std::vector<Point>& data, 
                                     const RGBA& color, SeriesType type,
                                     int lineWidth, int pointSize) {
    Series newSeries(name, data, color, type, lineWidth, pointSize);
    return addSeries(newSeries);
}

ChartBuilder& ChartBuilder::addHistogramData(const std::vector<int>& bins, 
                                           const RGBA& color, 
                                           bool showXAxisLabels) {
    this->hasHistogramData = true;
    this->histogramBins = bins;
    this->histogramColor = color;
    this->histogramShowXAxisLabels = showXAxisLabels;
    this->chartType = CHART_HISTOGRAM;
    return *this;
}

ChartBuilder& ChartBuilder::addCandlestickData(const std::vector<CandleData>& candles,
                                             const RGBA& bullishColor,
                                             const RGBA& bearishColor) {
    this->hasCandlestickData = true;
    this->candlestickData = candles;
    this->bullishColor = bullishColor;
    this->bearishColor = bearishColor;
    this->chartType = CHART_CANDLESTICK;
    return *this;
}

ChartBuilder& ChartBuilder::addClusterData(const std::vector<std::vector<double> >& data,
                                         const std::vector<int>& labels,
                                         const std::vector<std::vector<double> >& centroids) {
    this->hasClusterData = true;
    this->clusterData = data;
    this->clusterLabels = labels;
    this->centroids = centroids;
    this->chartType = CHART_CLUSTER;
    return *this;
}

ChartBuilder& ChartBuilder::addArrows(const std::vector<Arrow>& arrows) {
    this->arrows.insert(this->arrows.end(), arrows.begin(), arrows.end());
    return *this;
}

ChartBuilder& ChartBuilder::addArrow(const Arrow& arrow) {
    this->arrows.push_back(arrow);
    return *this;
}

ChartBuilder& ChartBuilder::addArrow(double startX, double startY, double endX, double endY, 
                                    const RGBA& color, int lineWidth, int arrowheadSize) {
    Arrow arrow(startX, startY, endX, endY, color, lineWidth, arrowheadSize);
    return addArrow(arrow);
}

void ChartBuilder::saveAs(const std::string& filename, const std::string& folder) {
    // Set optimal margins for the chart type
    plotter.calculateOptimalMargins(chartType);
    
    // Prepare the canvas
    plotter.prepareCanvas();
    
    // Determine the data bounds including all elements (series and arrows)
    std::vector<DataMapper::Point> allDataPoints;
    
    // Add series data points
    for (size_t i = 0; i < series.size(); i++) {
        for (size_t j = 0; j < series[i].data.size(); j++) {
            allDataPoints.push_back(DataMapper::Point(series[i].data[j].x, series[i].data[j].y));
        }
    }
    
    // Add arrow points
    for (size_t i = 0; i < arrows.size(); i++) {
        allDataPoints.push_back(DataMapper::Point(arrows[i].start.x, arrows[i].start.y));
        allDataPoints.push_back(DataMapper::Point(arrows[i].end.x, arrows[i].end.y));
    }
    
    // If we have data points, calculate a common axis range for all elements
    DataMapper::AxisRange commonXRange, commonYRange;
    bool hasCommonRange = false;
    
    if (!allDataPoints.empty()) {
        commonXRange = plotter.dataMapper->calculateXRange(allDataPoints);
        commonYRange = plotter.dataMapper->calculateYRange(allDataPoints);
        
        // Add padding to ensure all elements are visible
        double xPadding = (commonXRange.max - commonXRange.min) * 0.1;
        double yPadding = (commonYRange.max - commonYRange.min) * 0.1;
        
        // Ensure minimum padding
        xPadding = std::max(xPadding, 0.5);
        yPadding = std::max(yPadding, 0.5);
        
        // Apply padding
        commonXRange.min -= xPadding;
        commonXRange.max += xPadding;
        commonYRange.min -= yPadding;
        commonYRange.max += yPadding;
        
        printf("Chart display range - X: [%.2f, %.2f], Y: [%.2f, %.2f]\n", 
               commonXRange.min, commonXRange.max, commonYRange.min, commonYRange.max);
        
        // Store the ranges in the DataMapper for consistent scaling of all elements
        plotter.dataMapper->setCurrentXRange(commonXRange);
        plotter.dataMapper->setCurrentYRange(commonYRange);
        
        // Set up standard axis ticks with this common data range
        plotter.setupStandardAxisTicks(commonXRange, commonYRange, 50);
        
        hasCommonRange = true;
    }
    // If no data points, but we have arrows only, ensure we have reasonable default ranges
    else if (!arrows.empty()) {
        // Use default ranges that provide a reasonable viewing area
        commonXRange = DataMapper::AxisRange(-10.0, 10.0);
        commonYRange = DataMapper::AxisRange(-10.0, 10.0);
        
        printf("No data points found. Using default ranges for arrows - X: [%.2f, %.2f], Y: [%.2f, %.2f]\n", 
               commonXRange.min, commonXRange.max, commonYRange.min, commonYRange.max);
               
        // Store the ranges in the DataMapper
        plotter.dataMapper->setCurrentXRange(commonXRange);
        plotter.dataMapper->setCurrentYRange(commonYRange);
        
        // Set up standard axis ticks with default ranges
        plotter.setupStandardAxisTicks(commonXRange, commonYRange, 50);
        
        hasCommonRange = true;
    }
    
    // Render the appropriate chart based on the data provided
    if (!series.empty()) {
        // Plot the series data
        plotter.plotChart(series);
        
        // If we have both series and arrows, make sure the DataMapper is using
        // our common range and not just the series range
        if (!arrows.empty() && hasCommonRange) {
            plotter.dataMapper->setCurrentXRange(commonXRange);
            plotter.dataMapper->setCurrentYRange(commonYRange);
        }
    }
    else if (hasHistogramData) {
        // Plot histogram
        plotter.plotHistogram(histogramBins, histogramColor, histogramShowXAxisLabels);
    }
    else if (hasCandlestickData) {
        // Plot candlestick chart
        plotter.plotCandlestickChart(candlestickData, bullishColor, bearishColor);
    }
    else if (hasClusterData) {
        // Plot cluster chart
        plotter.plotClusters(clusterData, clusterLabels, centroids);
    }
    else if (arrows.empty()) {
        printf("Warning: No chart data provided to ChartBuilder. Nothing to render.\n");
    }
    
    // Draw any arrows that have been added
    if (!arrows.empty()) {
        // Make sure we're using the common ranges that include both series and arrows
        if (hasCommonRange) {
            printf("Drawing %lu arrows using common range - X: [%.2f, %.2f], Y: [%.2f, %.2f]\n", 
                   arrows.size(), 
                   commonXRange.min, commonXRange.max, 
                   commonYRange.min, commonYRange.max);
                   
            // Explicitly ensure DataMapper has the correct ranges right before drawing arrows
            // This is critical for Y-axis scaling to work correctly
            plotter.dataMapper->setCurrentXRange(commonXRange);
            plotter.dataMapper->setCurrentYRange(commonYRange);
            
            // Verify the ranges were properly set in the DataMapper
            DataMapper::AxisRange verifyX = plotter.dataMapper->getCurrentXRange();
            DataMapper::AxisRange verifyY = plotter.dataMapper->getCurrentYRange();
            printf("Verified DataMapper ranges - X: [%.2f, %.2f], Y: [%.2f, %.2f]\n",
                   verifyX.min, verifyX.max, verifyY.min, verifyY.max);
                   
            // Update axis ticks to match common ranges
            plotter.setupStandardAxisTicks(commonXRange, commonYRange, 50);
        } else {
            // Fallback to current ranges if common range wasn't calculated
            DataMapper::AxisRange currentXRange = plotter.dataMapper->getCurrentXRange();
            DataMapper::AxisRange currentYRange = plotter.dataMapper->getCurrentYRange();
            
            printf("Drawing %lu arrows using current range - X: [%.2f, %.2f], Y: [%.2f, %.2f]\n", 
                   arrows.size(), 
                   currentXRange.min, currentXRange.max, 
                   currentYRange.min, currentYRange.max);
        }
        
        // Plot the arrows without redrawing the background
        // This will use the DataMapper's current X and Y ranges that we already set
        plotter.plotArrows(arrows, false);
    }
    
    // Save the chart
    plotter.saveAsPNG(filename, folder);
}

// Add after ChartBuilder::axes
ChartBuilder& ChartBuilder::originAxes(bool show) {
    plotter.setShowOriginAxes(show);
    
    // If enabling origin axes, make sure we have proper initial ranges set
    if (show) {
        // Get current ranges from DataMapper
        DataMapper::AxisRange xRange = plotter.dataMapper->getCurrentXRange();
        DataMapper::AxisRange yRange = plotter.dataMapper->getCurrentYRange();
        
        // Check if ranges are set appropriately for origin axes
        bool rangesNeedAdjustment = false;
        
        // If ranges don't include zero or are too imbalanced, adjust them
        if (xRange.min >= 0 || xRange.max <= 0 || yRange.min >= 0 || yRange.max <= 0) {
            rangesNeedAdjustment = true;
        }
        
        // If we need to adjust ranges for origin axes
        if (rangesNeedAdjustment) {
            // If we don't have series data yet, use a balanced default range
            plotter.drawOriginAxes(-10.0, 10.0, -10.0, 10.0);
        }
    }
    
    return *this;
}

//
// Plotter Implementation
//
bool Plotter::fontLoaded = false;
FT_Library Plotter::ft = NULL;
FT_Face Plotter::face = NULL;

Plotter::Plotter(unsigned int width, unsigned int height, 
                 unsigned int margin_top, unsigned int margin_right, 
                 unsigned int margin_bottom, unsigned int margin_left,
                 unsigned int ssaa_factor)
    : hasLogo(false),
      currentXAxisRange(-10.0, 10.0),   // Default X range for origin axes
      currentYAxisRange(-10.0, 10.0)    // Default Y range for origin axes
{
    initialize_font("fonts/font.ttf");

    // Initialize helper components in the correct order
    colorManager = new ColorManager();
    
    chartLayout = new ChartLayout(width, height, margin_top, margin_right, margin_bottom, margin_left, ssaa_factor);
    
    ssaaManager = new SuperSamplingManager(width, height, ssaa_factor);
    
    shapeRenderer = new ShapeRenderer(*ssaaManager, *colorManager, *chartLayout);
    
    textRenderer = new TextRenderer(*ssaaManager, *colorManager, *chartLayout);
    textRenderer->initialize(ft, face);
    
    gridRenderer = new GridRenderer(*ssaaManager, *colorManager, *chartLayout, *shapeRenderer, ft, face);
    
    dataMapper = new DataMapper(*chartLayout);
    
    // Initialize dataMapper with the same default ranges
    dataMapper->setCurrentXRange(currentXAxisRange);
    dataMapper->setCurrentYRange(currentYAxisRange);
    
    chartStyler = new ChartStyler(*colorManager, *chartLayout, *shapeRenderer, 
                               *textRenderer, *gridRenderer, *dataMapper);
    
    // Allocate output image
    image.Allocate(width, height);
    
    // Complete initialization
    initialize();
}

// New constructor that automatically calculates margins
Plotter::Plotter(unsigned int width, unsigned int height, unsigned int ssaa_factor)
    : hasLogo(false),
      currentXAxisRange(-10.0, 10.0),   // Default X range for origin axes
      currentYAxisRange(-10.0, 10.0)    // Default Y range for origin axes
{
    initialize_font("fonts/font.ttf");

    // Initialize helper components in the correct order
    colorManager = new ColorManager();
    
    // Calculate default margins based on chart dimensions
    unsigned int margin_top = calculateTopMargin(CHART_DEFAULT, width, height);
    unsigned int margin_right = calculateRightMargin(CHART_DEFAULT, width, height);
    unsigned int margin_bottom = calculateBottomMargin(CHART_DEFAULT, width, height);
    unsigned int margin_left = calculateLeftMargin(CHART_DEFAULT, width, height);
    
    chartLayout = new ChartLayout(width, height, margin_top, margin_right, margin_bottom, margin_left, ssaa_factor);
    
    ssaaManager = new SuperSamplingManager(width, height, ssaa_factor);
    
    shapeRenderer = new ShapeRenderer(*ssaaManager, *colorManager, *chartLayout);
    
    textRenderer = new TextRenderer(*ssaaManager, *colorManager, *chartLayout);
    textRenderer->initialize(ft, face);
    
    gridRenderer = new GridRenderer(*ssaaManager, *colorManager, *chartLayout, *shapeRenderer, ft, face);
    
    dataMapper = new DataMapper(*chartLayout);
    
    // Initialize dataMapper with the same default ranges
    dataMapper->setCurrentXRange(currentXAxisRange);
    dataMapper->setCurrentYRange(currentYAxisRange);
    
    chartStyler = new ChartStyler(*colorManager, *chartLayout, *shapeRenderer, 
                               *textRenderer, *gridRenderer, *dataMapper);
    
    // Allocate output image
    image.Allocate(width, height);
    
    // Complete initialization
    initialize();
}

void Plotter::initialize_font(const std::string fontPath)
{
    if(fontLoaded)
	return;
    //
    //Initialize FreeType
    if(FT_Init_FreeType(&ft))
    {
	throw std::runtime_error("Could not initialize FreeType Library.");
    }

    // Load the font
    if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
        printf("Warning: Failed to load font: %s\n", fontPath.c_str());
        printf("Attempting to use a fallback font...\n");
        
        // Try some common system font locations
        const char* fallbackFonts[] = {
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/TTF/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/freefont/FreeSans.ttf"
        };
        
        bool fontLoaded = false;
        for (int i = 0; i < 5 && !fontLoaded; i++) {
            if (FT_New_Face(ft, fallbackFonts[i], 0, &face) == 0) {
                printf("Successfully loaded fallback font: %s\n", fallbackFonts[i]);
                fontLoaded = true;
            }
        }
        
        if (!fontLoaded) {
            printf("Error: Could not load any fonts. Text will not be rendered.\n");
            return;
        }
    } else {
        printf("Successfully loaded font: %s\n", fontPath.c_str());
    }

    fontLoaded = true;
}


Plotter::~Plotter()
{
    // Clean up components in reverse order
    delete chartStyler;
    delete dataMapper;
    delete gridRenderer;
    delete textRenderer;
    delete shapeRenderer;
    delete ssaaManager;
    delete chartLayout;
    delete colorManager;
}

void Plotter::initialize()
{
    // Prepare the canvas with background and grid
    prepareCanvas();
}

void Plotter::setShowGrid(bool show)
{
    chartLayout->setShowGrid(show);
    
    // Redraw the background when visibility changes
    prepareCanvas();
}

void Plotter::setShowAxes(bool show)
{
    chartLayout->setShowAxes(show);
    
    // Redraw the background when visibility changes
    prepareCanvas();
}

void Plotter::setCornerRadius(int radius)
{
    chartLayout->setCornerRadius(radius);
    
    // Redraw the background when corner radius changes
    prepareCanvas();
}

void Plotter::setSuperSamplingFactor(unsigned int factor)
{
    ssaaManager->setSuperSamplingFactor(factor);
    
    // Redraw with new supersampling settings
    prepareCanvas();
}

void Plotter::prepareCanvas()
{
    // Draw the background
    gridRenderer->drawBackground();
    
    // Draw grid if enabled
    if (chartLayout->isGridVisible()) {
        gridRenderer->drawGrid();
    }
    
    // Draw regular axes if enabled
    if (chartLayout->areAxesVisible() && !chartLayout->areOriginAxesVisible()) {
        gridRenderer->drawAxes();
    }
    
    // Draw origin-centered axes if enabled
    if (chartLayout->areOriginAxesVisible()) {
        // Use current data ranges from dataMapper if available
        DataMapper::AxisRange xRange = dataMapper->getCurrentXRange();
        DataMapper::AxisRange yRange = dataMapper->getCurrentYRange();
        
        // If no data range is set yet, use stored ranges or defaults
        if (xRange.min == xRange.max) {
            xRange = currentXAxisRange.min != currentXAxisRange.max ? 
                    currentXAxisRange : DataMapper::AxisRange(-10.0, 10.0);
        }
        
        if (yRange.min == yRange.max) {
            yRange = currentYAxisRange.min != currentYAxisRange.max ? 
                    currentYAxisRange : DataMapper::AxisRange(-10.0, 10.0);
        }
        
        gridRenderer->drawOriginAxes(xRange.min, xRange.max, yRange.min, yRange.max);
    }
}

void Plotter::addTitle(const std::string& text, unsigned int fontSize)
{
    textRenderer->addTitle(text, fontSize);
}

void Plotter::addAxisLabels(const std::string& xLabel, const std::string& yLabel, unsigned int fontSize)
{
    // First, store the Y-axis label for drawing in the final rendering
    textRenderer->setYAxisLabel(yLabel);
    
    // Store the font size for later use with Y-axis label
    textRenderer->setAxisLabelFontSize(fontSize);
    
    // Draw the X-axis label immediately - exact parameters from plotter.cpp
    RGBA textColor = colorManager->getElementColor("axisLabel");
    textColor.a = 0xCC; // 80% opacity for readability - exactly like plotter.cpp
    
    // Position X-axis label at the bottom center - exact positioning from plotter.cpp
    int xLabelX = chartLayout->getWidth() / 2; // Center horizontally - exact from plotter.cpp
    int xLabelY = chartLayout->getHeight() - chartLayout->getMarginBottom() / 3; // Position at 1/3 of bottom margin - exact from plotter.cpp
    
    // Draw X-axis label with proper styling - exact parameters from plotter.cpp
    textRenderer->drawText(xLabelX, xLabelY, xLabel, textColor, fontSize, true);
    
    // The Y-axis label is stored and will be drawn in saveAsPNG
}

int Plotter::addLegend(const std::vector<std::string>& labels, const std::vector<RGBA>& colors, 
                       int x, int y, unsigned int fontSize)
{
    return chartStyler->addLegend(labels, colors, x, y, fontSize);
}

void Plotter::setYAxisLabel(const std::string& label)
{
    textRenderer->setYAxisLabel(label);
}

void Plotter::loadLogo(const std::string& logoPath)
{
    // Try to load the logo image from the file
    try {
        logoImage = Image();
        shmea::PNGHelper::LoadPNG(logoImage, logoPath.c_str());
        
        if (logoImage.getWidth() > 0 && logoImage.getHeight() > 0) {
            hasLogo = true;
            
            // Get original dimensions
            int originalWidth = logoImage.getWidth();
            int originalHeight = logoImage.getHeight();
            printf("Logo loaded successfully from: %s (%d x %d)\n", 
                   logoPath.c_str(), originalWidth, originalHeight);
            
            // Calculate scaled dimensions right away
            int scaledWidth = originalWidth;
            int scaledHeight = originalHeight;
            
            // Apply the same scaling logic as in drawLogo
            const int maxLogoSize = 200;
            if (scaledWidth > maxLogoSize || scaledHeight > maxLogoSize) {
                float scale = maxLogoSize / static_cast<float>(std::max(scaledWidth, scaledHeight));
                scaledWidth = static_cast<int>(scaledWidth * scale);
                scaledHeight = static_cast<int>(scaledHeight * scale);
                printf("Logo will be scaled to: %d x %d\n", scaledWidth, scaledHeight);
            }
            
            // Store scaled logo dimensions in the chartLayout
            chartLayout->setLogoWidth(scaledWidth);
            chartLayout->setLogoHeight(scaledHeight);
        } else {
            hasLogo = false;
            
            // Reset logo dimensions in chartLayout
            chartLayout->setLogoWidth(0);
            chartLayout->setLogoHeight(0);
            
            printf("Failed to load logo from: %s (invalid dimensions)\n", logoPath.c_str());
        }
    } catch (...) {
        hasLogo = false;
        
        // Reset logo dimensions in chartLayout
        chartLayout->setLogoWidth(0);
        chartLayout->setLogoHeight(0);
        
        printf("Error loading logo from: %s (exception occurred)\n", logoPath.c_str());
    }
}

void Plotter::drawLogo()
{
    if (!hasLogo || logoImage.getWidth() == 0 || logoImage.getHeight() == 0) {
        // No logo to draw - make sure dimensions are reset
        chartLayout->setLogoWidth(0);
        chartLayout->setLogoHeight(0);
        printf("Logo not available or has invalid dimensions\n");
        return;
    }
    
    // Define position for the logo (top right corner)
    int logoWidth = logoImage.getWidth();
    int logoHeight = logoImage.getHeight();
    
    printf("Original logo dimensions: %d x %d\n", logoWidth, logoHeight);
    
    // Limit logo size to a reasonable maximum
    const int maxLogoSize = 200;
    if (logoWidth > maxLogoSize || logoHeight > maxLogoSize) {
        // Scale down while maintaining aspect ratio
        float scale = maxLogoSize / static_cast<float>(std::max(logoWidth, logoHeight));
        logoWidth = static_cast<int>(logoWidth * scale);
        logoHeight = static_cast<int>(logoHeight * scale);
        printf("Scaled logo dimensions: %d x %d (scale factor: %.2f)\n", logoWidth, logoHeight, scale);
    }
    
    // Always update the chart layout with the actual scaled logo dimensions
    // This ensures other components can properly position themselves
    chartLayout->setLogoWidth(logoWidth);
    chartLayout->setLogoHeight(logoHeight);
    
    // Calculate position in top right with padding
    int padX = 20;
    int padY = 20;
    int logoX = chartLayout->getWidth() - logoWidth - padX;
    int logoY = padY;
    
    printf("Logo position: x=%d, y=%d\n", logoX, logoY);
    
    // Scale coordinates for supersampling
    int ssaaLogoX = ssaaManager->scaleX(logoX);
    int ssaaLogoY = ssaaManager->scaleY(logoY);
    int ssaaLogoWidth = ssaaManager->scaleSize(logoWidth);
    int ssaaLogoHeight = ssaaManager->scaleSize(logoHeight);
    
    // Draw the logo with alpha blending
    for (int y = 0; y < ssaaLogoHeight; ++y) {
        for (int x = 0; x < ssaaLogoWidth; ++x) {
            // Get pixel from the logo - use bilinear interpolation for smoother scaling
            float srcX = x * (logoImage.getWidth() / static_cast<float>(ssaaLogoWidth));
            float srcY = y * (logoImage.getHeight() / static_cast<float>(ssaaLogoHeight));
            
            // Bilinear interpolation
            int x1 = static_cast<int>(srcX);
            int y1 = static_cast<int>(srcY);
            int x2 = x1 + 1;
            int y2 = y1 + 1;
            float xFrac = srcX - x1;
            float yFrac = srcY - y1;
            
            // Clamp source coordinates
            x1 = std::min(x1, static_cast<int>(logoImage.getWidth() - 1));
            y1 = std::min(y1, static_cast<int>(logoImage.getHeight() - 1));
            x2 = std::min(x2, static_cast<int>(logoImage.getWidth() - 1));
            y2 = std::min(y2, static_cast<int>(logoImage.getHeight() - 1));
            
            // Get the four surrounding pixels
            RGBA p11 = logoImage.GetPixel(x1, y1);
            RGBA p21 = logoImage.GetPixel(x2, y1);
            RGBA p12 = logoImage.GetPixel(x1, y2);
            RGBA p22 = logoImage.GetPixel(x2, y2);
            
            // Interpolate to get the final color
            RGBA logoPixel;
            logoPixel.r = static_cast<unsigned char>((1-xFrac)*(1-yFrac)*p11.r + xFrac*(1-yFrac)*p21.r + 
                                                    (1-xFrac)*yFrac*p12.r + xFrac*yFrac*p22.r);
            logoPixel.g = static_cast<unsigned char>((1-xFrac)*(1-yFrac)*p11.g + xFrac*(1-yFrac)*p21.g + 
                                                    (1-xFrac)*yFrac*p12.g + xFrac*yFrac*p22.g);
            logoPixel.b = static_cast<unsigned char>((1-xFrac)*(1-yFrac)*p11.b + xFrac*(1-yFrac)*p21.b + 
                                                    (1-xFrac)*yFrac*p12.b + xFrac*yFrac*p22.b);
            logoPixel.a = static_cast<unsigned char>((1-xFrac)*(1-yFrac)*p11.a + xFrac*(1-yFrac)*p21.a + 
                                                    (1-xFrac)*yFrac*p12.a + xFrac*yFrac*p22.a);
            
            // Skip fully transparent pixels
            if (logoPixel.a == 0) {
                continue;
            }
            
            // Calculate destination position
            int dstX = ssaaLogoX + x;
            int dstY = ssaaLogoY + y;
            
            // Blend the pixel
            float alpha = logoPixel.a / 255.0f;
            shapeRenderer->blendPixel(dstX, dstY, logoPixel, alpha);
        }
    }
}

void Plotter::saveAsPNG(const std::string& filename, const std::string& folder)
{
    // Draw any additional elements that should be rendered last
    
    // Draw Y-axis label - this is the ONLY place that draws the Y-axis label
    // Use exact parameters from plotter.cpp
    if (!textRenderer->getYAxisLabel().empty()) {
        // Get text color - exact parameters from plotter.cpp
        RGBA textColor = colorManager->getElementColor("axisLabel");
        textColor.a = 0xCC; // 80% opacity - exact value from plotter.cpp
        
        // Position for the Y-axis label - exact positioning from plotter.cpp
        int labelX = chartLayout->getMarginLeft() / 2; // Exact position from plotter.cpp
        int labelY = chartLayout->getHeight() / 2; // Center vertically - exact from plotter.cpp
        
        // Draw rotated label using the same font size as specified for the X-axis
        // Use the font size stored in the TextRenderer instead of hardcoded value
        textRenderer->drawVerticalText(textRenderer->getYAxisLabel(), labelX, labelY, 
                                      textRenderer->getAxisLabelFontSize(), textColor);
    }
    
    // Draw the logo if available
    if (hasLogo) {
        drawLogo();
    }
    
    // Downsample the supersampled image to the final output image
    ssaaManager->downsampleToOutput(image);
    
    // Make sure the directory exists
    std::string filenameWithPath = folder + "/" + filename;
    
    // Save PNG file using the png helper
    shmea::PNGHelper::SavePNG(image, filenameWithPath.c_str());
    printf("Saved chart as: %s\n", filenameWithPath.c_str());
}

void Plotter::setupChart(const ChartConfig& config, unsigned int* originalTopMargin, unsigned int* originalRightMargin, 
                         unsigned int newRightMargin, int* titleY, int* legendY)
{
    // Store original margins - exactly like plotter.cpp
    if (originalTopMargin)
        *originalTopMargin = chartLayout->getMarginTop();
    if (originalRightMargin)
        *originalRightMargin = chartLayout->getMarginRight();
    
    // Adjust right margin for axis labels - exactly like plotter.cpp
    chartLayout->setMarginRight(newRightMargin);
    
    // Initialize the canvas
    prepareCanvas();
    
    // Set default titleY if needed and provided - exact positioning from plotter.cpp
    if (titleY) {
        *titleY = 30; // Standard position below top margin - exact value from plotter.cpp
        
        // Add title - use exact parameters from plotter.cpp
        RGBA titleColor = colorManager->getElementColor("title");
        textRenderer->drawText(chartLayout->getMarginLeft(), *titleY, config.title, 
                           titleColor, config.titleFontSize, false);
        
        // Calculate legendY position - exactly like plotter.cpp
        if (legendY) {
            int titleHeight = config.titleFontSize - 10;
            *legendY = *titleY + titleHeight - 20; // Position legend closely below title - exact from plotter.cpp
        }
    }
}

void Plotter::plotPoints(const std::vector<Point>& points, const RGBA& color, int pointSize, bool redrawBackground)
{
    if (points.empty()) {
        printf("Warning: No points to plot in plotPoints\n");
        return;
    }
    
    printf("Plotting %lu points with pointSize = %d\n", points.size(), pointSize);
    
    // Convert Point vector to DataMapper::Point vector
    std::vector<DataMapper::Point> dataPoints = convertToDataPoints(points);
    
    // Calculate axis ranges for scaling
    DataMapper::AxisRange xRange, yRange;
    calculateDataRanges(dataPoints, xRange, yRange);
    
    // If we're adding to an existing chart, skip the setup
    if (!redrawBackground) {
        // Draw each point
        for (size_t i = 0; i < points.size(); ++i) {
            // Map point to screen coordinates
            DataMapper::Point p = dataMapper->mapDataToScreen(points[i].x, points[i].y, xRange, yRange);
            
            // Draw the point with supersampling
            int ssaaX = ssaaManager->scaleX(p.x);
            int ssaaY = ssaaManager->scaleY(p.y);
            int ssaaSize = ssaaManager->scaleSize(pointSize);
            
            // Ensure coordinates are valid
            if (isCoordinateValid(ssaaX, ssaaY)) {
                shapeRenderer->drawPoint(ssaaX, ssaaY, ssaaSize, color);
            }
        }
        
        return;
    }
    
    // Calculate optimal margins for scatter plot
    calculateOptimalMargins(CHART_SCATTER);
    
    // Variables for margin storage and positioning
    unsigned int originalTopMargin, originalRightMargin;
    int titleY, legendY;
    
    // Prepare the chart with standard configuration
    prepareStandardChart("Point Visualization", "X Value", "Y Value", 36, 28, 160,
                        &originalTopMargin, &originalRightMargin, &titleY, &legendY);
    
    // Set up standard axis ticks with 50px Y-axis label offset
    setupStandardAxisTicks(xRange, yRange, 50);
    
    // Draw each point
    for (size_t i = 0; i < points.size(); ++i) {
        // Map point to screen coordinates
        DataMapper::Point p = dataMapper->mapDataToScreen(points[i].x, points[i].y, xRange, yRange);
        
        // Print the first few points for debugging
        if (i < 5) {
            printf("Point %lu: data(%.2f, %.2f) -> screen(%.2f, %.2f)\n", 
                   i, points[i].x, points[i].y, p.x, p.y);
        }
        
        // Draw the point with supersampling
        int ssaaX = ssaaManager->scaleX(p.x);
        int ssaaY = ssaaManager->scaleY(p.y);
        int ssaaSize = ssaaManager->scaleSize(pointSize);
        
        // Ensure coordinates are valid
        if (isCoordinateValid(ssaaX, ssaaY)) {
            shapeRenderer->drawPoint(ssaaX, ssaaY, ssaaSize, color);
        }
        else {
            printf("Warning: Point %lu is outside the valid drawing area\n", i);
        }
    }
    
    // Restore original margins
    chartLayout->setMarginRight(originalRightMargin);
    chartLayout->setMarginTop(originalTopMargin);
}

void Plotter::plotLine(const std::vector<Point>& points, const RGBA& color, int lineWidth, bool redrawBackground)
{
    if (points.size() < 2) {
        printf("Warning: Not enough points to plot line in plotLine (need at least 2)\n");
        return;
    }
    
    printf("Plotting line with %lu points and lineWidth = %d\n", points.size(), lineWidth);
    
    // Convert Point vector to DataMapper::Point vector
    std::vector<DataMapper::Point> dataPoints = convertToDataPoints(points);
    
    // Calculate axis ranges for scaling
    DataMapper::AxisRange xRange, yRange;
    calculateDataRanges(dataPoints, xRange, yRange);
    
    // If we're adding to an existing chart, skip the setup
    if (!redrawBackground) {
        // Draw line segments between adjacent points
        for (size_t i = 1; i < points.size(); ++i) {
            // Map points to screen coordinates
            DataMapper::Point p1 = dataMapper->mapDataToScreen(points[i-1].x, points[i-1].y, xRange, yRange);
            DataMapper::Point p2 = dataMapper->mapDataToScreen(points[i].x, points[i].y, xRange, yRange);
            
            // Draw the line segment with proper clipping
            drawLineSegment(p1.x, p1.y, p2.x, p2.y, color, lineWidth, 0);
        }
        
        return;
    }
    
    // Calculate optimal margins for line chart
    calculateOptimalMargins(CHART_LINE);
    
    // Variables for margin storage and positioning
    unsigned int originalTopMargin, originalRightMargin;
    int titleY, legendY;
    
    // Prepare the chart with standard configuration
    prepareStandardChart("Line Visualization", "X Value", "Y Value", 36, 28, 160,
                        &originalTopMargin, &originalRightMargin, &titleY, &legendY);
    
    // Set up standard axis ticks with 50px Y-axis label offset
    setupStandardAxisTicks(xRange, yRange, 50);
    
    // Draw line segments between adjacent points
    for (size_t i = 1; i < points.size(); ++i) {
        // Map points to screen coordinates
        DataMapper::Point p1 = dataMapper->mapDataToScreen(points[i-1].x, points[i-1].y, xRange, yRange);
        DataMapper::Point p2 = dataMapper->mapDataToScreen(points[i].x, points[i].y, xRange, yRange);
        
        // Print the first few line segments for debugging
        if (i < 5) {
            printf("Line segment %lu: (%.2f,%.2f) to (%.2f,%.2f)\n", i, p1.x, p1.y, p2.x, p2.y);
        }
        
        // Draw the line segment with proper clipping
        drawLineSegment(p1.x, p1.y, p2.x, p2.y, color, lineWidth, i);
    }
    
    // Restore original margins
    chartLayout->setMarginRight(originalRightMargin);
    chartLayout->setMarginTop(originalTopMargin);
}

// Helper for calculating histogram bar dimensions
void Plotter::calculateHistogramBarDimensions(int totalBars, int& barWidth, int& barSpacing, int& startX) {
    // Use CSS-like styling for bars and spacing
    float barWidthPercentage = 0.6f; // Bar takes 60% of available space
    float spacingPercentage = 0.4f; // 40% for spacing
    
    // Calculate total space available per bar
    int totalBarSpace = chartLayout->getPlotWidth() / totalBars;
    barWidth = static_cast<int>(totalBarSpace * barWidthPercentage);
    barSpacing = static_cast<int>(totalBarSpace * spacingPercentage);
    
    // Ensure minimum size and spacing for readability
    barWidth = std::max(barWidth, 40); // Minimum 40px width for visibility
    barSpacing = std::max(barSpacing, 30); // Minimum 30px spacing for readability
    
    // Calculate start X position to center the bars
    startX = chartLayout->getMarginLeft() + (chartLayout->getPlotWidth() - 
            (totalBars * (barWidth + barSpacing) - barSpacing)) / 2;
}

// Original histogram method for backward compatibility
void Plotter::plotHistogram(const std::vector<int>& bins, const RGBA& color, bool showXAxisLabels)
{
    // Call the enhanced version with default parameters
    plotHistogram(bins, color, showXAxisLabels, "Data Distribution", "Value", "Frequency");
}

// Enhanced histogram method with more parameters
void Plotter::plotHistogram(const std::vector<int>& bins, 
                          const RGBA& color, 
                          bool showXAxisLabels,
                          const std::string& title,
                          const std::string& xAxisLabel,
                          const std::string& yAxisLabel)
{
    if (bins.empty()) {
        return;
    }
    
    // Use themeColors[0] if custom color not provided
    RGBA useColor = color;
    if (color.r == 0 && color.g == 0 && color.b == 0 && color.a == 0) {
        useColor = colorManager->getThemeColor(0); // Use first theme color
    }
    
    // Calculate optimal margins for histogram
    calculateOptimalMargins(CHART_HISTOGRAM);
    
    // Variables for margin storage and positioning
    unsigned int originalTopMargin, originalRightMargin;
    int titleY, legendY;
    
    // Prepare the chart with standard configuration - use 180px right margin for histograms
    prepareStandardChart(title, xAxisLabel, yAxisLabel, 42, 32, 180,
                        &originalTopMargin, &originalRightMargin, &titleY, &legendY);
    
    // Find the maximum value in bins for scaling
    int maxBinValue = *std::max_element(bins.begin(), bins.end());
    if (maxBinValue == 0) maxBinValue = 1; // Avoid division by zero
    
    // Calculate adjusted max Y value to match the 80% scaling of bars
    // Since bars are at 80% height, Y-axis needs to show 125% (1/0.8) of the max value
    int adjustedMaxYValue = static_cast<int>(maxBinValue / 0.8);
    
    // Create our legend labels and colors
    std::vector<std::string> legendLabels;
    legendLabels.push_back("Frequency");
    
    std::vector<RGBA> legendColors;
    legendColors.push_back(useColor);
    
    // Estimate legend height before drawing
    int estimatedLegendHeight = chartStyler->calculateInfoBoxHeight(legendLabels, 18);
    
    // Estimate stats box height (fixed at 90px as defined in drawHistogramStats)
    int statsBoxHeight = 90;
    
    // Calculate maximum Y position needed for legend and stats box
    int statsBoxMaxY = legendY + statsBoxHeight;
    int legendMaxY = legendY + estimatedLegendHeight;
    int maxElementsY = std::max(statsBoxMaxY, legendMaxY);
    
    // Dynamically set the top margin to accommodate title, legend, and stats box with buffer
    unsigned int newTopMargin = maxElementsY + 15; // 15px buffer
    chartLayout->setMarginTop(newTopMargin);
    
    // Redraw with proper margins
    prepareCanvas();
    
    // Redraw title after prepareCanvas
    textRenderer->drawText(chartLayout->getMarginLeft(), titleY, title, 
                      colorManager->getElementColor("title"), 42, false);
    
    // Now add the legend
    int actualLegendHeight = chartStyler->addLegend(legendLabels, legendColors, chartLayout->getMarginLeft(), legendY, 18);
    
    // Draw statistics info box with matching CSS styling - positioned right of the legend
    // We continue to display the true maxBinValue in the stats (not the adjusted one)
    chartStyler->drawHistogramStats(bins, maxBinValue, legendY, 16);
    
    // Draw axis labels with larger font size
    addAxisLabels(xAxisLabel, yAxisLabel, 32);
    
    // Draw Y-axis with value labels - use adjustedMaxYValue instead of maxBinValue
    // 5 ticks, values displayed as integers, 0 decimal places, 50px label offset
    gridRenderer->drawYAxisTicks(0, adjustedMaxYValue, 5, true, 0, 50);
    
    // Only draw X-axis tick labels if showXAxisLabels is true
    if (showXAxisLabels) {
        // For X-axis: create labels for each bin position
        std::vector<std::string> xLabels;
        for (size_t i = 0; i < bins.size(); i++) {
            // Create text label for each bin index
            char labelBuffer[32];
            std::sprintf(labelBuffer, "%d", static_cast<int>(i));
            xLabels.push_back(labelBuffer);
        }
        
        // Draw X-axis ticks with bin index labels - limit to 8 ticks for X-axis
        gridRenderer->drawXAxisTicks(xLabels, 8);
    }
    
    // Calculate bar dimensions
    int barWidth, barSpacing, startX;
    calculateHistogramBarDimensions(bins.size(), barWidth, barSpacing, startX);
    
    // Draw each histogram bar
    for (size_t i = 0; i < bins.size(); ++i) {
        // Calculate bar height based on bin value but cap it at 80% of the plot height
        float ratio = static_cast<float>(bins[i]) / maxBinValue;
        int barHeight = static_cast<int>(ratio * chartLayout->getPlotHeight() * 0.8); // Scale to 80% of original height
        
        // Ensure minimum height for visibility
        barHeight = std::max(barHeight, 8);
        
        // Calculate bar position
        int x = startX + i * (barWidth + barSpacing);
        int y = chartLayout->getHeight() - chartLayout->getMarginBottom() - barHeight;
        
        // Draw the bar with styling
        shapeRenderer->drawHistogramBar(
            ssaaManager->scaleX(x),
            ssaaManager->scaleY(y),
            ssaaManager->scaleSize(barWidth),
            ssaaManager->scaleSize(barHeight),
            useColor);
        
        // Add bar value label on top of taller bars
        if (barHeight > 45) { // Only for sufficiently tall bars
            char valueText[16];
            std::sprintf(valueText, "%d", bins[i]);
            
            // Position above the bar with proper spacing
            // Use font size 22 for better mobile readability
            textRenderer->drawText(x + barWidth / 2, 
                        chartLayout->getHeight() - chartLayout->getMarginBottom() - barHeight - 20, 
                        valueText, colorManager->getElementColor("legend"), 22, true);
        }
    }
    
    // Restore original margins
    chartLayout->setMarginRight(originalRightMargin);
    chartLayout->setMarginTop(originalTopMargin);
}

// Original candlestick method for backward compatibility
void Plotter::plotCandlestickChart(const std::vector<CandleData>& candles,
                                 const RGBA& bullishColor,
                                 const RGBA& bearishColor)
{
    // Call the enhanced version with default parameters
    plotCandlestickChart(candles, bullishColor, bearishColor, 
                         "Financial Data Analysis", "Date", "Price");
}

// Enhanced candlestick method with more parameters
void Plotter::plotCandlestickChart(const std::vector<CandleData>& candles,
                                 const RGBA& bullishColor,
                                 const RGBA& bearishColor,
                                 const std::string& title,
                                 const std::string& xAxisLabel,
                                 const std::string& yAxisLabel)
{
    if (candles.empty()) {
        return;
    }
    
    // Calculate optimal margins for candlestick chart
    calculateOptimalMargins(CHART_CANDLESTICK);
    
    // Use parameter colors or defaults from CSS if none provided
    RGBA useBullishColor = bullishColor;
    RGBA useBearishColor = bearishColor;
    
    // If colors are default values (with all zero components), use theme colors
    if (bullishColor.r == 0 && bullishColor.g == 0 && bullishColor.b == 0 && bullishColor.a == 0) {
        // Use bright green for bullish candles
        useBullishColor = colorManager->getElementColor("bullish");
    }
    if (bearishColor.r == 0 && bearishColor.g == 0 && bearishColor.b == 0 && bearishColor.a == 0) {
        // Use bright pink/red for bearish candles
        useBearishColor = colorManager->getElementColor("bearish");
    }
    
    // Variables for margin storage and positioning
    unsigned int originalTopMargin, originalRightMargin;
    int titleY, legendY;
    
    // Prepare the chart with standard configuration - use 220px right margin for candlestick charts
    prepareStandardChart(title, xAxisLabel, yAxisLabel, 36, 32, 220,
                        &originalTopMargin, &originalRightMargin, &titleY, &legendY);
    
    // Convert to DataMapper::CandleData
    std::vector<DataMapper::CandleData> mapperCandles = convertToCandleData(candles);
    
    // Create legend labels and colors
    std::vector<std::string> legendLabels;
    legendLabels.push_back("Bullish Candle");
    legendLabels.push_back("Bearish Candle");
    
    std::vector<RGBA> legendColors;
    legendColors.push_back(useBullishColor);
    legendColors.push_back(useBearishColor);
    
    // Add the legend - exact font size (16px) as in plotter.cpp
    int actualLegendHeight = chartStyler->addLegend(legendLabels, legendColors, chartLayout->getMarginLeft(), legendY, 16);
    
    // Draw price movement indicators and current price display
    chartStyler->drawCandlestickPriceInfo(mapperCandles, useBullishColor, useBearishColor, legendY);
    
    // Dynamically adjust top margin to fit both legend and price info with spacing
    unsigned int newTopMargin = std::max(static_cast<unsigned int>(legendY + actualLegendHeight + 15), 
                                         chartLayout->getMarginTop());
    chartLayout->setMarginTop(newTopMargin);
    
    // Redraw to ensure proper layout
    prepareCanvas();
    
    // Redraw the title since prepareCanvas clears everything
    textRenderer->drawText(chartLayout->getMarginLeft(), titleY, title, 
                       colorManager->getElementColor("title"), 36, false);
    
    // Add the legend again after the canvas redraw
    chartStyler->addLegend(legendLabels, legendColors, chartLayout->getMarginLeft(), legendY, 16);
    
    // Draw price info again after canvas redraw
    chartStyler->drawCandlestickPriceInfo(mapperCandles, useBullishColor, useBearishColor, legendY);
    
    // Draw axis labels with larger font size
    addAxisLabels(xAxisLabel, yAxisLabel, 32);
    
    // Calculate time and price ranges
    DataMapper::AxisRange timeRange, priceRange;
    calculateCandlestickRanges(mapperCandles, timeRange, priceRange);
    
    // Create time labels for X-axis (4 evenly spaced ticks)
    std::vector<std::string> timeLabels = createTimeLabels(timeRange.min, timeRange.max, 4);
    gridRenderer->drawXAxisTicks(timeLabels, 4);
    
    // Draw Y-axis ticks with appropriate values
    // 5 ticks, not displayed as integers, 2 decimal places precision, 70px label offset
    gridRenderer->drawYAxisTicks(priceRange.min, priceRange.max, 5, false, 2, 70);
    
    // Calculate optimal candle width and spacing to completely fill the plot width
    int totalCandles = mapperCandles.size();
    int plotWidth = chartLayout->getPlotWidth();
    
    // Calculate exact candle width to fill the entire plot width with no gaps
    int candleWidth = plotWidth / totalCandles;
    
    // Adjust start position to ensure candles are centered in the plot area
    int leftoverSpace = plotWidth - (candleWidth * totalCandles);
    int startX = chartLayout->getMarginLeft() + leftoverSpace / 2;
    
    // Draw each candle using the specialized ShapeRenderer method
    for (size_t i = 0; i < mapperCandles.size(); ++i) {
        const DataMapper::CandleData& candle = mapperCandles[i];
        
        // Determine if candle is bullish or bearish
        bool isBullish = candle.close >= candle.open;
        RGBA candleColor = isBullish ? useBullishColor : useBearishColor;
        
        // Calculate the center position of this candle
        int x = startX + i * candleWidth + candleWidth / 2;
        
        // Map price points to screen coordinates using DataMapper
        DataMapper::Point highPoint = dataMapper->mapDataToScreen(
            candle.timestamp, candle.high, timeRange, priceRange);
        DataMapper::Point lowPoint = dataMapper->mapDataToScreen(
            candle.timestamp, candle.low, timeRange, priceRange);
        DataMapper::Point openPoint = dataMapper->mapDataToScreen(
            candle.timestamp, candle.open, timeRange, priceRange);
        DataMapper::Point closePoint = dataMapper->mapDataToScreen(
            candle.timestamp, candle.close, timeRange, priceRange);
        
        // Scale coordinates for supersampling
        int ssaaX = ssaaManager->scaleX(x);
        int ssaaHighY = ssaaManager->scaleY(highPoint.y);
        int ssaaLowY = ssaaManager->scaleY(lowPoint.y);
        int ssaaOpenY = ssaaManager->scaleY(openPoint.y);
        int ssaaCloseY = ssaaManager->scaleY(closePoint.y);
        
        // Pass the exact candle width minus 1 pixel to ensure there's no overlap
        int adjustedCandleWidth = std::max(4, candleWidth - 1);
        
        shapeRenderer->drawCandlestick(
            ssaaX,                // Center of candle
            ssaaOpenY,            // Open price
            ssaaCloseY,           // Close price
            ssaaHighY,            // High price
            ssaaLowY,             // Low price
            candleColor,          // Bullish or bearish color
            adjustedCandleWidth   // Exact width to fill plot area
        );
    }
    
    // Restore original margins
    chartLayout->setMarginRight(originalRightMargin);
    chartLayout->setMarginTop(originalTopMargin);
}

// Original clusters method for backward compatibility
void Plotter::plotClusters(const std::vector<std::vector<double> >& data,
                         const std::vector<int>& labels,
                         const std::vector<std::vector<double> >& centroids)
{
    // Call the enhanced version with default parameters
    plotClusters(data, labels, centroids, "Cluster Analysis", "Feature X", "Feature Y");
}

// Enhanced clusters method with more parameters
void Plotter::plotClusters(const std::vector<std::vector<double> >& data,
                         const std::vector<int>& labels,
                         const std::vector<std::vector<double> >& centroids,
                         const std::string& title,
                         const std::string& xAxisLabel,
                         const std::string& yAxisLabel)
{
    if (data.empty() || data[0].size() < 2 || data.size() != labels.size()) {
        return;
    }
    
    // Calculate optimal margins for cluster visualization
    calculateOptimalMargins(CHART_CLUSTER);
    
    // Variables for margin storage and positioning
    unsigned int originalTopMargin, originalRightMargin;
    int titleY, legendY;
    
    // Prepare the chart with standard configuration - use 160px right margin for cluster visualization
    prepareStandardChart(title, xAxisLabel, yAxisLabel, 36, 32, 160,
                        &originalTopMargin, &originalRightMargin, &titleY, &legendY);
    
    // Find number of unique clusters
    int maxCluster = -1;
    for (size_t i = 0; i < labels.size(); ++i) {
        maxCluster = std::max(maxCluster, labels[i]);
    }
    
    // Total number of clusters
    int numClusters = maxCluster + 1;
    
    // Prepare colors for each cluster
    std::vector<RGBA> clusterColors = prepareClusterColors(numClusters);
    
    // Create legend labels
    std::vector<std::string> tempLegendLabels = createClusterLegendLabels(numClusters);
    
    // Calculate estimated legend height
    int estimatedLegendHeight = chartLayout->calculateInfoBoxHeight(tempLegendLabels, 16);
    
    // Add buffer for spacing between legend and the plot area
    int legendBuffer = 15; // 15px buffer for spacing
    
    // Dynamically set the top margin to accommodate title and legend with proper spacing
    unsigned int newTopMargin = legendY + estimatedLegendHeight + legendBuffer;
    chartLayout->setMarginTop(newTopMargin);
    
    // Redraw with proper margins
    prepareCanvas();
    
    // Redraw title after prepareCanvas
    textRenderer->drawText(chartLayout->getMarginLeft(), titleY, title, 
                      colorManager->getElementColor("title"), 36, false);
    
    // Now add the cluster legend after margins are adjusted
    int actualLegendHeight = chartStyler->createClusterLegend(clusterColors, numClusters, 
                                                        chartLayout->getMarginLeft(), legendY);
    
    // If the actual legend height is significantly different than the estimate, readjust margin
    if (std::abs(actualLegendHeight - estimatedLegendHeight) > 20) {
        unsigned int adjustedMargin = legendY + actualLegendHeight + legendBuffer;
        chartLayout->setMarginTop(adjustedMargin);
        // No need to redraw again as the margin change will only affect what's drawn next
    }
    
    // Draw axis labels with larger font size
    addAxisLabels(xAxisLabel, yAxisLabel, 32);
    
    // Calculate data ranges for X and Y
    DataMapper::AxisRange xRange = dataMapper->calculateXRange(data);
    DataMapper::AxisRange yRange = dataMapper->calculateYRange(data);
    
    // Draw axis ticks with appropriate ranges
    setupStandardAxisTicks(xRange, yRange, 50);
    
    // Vectors to store cluster visualization data
    std::vector<std::vector<DataMapper::Point> > clusterPoints(numClusters);
    std::vector<std::vector<std::pair<int, int> > > clusterPointPairs(numClusters);
    std::vector<DataMapper::Point> clusterCenters;
    std::vector<int> clusterRadii;
    
    // Draw each cluster's points and collect data for visualization
    for (int cluster = 0; cluster < numClusters; ++cluster) {
        // Calculate and store bounds for this cluster
        chartStyler->calculateClusterBounds(data, labels, cluster, clusterCenters, clusterRadii, xRange, yRange);
        
        // Map data points to screen coordinates and organize by cluster
        for (size_t i = 0; i < data.size(); ++i) {
            if (labels[i] == cluster) {
                double x = data[i][0]; // First dimension
                double y = data[i][1]; // Second dimension
                
                // Map data coordinates to screen coordinates
                DataMapper::Point screenPoint = dataMapper->mapDataToScreen(x, y, xRange, yRange);
                clusterPoints[cluster].push_back(screenPoint);
                clusterPointPairs[cluster].push_back(std::make_pair(screenPoint.x, screenPoint.y));
                
                // Draw point
                int ssaaX = ssaaManager->scaleX(screenPoint.x);
                int ssaaY = ssaaManager->scaleY(screenPoint.y);
                int pointSize = ssaaManager->scaleSize(8); // 8px point size
                
                // Draw a filled circle for each data point
                shapeRenderer->drawCircle(ssaaX, ssaaY, pointSize, clusterColors[cluster], true);
            }
        }
    }
    
    // Draw cluster circles
    chartStyler->drawClusterCircles(clusterCenters, clusterRadii, clusterColors);
    
    // Draw cluster labels
    chartStyler->drawClusterLabels(clusterCenters, clusterRadii, clusterPointPairs, clusterColors);
    
    // Draw centroids
    chartStyler->drawCentroids(centroids, clusterColors, xRange, yRange);
    
    // Restore original margins
    chartLayout->setMarginRight(originalRightMargin);
    chartLayout->setMarginTop(originalTopMargin);
}

void Plotter::setMarginTop(unsigned int margin)
{
    chartLayout->setMarginTop(margin);
}

void Plotter::setCustomColors(const std::vector<RGBA>& clusterColors) {
    // Set custom colors in the color manager
    if (colorManager != NULL) {
        colorManager->setThemeColors(clusterColors);
    }
}

void Plotter::use10ClusterColorScheme() {
    // Initialize the 10-cluster color scheme in the color manager
    if (colorManager != NULL) {
        colorManager->initialize10ClusterScheme();
    }
}

// Helper method to convert points to DataMapper points
std::vector<DataMapper::Point> Plotter::convertToDataPoints(const std::vector<Point>& points) {
    std::vector<DataMapper::Point> dataPoints;
    dataPoints.reserve(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        dataPoints.push_back(DataMapper::Point(points[i].x, points[i].y));
    }
    return dataPoints;
}

// Helper method to calculate axis ranges from a set of points
void Plotter::calculateDataRanges(const std::vector<DataMapper::Point>& dataPoints, 
                                  DataMapper::AxisRange& xRange, 
                                  DataMapper::AxisRange& yRange) {
    xRange = dataMapper->calculateXRange(dataPoints);
    yRange = dataMapper->calculateYRange(dataPoints);
    
    printf("X range: [%.2f, %.2f], Y range: [%.2f, %.2f]\n", 
           xRange.min, xRange.max, yRange.min, yRange.max);
}

// Helper method to set up standard axis ticks
void Plotter::setupStandardAxisTicks(const DataMapper::AxisRange& xRange, 
                                     const DataMapper::AxisRange& yRange, 
                                     int yLabelOffset) {
    // Draw X-axis ticks with 5 divisions and 1 decimal place
    gridRenderer->drawXAxisTicks(xRange.min, xRange.max, 5, 1);
    
    // Draw Y-axis ticks with 5 divisions, not as integers, 1 decimal place
    // and customizable label offset
    gridRenderer->drawYAxisTicks(yRange.min, yRange.max, 5, false, 1, yLabelOffset);
}

// Helper method for determining if a coordinate is valid within the viewport
bool Plotter::isCoordinateValid(int x, int y) {
    return (x >= 0 && x < static_cast<int>(ssaaManager->getWidth()) &&
            y >= 0 && y < static_cast<int>(ssaaManager->getHeight()));
}

// Helper method to clamp coordinates to viewport
int Plotter::clampToViewport(int value, int max) {
    return ChartLayout::clamp(value, 0, max - 1);
}

// Helper method for drawing line segments with viewport clipping
void Plotter::drawLineSegment(int x1, int y1, int x2, int y2, const RGBA& color, int width, size_t segmentIndex) {
    // Scale coordinates for supersampling
    int ssaaX1 = ssaaManager->scaleX(x1);
    int ssaaY1 = ssaaManager->scaleY(y1);
    int ssaaX2 = ssaaManager->scaleX(x2);
    int ssaaY2 = ssaaManager->scaleY(y2);
    int ssaaWidth = ssaaManager->scaleSize(width);
    
    // Check if line segment coordinates are valid
    bool validCoords = isCoordinateValid(ssaaX1, ssaaY1) && isCoordinateValid(ssaaX2, ssaaY2);
    
    if (validCoords) {
        // Draw the line segment with supersampling
        shapeRenderer->drawLine(ssaaX1, ssaaY1, ssaaX2, ssaaY2, color, ssaaWidth);
    }
    else {
        if (segmentIndex > 0) {
            printf("Warning: Line segment %lu has points outside the valid drawing area\n", segmentIndex);
        }
        
        // Try to draw with clipping to viewport
        int maxX = static_cast<int>(ssaaManager->getWidth());
        int maxY = static_cast<int>(ssaaManager->getHeight());
        
        shapeRenderer->drawLine(
            clampToViewport(ssaaX1, maxX),
            clampToViewport(ssaaY1, maxY),
            clampToViewport(ssaaX2, maxX),
            clampToViewport(ssaaY2, maxY),
            color,
            ssaaWidth);
    }
}

// Helper method to prepare a chart with standard configurations
void Plotter::prepareStandardChart(const std::string& title, 
                                  const std::string& xAxisLabel, 
                                  const std::string& yAxisLabel,
                                  unsigned int titleFontSize,
                                  unsigned int axisFontSize,
                                  unsigned int rightMargin,
                                  unsigned int* originalTopMargin,
                                  unsigned int* originalRightMargin,
                                  int* titleY,
                                  int* legendY) {
    // Create chart configuration
    ChartConfig config(title, titleFontSize, xAxisLabel, yAxisLabel, axisFontSize);
    
    // Use common setup for chart initialization
    setupChart(config, originalTopMargin, originalRightMargin, rightMargin, titleY, legendY);
    *legendY += 15; // Add space between title and legend
    
    // Draw axis labels
    addAxisLabels(config.xAxisLabel, config.yAxisLabel, config.axisFontSize);
}

// Helper method to convert CandleData to DataMapper::CandleData
std::vector<DataMapper::CandleData> Plotter::convertToCandleData(const std::vector<CandleData>& candles) {
    std::vector<DataMapper::CandleData> mapperCandles;
    mapperCandles.reserve(candles.size());
    
    for (size_t i = 0; i < candles.size(); ++i) {
        const CandleData& candle = candles[i];
        mapperCandles.push_back(DataMapper::CandleData(
            candle.timestamp, candle.open, candle.close, candle.high, candle.low
        ));
    }
    
    return mapperCandles;
}

// Helper method to calculate time and price ranges for candlestick chart
void Plotter::calculateCandlestickRanges(const std::vector<DataMapper::CandleData>& candles,
                                        DataMapper::AxisRange& timeRange,
                                        DataMapper::AxisRange& priceRange) {
    if (candles.empty()) return;
    
    double minTime = candles[0].timestamp;
    double maxTime = candles[0].timestamp;
    double minPrice = candles[0].low;
    double maxPrice = candles[0].high;
    
    for (size_t i = 1; i < candles.size(); ++i) {
        minTime = std::min(minTime, candles[i].timestamp);
        maxTime = std::max(maxTime, candles[i].timestamp);
        minPrice = std::min(minPrice, candles[i].low);
        maxPrice = std::max(maxPrice, candles[i].high);
    }
    
    // Add padding to the price range (asymmetric padding)
    double priceSpan = maxPrice - minPrice;
    if (priceSpan < 1e-10) priceSpan = 1.0;
    minPrice -= priceSpan * 0.05; // 5% padding below
    maxPrice += priceSpan * 0.10; // 10% padding above
    
    // Create axis ranges with padding
    timeRange = DataMapper::AxisRange(minTime, maxTime, 0.02); // 2% padding
    priceRange = DataMapper::AxisRange(minPrice, maxPrice, 0.05); // 5% padding
}

// Helper method to create time labels for X-axis
std::vector<std::string> Plotter::createTimeLabels(double minTime, double maxTime, int numLabels) {
    std::vector<std::string> timeLabels;
    
    for (int i = 0; i < numLabels; i++) {
        // Calculate evenly spaced time points
        double timestamp = minTime + (i * (maxTime - minTime) / (numLabels - 1.0));
        std::time_t time = static_cast<std::time_t>(timestamp);
        struct tm* timeinfo = std::localtime(&time);
        char dateText[32];
        // Format as MM/DD
        std::strftime(dateText, sizeof(dateText), "%m/%d", timeinfo);
        timeLabels.push_back(std::string(dateText));
    }
    
    return timeLabels;
}

void Plotter::setMarginRight(unsigned int margin)
{
    chartLayout->setMarginRight(margin);
}

void Plotter::setMarginBottom(unsigned int margin)
{
    chartLayout->setMarginBottom(margin);
}

void Plotter::setMarginLeft(unsigned int margin)
{
    chartLayout->setMarginLeft(margin);
}

// Calculate optimal margins for a specific chart type
void Plotter::calculateOptimalMargins(ChartType chartType)
{
    unsigned int width = chartLayout->getWidth();
    unsigned int height = chartLayout->getHeight();
    
    unsigned int top = calculateTopMargin(chartType, width, height);
    unsigned int right = calculateRightMargin(chartType, width, height);
    unsigned int bottom = calculateBottomMargin(chartType, width, height);
    unsigned int left = calculateLeftMargin(chartType, width, height);
    
    // Special case for line and scatter plots - give them extra vertical space
    // to ensure legends and info boxes don't overlap with the graph
    if (chartType == CHART_LINE || chartType == CHART_SCATTER) {
        // Add an additional buffer to prevent overlap issues
        top = std::max(top, static_cast<unsigned int>(height * 0.18f)); // At least 18% of height
        
        // For shorter graph heights (< 900px), give even more margin
        if (height < 900) {
            top = std::max(top, static_cast<unsigned int>(height * 0.22f)); // Up to 22% for small graphs
        }
    }
    
    chartLayout->setMarginTop(top);
    chartLayout->setMarginRight(right);
    chartLayout->setMarginBottom(bottom);
    chartLayout->setMarginLeft(left);
    
    // Redraw with new margins if needed
    prepareCanvas();
}

// Calculate top margin based on chart type and dimensions
unsigned int Plotter::calculateTopMargin(ChartType chartType, unsigned int width, unsigned int height)
{
    // Base value for top margin (accommodates title and potential legend)
    float baseValue = 0.0f;
    
    switch (chartType) {
        case CHART_HISTOGRAM:
            // Histograms need more space for statistics box at the top
            baseValue = 0.12f; // 12% of height
            break;
        case CHART_CANDLESTICK:
            // Candlestick charts need space for price information
            baseValue = 0.12f; // 12% of height
            break;
        case CHART_CLUSTER:
            // Cluster charts need space for legend and cluster information
            baseValue = 0.09f; // 9% of height 
            break;
        case CHART_LINE:
        case CHART_SCATTER:
            // Line and scatter plots need more space for title, legend, and info boxes
            // Increase from 10% to 15% to prevent overlap with graph
            baseValue = 0.15f; // 15% of height
            break;
        case CHART_DEFAULT:
        default:
            // Default value for standard charts
            baseValue = 0.08f; // 8% of height
            break;
    }
    
    // Calculate top margin as percentage of height with a minimum value
    unsigned int margin = static_cast<unsigned int>(height * baseValue);
    return std::max(margin, 60u); // Minimum 60px
}

// Calculate right margin based on chart type and dimensions
unsigned int Plotter::calculateRightMargin(ChartType chartType, unsigned int width, unsigned int height)
{
    // Base value for right margin (accommodates Y-axis labels and potential legend)
    float baseValue = 0.0f;
    
    switch (chartType) {
        case CHART_HISTOGRAM:
            // Histograms need moderate space for Y-axis labels
            baseValue = 0.06f; // 6% of width
            break;
        case CHART_CANDLESTICK:
            // Candlestick charts need more space for price labels
            baseValue = 0.08f; // 8% of width
            break;
        case CHART_CLUSTER:
            // Cluster charts need moderate space
            baseValue = 0.05f; // 5% of width
            break;
        case CHART_LINE:
        case CHART_SCATTER:
            // Line and scatter plots need more space for precise Y values
            baseValue = 0.1f; // 10% of width
            break;
        case CHART_DEFAULT:
        default:
            // Default value for standard charts
            baseValue = 0.05f; // 5% of width
            break;
    }
    
    // Calculate right margin as percentage of width with a minimum value
    unsigned int margin = static_cast<unsigned int>(width * baseValue);
    return std::max(margin, 80u); // Minimum 80px
}

// Calculate bottom margin based on chart type and dimensions
unsigned int Plotter::calculateBottomMargin(ChartType chartType, unsigned int width, unsigned int height)
{
    // Base value for bottom margin (accommodates X-axis labels)
    float baseValue = 0.0f;
    
    switch (chartType) {
        case CHART_HISTOGRAM:
            // Histograms need more space for bin labels
            baseValue = 0.12f; // 12% of height
            break;
        case CHART_CANDLESTICK:
            // Candlestick charts need space for date labels
            baseValue = 0.15f; // 15% of height
            break;
        case CHART_CLUSTER:
            // Cluster charts need moderate space
            baseValue = 0.08f; // 8% of height
            break;
        case CHART_LINE:
        case CHART_SCATTER:
            // Line and scatter plots need space for X values
            baseValue = 0.1f; // 10% of height
            break;
        case CHART_DEFAULT:
        default:
            // Default value for standard charts
            baseValue = 0.08f; // 8% of height
            break;
    }
    
    // Calculate bottom margin as percentage of height with a minimum value
    unsigned int margin = static_cast<unsigned int>(height * baseValue);
    return std::max(margin, 60u); // Minimum 60px
}

// Calculate left margin based on chart type and dimensions
unsigned int Plotter::calculateLeftMargin(ChartType chartType, unsigned int width, unsigned int height)
{
    // Base value for left margin (accommodates Y-axis labels)
    float baseValue = 0.0f;
    
    switch (chartType) {
        case CHART_HISTOGRAM:
            // Histograms need more space for frequency labels
            baseValue = 0.08f; // 8% of width
            break;
        case CHART_CANDLESTICK:
            // Candlestick charts need space for price labels
            baseValue = 0.08f; // 8% of width
            break;
        case CHART_CLUSTER:
            // Cluster charts need moderate space
            baseValue = 0.05f; // 5% of width
            break;
        case CHART_LINE:
        case CHART_SCATTER:
            // Line and scatter plots need space for precise Y values
            baseValue = 0.04f; // 4% of width
            break;
        case CHART_DEFAULT:
        default:
            // Default value for standard charts
            baseValue = 0.05f; // 5% of width
            break;
    }
    
    // Calculate left margin as percentage of width with a minimum value
    unsigned int margin = static_cast<unsigned int>(width * baseValue);
    return std::max(margin, 80u); // Minimum 80px
}

ChartBuilder Plotter::chart() {
    return ChartBuilder(*this);
}

// New plotChart implementation
void Plotter::plotChart(const std::vector<Series>& seriesList,
                  const std::string& title,
                  const std::string& xAxisLabel,
                  const std::string& yAxisLabel)
{
    if (seriesList.empty()) {
        printf("Error: No series data to plot.\n");
        return;
    }
    
    // Combine all points to calculate global axis ranges
    std::vector<DataMapper::Point> allDataPoints;
    for (size_t i = 0; i < seriesList.size(); ++i) {
        const Series& series = seriesList[i];
        if (series.data.empty()) {
            continue;
        }
        
        // Convert to DataMapper points and add to the combined list
        std::vector<DataMapper::Point> seriesPoints = convertToDataPoints(series.data);
        allDataPoints.insert(allDataPoints.end(), seriesPoints.begin(), seriesPoints.end());
    }
    
    if (allDataPoints.empty()) {
        printf("Error: No valid data points to plot.\n");
        return;
    }
    
    // Calculate overall data ranges for X and Y axes
    DataMapper::AxisRange xRange, yRange;
    xRange = dataMapper->calculateXRange(allDataPoints);
    yRange = dataMapper->calculateYRange(allDataPoints);
    
    // Important: Log the Y range before any potential adjustments
    printf("Series data Y range calculated: [%.2f, %.2f]\n", yRange.min, yRange.max);
    
    // Store these ranges for origin axes if enabled
    if (chartLayout->areOriginAxesVisible()) {
        // If origin (0,0) is within the ranges, adjust them to ensure it's visible
        if ((xRange.min < 0 && xRange.max > 0) || (yRange.min < 0 && yRange.max > 0)) {
            // Ensure X-axis range includes zero if close
            if (xRange.min > -0.1 * (xRange.max - xRange.min)) xRange.min = -0.1 * (xRange.max - xRange.min);
            if (xRange.max < 0.1 * (xRange.max - xRange.min)) xRange.max = 0.1 * (xRange.max - xRange.min);
            
            // Ensure Y-axis range includes zero if close
            if (yRange.min > -0.1 * (yRange.max - yRange.min)) yRange.min = -0.1 * (yRange.max - yRange.min);
            if (yRange.max < 0.1 * (yRange.max - yRange.min)) yRange.max = 0.1 * (yRange.max - yRange.min);
            
            printf("Adjusted ranges for origin axes: X [%.2f, %.2f], Y [%.2f, %.2f]\n", 
                  xRange.min, xRange.max, yRange.min, yRange.max);
            
            // Update the ranges in the DataMapper
            dataMapper->setCurrentXRange(xRange);
            dataMapper->setCurrentYRange(yRange);
        }
    }
    
    // Check if the dataMapper already has valid ranges (might be set by a common range calculation)
    DataMapper::AxisRange currentXRange = dataMapper->getCurrentXRange();
    DataMapper::AxisRange currentYRange = dataMapper->getCurrentYRange();
    
    // Use the stored ranges if they exist and have been explicitly set to include arrows
    bool useStoredRanges = (currentXRange.min != 0.0 || currentXRange.max != 1.0) &&
                         (currentYRange.min != 0.0 || currentYRange.max != 1.0);
    
    if (useStoredRanges) {
        printf("Using pre-calculated data ranges including arrows - X: [%.2f, %.2f], Y: [%.2f, %.2f]\n",
             currentXRange.min, currentXRange.max, currentYRange.min, currentYRange.max);
        
        // Use these ranges instead of the series-only ranges
        xRange = currentXRange;
        yRange = currentYRange;
    } else {
        printf("Setting new data ranges from series only - X: [%.2f, %.2f], Y: [%.2f, %.2f]\n", 
             xRange.min, xRange.max, yRange.min, yRange.max);
             
        // Store the calculated ranges
        dataMapper->setCurrentXRange(xRange);
        dataMapper->setCurrentYRange(yRange);
    }
    
    // Calculate optimal margins for the chart type
    calculateOptimalMargins(CHART_LINE); // Use line chart margins as a base
    
    // Variables for margin storage and positioning
    unsigned int originalTopMargin, originalRightMargin;
    int titleY, legendY;
    
    // Prepare the chart with standard configuration and custom title
    prepareStandardChart(title, xAxisLabel, yAxisLabel, 36, 28, 180,
                       &originalTopMargin, &originalRightMargin, &titleY, &legendY);
    
    // Set up standard axis ticks with yRange, ensuring Y-axis ticks are correct
    setupStandardAxisTicks(xRange, yRange, 70);
    
    // Prepare legend data
    std::vector<std::string> legendLabels;
    std::vector<RGBA> legendColors;
    
    for (size_t i = 0; i < seriesList.size(); ++i) {
        legendLabels.push_back(seriesList[i].name);
        legendColors.push_back(seriesList[i].color);
    }
    
    // Add the legend
    int legendHeight = addLegend(legendLabels, legendColors, chartLayout->getMarginLeft(), legendY, 16);
    
    // Calculate and set a sufficient top margin to ensure legend doesn't overlap with the chart
    // Allow 20px padding below the legend
    unsigned int newTopMargin = legendY + legendHeight + 20;
    chartLayout->setMarginTop(newTopMargin);
    
    // Redraw with the new margin
    prepareCanvas();
    
    // Redraw the title after adjusting margins
    textRenderer->drawText(chartLayout->getMarginLeft(), titleY, title, 
                         colorManager->getElementColor("title"), 36, false);
    
    // Redraw the legend after adjusting margins
    addLegend(legendLabels, legendColors, chartLayout->getMarginLeft(), legendY, 16);
    
    // Plot each series in order
    for (size_t i = 0; i < seriesList.size(); ++i) {
        const Series& series = seriesList[i];
        if (series.data.empty()) {
            continue;
        }
        
        RGBA color = series.color;
        
        if (series.type == SERIES_LINE && series.data.size() >= 2) {
            // Draw as a line series
            // Draw line segments between adjacent points
            for (size_t j = 1; j < series.data.size(); ++j) {
                // Map points to screen coordinates
                DataMapper::Point p1 = dataMapper->mapDataToScreen(
                    series.data[j-1].x, series.data[j-1].y, xRange, yRange);
                DataMapper::Point p2 = dataMapper->mapDataToScreen(
                    series.data[j].x, series.data[j].y, xRange, yRange);
                
                // Draw the line segment with proper clipping
                drawLineSegment(p1.x, p1.y, p2.x, p2.y, color, series.lineWidth, j);
            }
        } else if (series.type == SERIES_SCATTER) {
            // Draw as scatter points
            for (size_t j = 0; j < series.data.size(); ++j) {
                // Map point to screen coordinates
                DataMapper::Point p = dataMapper->mapDataToScreen(
                    series.data[j].x, series.data[j].y, xRange, yRange);
                
                // Draw the point with supersampling
                int ssaaX = ssaaManager->scaleX(p.x);
                int ssaaY = ssaaManager->scaleY(p.y);
                int ssaaSize = ssaaManager->scaleSize(series.pointSize);
                
                // Ensure coordinates are valid
                if (isCoordinateValid(ssaaX, ssaaY)) {
                    shapeRenderer->drawPoint(ssaaX, ssaaY, ssaaSize, color);
                }
            }
        } else if (series.type == SERIES_AREA) {
            // TODO: Implement area charts if needed
            printf("Area charts not yet implemented.\n");
        }
    }
    
    // Restore original margins
    chartLayout->setMarginRight(originalRightMargin);
    chartLayout->setMarginTop(originalTopMargin);
}

// Helper method to prepare cluster colors
std::vector<RGBA> Plotter::prepareClusterColors(int numClusters) {
    std::vector<RGBA> clusterColors;
    
    // If we have 10 clusters, use the special 10-cluster color scheme
    if (numClusters == 10) {
        colorManager->initialize10ClusterScheme();
    }
    
    // Use colors from themeColors
    for (int i = 0; i < numClusters; ++i) {
        clusterColors.push_back(colorManager->getThemeColor(i));
    }
    
    return clusterColors;
}

// Helper method to create cluster legend labels
std::vector<std::string> Plotter::createClusterLegendLabels(int numClusters) {
    std::vector<std::string> legendLabels;
    
    for (int i = 0; i < numClusters; ++i) {
        char labelBuffer[32];
        std::sprintf(labelBuffer, "Cluster %d", i);
        legendLabels.push_back(labelBuffer);
    }
    
    legendLabels.push_back("Centroid");
    
    return legendLabels;
}

void Plotter::plotMultiSeries(const std::vector<std::vector<Point> >& seriesData,
                             const std::vector<std::string>& seriesLabels,
                             const std::vector<RGBA>& seriesColors,
                             const std::vector<bool>& isLineStyleSeries,
                             const std::string& title,
                             const std::string& xAxisLabel,
                             const std::string& yAxisLabel)
{
    // Validate inputs
    if (seriesData.empty() || seriesLabels.size() != seriesData.size() || 
        seriesColors.size() != seriesData.size() || isLineStyleSeries.size() != seriesData.size()) {
        printf("Error: Invalid inputs to plotMultiSeries. Sizes must match.\n");
        return;
    }
    
    // Convert to new Series format and call plotChart
    std::vector<Series> seriesList;
    for (size_t i = 0; i < seriesData.size(); ++i) {
        SeriesType type = isLineStyleSeries[i] ? SERIES_LINE : SERIES_SCATTER;
        Series series(seriesLabels[i], seriesData[i], seriesColors[i], type);
        seriesList.push_back(series);
    }
    
    // Use the new plotChart method
    plotChart(seriesList, title, xAxisLabel, yAxisLabel);
}

// After the plotLine method, add the arrow implementations:

void Plotter::plotArrows(const std::vector<Arrow>& arrows, bool redrawBackground) {
    if (arrows.empty()) {
        printf("Warning: No arrows to plot in plotArrows\n");
        return;
    }
    
    printf("Plotting %lu arrows\n", arrows.size());
    
    // Determine whether to calculate new ranges or use existing ones
    DataMapper::AxisRange xRange, yRange;
    bool calculateNewRanges = redrawBackground;
    
    // Check if DataMapper already has valid ranges (from previous data)
    if (!calculateNewRanges) {
        xRange = dataMapper->getCurrentXRange();
        yRange = dataMapper->getCurrentYRange();
        
        // Verify ranges are valid (not default values)
        bool validRanges = true;
        
        // Check if x-range is valid
        if (xRange.min == xRange.max || 
            std::abs(xRange.max - xRange.min) < 1e-8) {
            validRanges = false;
            printf("Invalid X range detected: [%.2f, %.2f]\n", xRange.min, xRange.max);
        }
        
        // Check if y-range is valid
        if (yRange.min == yRange.max || 
            std::abs(yRange.max - yRange.min) < 1e-8) {
            validRanges = false;
            printf("Invalid Y range detected: [%.2f, %.2f]\n", yRange.min, yRange.max);
        }
        
        if (!validRanges) {
            calculateNewRanges = true;
        } else {
            printf("Using existing ranges - X: [%.2f, %.2f], Y: [%.2f, %.2f]\n", 
                  xRange.min, xRange.max, yRange.min, yRange.max);
            
            // Check if any arrow points fall outside the current range
            bool arrowsOutsideRange = false;
            for (size_t i = 0; i < arrows.size(); i++) {
                if (arrows[i].start.x < xRange.min || arrows[i].start.x > xRange.max ||
                    arrows[i].start.y < yRange.min || arrows[i].start.y > yRange.max ||
                    arrows[i].end.x < xRange.min || arrows[i].end.x > xRange.max ||
                    arrows[i].end.y < yRange.min || arrows[i].end.y > yRange.max) {
                    arrowsOutsideRange = true;
                    printf("Arrow %zu has points outside the current range\n", i);
                    break;
                }
            }
            
            // If any arrows are outside the range, recalculate
            if (arrowsOutsideRange) {
                printf("Some arrows are outside the current range. Calculating expanded range...\n");
                calculateNewRanges = true;
            }
        }
    }
    
    // Calculate new ranges if needed
    if (calculateNewRanges) {
        // Convert all arrow endpoints to data points for range calculation
        std::vector<DataMapper::Point> allDataPoints;
        
        // First, add existing data range points to preserve the current view
        if (!redrawBackground && 
            std::abs(xRange.max - xRange.min) > 1e-8 && 
            std::abs(yRange.max - yRange.min) > 1e-8) {
            // Add the four corners of the current range to preserve it
            allDataPoints.push_back(DataMapper::Point(xRange.min, yRange.min));
            allDataPoints.push_back(DataMapper::Point(xRange.min, yRange.max));
            allDataPoints.push_back(DataMapper::Point(xRange.max, yRange.min));
            allDataPoints.push_back(DataMapper::Point(xRange.max, yRange.max));
        }
        
        // Add all arrow points
        for (size_t i = 0; i < arrows.size(); ++i) {
            allDataPoints.push_back(DataMapper::Point(arrows[i].start.x, arrows[i].start.y));
            allDataPoints.push_back(DataMapper::Point(arrows[i].end.x, arrows[i].end.y));
            printf("Arrow %zu: (%.2f, %.2f) -> (%.2f, %.2f)\n", i, 
                   arrows[i].start.x, arrows[i].start.y, arrows[i].end.x, arrows[i].end.y);
        }
        
        // Calculate appropriate ranges to include all arrows
        if (!allDataPoints.empty()) {
            xRange = dataMapper->calculateXRange(allDataPoints);
            yRange = dataMapper->calculateYRange(allDataPoints);
            
            // Add padding to ensure arrows are fully visible
            double xPadding = (xRange.max - xRange.min) * 0.1;
            double yPadding = (yRange.max - yRange.min) * 0.1;
            
            // Ensure minimum padding
            xPadding = std::max(xPadding, 0.5);
            yPadding = std::max(yPadding, 0.5);
            
            // Apply padding
            xRange.min -= xPadding;
            xRange.max += xPadding;
            yRange.min -= yPadding;
            yRange.max += yPadding;
            
            printf("Using new ranges - X: [%.2f, %.2f], Y: [%.2f, %.2f]\n", 
                   xRange.min, xRange.max, yRange.min, yRange.max);
        } else {
            // Fallback to default ranges if no data points were added
            xRange = DataMapper::AxisRange(-10.0, 10.0);
            yRange = DataMapper::AxisRange(-10.0, 10.0);
            
            printf("No valid data points for range calculation. Using default ranges - X: [%.2f, %.2f], Y: [%.2f, %.2f]\n", 
                   xRange.min, xRange.max, yRange.min, yRange.max);
        }
    }
    
    // If we need to prepare a new chart background
    if (redrawBackground) {
        // Calculate optimal margins
        calculateOptimalMargins(CHART_LINE);
        
        // Variables for margin storage and positioning
        unsigned int originalTopMargin, originalRightMargin;
        int titleY, legendY;
        
        // Prepare the chart with standard configuration
        prepareStandardChart("Arrow Visualization", "X Value", "Y Value", 36, 28, 160,
                          &originalTopMargin, &originalRightMargin, &titleY, &legendY);
        
        // Set up standard axis ticks with 50px Y-axis label offset
        setupStandardAxisTicks(xRange, yRange, 50);
        
        // Store the current data ranges in the DataMapper
        dataMapper->setCurrentXRange(xRange);
        dataMapper->setCurrentYRange(yRange);
    } else if (calculateNewRanges) {
        // If we've calculated new ranges but aren't redrawing the background,
        // make sure we update DataMapper with the new ranges
        dataMapper->setCurrentXRange(xRange);
        dataMapper->setCurrentYRange(yRange);
        
        // Update axis ticks to match the new ranges without redrawing background
        setupStandardAxisTicks(xRange, yRange, 50);
    }
    
    // Draw each arrow
    for (size_t i = 0; i < arrows.size(); ++i) {
        // Get current ranges directly from DataMapper to ensure consistency
        DataMapper::AxisRange currentXRange = dataMapper->getCurrentXRange();
        DataMapper::AxisRange currentYRange = dataMapper->getCurrentYRange();
        
        // Log actual Y-axis range being used for mapping
        printf("Using Y-range for arrow %zu: [%.2f, %.2f]\n", i, currentYRange.min, currentYRange.max);
        
        // Map arrow start point to screen coordinates using current ranges
        DataMapper::Point startPoint = dataMapper->mapDataToScreen(
            arrows[i].start.x, arrows[i].start.y, currentXRange, currentYRange);
        
        // Map arrow end point to screen coordinates using current ranges
        DataMapper::Point endPoint = dataMapper->mapDataToScreen(
            arrows[i].end.x, arrows[i].end.y, currentXRange, currentYRange);
        
        printf("Arrow %zu screen coords: (%.2f, %.2f) -> (%.2f, %.2f)\n", 
               i, startPoint.x, startPoint.y, endPoint.x, endPoint.y);
        
        // Scale coordinates and sizes for supersampling
        int ssaaStartX = ssaaManager->scaleX(startPoint.x);
        int ssaaStartY = ssaaManager->scaleY(startPoint.y);
        int ssaaEndX = ssaaManager->scaleX(endPoint.x);
        int ssaaEndY = ssaaManager->scaleY(endPoint.y);
        int ssaaLineWidth = ssaaManager->scaleSize(arrows[i].lineWidth);
        int ssaaArrowheadSize = ssaaManager->scaleSize(arrows[i].arrowheadSize);
        
        // Get appropriate color
        RGBA useColor = arrows[i].color;
        if (useColor.r == 0 && useColor.g == 0 && useColor.b == 0 && useColor.a == 0) {
            useColor = colorManager->getThemeColor(i % 10); // Cycle through theme colors
        }
        
        // Verify coordinates are within drawable area
        bool coordsValid = isCoordinateValid(ssaaStartX, ssaaStartY) && isCoordinateValid(ssaaEndX, ssaaEndY);
        if (!coordsValid) {
            printf("Warning: Arrow %zu has coordinates outside drawable area. Clipping to viewport.\n", i);
            
            // Clamp coordinates to viewport
            int maxX = static_cast<int>(ssaaManager->getWidth());
            int maxY = static_cast<int>(ssaaManager->getHeight());
            
            ssaaStartX = clampToViewport(ssaaStartX, maxX);
            ssaaStartY = clampToViewport(ssaaStartY, maxY);
            ssaaEndX = clampToViewport(ssaaEndX, maxX);
            ssaaEndY = clampToViewport(ssaaEndY, maxY);
        }
        
        // Draw the arrow using the ShapeRenderer
        shapeRenderer->drawArrow(
            ssaaStartX, ssaaStartY, ssaaEndX, ssaaEndY, useColor, ssaaLineWidth, ssaaArrowheadSize
        );
    }
}

void Plotter::plotArrow(const Arrow& arrow, bool redrawBackground) {
    // Call the more detailed implementation
    plotArrow(arrow.start.x, arrow.start.y, arrow.end.x, arrow.end.y, 
              arrow.color, arrow.lineWidth, arrow.arrowheadSize, redrawBackground);
}

void Plotter::plotArrow(double startX, double startY, double endX, double endY, 
                       const RGBA& color, int lineWidth, int arrowheadSize, bool redrawBackground) {
    // If the color is not specified (all zeros), use the first theme color
    RGBA useColor = color;
    if (color.r == 0 && color.g == 0 && color.b == 0 && color.a == 0) {
        useColor = colorManager->getThemeColor(0);
    }
    
    printf("Plotting arrow: (%.2f, %.2f) -> (%.2f, %.2f)\n", startX, startY, endX, endY);
    
    // Determine whether to calculate new ranges or use existing ones
    DataMapper::AxisRange xRange, yRange;
    bool calculateNewRanges = redrawBackground;
    
    // Check if DataMapper already has valid ranges (from previous data)
    if (!calculateNewRanges) {
        xRange = dataMapper->getCurrentXRange();
        yRange = dataMapper->getCurrentYRange();
        
        // Verify ranges are valid (not default values)
        bool validRanges = true;
        
        // Check if x-range is valid
        if (xRange.min == xRange.max || 
            std::abs(xRange.max - xRange.min) < 1e-8) {
            validRanges = false;
            printf("Invalid X range detected: [%.2f, %.2f]\n", xRange.min, xRange.max);
        }
        
        // Check if y-range is valid
        if (yRange.min == yRange.max || 
            std::abs(yRange.max - yRange.min) < 1e-8) {
            validRanges = false;
            printf("Invalid Y range detected: [%.2f, %.2f]\n", yRange.min, yRange.max);
        }
        
        if (!validRanges) {
            calculateNewRanges = true;
        } else {
            printf("Using existing ranges - X: [%.2f, %.2f], Y: [%.2f, %.2f]\n", 
                   xRange.min, xRange.max, yRange.min, yRange.max);
            
            // Check if arrow points fall outside the current range
            if (startX < xRange.min || startX > xRange.max ||
                startY < yRange.min || startY > yRange.max ||
                endX < xRange.min || endX > xRange.max ||
                endY < yRange.min || endY > yRange.max) {
                printf("Arrow has points outside the current range. Calculating expanded range...\n");
                calculateNewRanges = true;
            }
        }
    }
    
    // Calculate new ranges if needed
    if (calculateNewRanges) {
        // Convert data points to a full vector for range determination
        std::vector<Point> points;
        
        // First, if we have valid existing ranges, add them to preserve the current view
        if (!redrawBackground && 
            std::abs(xRange.max - xRange.min) > 1e-8 && 
            std::abs(yRange.max - yRange.min) > 1e-8) {
            // Add the four corners of the current range to preserve it
            points.push_back(Point(xRange.min, yRange.min));
            points.push_back(Point(xRange.min, yRange.max));
            points.push_back(Point(xRange.max, yRange.min));
            points.push_back(Point(xRange.max, yRange.max));
        }
        
        // Add arrow endpoints
        points.push_back(Point(startX, startY));
        points.push_back(Point(endX, endY));
        
        // Convert to DataMapper::Point vector
        std::vector<DataMapper::Point> dataPoints = convertToDataPoints(points);
        
        // Calculate axis ranges for scaling
        if (!dataPoints.empty()) {
            calculateDataRanges(dataPoints, xRange, yRange);
            
            // Add padding to ensure arrows are fully visible
            double xPadding = (xRange.max - xRange.min) * 0.1;
            double yPadding = (yRange.max - yRange.min) * 0.1;
            
            // Ensure minimum padding
            xPadding = std::max(xPadding, 0.5);
            yPadding = std::max(yPadding, 0.5);
            
            // Apply padding
            xRange.min -= xPadding;
            xRange.max += xPadding;
            yRange.min -= yPadding;
            yRange.max += yPadding;
            
            printf("Using new ranges - X: [%.2f, %.2f], Y: [%.2f, %.2f]\n", 
                   xRange.min, xRange.max, yRange.min, yRange.max);
        } else {
            // Fallback to default ranges if no data points were added
            xRange = DataMapper::AxisRange(-10.0, 10.0);
            yRange = DataMapper::AxisRange(-10.0, 10.0);
            
            printf("No valid data points for range calculation. Using default ranges - X: [%.2f, %.2f], Y: [%.2f, %.2f]\n", 
                   xRange.min, xRange.max, yRange.min, yRange.max);
        }
    }
    
    if (redrawBackground) {
        // Calculate optimal margins
        calculateOptimalMargins(CHART_LINE);
        
        // Variables for margin storage and positioning
        unsigned int originalTopMargin, originalRightMargin;
        int titleY, legendY;
        
        // Prepare the chart with standard configuration
        prepareStandardChart("Arrow Visualization", "X Value", "Y Value", 36, 28, 160,
                          &originalTopMargin, &originalRightMargin, &titleY, &legendY);
        
        // Set up standard axis ticks with 50px Y-axis label offset
        setupStandardAxisTicks(xRange, yRange, 50);
        
        // Store the current data ranges in the DataMapper
        dataMapper->setCurrentXRange(xRange);
        dataMapper->setCurrentYRange(yRange);
    } else if (calculateNewRanges) {
        // If we've calculated new ranges but aren't redrawing the background,
        // make sure we update DataMapper with the new ranges
        dataMapper->setCurrentXRange(xRange);
        dataMapper->setCurrentYRange(yRange);
        
        // Update axis ticks to match the new ranges without redrawing background
        setupStandardAxisTicks(xRange, yRange, 50);
    }
    
    // Get current ranges directly from DataMapper to ensure consistency  
    DataMapper::AxisRange currentXRange = dataMapper->getCurrentXRange();
    DataMapper::AxisRange currentYRange = dataMapper->getCurrentYRange();
    
    // Log actual Y-axis range being used for mapping
    printf("Using Y-range for single arrow: [%.2f, %.2f]\n", currentYRange.min, currentYRange.max);
    
    // Map arrow start point to screen coordinates using current ranges
    DataMapper::Point startPoint = dataMapper->mapDataToScreen(startX, startY, currentXRange, currentYRange);
    
    // Map arrow end point to screen coordinates using current ranges
    DataMapper::Point endPoint = dataMapper->mapDataToScreen(endX, endY, currentXRange, currentYRange);
    
    printf("Arrow screen coords: (%.2f, %.2f) -> (%.2f, %.2f)\n", 
           startPoint.x, startPoint.y, endPoint.x, endPoint.y);
    
    // Scale coordinates and sizes for supersampling
    int ssaaStartX = ssaaManager->scaleX(startPoint.x);
    int ssaaStartY = ssaaManager->scaleY(startPoint.y);
    int ssaaEndX = ssaaManager->scaleX(endPoint.x);
    int ssaaEndY = ssaaManager->scaleY(endPoint.y);
    int ssaaLineWidth = ssaaManager->scaleSize(lineWidth);
    int ssaaArrowheadSize = ssaaManager->scaleSize(arrowheadSize);
    
    // Verify coordinates are within drawable area
    bool coordsValid = isCoordinateValid(ssaaStartX, ssaaStartY) && isCoordinateValid(ssaaEndX, ssaaEndY);
    if (!coordsValid) {
        printf("Warning: Arrow has coordinates outside drawable area. Clipping to viewport.\n");
        
        // Clamp coordinates to viewport
        int maxX = static_cast<int>(ssaaManager->getWidth());
        int maxY = static_cast<int>(ssaaManager->getHeight());
        
        ssaaStartX = clampToViewport(ssaaStartX, maxX);
        ssaaStartY = clampToViewport(ssaaStartY, maxY);
        ssaaEndX = clampToViewport(ssaaEndX, maxX);
        ssaaEndY = clampToViewport(ssaaEndY, maxY);
    }
    
    // Draw the arrow using the ShapeRenderer
    shapeRenderer->drawArrow(
        ssaaStartX, ssaaStartY, ssaaEndX, ssaaEndY, useColor, ssaaLineWidth, ssaaArrowheadSize
    );
}

// Add implementation for setShowOriginAxes method
void Plotter::setShowOriginAxes(bool show)
{
    chartLayout->setShowOriginAxes(show);
    
    // Redraw the background when visibility changes
    prepareCanvas();
}

// Add implementation for drawOriginAxes method
void Plotter::drawOriginAxes(double xMin, double xMax, double yMin, double yMax)
{
    // Create ranges from the provided values
    DataMapper::AxisRange xRange(xMin, xMax);
    DataMapper::AxisRange yRange(yMin, yMax);
    
    // Store the ranges both in Plotter and DataMapper
    currentXAxisRange = xRange;
    currentYAxisRange = yRange;
    
    // Update the DataMapper's current ranges
    dataMapper->setCurrentXRange(xRange);
    dataMapper->setCurrentYRange(yRange);
    
    // Draw the origin axes with the specified ranges
    gridRenderer->drawOriginAxes(xMin, xMax, yMin, yMax);
}

} // namespace shmea 
