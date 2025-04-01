//PNGPlotter.cpp
#include "PNGPlotter.h"
#include "../Database/png-helper.h"
#include <sstream>
#include <map>

namespace shmea {

// Parse helper function for options map
template<typename T>
T getOptionValue(const std::map<std::string, std::string>& options, const std::string& key, T defaultValue) {
    if (options.find(key) != options.end()) {
        std::istringstream iss(options.at(key));
        T value;
        if (iss >> value) {
            return value;
        }
    }
    return defaultValue;
}

// Constructor that uses options map
PNGPlotter::PNGPlotter(unsigned int width, unsigned int height, int graphSize, 
                      const std::map<std::string, std::string>& options)
    : image(), width(width), height(height),
      bounds(getOptionValue(options, "min_price", 0.0f), getOptionValue(options, "max_price", 100.0f)),
      fourQuadrants(getOptionValue(options, "four_quadrants", false)),
      last_timestamp(0),
      total_candles_drawn(0),
      graphSize(graphSize),
      candle_width(static_cast<int>(graphSize != 0 ? (width - width * 0.15 - width * 0.1) / graphSize : 1)),
      last_candle_pos(static_cast<int>(candle_width / 2)),
      lines(getOptionValue(options, "lines", 0)),
      color_bullish(0x00, 0xFF, 0x00, 0xFF), 
      color_bearish(0xFF, 0x00, 0x00, 0xFF),
      gridDrawer(NULL),
      candlestickDrawer(NULL),
      arrowDrawer(NULL),
      histogramDrawer(NULL),
      labelsDrawer(NULL),
      dataDrawer(NULL),
      legendDrawer(NULL),
      title(getOptionValue(options, "title", std::string("Data Visualization"))),
      xAxisLabel(getOptionValue(options, "x_axis_label", std::string("Time"))),
      yAxisLabel(getOptionValue(options, "y_axis_label", std::string("Value"))),
      titleColor(0xFF, 0xFF, 0xFF, 0xFF),
      xAxisLabelColor(0xFF, 0xFF, 0xFF, 0xFF),
      yAxisLabelColor(0xFF, 0xFF, 0xFF, 0xFF),
      titleFontSize(240), // Reduced from 300 to 240 for better proportions
      axisLabelFontSize(180) // Reduced from 200 to 180 for better proportions
{
    image.Allocate(width, height);
    RGBA DarkGray(0x40, 0x40, 0x40, 0xFF);
    RGBA Black(0x00, 0x00, 0x00, 0xFF); // Full opacity for the background
    
    // Fill the entire image with background color
    for (unsigned int y = 0; y < height; ++y) {
        for (unsigned int x = 0; x < width; ++x) {
            float ratio = static_cast<float>(y) / height;
            RGBA gradientColor(
                static_cast<unsigned char>(DarkGray.r * (1 - ratio) + Black.r * ratio),
                static_cast<unsigned char>(DarkGray.g * (1 - ratio) + Black.g * ratio),
                static_cast<unsigned char>(DarkGray.b * (1 - ratio) + Black.b * ratio),
                0xFF
            );
            image.SetPixel(x, y, gradientColor);
        }
    }

    std::cout << "Initializing PNGPlotter with FreeType text rendering" << std::endl;

    // Initialize all specialized drawers
    gridDrawer = new GridDrawer(image, width, height, bounds);
    candlestickDrawer = new CandlestickDrawer(image, width, height, candle_width, color_bullish, color_bearish);
    arrowDrawer = new ArrowDrawer(image, width, height);
    histogramDrawer = new HistogramDrawer(image, width, height, bounds, graphSize);
    labelsDrawer = new LabelsDrawer(image, width, height);
    dataDrawer = new DataDrawer(image, width, height, bounds, graphSize, lines);
    legendDrawer = new LegendDrawer(image, width, height);

    // Connect the GridDrawer to the LabelsDrawer for axis labels
    gridDrawer->setLabelsDrawer(labelsDrawer);
    
    // Draw grid if in four quadrants mode
    if (fourQuadrants) {
        gridDrawer->drawFourQuadrants();
    }

    // Add default title and axis labels with appropriate font sizes
    setTitle(title, titleFontSize, titleColor);
    setXAxisLabel(xAxisLabel, axisLabelFontSize, xAxisLabelColor);
    setYAxisLabel(yAxisLabel, axisLabelFontSize, yAxisLabelColor);
    
    // Add some default legend entries if lines were provided
    if (lines > 0) {
        addLegendEntry("Series 1", RGBA(0x00, 0x00, 0xFF, 0xFF)); // Blue
        if (lines > 1) {
            addLegendEntry("Series 2", RGBA(0xFF, 0xA5, 0x00, 0xFF)); // Orange
        }
        if (lines > 2) {
            addLegendEntry("Series 3", RGBA(0x80, 0x00, 0x80, 0xFF)); // Purple
        }
        // Draw the legend
        drawLegend();
    }

    // Initialize AGG_SIZE mapping
    AGG_SIZE[1] = "1m";
    AGG_SIZE[2] = "2m";
    AGG_SIZE[3] = "3m";
    AGG_SIZE[5] = "5m";
    AGG_SIZE[15] = "15m";
    AGG_SIZE[30] = "30m";
    AGG_SIZE[60] = "1h";
    AGG_SIZE[240] = "4h";
    AGG_SIZE[390] = "1D";
    AGG_SIZE[1950] = "1W";
    AGG_SIZE[8190] = "1MO";
    AGG_SIZE[1440] = "1D";
    AGG_SIZE[10080] = "1W";
    AGG_SIZE[43200] = "1MO";
}

PNGPlotter::~PNGPlotter() {
    delete gridDrawer;
    delete candlestickDrawer;
    delete arrowDrawer;
    delete histogramDrawer;
    delete labelsDrawer;
    delete dataDrawer;
    delete legendDrawer;
}

int PNGPlotter::getWidth() {
    return width;
}

int PNGPlotter::getHeight() {
    return height;
}

void PNGPlotter::drawYGrid() {
    gridDrawer->drawYGrid();
}

void PNGPlotter::drawXGrid(int64_t start, int64_t end) {
    gridDrawer->drawXGrid(start, end, graphSize);
}

void PNGPlotter::drawGridWithLabels(int64_t start, int64_t end) {
    // First draw the grid lines
    drawYGrid();
    drawXGrid(start, end);
    
    // Then draw the labels with proper font size
    gridDrawer->drawYAxisLabels(180);  // Smaller font size for Y-axis labels
    gridDrawer->drawXAxisLabels(start, end, graphSize, 180);  // Smaller font size for X-axis labels
}

void PNGPlotter::addDataPointWithIndicator(double newPrice, int portIndex, std::string indicator, std::string value) {
    dataDrawer->addDataPointWithIndicator(newPrice, portIndex, indicator, value);
    
    // If this is the last point for the indicator, add a label
    if (indicator != "" && portIndex == lines - 1) {
        // Calculate margins for context
        int margin_right = width * 0.1;
        int margin_bottom = height * 0.15;
        int margin_top = height * 0.1;
        
        float adjusted_max = bounds.getRange();
        float adjusted_tick = newPrice - bounds.getMinPrice();
        int y = height - margin_bottom - static_cast<int>(adjusted_tick / adjusted_max * (height - margin_top - margin_bottom));
        
        std::ostringstream oss;
        oss << newPrice;
        std::string numberY = oss.str();
        
        const RGBA* indicatorColor = dataDrawer->getIndicatorColor(indicator);
        if (indicatorColor != NULL) {
            GraphLabel(width - margin_right, y, numberY, 480, 100, 100, true, *indicatorColor, RGBA(0xFF, 0xFF, 0xFF, 0xFF));
        }
    }
}

void PNGPlotter::addDataPoint(double newPrice, int portIndex, bool draw, RGBA* lineColor, int lineWidth) {
    dataDrawer->addDataPoint(newPrice, portIndex, draw, lineColor, lineWidth);
}

void PNGPlotter::addDataPointsPCA(const std::vector<std::vector<double> >& data, const RGBA& pointColor) {
    dataDrawer->addDataPointsPCA(data, pointColor);
}

void PNGPlotter::addDataPointsKMeans(const std::string& graphName, const std::vector<std::vector<double> >& data, const std::vector<int>& labels, const std::vector<std::vector<float> >& centroids) {
    dataDrawer->addDataPointsKMeans(graphName, data, labels, centroids);
}

void PNGPlotter::addArrow(const std::vector<std::vector<double> >& sorted_eig_vecs, const std::vector<double>& variance_explained, const RGBA& arrowColor) {
    arrowDrawer->addArrow(sorted_eig_vecs, variance_explained, arrowColor);
}

void PNGPlotter::addHistogram(const std::vector<int>& bins, RGBA& barColor) {
    histogramDrawer->addHistogram(bins, barColor);
}

void PNGPlotter::drawNewCandle(long timestamp, float raw_open, float raw_close, float raw_high, float raw_low) {
    // Calculate margins for context
    int margin_left = width * 0.15;
    int margin_bottom = height * 0.15;
    int margin_top = height * 0.1;
    
    // Adjust prices by subtracting min_price for normalization
    float adjusted_open = raw_open - bounds.getMinPrice();
    float adjusted_close = raw_close - bounds.getMinPrice();
    float adjusted_high = raw_high - bounds.getMinPrice();
    float adjusted_low = raw_low - bounds.getMinPrice();
    float adjusted_max = bounds.getRange();

    // Calculate y-coordinates with adjusted prices
    int y_open = height - margin_bottom - static_cast<int>(adjusted_open / adjusted_max * (height - margin_top - margin_bottom));
    int y_close = height - margin_bottom - static_cast<int>(adjusted_close / adjusted_max * (height - margin_top - margin_bottom));
    int y_high = height - margin_bottom - static_cast<int>(adjusted_high / adjusted_max * (height - margin_top - margin_bottom));
    int y_low = height - margin_bottom - static_cast<int>(adjusted_low / adjusted_max * (height - margin_top - margin_bottom));

    // Determine the color based on whether the candlestick is bullish or bearish
    const RGBA& color = (raw_close >= raw_open) ? color_bullish : color_bearish;

    // Draw the candlestick at the calculated x position
    candlestickDrawer->drawCandleStick(last_candle_pos + margin_left, y_open, y_close, y_high, y_low, color);

    // Update the last drawn x position
    last_candle_pos += candle_width;
}

Image PNGPlotter::downsampleToTargetSize() {
    Image downsampledImage;
    downsampledImage.Allocate(TARGET_WIDTH, TARGET_HEIGHT);

    // Calculate the size of each block of high-res pixels that corresponds to one low-res pixel
    float scaleX = static_cast<float>(width) / TARGET_WIDTH;
    float scaleY = static_cast<float>(height) / TARGET_HEIGHT;

    for (unsigned int y = 0; y < TARGET_HEIGHT; ++y) {
        for (unsigned int x = 0; x < TARGET_WIDTH; ++x) {
            int startX = static_cast<int>(x * scaleX);
            int startY = static_cast<int>(y * scaleY);
            int blockWidth = static_cast<int>(std::ceil(scaleX));
            int blockHeight = static_cast<int>(std::ceil(scaleY));

            // Average the colors over the block of high-res pixels
            RGBA avgColor = image.averageColor(startX, startY, blockWidth, blockHeight);
            downsampledImage.SetPixel(x, y, avgColor);
        }
    }

    return downsampledImage;
}

void PNGPlotter::GraphLabel(unsigned int penX, unsigned int penY, const std::string& text, unsigned int fontSize, unsigned int xOffset, unsigned int yOffset, bool hasBox, RGBA labelColor, RGBA textColor) {
    // Adjust fontSize for better proportions with high-resolution graphics
    unsigned int adjustedFontSize = static_cast<unsigned int>(fontSize * 0.9); // Slightly reduce font size for better proportions
    labelsDrawer->drawLabel(penX, penY, text, adjustedFontSize, xOffset, yOffset, hasBox, labelColor, textColor);
}

void PNGPlotter::HeaderPNG(const std::string& text, unsigned int fontSize, unsigned int headerPos, unsigned int rePositionY, RGBA headerTextColor) {
    // Adjust fontSize for better proportions with high-resolution graphics
    unsigned int adjustedFontSize = static_cast<unsigned int>(fontSize * 0.9); // Slightly reduce font size for better proportions
    labelsDrawer->drawHeader(text, adjustedFontSize, headerPos, rePositionY, headerTextColor);
}

void PNGPlotter::setTitle(const std::string& title, unsigned int fontSize, RGBA titleColor) {
    this->title = title;
    this->titleFontSize = fontSize;
    this->titleColor = titleColor;
    
    // Calculate margin_top for context
    int margin_top = height * 0.1;
    
    // Position the title significantly above the top margin to ensure visibility
    unsigned int titleX = width / 2;
    // Move title further up, away from the graph area
    unsigned int titleY = margin_top / 2;
    
    // Ensure there's minimum padding
    if (titleY < fontSize / 3) {
        titleY = fontSize / 3;
    }
    
    // Apply gradient background in title area to match the rest of the image
    RGBA DarkGray(0x40, 0x40, 0x40, 0xFF);
    RGBA Black(0x00, 0x00, 0x00, 0xFF);
    
    for (unsigned int y = 0; y < margin_top - 2; ++y) {
        float ratio = static_cast<float>(y) / height;
        RGBA gradientColor(
            static_cast<unsigned char>(DarkGray.r * (1 - ratio) + Black.r * ratio),
            static_cast<unsigned char>(DarkGray.g * (1 - ratio) + Black.g * ratio),
            static_cast<unsigned char>(DarkGray.b * (1 - ratio) + Black.b * ratio),
            0xFF
        );
        
        for (unsigned int x = 0; x < width; ++x) {
            image.SetPixel(x, y, gradientColor);
        }
    }
    
    labelsDrawer->drawCenteredText(titleX, titleY, title, fontSize, titleColor);
}

void PNGPlotter::setXAxisLabel(const std::string& label, unsigned int fontSize, RGBA labelColor) {
    this->xAxisLabel = label;
    this->axisLabelFontSize = fontSize;
    this->xAxisLabelColor = labelColor;
    
    // Calculate margins for context
    int margin_left = width * 0.15;
    int margin_right = width * 0.1;
    int margin_bottom = height * 0.15;
    
    // Position the X-axis label at the bottom center of the graph
    unsigned int labelX = margin_left + (width - margin_left - margin_right) / 2;
    unsigned int labelY = height - margin_bottom / 2;
    
    // Ensure label has enough space below the bottom margin
    if (labelY >= height - fontSize / 3) {
        labelY = height - fontSize / 3 - 10; // Add extra padding
    }
    
    // Apply gradient background in bottom margin area to match the rest of the image
    RGBA DarkGray(0x40, 0x40, 0x40, 0xFF);
    RGBA Black(0x00, 0x00, 0x00, 0xFF);
    
    for (unsigned int y = height - margin_bottom + 2; y < height; ++y) {
        float ratio = static_cast<float>(y) / height;
        RGBA gradientColor(
            static_cast<unsigned char>(DarkGray.r * (1 - ratio) + Black.r * ratio),
            static_cast<unsigned char>(DarkGray.g * (1 - ratio) + Black.g * ratio),
            static_cast<unsigned char>(DarkGray.b * (1 - ratio) + Black.b * ratio),
            0xFF
        );
        
        for (unsigned int x = 0; x < width; ++x) {
            image.SetPixel(x, y, gradientColor);
        }
    }
    
    labelsDrawer->drawCenteredText(labelX, labelY, label, fontSize, labelColor);
}

void PNGPlotter::setYAxisLabel(const std::string& label, unsigned int fontSize, RGBA labelColor) {
    this->yAxisLabel = label;
    this->axisLabelFontSize = fontSize;
    this->yAxisLabelColor = labelColor;
    
    // Calculate margins for context
    int margin_left = width * 0.15;
    int margin_top = height * 0.1;
    int margin_bottom = height * 0.15;
    
    // Move Y-axis label further left and ensure it's centered
    unsigned int labelX = fontSize / 2; // Adjusted for better positioning
    unsigned int labelY = margin_top + (height - margin_top - margin_bottom) / 2;
    
    // Ensure label is clear of the left edge
    labelX = std::max(labelX, static_cast<unsigned int>(fontSize * 0.8));
    
    // Apply gradient background in left margin area to match the rest of the image
    RGBA DarkGray(0x40, 0x40, 0x40, 0xFF);
    RGBA Black(0x00, 0x00, 0x00, 0xFF);
    
    for (unsigned int y = margin_top; y < height - margin_bottom; ++y) {
        float ratio = static_cast<float>(y) / height;
        RGBA gradientColor(
            static_cast<unsigned char>(DarkGray.r * (1 - ratio) + Black.r * ratio),
            static_cast<unsigned char>(DarkGray.g * (1 - ratio) + Black.g * ratio),
            static_cast<unsigned char>(DarkGray.b * (1 - ratio) + Black.b * ratio),
            0xFF
        );
        
        for (unsigned int x = 0; x < margin_left - 2; ++x) {
            image.SetPixel(x, y, gradientColor);
        }
    }
    
    labelsDrawer->drawRotatedText(labelX, labelY, label, fontSize, labelColor);
}

void PNGPlotter::addLegendEntry(const std::string& label, const RGBA& color) {
    legendDrawer->addEntry(label, color);
}

void PNGPlotter::drawLegend() {
    // Calculate margin_top for context
    int margin_top = height * 0.1;
    
    // Position the legend in the top-left corner, but completely within the padding area
    // Make sure it stays above the plot area boundary
    unsigned int legendX = 5; // Small offset from the left edge
    unsigned int legendY = 5; // Small offset from the top edge
    
    // Make sure the legend is drawn completely above the graph
    legendDrawer->setMaxHeight(margin_top - 10); // Set maximum height to be within top margin
    legendDrawer->setFontSize(180); // Use a smaller font size for the legend
    legendDrawer->setBorderColor(RGBA(0xA0, 0xA0, 0xA0, 0xFF)); // Lighter border
    
    legendDrawer->draw(legendX, legendY);
}

std::string PNGPlotter::aggString(int aggSize) {
    if (AGG_SIZE.find(aggSize) != AGG_SIZE.end()) {
        return AGG_SIZE[aggSize];
    }
    return "";
}

void PNGPlotter::SavePNG(const std::string& filename, const std::string& folder) {
    Image downsampleImage = downsampleToTargetSize();

    std::string full_path = folder;
    full_path.append("/");
    full_path.append(filename);
    downsampleImage.SavePNG(full_path.c_str());
}

} // namespace shmea
