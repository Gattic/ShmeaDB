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
      hasLabeledHistogramData(false),
      hasCandlestickData(false),
      bullishColor(0x03, 0xC0, 0x3C, 0xFF),
      bearishColor(0xFF, 0x47, 0x45, 0xFF),
      hasClusterData(false),
      alignCentroids(true)  // Default to aligning centroids with cluster centers
{
    // Initialize boolean flags
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
    // Save original state for restoration at the end
    plotter.saveState();
    
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
    if (!allDataPoints.empty()) {
        DataMapper::AxisRange xRange = plotter.dataMapper->calculateXRange(allDataPoints);
        DataMapper::AxisRange yRange = plotter.dataMapper->calculateYRange(allDataPoints);
        
        // Add padding to ensure all elements are visible
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
        
        printf("Chart display range - X: [%.2f, %.2f], Y: [%.2f, %.2f]\n", 
               xRange.min, xRange.max, yRange.min, yRange.max);
        
        // If origin axes are enabled, adjust ranges to include origin
        if (plotter.chartLayout->areOriginAxesVisible()) {
            // Make sure origin (0,0) is included in both ranges
            if (xRange.min >= 0) xRange.min = -xRange.max * 0.1;
            if (xRange.max <= 0) xRange.max = -xRange.min * 0.1;
            if (yRange.min >= 0) yRange.min = -yRange.max * 0.1;
            if (yRange.max <= 0) yRange.max = -yRange.min * 0.1;
            
            // Update origin axes with these ranges
            plotter.currentXAxisRange = xRange;
            plotter.currentYAxisRange = yRange;
        }
        
        // Store the ranges in the DataMapper for consistent scaling of all elements
        plotter.dataMapper->setCurrentXRange(xRange);
        plotter.dataMapper->setCurrentYRange(yRange);
    }
    
    // Set optimal margins for the chart type - MUST happen BEFORE prepareCanvas
    plotter.calculateOptimalMargins(chartType);
    
    // Prepare the canvas after ranges are set
    plotter.prepareCanvas();
    
    // Set up standard axis ticks with the common data range
    if (!allDataPoints.empty()) {
        DataMapper::AxisRange xRange = plotter.dataMapper->getCurrentXRange();
        DataMapper::AxisRange yRange = plotter.dataMapper->getCurrentYRange();
        plotter.setupStandardAxisTicks(xRange, yRange, 50);
    }
    
    // Render the appropriate chart based on the data provided
    if (!series.empty()) {
        // Plot the series data
        plotter.plotChart(series);
    }
    else if (hasLabeledHistogramData) {
        // Plot histogram with custom labels
        plotter.plotHistogramWithLabels(histogramBins, histogramLabels, histogramColor);
    }
    else if (hasHistogramData) {
        // Plot standard histogram
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
        printf("Drawing %lu arrows in the coordinate system - X: [%.2f, %.2f], Y: [%.2f, %.2f]\n", 
               arrows.size(), 
               plotter.dataMapper->getCurrentXRange().min, 
               plotter.dataMapper->getCurrentXRange().max,
               plotter.dataMapper->getCurrentYRange().min, 
               plotter.dataMapper->getCurrentYRange().max);
               
        // Plot the arrows without redrawing the background
        // This will use the DataMapper's current X and Y ranges that we already set
        plotter.plotArrows(arrows, false);
    }
    
    // Save the chart
    plotter.saveAsPNG(filename, folder);
    
    // Restore original state after chart is saved
    plotter.restoreState();
}

