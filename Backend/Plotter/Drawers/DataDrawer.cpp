#include "DataDrawer.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <sstream>

namespace shmea {

DataDrawer::DataDrawer(Image& image, unsigned int width, unsigned int height, 
                     int margin_top, int margin_right, int margin_bottom, int margin_left,
                     double min_price, double max_price, int graphSize, int lines)
    : BaseDrawer(image, width, height, margin_top, margin_right, margin_bottom, margin_left),
      min_price(min_price), max_price(max_price), graphSize(graphSize), lines(lines),
      first_line_point(lines, false), last_price_pos(lines, 0), last_line_drawn(0) {
    
    candle_width = (width - margin_left - margin_right) / (graphSize > 0 ? graphSize : 1);
    initialize_colors();
}

void DataDrawer::initialize_colors() {
    // Standard line colors
    line_colors.push_back(RGBA(0x00, 0x00, 0xFF, 0xFF));    // Blue
    line_color_names.push_back("Blue");

    line_colors.push_back(RGBA(0xFF, 0xA5, 0x00, 0xFF));  // Orange
    line_color_names.push_back("Orange");

    line_colors.push_back(RGBA(0x80, 0x00, 0x80, 0xFF));  // Purple
    line_color_names.push_back("Purple");

    line_colors.push_back(RGBA(0x00, 0xFF, 0xFF, 0xFF));  // Cyan
    line_color_names.push_back("Cyan");

    line_colors.push_back(RGBA(0xFF, 0xFF, 0x00, 0xFF));  // Yellow
    line_color_names.push_back("Yellow");

    line_colors.push_back(RGBA(0xFF, 0x00, 0xFF, 0xFF));  // Magenta
    line_color_names.push_back("Magenta");

    line_colors.push_back(RGBA(0x00, 0x80, 0x80, 0xFF));  // Teal
    line_color_names.push_back("Teal");

    line_colors.push_back(RGBA(0xFF, 0x8C, 0x00, 0xFF));  // Dark Orange
    line_color_names.push_back("Dark Orange");

    line_colors.push_back(RGBA(0xFF, 0x69, 0xB4, 0xFF));  // Pink
    line_color_names.push_back("Pink");

    line_colors.push_back(RGBA(0xAD, 0xD8, 0xE6, 0xFF));  // Light Blue
    line_color_names.push_back("Light Blue");

    // Indicator colors
    indicatorColors["BBLow"] = RGBA(0x00, 0x00, 0xFF, 0xFF); // Blue
    indicatorColors["BBHigh"] = RGBA(0x00, 0x00, 0xFF, 0xFF); // Blue
    indicatorTextColor["BBHigh"] = RGBA(0x00, 0x00, 0x50, 0xFF);
    indicatorTextColor["BBLow"] = RGBA(0x00, 0x00, 0x50, 0xFF);

    indicatorColors["EMA"] = RGBA(0xFF, 0xBF, 0x00, 0xFF); // Yellow
    indicatorTextColor["EMA"] = RGBA(0x72, 0x56, 0x00, 0xFF);

    indicatorColors["SMA"] = RGBA(0xFF, 0x5C, 0x00, 0xFF); // Orange
    indicatorTextColor["SMA"] = RGBA(0x72, 0x29, 0x00, 0xFF);
    
    // Initialize indicator tracking
    indicatorPoint["BBLow"] = 0;
    indicatorPoint["BBHigh"] = 0;
    indicatorPoint["EMA"] = 0;
    indicatorPoint["SMA"] = 0;
}

void DataDrawer::addDataPoint(double newPrice, int portIndex, bool draw, RGBA* lineColor, int lineWidth) {
    if (lines == 0) { 
        std::cerr << "PNG Plotter not initialized with the correct amount of lines to draw" << std::endl;
        return;
    }

    if (lines - 1 < portIndex) {
        std::cerr << "PNG Plotter does not have the index for this line assigned" << std::endl;
        return;
    }

    if (lineColor == NULL) {
        lineColor = &line_colors[portIndex];
    }

    int y = height - margin_bottom - static_cast<int>((newPrice - min_price) / (max_price - min_price) * (height - margin_top - margin_bottom));
    y = clamp(y, margin_top, height - margin_bottom);    

    if (draw) {
        if (!first_line_point[portIndex]) {
            first_line_point[portIndex] = true;
            last_line_drawn = -1 * static_cast<int>(candle_width / 2);
        } else {
            int startX = last_line_drawn + margin_left;
            int endX = last_line_drawn + candle_width + margin_left;

            drawLine(startX, last_price_pos[portIndex], endX, y, *lineColor, lineWidth);
        }

        // Update the previous-y coordinate
        last_price_pos[portIndex] = y;
    }

    // Only update X once all the lines have been drawn
    if (portIndex == lines - 1)
        last_line_drawn += candle_width;
}

void DataDrawer::addDataPointWithIndicator(double newPrice, int portIndex, const std::string& indicator, const std::string& value) {
    if (indicator.empty()) {
        std::cerr << "Error: You need to define an indicator to draw a line." << std::endl;
        return;
    }

    if (indicatorColors.find(indicator) == indicatorColors.end()) {
        std::cerr << "Error: The specified indicator '" << indicator << "' is not recognized." << std::endl;
        return;
    }

    addDataPoint(newPrice, portIndex, true, &indicatorColors[indicator], 25); 
    indicatorPoint[indicator] += 1;
}

const RGBA* DataDrawer::getIndicatorColor(const std::string& indicator) const {
    std::map<std::string, RGBA>::const_iterator it = indicatorColors.find(indicator);
    if (it != indicatorColors.end()) {
        return &(it->second);
    }
    return NULL;
}

const RGBA* DataDrawer::getIndicatorTextColor(const std::string& indicator) const {
    std::map<std::string, RGBA>::const_iterator it = indicatorTextColor.find(indicator);
    if (it != indicatorTextColor.end()) {
        return &(it->second);
    }
    return NULL;
}

void DataDrawer::addDataPointsPCA(const std::vector<std::vector<double> >& data, const RGBA& pointColor) {
    // Calculate the effective plotting area considering the margins
    int effectiveWidth = width - margin_left - margin_right;
    int effectiveHeight = height - margin_top - margin_bottom;

    // If no data or empty data, return
    if (data.empty() || data[0].empty()) {
        return;
    }

    // Add a white background
    RGBA white(0xFF, 0xFF, 0xFF, 0xFF);
    for (int x = margin_left; x < width - margin_right; ++x) {
        for (int y = margin_top; y < height - margin_bottom; ++y) {
            image.SetPixel(x, y, white);
        }
    }

    // Determine number of features (columns) in the dataset
    size_t numFeatures = data[0].size();
    
    // Create color variations for each feature
    std::vector<RGBA> featureColors;
    
    // Generate colors for each feature
    for (size_t i = 0; i < numFeatures; i++) {
        // Create a distinct color for each feature
        unsigned char r = (pointColor.r + i * 40) % 256;
        unsigned char g = (pointColor.g + i * 60) % 256;
        unsigned char b = (pointColor.b + i * 80) % 256;
        
        featureColors.push_back(RGBA(r, g, b, pointColor.a));
    }

    // Find min and max values for each feature for proper scaling
    std::vector<double> minValues(numFeatures, std::numeric_limits<double>::max());
    std::vector<double> maxValues(numFeatures, -std::numeric_limits<double>::max());

    for (size_t i = 0; i < data.size(); ++i) {
        for (size_t j = 0; j < numFeatures && j < data[i].size(); ++j) {
            minValues[j] = std::min(minValues[j], data[i][j]);
            maxValues[j] = std::max(maxValues[j], data[i][j]);
        }
    }

    // Calculate the center (origin) of the plot
    int centerX = margin_left + effectiveWidth / 2;
    int centerY = margin_top + effectiveHeight / 2;

    // Calculate point thickness based on data density
    int pointThickness = std::max(6, std::min(16, static_cast<int>(8.0 * (effectiveWidth / 800.0))));

    // Calculate plot area padding (percentage of effective dimensions)
    double paddingRatio = 0.1; // 10% padding
    int paddingX = static_cast<int>(effectiveWidth * paddingRatio);
    int paddingY = static_cast<int>(effectiveHeight * paddingRatio);

    // Usable plotting area after padding
    int usableWidth = effectiveWidth - 2 * paddingX;
    int usableHeight = effectiveHeight - 2 * paddingY;

    // Draw grid lines
    RGBA majorGridColor(0xD0, 0xD0, 0xD0, 0xFF);  // Light gray for major grid lines
    RGBA minorGridColor(0xE0, 0xE0, 0xE0, 0xAA);  // Very light gray for minor grid lines
    
    // Number of grid divisions
    int majorGridDivisions = 4;   // Number of major grid divisions (quadrants)
    int minorGridDivisions = 16;  // Number of minor grid divisions
    
    // Draw minor grid lines first (so they appear behind major lines)
    for (int i = 0; i <= minorGridDivisions; i++) {
        float percentage = static_cast<float>(i) / minorGridDivisions;
        int x = margin_left + static_cast<int>(percentage * effectiveWidth);
        int y = margin_top + static_cast<int>(percentage * effectiveHeight);
        
        // Draw vertical minor grid line
        if (i % (minorGridDivisions / majorGridDivisions) != 0) { // Skip where major lines will be
            drawLine(x, margin_top, x, height - margin_bottom, minorGridColor);
        }
        
        // Draw horizontal minor grid line
        if (i % (minorGridDivisions / majorGridDivisions) != 0) { // Skip where major lines will be
            drawLine(margin_left, y, width - margin_right, y, minorGridColor);
        }
    }
    
    // Draw major grid lines
    for (int i = 0; i <= majorGridDivisions; i++) {
        float percentage = static_cast<float>(i) / majorGridDivisions;
        int x = margin_left + static_cast<int>(percentage * effectiveWidth);
        int y = margin_top + static_cast<int>(percentage * effectiveHeight);
        
        // Draw vertical major grid line
        drawLine(x, margin_top, x, height - margin_bottom, majorGridColor);
        
        // Draw horizontal major grid line
        drawLine(margin_left, y, width - margin_right, y, majorGridColor);
    }

    // Draw axes at the center of the plot with thicker lines
    RGBA axisColor(0x50, 0x50, 0x50, 0xFF);  // Darker gray for axes
    
    // X-axis (thicker line)
    for (int offset = -1; offset <= 1; offset++) {
        drawLine(margin_left, centerY + offset, width - margin_right, centerY + offset, axisColor);
    }
    
    // Y-axis (thicker line)
    for (int offset = -1; offset <= 1; offset++) {
        drawLine(centerX + offset, margin_top, centerX + offset, height - margin_bottom, axisColor);
    }

    // For each pair of features, create a plot
    for (size_t i = 0; i < numFeatures; ++i) {
        for (size_t j = i + 1; j < numFeatures; ++j) {
            // Calculate scales for this feature pair
            double rangeX = maxValues[i] - minValues[i];
            double rangeY = maxValues[j] - minValues[j];

            // Avoid division by zero
            if (rangeX < 1e-10) rangeX = 1.0;
            if (rangeY < 1e-10) rangeY = 1.0;

            // Use the same scale for both axes to maintain aspect ratio
            double scaleByX = usableWidth / rangeX;
            double scaleByY = usableHeight / rangeY;
            double commonScale = std::min(scaleByX, scaleByY);
            
            double scaleX = commonScale;
            double scaleY = commonScale;

            // Draw each data point for this feature pair
            for (size_t k = 0; k < data.size(); ++k) {
                if (data[k].size() <= std::max(i, j)) continue; // Skip if not enough dimensions
                
                // Map data coordinates to screen coordinates
                double valueX = data[k][i];
                double valueY = data[k][j];
                
                // Scale relative to center
                int screenX = centerX + static_cast<int>((valueX - (minValues[i] + maxValues[i]) / 2) * scaleX);
                int screenY = centerY - static_cast<int>((valueY - (minValues[j] + maxValues[j]) / 2) * scaleY);
                
                // Ensure the point is within the plotting area
                screenX = clamp(screenX, margin_left + paddingX, width - margin_right - paddingX);
                screenY = clamp(screenY, margin_top + paddingY, height - margin_bottom - paddingY);
                
                // Use a color that combines the colors of both features
                RGBA combinedColor(
                    (featureColors[i].r + featureColors[j].r) / 2,
                    (featureColors[i].g + featureColors[j].g) / 2,
                    (featureColors[i].b + featureColors[j].b) / 2,
                    pointColor.a
                );
                
                // Draw the point with proper bounds checking
                drawPoint(screenX, screenY, pointThickness, combinedColor);
            }
        }
    }
}

void DataDrawer::addDataPointsKMeans(const std::string& graphName, 
                                  const std::vector<std::vector<double> >& data, 
                                  const std::vector<int>& labels, 
                                  const std::vector<std::vector<float> >& centroids) {
    // Calculate the effective plotting area considering the margins
    int effectiveWidth = width - margin_left - margin_right;
    int effectiveHeight = height - margin_top - margin_bottom;

    // If no data or empty data, return
    if (data.empty() || data[0].empty()) {
        return;
    }

    // Add a gradient background
    RGBA gradientTop(0x20, 0x20, 0x40, 0xFF);  // Dark blue-gray at top
    RGBA gradientBottom(0x10, 0x10, 0x20, 0xFF);  // Darker at bottom
    for (int y = margin_top; y < height - margin_bottom; ++y) {
        float ratio = static_cast<float>(y - margin_top) / effectiveHeight;
        RGBA gradientColor(
            static_cast<unsigned char>(gradientTop.r * (1 - ratio) + gradientBottom.r * ratio),
            static_cast<unsigned char>(gradientTop.g * (1 - ratio) + gradientBottom.g * ratio),
            static_cast<unsigned char>(gradientTop.b * (1 - ratio) + gradientBottom.b * ratio),
            0xFF
        );
        
        for (int x = margin_left; x < width - margin_right; ++x) {
            image.SetPixel(x, y, gradientColor);
        }
    }

    // Determine number of features in the dataset
    size_t numFeatures = data[0].size();
    
    // Find the number of unique clusters
    int maxCluster = -1;
    if (!labels.empty()) {
        for (size_t i = 0; i < labels.size(); i++) {
            if (labels[i] > maxCluster) {
                maxCluster = labels[i];
            }
        }
    }
    
    // Generate colors for each cluster - using brighter, more vibrant colors for better contrast
    std::vector<RGBA> clusterColors;
    
    // Pre-defined high-contrast colors for better visibility
    std::vector<RGBA> distinctColors;
    distinctColors.push_back(RGBA(255, 100, 100, 255));  // Light Red
    distinctColors.push_back(RGBA(100, 100, 255, 255));  // Light Blue
    distinctColors.push_back(RGBA(100, 255, 100, 255));  // Light Green
    distinctColors.push_back(RGBA(255, 255, 100, 255));  // Light Yellow
    distinctColors.push_back(RGBA(255, 100, 255, 255));  // Light Magenta
    distinctColors.push_back(RGBA(100, 255, 255, 255));  // Light Cyan
    distinctColors.push_back(RGBA(255, 180, 100, 255));  // Light Orange
    distinctColors.push_back(RGBA(180, 100, 255, 255));  // Light Purple
    distinctColors.push_back(RGBA(100, 180, 100, 255));  // Medium Green
    distinctColors.push_back(RGBA(180, 180, 255, 255));  // Medium Blue
    
    // Use pre-defined colors for the first few clusters, then generate additional colors if needed
    for (int i = 0; i <= maxCluster; i++) {
        if (i < static_cast<int>(distinctColors.size())) {
            clusterColors.push_back(distinctColors[i]);
        } else {
            // Generate additional colors with high contrast for clusters beyond our predefined list
            unsigned char r = (73 * (i + 1)) % 256;
            unsigned char g = (121 * (i + 1)) % 256;
            unsigned char b = (167 * (i + 1)) % 256;
            clusterColors.push_back(RGBA(r, g, b, 255));
        }
    }

    // Find min and max values for each feature for proper scaling
    std::vector<double> minValues(numFeatures, std::numeric_limits<double>::max());
    std::vector<double> maxValues(numFeatures, -std::numeric_limits<double>::max());

    for (size_t i = 0; i < data.size(); ++i) {
        for (size_t j = 0; j < numFeatures && j < data[i].size(); ++j) {
            minValues[j] = std::min(minValues[j], data[i][j]);
            maxValues[j] = std::max(maxValues[j], data[i][j]);
        }
    }

    // Calculate the center (origin) of the plot
    int centerX = margin_left + effectiveWidth / 2;
    int centerY = margin_top + effectiveHeight / 2;

    // Calculate point thickness based on data density
    int pointThickness = std::max(6, std::min(16, static_cast<int>(8.0 * (effectiveWidth / 800.0))));

    // Calculate plot area padding (percentage of effective dimensions)
    double paddingRatio = 0.1; // 10% padding
    int paddingX = static_cast<int>(effectiveWidth * paddingRatio);
    int paddingY = static_cast<int>(effectiveHeight * paddingRatio);

    // Usable plotting area after padding
    int usableWidth = effectiveWidth - 2 * paddingX;
    int usableHeight = effectiveHeight - 2 * paddingY;
    
    // Draw border and grid lines with similar code from the original implementation
    RGBA borderColor(0xFF, 0xFF, 0xFF, 0xFF);  // White border
    
    // Draw border (top, bottom, left, right)
    for (int x = margin_left - 2; x <= width - margin_right + 2; x++) {
        for (int t = 0; t < 2; t++) {
            int y = margin_top - 2 + t;
            if (y >= 0 && y < height && x >= 0 && x < width) {
                image.SetPixel(x, y, borderColor);
            }
        }
    }
    
    for (int x = margin_left - 2; x <= width - margin_right + 2; x++) {
        for (int t = 0; t < 2; t++) {
            int y = height - margin_bottom + t;
            if (y >= 0 && y < height && x >= 0 && x < width) {
                image.SetPixel(x, y, borderColor);
            }
        }
    }
    
    for (int y = margin_top - 2; y <= height - margin_bottom + 2; y++) {
        for (int t = 0; t < 2; t++) {
            int x = margin_left - 2 + t;
            if (y >= 0 && y < height && x >= 0 && x < width) {
                image.SetPixel(x, y, borderColor);
            }
        }
    }
    
    for (int y = margin_top - 2; y <= height - margin_bottom + 2; y++) {
        for (int t = 0; t < 2; t++) {
            int x = width - margin_right + t;
            if (y >= 0 && y < height && x >= 0 && x < width) {
                image.SetPixel(x, y, borderColor);
            }
        }
    }
    
    // Draw grid lines
    RGBA majorGridColor(0xD0, 0xD0, 0xD0, 0xAA);  // Light gray for major grid lines
    RGBA minorGridColor(0xA0, 0xA0, 0xA0, 0x55);  // Very light gray for minor grid lines
    
    // Number of grid divisions
    int majorGridDivisions = 4;   
    int minorGridDivisions = 16;  
    
    // Draw minor and major grid lines
    for (int i = 0; i <= minorGridDivisions; i++) {
        float percentage = static_cast<float>(i) / minorGridDivisions;
        int x = margin_left + static_cast<int>(percentage * effectiveWidth);
        int y = margin_top + static_cast<int>(percentage * effectiveHeight);
        
        if (i % (minorGridDivisions / majorGridDivisions) != 0) { 
            drawLine(x, margin_top, x, height - margin_bottom, minorGridColor);
            drawLine(margin_left, y, width - margin_right, y, minorGridColor);
        } else {
            drawLine(x, margin_top, x, height - margin_bottom, majorGridColor);
            drawLine(margin_left, y, width - margin_right, y, majorGridColor);
        }
    }
    
    // Draw axes at the center with thicker lines
    RGBA axisColor(0xF0, 0xF0, 0xF0, 0xFF);  // Brighter gray for axes
    
    // X-axis (thicker line)
    for (int offset = -2; offset <= 2; offset++) {
        drawLine(margin_left, centerY + offset, width - margin_right, centerY + offset, axisColor);
    }
    
    // Y-axis (thicker line)
    for (int offset = -2; offset <= 2; offset++) {
        drawLine(centerX + offset, margin_top, centerX + offset, height - margin_bottom, axisColor);
    }

    // For each pair of features, plot the data points
    for (size_t i = 0; i < numFeatures; ++i) {
        for (size_t j = i + 1; j < numFeatures; ++j) {
            // Calculate scales maintaining aspect ratio
            double rangeX = maxValues[i] - minValues[i];
            double rangeY = maxValues[j] - minValues[j];

            if (rangeX < 1e-10) rangeX = 1.0;
            if (rangeY < 1e-10) rangeY = 1.0;

            double scaleByX = usableWidth / rangeX;
            double scaleByY = usableHeight / rangeY;
            double commonScale = std::min(scaleByX, scaleByY);
            
            double scaleX = commonScale;
            double scaleY = commonScale;
            
            // Structure to store points for each cluster
            std::vector<std::vector<std::pair<int, int> > > clusterPoints;
            for (int i = 0; i <= maxCluster; ++i) {
                std::vector<std::pair<int, int> > empty;
                clusterPoints.push_back(empty);
            }
            
            // Draw each data point for this feature pair and collect cluster points
            for (size_t k = 0; k < data.size(); ++k) {
                if (data[k].size() <= std::max(i, j)) continue; 
                
                // Map data coordinates to screen coordinates
                double valueX = data[k][i];
                double valueY = data[k][j];
                
                // Scale relative to center
                int screenX = centerX + static_cast<int>((valueX - (minValues[i] + maxValues[i]) / 2) * scaleX);
                int screenY = centerY - static_cast<int>((valueY - (minValues[j] + maxValues[j]) / 2) * scaleY);
                
                screenX = clamp(screenX, margin_left + paddingX, width - margin_right - paddingX);
                screenY = clamp(screenY, margin_top + paddingY, height - margin_bottom - paddingY);
                
                // Store the point in its cluster collection
                if (k < labels.size() && labels[k] >= 0 && labels[k] <= maxCluster) {
                    clusterPoints[labels[k]].push_back(std::make_pair(screenX, screenY));
                }
                
                // Select color based on the cluster label
                RGBA pointColorToUse;
                if (k < labels.size() && labels[k] >= 0 && labels[k] < clusterColors.size()) {
                    pointColorToUse = clusterColors[labels[k]];
                } else {
                    // Default color if label is invalid
                    pointColorToUse = RGBA(128, 128, 128, 255); 
                }
                
                drawPoint(screenX, screenY, pointThickness, pointColorToUse);
            }
            
            // Draw circles around each cluster
            for (int clusterID = 0; clusterID <= maxCluster; ++clusterID) {
                if (clusterPoints[clusterID].empty()) continue;
                
                // Get the color for this cluster
                RGBA clusterColor = clusterID < clusterColors.size() ? 
                                    clusterColors[clusterID] : 
                                    RGBA(128, 128, 128, 255);
                
                // Make the outline semi-transparent
                RGBA outlineColor = clusterColor;
                outlineColor.a = 150; // Semi-transparent
                
                // Find the centroid of this cluster in screen coordinates
                int sumX = 0, sumY = 0;
                for (size_t p = 0; p < clusterPoints[clusterID].size(); ++p) {
                    sumX += clusterPoints[clusterID][p].first;
                    sumY += clusterPoints[clusterID][p].second;
                }
                int centroidX = sumX / clusterPoints[clusterID].size();
                int centroidY = sumY / clusterPoints[clusterID].size();
                
                // Find the maximum distance from centroid to any point in the cluster
                int maxDist = 0;
                for (size_t p = 0; p < clusterPoints[clusterID].size(); ++p) {
                    int dx = clusterPoints[clusterID][p].first - centroidX;
                    int dy = clusterPoints[clusterID][p].second - centroidY;
                    int dist = static_cast<int>(std::sqrt(static_cast<double>(dx*dx + dy*dy)));
                    maxDist = std::max(maxDist, dist);
                }
                
                // Add some padding to the radius
                int radius = maxDist + pointThickness * 2;
                
                // Draw a circle to encompass all points in the cluster
                drawCircle(centroidX, centroidY, radius, outlineColor);
                
                // Draw actual centroids with a special marker (if available)
                if (centroids.size() > clusterID && centroids[clusterID].size() > j) {
                    // Map the actual centroid coordinates to screen coordinates
                    double centValueX = centroids[clusterID][i];
                    double centValueY = centroids[clusterID][j];
                    
                    int centScreenX = centerX + static_cast<int>((centValueX - (minValues[i] + maxValues[i]) / 2) * scaleX);
                    int centScreenY = centerY - static_cast<int>((centValueY - (minValues[j] + maxValues[j]) / 2) * scaleY);
                    
                    // Ensure the centroid is within the plotting area
                    centScreenX = clamp(centScreenX, margin_left + paddingX, width - margin_right - paddingX);
                    centScreenY = clamp(centScreenY, margin_top + paddingY, height - margin_bottom - paddingY);
                    
                    // Draw a special marker for the actual centroid
                    RGBA centroidMarkerColor = RGBA(0xFF, 0xFF, 0xFF, 0xFF); // White
                    
                    // Draw a white cross inside a colored circle
                    drawPoint(centScreenX, centScreenY, pointThickness * 1.5, clusterColor);
                    
                    // Cross lines
                    for (int lineOffset = -pointThickness; lineOffset <= pointThickness; lineOffset++) {
                        if (centScreenX + lineOffset >= margin_left && 
                            centScreenX + lineOffset < width - margin_right &&
                            centScreenY >= margin_top && 
                            centScreenY < height - margin_bottom) {
                            image.SetPixel(centScreenX + lineOffset, centScreenY, centroidMarkerColor);
                        }
                        
                        if (centScreenX >= margin_left && 
                            centScreenX < width - margin_right &&
                            centScreenY + lineOffset >= margin_top && 
                            centScreenY + lineOffset < height - margin_bottom) {
                            image.SetPixel(centScreenX, centScreenY + lineOffset, centroidMarkerColor);
                        }
                    }
                }
            }
        }
    }
}

void DataDrawer::drawCentroidCircle(int x, int y, int radius, const RGBA& color) {
    drawCircle(x, y, radius, color);
}

void DataDrawer::drawClusterCircle(int x, int y, int radius, const RGBA& color) {
    drawCircle(x, y, radius, color);
}

}  // namespace shmea
