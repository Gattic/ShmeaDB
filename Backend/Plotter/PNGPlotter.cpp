//PNGPlotter.cpp
#include "PNGPlotter.h"
#include "../Database/png-helper.h"
#include <sstream>

namespace shmea {

PNGPlotter::PNGPlotter(unsigned width, unsigned height, int graphSize, double max_price, double low_price, int lines, int margin_top, int margin_right, int margin_bottom, int margin_left, bool fourQuadrants)
    : image(), width(width), height(height), 
      min_price(low_price),
      max_price(max_price),
      margin_top(margin_top),
      margin_right(margin_right),
      margin_bottom(margin_bottom),
      margin_left(margin_left),
      fourQuadrants(fourQuadrants),
      last_timestamp(0),
      total_candles_drawn(0),
      graphSize(graphSize),
      candle_width(static_cast<int>(graphSize != 0 ? (width - margin_left - margin_right) / graphSize : 1)),
      last_candle_pos(static_cast<int>(candle_width / 2)),
      lines(lines),
      color_bullish(0x00, 0xFF, 0x00, 0xFF), 
      color_bearish(0xFF, 0x00, 0x00, 0xFF),
      gridDrawer(NULL),
      candlestickDrawer(NULL),
      arrowDrawer(NULL),
      histogramDrawer(NULL),
      labelsDrawer(NULL),
      dataDrawer(NULL)
{
    image.Allocate(width, height);
    RGBA DarkGray(0x40, 0x40, 0x40, 0xFF);
    RGBA Black(0x00, 0x00, 0x00, 0x7F);
    image.drawVerticalGradient(0, 0, DarkGray, Black, 0);

    std::cout << "Initializing PNGPlotter with FreeType text rendering" << std::endl;

    // Initialize all specialized drawers
    gridDrawer = new GridDrawer(image, width, height, margin_top, margin_right, margin_bottom, margin_left, min_price, max_price);
    candlestickDrawer = new CandlestickDrawer(image, width, height, margin_top, margin_right, margin_bottom, margin_left, candle_width, color_bullish, color_bearish);
    arrowDrawer = new ArrowDrawer(image, width, height, margin_top, margin_right, margin_bottom, margin_left);
    histogramDrawer = new HistogramDrawer(image, width, height, margin_top, margin_right, margin_bottom, margin_left, min_price, max_price, graphSize);
    labelsDrawer = new LabelsDrawer(image, width, height, margin_top, margin_right, margin_bottom, margin_left);
    dataDrawer = new DataDrawer(image, width, height, margin_top, margin_right, margin_bottom, margin_left, min_price, max_price, graphSize, lines);

    if (fourQuadrants) {
        gridDrawer->drawFourQuadrants();
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

void PNGPlotter::addDataPointWithIndicator(double newPrice, int portIndex, std::string indicator, std::string value) {
    dataDrawer->addDataPointWithIndicator(newPrice, portIndex, indicator, value);
    
    // If this is the last point for the indicator, add a label
    if (indicator != "" && portIndex == lines - 1) {
        float adjusted_max = max_price - min_price;
        float adjusted_tick = newPrice - min_price;
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
    // Adjust prices by subtracting min_price for normalization
    float adjusted_open = raw_open - min_price;
    float adjusted_close = raw_close - min_price;
    float adjusted_high = raw_high - min_price;
    float adjusted_low = raw_low - min_price;
    float adjusted_max = max_price - min_price;

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
    labelsDrawer->drawLabel(penX, penY, text, fontSize, xOffset, yOffset, hasBox, labelColor, textColor);
}

void PNGPlotter::HeaderPNG(const std::string& text, unsigned int fontSize, unsigned int headerPos, unsigned int rePositionY, RGBA headerTextColor) {
    labelsDrawer->drawHeader(text, fontSize, headerPos, rePositionY, headerTextColor);
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