// Add after ChartBuilder::axes
ChartBuilder& ChartBuilder::originAxes(bool show) {
    plotter.setShowOriginAxes(show);
    
    // If enabling origin axes, make sure we have proper initial ranges set
    if (show) {
        // Get current ranges from DataMapper
        DataMapper::AxisRange xRange = plotter.dataMapper->getCurrentXRange();
        DataMapper::AxisRange yRange = plotter.dataMapper->getCurrentYRange();
        
        // Always force ranges to include the origin point (0,0)
        if (xRange.min >= 0) xRange.min = -10.0;  // Default negative range if only positive values
        if (xRange.max <= 0) xRange.max = 10.0;   // Default positive range if only negative values
        if (yRange.min >= 0) yRange.min = -10.0;  // Default negative range if only positive values
        if (yRange.max <= 0) yRange.max = 10.0;   // Default positive range if only negative values
        
        // Ensure reasonable proportions if we have data
        if (xRange.min < 0 && xRange.max > 0) {
            // Make sure negative and positive sides have reasonable proportions
            double negXSize = std::abs(xRange.min);
            double posXSize = xRange.max;
            
            // Balance the axis if one side is much larger than the other
            if (negXSize > posXSize * 5) {
                xRange.min = -posXSize * 2; // Reduce negative side
            } else if (posXSize > negXSize * 5) {
                xRange.max = negXSize * 2;  // Reduce positive side
            }
        }
        
        if (yRange.min < 0 && yRange.max > 0) {
            // Make sure negative and positive sides have reasonable proportions
            double negYSize = std::abs(yRange.min);
            double posYSize = yRange.max;
            
            // Balance the axis if one side is much larger than the other
            if (negYSize > posYSize * 5) {
                yRange.min = -posYSize * 2; // Reduce negative side
            } else if (posYSize > negYSize * 5) {
                yRange.max = negYSize * 2;  // Reduce positive side
            }
        }
        
        // Make ranges more balanced if very different
        double xSpan = xRange.max - xRange.min;
        double ySpan = yRange.max - yRange.min;
        
        if (xSpan > ySpan * 5) {
            // X range is much wider than Y range, make Y more proportionate
            double yCenter = (yRange.min + yRange.max) / 2;
            double halfXSpan = xSpan / 2;
            yRange.min = yCenter - halfXSpan * 0.8;
            yRange.max = yCenter + halfXSpan * 0.8;
        } else if (ySpan > xSpan * 5) {
            // Y range is much taller than X range, make X more proportionate
            double xCenter = (xRange.min + xRange.max) / 2;
            double halfYSpan = ySpan / 2;
            xRange.min = xCenter - halfYSpan * 0.8;
            xRange.max = xCenter + halfYSpan * 0.8;
        }
        
        // If we still don't have valid ranges (no data added yet), use defaults
        if (xRange.min >= xRange.max || yRange.min >= yRange.max) {
            xRange.min = -10.0;
            xRange.max = 10.0;
            yRange.min = -10.0;
            yRange.max = 10.0;
        }
        
        // Store the ranges for the plotter and data mapper
        plotter.currentXAxisRange = xRange;
        plotter.currentYAxisRange = yRange;
        plotter.dataMapper->setCurrentXRange(xRange);
        plotter.dataMapper->setCurrentYRange(yRange);
        
        // Force drawing the origin axes with these ranges 
        plotter.drawOriginAxes(xRange.min, xRange.max, yRange.min, yRange.max);
        
        // Force canvas redraw to apply the changes
        plotter.prepareCanvas();
    }
    
    return *this;
}

// Add to the ChartBuilder class implementations

ChartBuilder& ChartBuilder::alignCentroidsWithClusters(bool align) {
    // Store the alignment preference
    alignCentroids = align;
    
    // Pass the setting to the plotter
    plotter.setAlignCentroidsWithClusters(align);
    
    return *this;
}

// Let's implement the new method for histogram with custom labels
ChartBuilder& ChartBuilder::addHistogramDataWithLabels(const std::vector<int>& bins,
                                                    const std::vector<std::string>& labels,
                                                    const RGBA& color)
{
    if (bins.empty() || labels.empty() || bins.size() != labels.size()) {
        printf("Warning: Invalid input for histogram with labels. Bins and labels must be non-empty and have the same size.\n");
        return *this;
    }
    
    this->hasLabeledHistogramData = true;
    this->histogramBins = bins;
    this->histogramLabels = labels;
    this->histogramColor = color;
    this->chartType = CHART_HISTOGRAM;
    
    // Disable standard histogram display
    this->hasHistogramData = false;
    return *this;
}

//
// Plotter Implementation
//
bool Plotter::fontLoaded = false;
FT_Library Plotter::ft = NULL;
FT_Face Plotter::face = NULL;

bool Plotter::logoLoaded = false;
bool Plotter::hasLogo = false;
Image Plotter::logoImage = Image();
int Plotter::logoWidth = 0;
int Plotter::logoHeight = 0;
int Plotter::logoWidthScaled = 0;
int Plotter::logoHeightScaled = 0;

Plotter::Plotter(unsigned int width, unsigned int height, unsigned int ssaa_factor)
    : currentXAxisRange(-10.0, 10.0),   // Default X range for origin axes
      currentYAxisRange(-10.0, 10.0),   // Default Y range for origin axes
      savedMarginTop(0),
      savedMarginRight(0),
      savedMarginBottom(0),
      savedMarginLeft(0),
      chartTitle(""),
      chartTitleFontSize(36)
{
    // Initialize boolean flags
    forceAlignCentroids = true;  // Default to aligning centroids with cluster centers
    
    initialize_font("fonts/font.ttf");
    loadLogo("logo.png", true);

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
    
    // Draw origin-centered axes if enabled (check this first)
    if (chartLayout->areOriginAxesVisible()) {
        // Use stored ranges that have already been prepared for origin axes
        DataMapper::AxisRange xRange = currentXAxisRange;
        DataMapper::AxisRange yRange = currentYAxisRange;
        
        // Ensure we have valid ranges that include the origin
        bool rangesNeedFix = false;
        
        // Check if ranges include the origin properly
        if (xRange.min >= 0 || xRange.max <= 0 || yRange.min >= 0 || yRange.max <= 0) {
            rangesNeedFix = true;
        }
        
        // Check if ranges are valid
        if (xRange.min >= xRange.max || yRange.min >= yRange.max) {
            rangesNeedFix = true;
        }
        
        // Fix ranges if needed
        if (rangesNeedFix) {
            if (xRange.min >= 0 || xRange.max <= 0) {
                xRange.min = -10.0;
                xRange.max = 10.0;
            }
            
            if (yRange.min >= 0 || yRange.max <= 0) {
                yRange.min = -10.0;
                yRange.max = 10.0;
            }
            
            // Update our stored ranges
            currentXAxisRange = xRange;
            currentYAxisRange = yRange;
            dataMapper->setCurrentXRange(xRange);
            dataMapper->setCurrentYRange(yRange);
            
            printf("Fixed origin axes ranges: X=[%.2f, %.2f], Y=[%.2f, %.2f]\n", 
                   xRange.min, xRange.max, yRange.min, yRange.max);
        }
        
        // Draw the origin axes with the proper ranges
        gridRenderer->drawOriginAxes(xRange.min, xRange.max, yRange.min, yRange.max);
    }
    // Draw regular axes if enabled and origin axes are not
    else if (chartLayout->areAxesVisible()) {
        gridRenderer->drawAxes();
    }
    
    // Redraw title if one is set
    if (!chartTitle.empty()) {
        int titleY = 30;  // Standard position below top margin
        RGBA titleColor = colorManager->getElementColor("title");
        textRenderer->drawText(chartLayout->getMarginLeft(), titleY, 
                            chartTitle, titleColor, chartTitleFontSize, false);
    }
}

void Plotter::addTitle(const std::string& text, unsigned int fontSize)
{
    // Store the title for later use when canvas is redrawn
    chartTitle = text;
    chartTitleFontSize = fontSize;
    
    // Draw the title immediately
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

void Plotter::loadLogo(const std::string& logoPath, bool isInitializationCall)
{
    if(logoLoaded)
    {
	if(!isInitializationCall)
	{
	    chartLayout->setLogoWidth(logoWidthScaled);
	    chartLayout->setLogoHeight(logoHeightScaled);
	}

	return;
    }

    // Try to load the logo image from the file
    try {
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
	    if(!isInitializationCall)
	    {
		chartLayout->setLogoWidth(scaledWidth);
		chartLayout->setLogoHeight(scaledHeight);
	    }

	    logoWidth = originalWidth;
	    logoHeight = originalHeight;
	    logoWidthScaled = scaledWidth;
	    logoHeightScaled = scaledHeight;
        } else {
            hasLogo = false;
            
            // Reset logo dimensions in chartLayout
	    if(!isInitializationCall)
	    {
		chartLayout->setLogoWidth(0);
		chartLayout->setLogoHeight(0);
	    }
            
            printf("Failed to load logo from: %s (invalid dimensions)\n", logoPath.c_str());
        }
    } catch (...) {
        hasLogo = false;
        
        // Reset logo dimensions in chartLayout
	if(!isInitializationCall)
	{
	    chartLayout->setLogoWidth(0);
	    chartLayout->setLogoHeight(0);
	}

	logoWidth = 0;
	logoHeight = 0;
	logoWidthScaled = 0;
	logoHeightScaled = 0;
        
        printf("Error loading logo from: %s (exception occurred)\n", logoPath.c_str());
    }

    // This is in order load the logo without using it for this particular class
    if(isInitializationCall)
	hasLogo = false;
    else
	logoLoaded = true;
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
        
        // Use stored title if available, otherwise use config title
        std::string displayTitle = !chartTitle.empty() ? chartTitle : config.title;
        unsigned int displayFontSize = !chartTitle.empty() ? chartTitleFontSize : config.titleFontSize;
        
        // Add title - use exact parameters from plotter.cpp
        RGBA titleColor = colorManager->getElementColor("title");
        textRenderer->drawText(chartLayout->getMarginLeft(), *titleY, displayTitle, 
                           titleColor, displayFontSize, false);
        
        // Calculate legendY position - exactly like plotter.cpp
        if (legendY) {
            int titleHeight = displayFontSize - 10;
            *legendY = *titleY + titleHeight - 20; // Position legend closely below title - exact from plotter.cpp
        }
    }
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
    int estimatedLegendHeight = chartStyler->calculateInfoBoxHeight(legendLabels, 16);
    
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
    std::string displayTitle = !chartTitle.empty() ? chartTitle : title;
    unsigned int displayFontSize = !chartTitle.empty() ? chartTitleFontSize : 42;
    textRenderer->drawText(chartLayout->getMarginLeft(), titleY, displayTitle, 
                      colorManager->getElementColor("title"), displayFontSize, false);
    
    // Now add the legend
    int actualLegendHeight = chartStyler->addLegend(legendLabels, legendColors, chartLayout->getMarginLeft(), legendY, 16);
    
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
    
    // Remove margin restoration - keeping consistent margins for subsequent elements
    // chartLayout->setMarginRight(originalRightMargin);
    // chartLayout->setMarginTop(originalTopMargin);
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
    std::string displayTitle = !chartTitle.empty() ? chartTitle : title;
    unsigned int displayFontSize = !chartTitle.empty() ? chartTitleFontSize : 36;
    textRenderer->drawText(chartLayout->getMarginLeft(), titleY, displayTitle, 
                       colorManager->getElementColor("title"), displayFontSize, false);
    
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
    
    // Remove margin restoration - keeping consistent margins for subsequent elements
    // chartLayout->setMarginRight(originalRightMargin);
    // chartLayout->setMarginTop(originalTopMargin);
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
    
    // Print diagnostic information about input data
    printf("Plotting %zu data points with %zu labels and %zu centroids\n", 
           data.size(), labels.size(), centroids.size());
           
    // Verify centroid format
    for (size_t i = 0; i < centroids.size(); i++) {
        if (centroids[i].size() < 2) {
            printf("Warning: Centroid %zu does not have enough dimensions (has %zu, needs 2)\n", 
                   i, centroids[i].size());
        }
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
    std::string displayTitle = !chartTitle.empty() ? chartTitle : title;
    unsigned int displayFontSize = !chartTitle.empty() ? chartTitleFontSize : 36;
    textRenderer->drawText(chartLayout->getMarginLeft(), titleY, displayTitle, 
                      colorManager->getElementColor("title"), displayFontSize, false);
    
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
    
    // Calculate additional range that includes centroids to ensure they're visible
    if (!centroids.empty()) {
        // Start with data ranges
        double minX = xRange.min;
        double maxX = xRange.max;
        double minY = yRange.min;
        double maxY = yRange.max;
        
        // Extend range to include centroids
        for (size_t i = 0; i < centroids.size(); i++) {
            if (centroids[i].size() >= 2) {
                minX = std::min(minX, centroids[i][0]);
                maxX = std::max(maxX, centroids[i][0]);
                minY = std::min(minY, centroids[i][1]);
                maxY = std::max(maxY, centroids[i][1]);
            }
        }
        
        // Update ranges
        xRange.min = minX;
        xRange.max = maxX;
        yRange.min = minY;
        yRange.max = maxY;
        
        // Add padding
        double xPadding = (xRange.max - xRange.min) * xRange.padding;
        double yPadding = (yRange.max - yRange.min) * yRange.padding;
        
        // Ensure minimum padding
        if (xPadding < 1e-10) xPadding = 1.0;
        if (yPadding < 1e-10) yPadding = 1.0;
        
        xRange.min -= xPadding;
        xRange.max += xPadding;
        yRange.min -= yPadding;
        yRange.max += yPadding;
        
        // Update DataMapper with the extended ranges
        dataMapper->setCurrentXRange(xRange);
        dataMapper->setCurrentYRange(yRange);
        
        printf("Extended ranges to include centroids: X=[%.2f, %.2f], Y=[%.2f, %.2f]\n",
               xRange.min, xRange.max, yRange.min, yRange.max);
    }
    
    // Draw axis ticks with appropriate ranges
    setupStandardAxisTicks(xRange, yRange, 50);
    
    // Vectors to store cluster visualization data
    std::vector<std::vector<DataMapper::Point> > clusterPoints(numClusters);
    std::vector<std::vector<std::pair<int, int> > > clusterPointPairs(numClusters);
    std::vector<DataMapper::Point> clusterCenters;
    std::vector<int> clusterRadii;
    
    // Store the calculated data-space cluster centers for comparison with centroids
    std::vector<std::pair<double, double> > calculatedDataCenters;
    
    // Draw each cluster's points and collect data for visualization
    for (int cluster = 0; cluster < numClusters; ++cluster) {
        // Calculate and store bounds for this cluster
        chartStyler->calculateClusterBounds(data, labels, cluster, clusterCenters, clusterRadii, xRange, yRange);
        
        // Calculate the data-space center for this cluster
        double sumX = 0.0;
        double sumY = 0.0;
        int count = 0;
        
        for (size_t i = 0; i < data.size(); ++i) {
            if (labels[i] == cluster) {
                sumX += data[i][0];
                sumY += data[i][1];
                count++;
            }
        }
        
        // Store the calculated data-space center if we have points in this cluster
        if (count > 0) {
            double avgX = sumX / count;
            double avgY = sumY / count;
            calculatedDataCenters.push_back(std::make_pair(avgX, avgY));
            
            // Print debug info about calculated cluster center and provided centroid
            if (cluster < static_cast<int>(centroids.size()) && centroids[cluster].size() >= 2) {
                printf("Cluster %d: Calculated center (%.2f, %.2f), Provided centroid (%.2f, %.2f)\n",
                      cluster, avgX, avgY, centroids[cluster][0], centroids[cluster][1]);
            }
        } else {
            // Add a dummy entry to maintain index alignment
            calculatedDataCenters.push_back(std::make_pair(0.0, 0.0));
        }
        
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
    
    // Prepare a vector of centroids with coordinates matching cluster centers
    std::vector<std::vector<double> > adjustedCentroids = centroids;
    
    // If any cluster doesn't have a centroid, create one at the cluster center
    if (!clusterCenters.empty() && centroids.size() < clusterCenters.size()) {
        std::vector<double> dummyCentroid(2, 0.0);
        adjustedCentroids.resize(clusterCenters.size(), dummyCentroid);
    }
    
    // Decide which centroids to use: the original ones or ones that are aligned with cluster centers
    // Set this to true to position centroids exactly at the cluster centers
    // Set to false to use the original centroids provided by the user
    //bool forceAlignCentroids = true;  // Replace this local variable with the instance variable
    
    if (forceAlignCentroids) {
        // Create a map from data-space to screen-space for each cluster center
        // This ensures perfect alignment of centroids with cluster centers
        for (size_t i = 0; i < clusterCenters.size() && i < adjustedCentroids.size(); i++) {
            // We need to back-map from screen coordinates to data coordinates
            // First get the cluster center in screen space
            DataMapper::Point screenCenter = clusterCenters[i];
            
            // Find range extents for calculating ratios
            double xSpan = xRange.max - xRange.min;
            double ySpan = yRange.max - yRange.min;
            
            // Ensure minimum spans to prevent division by zero
            if (xSpan < 1e-10) xSpan = 1.0;
            if (ySpan < 1e-10) ySpan = 1.0;
            
            // Calculate plot dimensions
            int plotWidth = chartLayout->getPlotWidth();
            int plotHeight = chartLayout->getPlotHeight();
            
            // Calculate normalized position within the plot area
            double normX = (screenCenter.x - chartLayout->getMarginLeft()) / static_cast<double>(plotWidth);
            double normY = 1.0 - (screenCenter.y - chartLayout->getMarginTop()) / static_cast<double>(plotHeight);
            
            // Map back to data space
            double dataX = xRange.min + normX * xSpan;
            double dataY = yRange.min + normY * ySpan;
            
            // Store the derived data coordinates in the adjusted centroids vector
            if (adjustedCentroids[i].size() >= 2) {
                adjustedCentroids[i][0] = dataX;
                adjustedCentroids[i][1] = dataY;
                
                printf("Cluster %zu: Adjusted centroid to (%.2f, %.2f) to match circle center\n", 
                      i, dataX, dataY);
            }
        }
    }
    
    // Draw the centroids (either original or adjusted)
    chartStyler->drawCentroids(forceAlignCentroids ? adjustedCentroids : centroids, 
                           clusterColors, xRange, yRange);
    
    // Remove margin restoration - keeping consistent margins for subsequent elements
    // chartLayout->setMarginRight(originalRightMargin);
    // chartLayout->setMarginTop(originalTopMargin);
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
    // Use stored title if available, otherwise use the provided title
    std::string displayTitle = !chartTitle.empty() ? chartTitle : title;
    unsigned int displayFontSize = !chartTitle.empty() ? chartTitleFontSize : titleFontSize;
    
    // Create chart configuration
    ChartConfig config(displayTitle, displayFontSize, xAxisLabel, yAxisLabel, axisFontSize);
    
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
    
    // Special case for origin axes - use more balanced margins for better appearance
    if (chartLayout->areOriginAxesVisible()) {
        // Use chartType = CHART_SCATTER for better margins with origin axes
        chartType = CHART_SCATTER;
        
        // Recalculate using CHART_SCATTER type
        top = std::max(top, calculateTopMargin(chartType, width, height));
        right = std::max(right, calculateRightMargin(chartType, width, height));
        bottom = std::max(bottom, calculateBottomMargin(chartType, width, height));
        left = std::max(left, calculateLeftMargin(chartType, width, height));
        
        // Ensure margins are large enough for axis labels at the origin
        // This prevents axis labels being cut off at origin
        left = std::max(left, 100u);  // At least 100px left margin
        bottom = std::max(bottom, 80u); // At least 80px bottom margin
        
        // Add extra margin on top for title and legend
        top = std::max(top, static_cast<unsigned int>(height * 0.15f));
        
        printf("Using optimized margins for origin axes: T=%u, R=%u, B=%u, L=%u\n", 
               top, right, bottom, left);
    }
    // Special case for line and scatter plots - give them extra vertical space
    // to ensure legends and info boxes don't overlap with the graph
    else if (chartType == CHART_LINE || chartType == CHART_SCATTER) {
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
            
            // Update the ranges in the DataMapper
            dataMapper->setCurrentXRange(xRange);
            dataMapper->setCurrentYRange(yRange);
            
            // Store ranges for origin axes
            currentXAxisRange = xRange;
            currentYAxisRange = yRange;
        }
    }
    
    printf("Global X range: [%.2f, %.2f], Y range: [%.2f, %.2f]\n", 
           xRange.min, xRange.max, yRange.min, yRange.max);
    
    // Calculate optimal margins for the chart type
    // Use CHART_SCATTER if we have scatter series or origin axes are enabled
    ChartType chartTypeToUse = CHART_LINE;
    
    // Check if any series is a scatter plot
    for (size_t i = 0; i < seriesList.size(); ++i) {
        if (seriesList[i].type == SERIES_SCATTER) {
            chartTypeToUse = CHART_SCATTER;
            break;
        }
    }
    
    // If origin axes are enabled, use CHART_SCATTER margins for better alignment
    if (chartLayout->areOriginAxesVisible()) {
        chartTypeToUse = CHART_SCATTER;
    }
    
    calculateOptimalMargins(chartTypeToUse);
    
    // Variables for margin storage and positioning
    unsigned int originalTopMargin, originalRightMargin;
    int titleY, legendY;
    
    // Prepare the chart with standard configuration and custom title
    prepareStandardChart(title, xAxisLabel, yAxisLabel, 36, 28, 180,
                       &originalTopMargin, &originalRightMargin, &titleY, &legendY);
    
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
    std::string displayTitle = !chartTitle.empty() ? chartTitle : title;
    unsigned int displayFontSize = !chartTitle.empty() ? chartTitleFontSize : 36;
    textRenderer->drawText(chartLayout->getMarginLeft(), titleY, displayTitle, 
                         colorManager->getElementColor("title"), displayFontSize, false);
    
    // Redraw the legend after adjusting margins
    addLegend(legendLabels, legendColors, chartLayout->getMarginLeft(), legendY, 16);
    
    // Set up standard axis ticks with 70px Y-axis label offset
    setupStandardAxisTicks(xRange, yRange, 70);
    
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
    
    // Remove margin restoration - keeping consistent margins for subsequent elements
    // chartLayout->setMarginRight(originalRightMargin);
    // chartLayout->setMarginTop(originalTopMargin);
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

void Plotter::plotArrows(const std::vector<Arrow>& arrows, bool redrawBackground) {
    if (arrows.empty()) {
        printf("Warning: No arrows to plot in plotArrows\n");
        return;
    }
    
    printf("Plotting %lu arrows\n", arrows.size());
    
    // Get the current data ranges from DataMapper
    DataMapper::AxisRange xRange = dataMapper->getCurrentXRange();
    DataMapper::AxisRange yRange = dataMapper->getCurrentYRange();
    
    // If Origin axes are enabled, use the stored ranges from plotter
    if (chartLayout->areOriginAxesVisible()) {
        // Only use stored ranges if they're not default values
        if (currentXAxisRange.min != currentXAxisRange.max && 
            currentYAxisRange.min != currentYAxisRange.max) {
            xRange = currentXAxisRange;
            yRange = currentYAxisRange;
            
            // Make sure these are set in the dataMapper as well for consistency
            dataMapper->setCurrentXRange(xRange);
            dataMapper->setCurrentYRange(yRange);
        }
    }
    
    printf("Using ranges for arrows - X: [%.2f, %.2f], Y: [%.2f, %.2f]\n", 
           xRange.min, xRange.max, yRange.min, yRange.max);
    
    // If we need to prepare a new chart background
    if (redrawBackground) {
        // Calculate optimal margins
        // Use CHART_SCATTER if we're in a scatter plot context with origin axes
        ChartType chartTypeToUse = CHART_LINE;
        if (chartLayout->areOriginAxesVisible()) {
            chartTypeToUse = CHART_SCATTER;
        }
        
        calculateOptimalMargins(chartTypeToUse);
        
        // Variables for margin storage and positioning
        unsigned int originalTopMargin, originalRightMargin;
        int titleY, legendY;
        
        // Prepare the chart with standard configuration
        prepareStandardChart("Arrow Visualization", "X Value", "Y Value", 36, 28, 160,
                          &originalTopMargin, &originalRightMargin, &titleY, &legendY);
        
        // Set up standard axis ticks with 50px Y-axis label offset
        setupStandardAxisTicks(xRange, yRange, 50);
    }
    
    // Draw each arrow
    for (size_t i = 0; i < arrows.size(); ++i) {
        // Map arrow start point to screen coordinates
        DataMapper::Point startPoint = dataMapper->mapDataToScreen(
            arrows[i].start.x, arrows[i].start.y, xRange, yRange);
        
        // Map arrow end point to screen coordinates
        DataMapper::Point endPoint = dataMapper->mapDataToScreen(
            arrows[i].end.x, arrows[i].end.y, xRange, yRange);
        
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
    
    // Get the current data ranges from DataMapper
    DataMapper::AxisRange xRange = dataMapper->getCurrentXRange();
    DataMapper::AxisRange yRange = dataMapper->getCurrentYRange();
    
    // If Origin axes are enabled, use the stored ranges from plotter
    if (chartLayout->areOriginAxesVisible()) {
        // Only use stored ranges if they're not default values
        if (currentXAxisRange.min != currentXAxisRange.max && 
            currentYAxisRange.min != currentYAxisRange.max) {
            xRange = currentXAxisRange;
            yRange = currentYAxisRange;
            
            // Make sure these are set in the dataMapper as well for consistency
            dataMapper->setCurrentXRange(xRange);
            dataMapper->setCurrentYRange(yRange);
        }
    }
    
    printf("Using ranges for arrow - X: [%.2f, %.2f], Y: [%.2f, %.2f]\n", 
           xRange.min, xRange.max, yRange.min, yRange.max);
    
    if (redrawBackground) {
        // Calculate optimal margins
        // Use CHART_SCATTER if we're in a scatter plot context with origin axes
        ChartType chartTypeToUse = CHART_LINE;
        if (chartLayout->areOriginAxesVisible()) {
            chartTypeToUse = CHART_SCATTER;
        }
        
        calculateOptimalMargins(chartTypeToUse);
        
        // Variables for margin storage and positioning
        unsigned int originalTopMargin, originalRightMargin;
        int titleY, legendY;
        
        // Prepare the chart with standard configuration
        prepareStandardChart("Arrow Visualization", "X Value", "Y Value", 36, 28, 160,
                          &originalTopMargin, &originalRightMargin, &titleY, &legendY);
        
        // Set up standard axis ticks with 50px Y-axis label offset
        setupStandardAxisTicks(xRange, yRange, 50);
    }
    
    // Map arrow start point to screen coordinates
    DataMapper::Point startPoint = dataMapper->mapDataToScreen(startX, startY, xRange, yRange);
    
    // Map arrow end point to screen coordinates
    DataMapper::Point endPoint = dataMapper->mapDataToScreen(endX, endY, xRange, yRange);
    
    printf("Arrow screen coords: (%.2f, %.2f) -> (%.2f, %.2f)\n", 
           startPoint.x, startPoint.y, endPoint.x, endPoint.y);
    
    // Scale coordinates and sizes for supersampling
    int ssaaStartX = ssaaManager->scaleX(startPoint.x);
    int ssaaStartY = ssaaManager->scaleY(startPoint.y);
    int ssaaEndX = ssaaManager->scaleX(endPoint.x);
    int ssaaEndY = ssaaManager->scaleY(endPoint.y);
    int ssaaLineWidth = ssaaManager->scaleSize(lineWidth);
    int ssaaArrowheadSize = ssaaManager->scaleSize(arrowheadSize);
    
    // Draw the arrow using the ShapeRenderer
    shapeRenderer->drawArrow(
        ssaaStartX, ssaaStartY, ssaaEndX, ssaaEndY, useColor, ssaaLineWidth, ssaaArrowheadSize
    );
}

// Add implementation for setShowOriginAxes method
void Plotter::setShowOriginAxes(bool show)
{
    // Only redraw if the visibility state is actually changing
    bool currentlyVisible = chartLayout->areOriginAxesVisible();
    
    // Update the layout setting
    chartLayout->setShowOriginAxes(show);
    
    // If enabling axes, make sure we have proper ranges that include the origin
    if (show) {
        // Get current ranges from DataMapper
        DataMapper::AxisRange xRange = dataMapper->getCurrentXRange();
        DataMapper::AxisRange yRange = dataMapper->getCurrentYRange();
        
        // Force ranges to include the origin point (0,0)
        if (xRange.min >= 0) xRange.min = -10.0;  // Default negative range if only positive values
        if (xRange.max <= 0) xRange.max = 10.0;   // Default positive range if only negative values 
        if (yRange.min >= 0) yRange.min = -10.0;  // Default negative range if only positive values
        if (yRange.max <= 0) yRange.max = 10.0;   // Default positive range if only negative values
        
        // If no valid range or zeros, use default
        if (xRange.min >= xRange.max || yRange.min >= yRange.max) {
            xRange.min = -10.0;
            xRange.max = 10.0;
            yRange.min = -10.0;
            yRange.max = 10.0;
        }
        
        // Update ranges
        currentXAxisRange = xRange;
        currentYAxisRange = yRange;
        dataMapper->setCurrentXRange(xRange);
        dataMapper->setCurrentYRange(yRange);
        
        printf("Origin axes enabled with ranges - X: [%.2f, %.2f], Y: [%.2f, %.2f]\n", 
               xRange.min, xRange.max, yRange.min, yRange.max);
    }
    
    // Always redraw the canvas when changing axes
    prepareCanvas();
}

// Add implementation for drawOriginAxes method
void Plotter::drawOriginAxes(double xMin, double xMax, double yMin, double yMax)
{
    // Validate the ranges to make sure they include origin
    bool rangesValid = true;
    
    // Check if x-range includes origin
    if (xMin >= 0 || xMax <= 0) {
        printf("Warning: X range [%.2f, %.2f] doesn't include origin. Adjusting ranges.\n", xMin, xMax);
        rangesValid = false;
    }
    
    // Check if y-range includes origin
    if (yMin >= 0 || yMax <= 0) {
        printf("Warning: Y range [%.2f, %.2f] doesn't include origin. Adjusting ranges.\n", yMin, yMax);
        rangesValid = false;
    }
    
    // Adjust ranges if needed
    if (!rangesValid) {
        // Get the maximum absolute value from the ranges
        double maxAbsVal = std::max(std::max(std::abs(xMin), std::abs(xMax)), 
                                  std::max(std::abs(yMin), std::abs(yMax)));
        
        // Ensure it's at least 10 for a reasonable default
        maxAbsVal = std::max(maxAbsVal, 10.0);
        
        // Set symmetric ranges around the origin
        if (xMin >= 0) xMin = -maxAbsVal;
        if (xMax <= 0) xMax = maxAbsVal;
        if (yMin >= 0) yMin = -maxAbsVal;
        if (yMax <= 0) yMax = maxAbsVal;
    }
    
    // Create ranges from the adjusted values
    DataMapper::AxisRange xRange(xMin, xMax);
    DataMapper::AxisRange yRange(yMin, yMax);
    
    // Store the ranges both in Plotter and DataMapper
    currentXAxisRange = xRange;
    currentYAxisRange = yRange;
    
    // Update the DataMapper's current ranges
    dataMapper->setCurrentXRange(xRange);
    dataMapper->setCurrentYRange(yRange);
    
    // Enable origin axes if not already enabled
    if (!chartLayout->areOriginAxesVisible()) {
        chartLayout->setShowOriginAxes(true);
        printf("Enabled origin axes visibility\n");
    }
    
    // Draw the origin axes with the specified ranges
    gridRenderer->drawOriginAxes(xMin, xMax, yMin, yMax);
    
    printf("Origin axes set up with ranges - X: [%.2f, %.2f], Y: [%.2f, %.2f]\n", 
           xMin, xMax, yMin, yMax);
}

// Add the new methods after the initialize() method:

// Store current margin settings for later restoration
void Plotter::saveState() {
    savedMarginTop = chartLayout->getMarginTop();
    savedMarginRight = chartLayout->getMarginRight();
    savedMarginBottom = chartLayout->getMarginBottom();
    savedMarginLeft = chartLayout->getMarginLeft();
    
    printf("Saved margins: T=%u, R=%u, B=%u, L=%u\n", 
           savedMarginTop, savedMarginRight, savedMarginBottom, savedMarginLeft);
}

// Restore previously saved margin settings
void Plotter::restoreState() {
    printf("Restoring margins: T=%u, R=%u, B=%u, L=%u\n", 
           savedMarginTop, savedMarginRight, savedMarginBottom, savedMarginLeft);
           
    chartLayout->setMarginTop(savedMarginTop);
    chartLayout->setMarginRight(savedMarginRight);
    chartLayout->setMarginBottom(savedMarginBottom);
    chartLayout->setMarginLeft(savedMarginLeft);
}

// Public API to control centroid alignment
void Plotter::setAlignCentroidsWithClusters(bool align) {
    forceAlignCentroids = align;
}

bool Plotter::getAlignCentroidsWithClusters() const {
    return forceAlignCentroids;
}

// New method for histograms with custom labels
void Plotter::plotHistogramWithLabels(const std::vector<int>& bins,
                                    const std::vector<std::string>& labels,
                                    const RGBA& color,
                                    const std::string& title,
                                    const std::string& xAxisLabel,
                                    const std::string& yAxisLabel)
{
    if (bins.empty() || labels.empty() || bins.size() != labels.size()) {
        printf("Error: Invalid input for histogram with labels. Bins and labels must be non-empty and have the same size.\n");
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
    std::string displayTitle = !chartTitle.empty() ? chartTitle : title;
    unsigned int displayFontSize = !chartTitle.empty() ? chartTitleFontSize : 42;
    textRenderer->drawText(chartLayout->getMarginLeft(), titleY, displayTitle, 
                      colorManager->getElementColor("title"), displayFontSize, false);
    
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
    
    // Don't use the default X-axis labels since we're adding custom ones
    // Set showXAxisLabels to false in the internal code
    
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
        
        // Add custom label centered under each bar
        // Calculate the center position of the bar
        int labelX = x + barWidth / 2;
        
        // Position the label below the X-axis (use a font size of 22 to match other labels)
        int labelY = chartLayout->getHeight() - chartLayout->getMarginBottom() + 25;
        
        // Get the label text
        const std::string& labelText = labels[i];
        
        // Draw the label with center alignment (true for centerAligned parameter)
        RGBA labelColor = colorManager->getElementColor("axisLabel");
        labelColor.a = 0xCC; // 80% opacity to match X-axis labels
        textRenderer->drawText(labelX, labelY, labelText, labelColor, 22, true);
    }
}

} // namespace shmea 
