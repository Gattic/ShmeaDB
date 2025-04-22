#include "ChartStyler.h"
#include <cstdio>
#include <cmath>
#include <limits>

namespace shmea {

ChartStyler::ChartStyler(ColorManager& colorManager, ChartLayout& chartLayout,
                     ShapeRenderer& shapeRenderer, TextRenderer& textRenderer,
                     GridRenderer& gridRenderer, DataMapper& mapper)
    : colors(colorManager),
      layout(chartLayout),
      shapes(shapeRenderer),
      text(textRenderer),
      grid(gridRenderer),
      dataMapper(mapper)
{
}

ChartStyler::~ChartStyler() {
    // Nothing to clean up
}

void ChartStyler::prepareLegendColors(const std::vector<std::string>& labels, const std::vector<RGBA>& colors,
                                 std::vector<std::string>& outLabels, std::vector<RGBA>& outColors) {
    outLabels.clear();
    outColors.clear();
    
    // Make sure both vectors have the same size
    if (labels.size() != colors.size() || labels.empty()) {
        return;
    }
    
    // Copy labels
    outLabels = labels;
    
    // Make sure colors are fully opaque for legend dots
    for (size_t i = 0; i < colors.size(); ++i) {
        RGBA legendColor = colors[i];
        legendColor.a = 0xFF; // Full opacity for legend dots
        outColors.push_back(legendColor);
    }
}

int ChartStyler::addLegend(const std::vector<std::string>& labels, const std::vector<RGBA>& colors, 
                         int x, int y, unsigned int fontSize) {
    if (labels.size() != colors.size() || labels.empty()) {
        return 0;
    }
    
    // Calculate dimensions based on content
    int itemHeight = fontSize + 6;
    int colorIndicatorSize = fontSize - 2;
    int colorTextPadding = 12;
    int minItemSpacing = 30; // Minimum spacing between items
    
    // Calculate legend width based on text length and horizontal layout
    int totalWidth = 0;
    std::vector<int> textWidths; // Store individual text widths for later use
    
    for (size_t i = 0; i < labels.size(); ++i) {
        int textWidth = layout.estimateTextWidth(labels[i], fontSize);
        textWidths.push_back(textWidth);
        totalWidth += colorIndicatorSize + colorTextPadding + textWidth;
        if (i < labels.size() - 1) {
            totalWidth += minItemSpacing;
        }
    }
    
    // Add padding to total width
    int sidePadding = 30; // 15px padding on each side
    int legendWidth = totalWidth + sidePadding * 2;
    int legendHeight = itemHeight + 24; // Single row height + top/bottom padding
    
    // Draw the info box with gradient background
    grid.drawInfoBox(x, y, legendWidth, legendHeight, "", fontSize);
    
    // Draw each legend item with exact positioning - now horizontally aligned
    int currentX = x + sidePadding; // Start with left padding
    
    for (size_t i = 0; i < labels.size(); ++i) {
        // Draw color indicator dot
        int dotX = currentX;
        int dotY = y + legendHeight/2; // Centered vertically
        
        // Scale dot coordinates for supersampling
        int ssaaX = layout.getSsaaFactor() * dotX;
        int ssaaY = layout.getSsaaFactor() * dotY;
        
        // Make sure the radius is an exact multiple of sampling factor for perfect circles
        int ssaaRadius = layout.getSsaaFactor() * (colorIndicatorSize/2);
        
        // Check if this is the "Centroid" label which should use the special centroid style
        bool isCentroid = (i == labels.size() - 1 && labels[i] == "Centroid");
        
        if (isCentroid) {
            // Draw white circle with colored cross, exactly matching the main visualization style
            
            // Draw white filled circle
            RGBA whiteFill(0xFF, 0xFF, 0xFF, 0xFF); // Solid white
            shapes.drawCircle(ssaaX, ssaaY, ssaaRadius, whiteFill, true);
            
            // Get appropriate cross color (darker version of the cluster color)
            // For centroid we use a dark gray cross to match the style in the visualization
            RGBA crossColor = RGBA(0x40, 0x40, 0x40, 0xFF); // Dark gray
            
            // Draw cross with proper thickness
            int crossThickness = layout.getSsaaFactor(); // 1px thickness
            int crossLength = layout.getSsaaFactor() * (colorIndicatorSize/3); // Slightly smaller than radius
            
            // Draw horizontal line
            for (int y = -crossThickness; y <= crossThickness; ++y) {
                shapes.drawLine(
                    ssaaX - crossLength, ssaaY + y,
                    ssaaX + crossLength, ssaaY + y,
                    crossColor
                );
            }
            
            // Draw vertical line
            for (int x = -crossThickness; x <= crossThickness; ++x) {
                shapes.drawLine(
                    ssaaX + x, ssaaY - crossLength,
                    ssaaX + x, ssaaY + crossLength,
                    crossColor
                );
            }
        } else {
            // For regular cluster colors, draw a solid circle
            shapes.drawCircle(
                ssaaX,           // Use scaled x coordinate
                ssaaY,           // Use scaled y coordinate
                ssaaRadius,      // Use scaled radius 
                colors[i],
                true);
        }
        
        // Draw label text - no need to scale these as TextRenderer handles it
        text.drawText(
            dotX + colorTextPadding,
            dotY,
            labels[i],
            this->colors.getElementColor("legend"),
            fontSize,
            false);
        
        // Move to next item position based on actual width
        currentX += colorIndicatorSize + colorTextPadding + textWidths[i] + minItemSpacing;
    }
    
    // Return the height of the legend box
    return legendHeight;
}

int ChartStyler::createClusterLegend(const std::vector<RGBA>& clusterColors, int numClusters, int x, int y) {
    std::vector<std::string> legendLabels;
    std::vector<RGBA> legendColors;
    
    // Add cluster entries
    for (int i = 0; i < numClusters; ++i) {
        char label[32];
        std::sprintf(label, "Cluster %d", i);
        legendLabels.push_back(label);
        legendColors.push_back(clusterColors[i]);
    }
    
    // Add the centroid legend item
    legendLabels.push_back("Centroid");
    legendColors.push_back(RGBA(0xFF, 0xFF, 0xFF, 0xFF)); // White for centroid
    
    // Process colors to ensure opacity
    std::vector<std::string> processedLabels;
    std::vector<RGBA> processedColors;
    prepareLegendColors(legendLabels, legendColors, processedLabels, processedColors);
    
    // Add the legend to the visualization
    return addLegend(processedLabels, processedColors, x, y, 16);
}

void ChartStyler::drawHistogramStats(const std::vector<int>& bins, int maxBinValue, int legendY, unsigned int fontSize) {
    // Print debug info to track issue
    printf("Drawing histogram stats box at legendY=%d\n", legendY);
    
    // Calculate statistics
    int sum = 0;
    int count = 0;
    int maxIndex = 0;
    
    for (size_t i = 0; i < bins.size(); ++i) {
        sum += bins[i];
        count += bins[i];
        if (bins[i] > bins[maxIndex]) {
            maxIndex = i;
        }
    }
    
    double mean = sum / (double)bins.size();
    double mode = maxIndex; // index with highest frequency - exactly like plotter.cpp
    
    // Format stats values with 1 decimal place - exact format from plotter.cpp
    char meanText[64], modeText[64], maxText[64], sumText[64];
    std::sprintf(meanText, "Mean: %.1f", mean);
    std::sprintf(modeText, "Mode: %.1f", mode); // Display mode as a float with 1 decimal place - exact format from plotter.cpp
    // Note: maxBinValue shows the actual maximum bin value, not the scaled display height
    std::sprintf(maxText, "Max: %d", maxBinValue);
    std::sprintf(sumText, "Sum: %d", sum);
    
    // Calculate required widths
    int titleWidth = layout.estimateTextWidth("Histogram Statistics", 22);
    int meanWidth = layout.estimateTextWidth(meanText, 20);
    int modeWidth = layout.estimateTextWidth(modeText, 20);
    int maxWidth = layout.estimateTextWidth(maxText, 20);
    int sumWidth = layout.estimateTextWidth(sumText, 20);
    
    // Calculate minimum spacing between stats
    int minStatSpacing = 30;
    
    // Calculate box dimensions
    int sidePadding = 20;
    int boxWidth = sidePadding * 2 + meanWidth + modeWidth + maxWidth + sumWidth + minStatSpacing * 3;
    boxWidth = std::max(boxWidth, titleWidth + sidePadding * 2); // Ensure box is wide enough for title
    int boxHeight = 90; // Single row height + padding
    
    // Position the box aligned with the right edge of the chart
    int boxX = layout.getWidth() - layout.getMarginRight() - boxWidth;
    
    // Debug the initial position
    printf("Initial boxX=%d, width=%d, layout.width=%d, margin_right=%d\n",
           boxX, boxWidth, layout.getWidth(), layout.getMarginRight());
    
    // Check if we need to shift the box left to avoid overlapping with the logo
    // Logo is typically positioned in the top right with 20px padding
    int logoWidth = layout.getLogoWidth();
    int logoHeight = layout.getLogoHeight();
    
    printf("Logo dimensions: width=%d, height=%d\n", logoWidth, logoHeight);
    
    // Only reposition if the logo actually exists and has positive dimensions
    printf("HERE0: boxX=%d, boxWidth=%d, layout.width=%d, logoWidth=%d\n",
		   boxX, boxWidth, layout.getWidth(), logoWidth);
    if (logoWidth > 0 && logoHeight > 0) {
        // Logo is present - need to shift the box left if it would overlap
        // Logo is positioned at: (width - logoWidth - 15, 20)
        int logoLeft = layout.getWidth() - logoWidth - 15;
        int logoBottom = 20 + logoHeight;
        
        printf("Logo position: left=%d, bottom=%d, legendY=%d\n", logoLeft, logoBottom, legendY);
        
        // Check if there would be any overlap - be more conservative
        if (boxX + boxWidth >= logoLeft && legendY <= logoBottom) {
            // Shift the box left to avoid the logo
            // Allow 15px padding between logo and stats box
            int newBoxX = logoLeft - boxWidth - 15;
            printf("Moving box from x=%d to x=%d to avoid logo\n", boxX, newBoxX);
            boxX = newBoxX;
        }
    }
    
    int boxY = legendY; // Same position as legend
    
    // Print final box position
    printf("Final histogram stats box: x=%d, y=%d, width=%d, height=%d\n", 
           boxX, boxY, boxWidth, boxHeight);
    
    // Draw the info box with gradient background
    grid.drawInfoBox(boxX, boxY, boxWidth, boxHeight, "", fontSize);
    
    // Draw the text with styling
    RGBA textColor = colors.getElementColor("legend"); // White text
    
    // Main title
    text.drawText(boxX + sidePadding, boxY + 20, "Histogram Statistics", textColor, 22, false);
    
    // Horizontal separator
    RGBA lineColor(0xFF, 0xFF, 0xFF, 0x40); // 25% opacity white
    shapes.drawLine(
        layout.getSsaaFactor() * (boxX + sidePadding),
        layout.getSsaaFactor() * (boxY + 32),
        layout.getSsaaFactor() * (boxX + boxWidth - sidePadding),
        layout.getSsaaFactor() * (boxY + 32),
        lineColor);
    
    // Calculate positions for each stat to evenly distribute them
    int statY = boxY + 55;
    int contentWidth = boxWidth - (sidePadding * 2);
    int usedWidth = meanWidth + modeWidth + maxWidth + sumWidth;
    int extraSpace = contentWidth - usedWidth;
    int spacing = extraSpace / 3; // Three spaces between four stats
    
    // Draw stats with dynamically calculated positions - exact positioning from plotter.cpp
    int statX = boxX + sidePadding;
    text.drawText(statX, statY, meanText, textColor, 20, false);
    
    statX += meanWidth + spacing;
    text.drawText(statX, statY, modeText, textColor, 20, false);
    
    statX += modeWidth + spacing;
    text.drawText(statX, statY, maxText, textColor, 20, false);
    
    statX += maxWidth + spacing;
    text.drawText(statX, statY, sumText, textColor, 20, false);
}

void ChartStyler::drawCandlestickPriceInfo(const std::vector<DataMapper::CandleData>& candles,
                                        const RGBA& bullishColor, const RGBA& bearishColor,
                                        int legendY)
{
    if (candles.empty()) {
        return;
    }
    
    const DataMapper::CandleData& latestCandle = candles[candles.size() - 1];
    const DataMapper::CandleData& firstCandle = candles[0];
    
    // Calculate price change
    double priceChange = latestCandle.close - firstCandle.open;
    double percentChange = (priceChange / firstCandle.open) * 100.0;
    
    // Determine if overall trend is bullish or bearish
    bool isBullish = priceChange >= 0;
    RGBA trendColor = isBullish ? bullishColor : bearishColor;
    
    // Format price info components
    char closeText[64], changeText[64], percentText[64];
    std::sprintf(closeText, "Close: %.2f", latestCandle.close);
    std::sprintf(changeText, "Change: %.2f", priceChange);
    std::sprintf(percentText, "(%.2f%%)", percentChange);
    
    // Calculate required widths
    int closeWidth = layout.estimateTextWidth(closeText, 22);
    int changeWidth = layout.estimateTextWidth(changeText, 22);
    int percentWidth = layout.estimateTextWidth(percentText, 22);
    
    // Calculate minimum spacing between components
    int minComponentSpacing = 30;
    int indicatorSize = 8;
    int indicatorSpace = 40; // Space for indicator including padding
    
    // Calculate box dimensions
    int sidePadding = 20;
    int boxWidth = sidePadding * 2 + closeWidth + changeWidth + percentWidth + indicatorSpace + 
                 minComponentSpacing * 2; // Two spaces between three components plus indicator
    int boxHeight = 40;
    
    // Calculate box position - align right edge with the chart's right edge
    int boxX = layout.getWidth() - layout.getMarginRight() - boxWidth;
    int boxY = legendY; // Same position as legend

    // Draw the info box with gradient background
    grid.drawInfoBox(boxX, boxY, boxWidth, boxHeight, "", 22);
    
    // Calculate positions for each component to evenly distribute them
    int textY = boxY + (boxHeight / 2);
    int contentWidth = boxWidth - (sidePadding * 2) - indicatorSpace;
    int usedWidth = closeWidth + changeWidth + percentWidth;
    int extraSpace = contentWidth - usedWidth;
    int spacing = extraSpace / 2; // Two spaces between three components
    
    // Draw components with dynamically calculated positions
    int textX = boxX + sidePadding;
    text.drawText(textX, textY, closeText, colors.getElementColor("legend"), 22, false);
    
    textX += closeWidth + spacing;
    text.drawText(textX, textY, changeText, trendColor, 22, false);
    
    textX += changeWidth + spacing;
    text.drawText(textX, textY, percentText, trendColor, 22, false);
    
    // Add a small colored indicator box to show trend
    int indicatorX = boxX + boxWidth - sidePadding - indicatorSize;
    int indicatorY = boxY + (boxHeight - indicatorSize) / 2;
    
    // Draw filled rectangle with the appropriate color
    shapes.drawRect(
        layout.getSsaaFactor() * indicatorX,
        layout.getSsaaFactor() * indicatorY,
        layout.getSsaaFactor() * indicatorSize,
        layout.getSsaaFactor() * indicatorSize,
        trendColor, true);
}

void ChartStyler::calculateClusterBounds(
    const std::vector<std::vector<double> >& data,
    const std::vector<int>& labels,
    int cluster,
    std::vector<DataMapper::Point>& clusterCenters,
    std::vector<int>& clusterRadii,
    const DataMapper::AxisRange& xRange,
    const DataMapper::AxisRange& yRange)
{
    // Filter points belonging to this cluster
    std::vector<DataMapper::Point> clusterPoints;
    
    // Keep track of the data-space coordinates of points in this cluster
    std::vector<std::pair<double, double> > dataSpacePoints;
    
    // Calculate min/max values for the cluster points
    double minX = std::numeric_limits<double>::max();
    double maxX = -std::numeric_limits<double>::max();
    double minY = std::numeric_limits<double>::max();
    double maxY = -std::numeric_limits<double>::max();
    
    // Calculate center position
    double sumX = 0.0;
    double sumY = 0.0;
    
    // Also calculate data-space center (important for matching centroids)
    double dataSpaceSumX = 0.0;
    double dataSpaceSumY = 0.0;
    int count = 0;
    
    // Process data points
    for (size_t i = 0; i < data.size(); ++i) {
        if (labels[i] == cluster && data[i].size() >= 2) {
            // Store data-space coordinates
            double dataX = data[i][0];
            double dataY = data[i][1];
            dataSpacePoints.push_back(std::make_pair(dataX, dataY));
            
            // Accumulate for data-space center calculation
            dataSpaceSumX += dataX;
            dataSpaceSumY += dataY;
            
            // Map data point to screen
            DataMapper::Point p = dataMapper.mapDataToScreen(dataX, dataY, xRange, yRange);
            
            // Update min/max
            minX = std::min(minX, static_cast<double>(p.x));
            maxX = std::max(maxX, static_cast<double>(p.x));
            minY = std::min(minY, static_cast<double>(p.y));
            maxY = std::max(maxY, static_cast<double>(p.y));
            
            // Update sum for center calculation
            sumX += p.x;
            sumY += p.y;
            count++;
            
            // Store point for further processing
            clusterPoints.push_back(p);
        }
    }
    
    // If cluster has points, calculate center and radius
    if (count > 0) {
        // Calculate data-space cluster center
        double dataSpaceClusterX = dataSpaceSumX / count;
        double dataSpaceClusterY = dataSpaceSumY / count;
        
        // Map data-space center to screen space - this ensures consistency with how centroids are drawn
        DataMapper::Point centerPoint = dataMapper.mapDataToScreen(
            dataSpaceClusterX, dataSpaceClusterY, xRange, yRange);
        
        int centerX = static_cast<int>(centerPoint.x);
        int centerY = static_cast<int>(centerPoint.y);
        
        printf("Cluster %d data-space center: (%.2f, %.2f) -> screen center: (%d, %d)\n", 
              cluster, dataSpaceClusterX, dataSpaceClusterY, centerX, centerY);
        
        clusterCenters.push_back(DataMapper::Point(centerX, centerY));
        
        // Calculate radius to encompass all points
        int maxDistSquared = 0;
        for (size_t i = 0; i < clusterPoints.size(); ++i) {
            int dx = clusterPoints[i].x - centerX;
            int dy = clusterPoints[i].y - centerY;
            int distSquared = dx*dx + dy*dy;
            maxDistSquared = std::max(maxDistSquared, distSquared);
        }
        
        // Convert to radius and add some padding
        int radius = static_cast<int>(sqrt(maxDistSquared)) + 20;
        clusterRadii.push_back(radius);
    }
}

void ChartStyler::drawClusterCircles(
    const std::vector<DataMapper::Point>& clusterCenters,
    const std::vector<int>& clusterRadii,
    const std::vector<RGBA>& clusterColors)
{
    // Ensure we have valid data
    if (clusterCenters.size() != clusterRadii.size() || 
        clusterCenters.empty() || clusterColors.empty()) {
        return;
    }
    
    // Draw each cluster circle with the exact styling from CSS
    for (size_t cluster = 0; cluster < clusterCenters.size(); ++cluster) {
        // Skip empty clusters
        if (clusterRadii[cluster] == 0) continue;
        
        // Get the cluster color
        RGBA circleColor = clusterColors[cluster % clusterColors.size()];
        
        // Use the appropriate shadow color for all 10 clusters
        RGBA shadowColor;
        
        // Try to find a specific shadow color for this cluster
        char shadowKeyBuffer[32];
        std::sprintf(shadowKeyBuffer, "cluster%dShadow", static_cast<int>(cluster+1));
        std::string shadowKey(shadowKeyBuffer);
        
        // Use the specific cluster shadow color if defined
        if (colors.hasElementColor(shadowKey)) {
            shadowColor = colors.getElementColor(shadowKey);
        } else {
            // Otherwise use the semi-transparent cluster color as fallback
            shadowColor = RGBA(
                circleColor.r,
                circleColor.g,
                circleColor.b,
                0x80 // 50% opacity
            );
        }
        
        // Scale the coordinates and dimensions for supersampling
        int radius = layout.getSsaaFactor() * clusterRadii[cluster];
        int centerX = layout.getSsaaFactor() * clusterCenters[cluster].x;
        int centerY = layout.getSsaaFactor() * clusterCenters[cluster].y;
        
        // Draw filled circle with subtle gradient effect - exactly as in CSS
        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                // Calculate exact distance from center
                float dist = std::sqrt(dx*dx + dy*dy);
                
                // Skip pixels outside the circle
                if (dist > radius) continue;
                
                // Calculate the normalized distance (0 at center, 1 at edge)
                float normDist = dist / radius;
                
                int drawX = centerX + dx;
                int drawY = centerY + dy;
                
                // Skip pixels outside the image bounds
                if (drawX < 0 || drawX >= static_cast<int>(layout.getWidth() * layout.getSsaaFactor()) ||
                    drawY < 0 || drawY >= static_cast<int>(layout.getHeight() * layout.getSsaaFactor())) {
                    continue;
                }
                
                // Prepare the fill color with appropriate opacity - matching CSS
                RGBA fillColor = RGBA(
                    circleColor.r,
                    circleColor.g,
                    circleColor.b,
                    38  // 15% opacity, exactly as in CSS
                );
                
                // Prepare border color - smaller border (thinner than 1px)
                if (normDist > 0.985f) { // Increased from 0.97f for thinner border
                    RGBA borderColor = RGBA(
                        circleColor.r, 
                        circleColor.g, 
                        circleColor.b, 
                        184  // 72% opacity
                    );
                    
                    // Use the ShapeRenderer to blend the border pixel
                    shapes.blendPixel(drawX, drawY, borderColor, borderColor.a / 255.0f);
                    continue;
                }
                
                // For inner pixels, blend the fill color first
                shapes.blendPixel(drawX, drawY, fillColor, fillColor.a / 255.0f);
                
                // Apply inner shadow effect exactly as in CSS
                // Inner shadow is stronger near edge and fades toward center
                if (normDist > 0.3f) {
                    // Calculate shadow intensity based on distance from edge
                    // More intense near edge, fades toward center
                    float shadowFactor = (normDist - 0.3f) / 0.7f; // 0 at 30%, 1 at edge
                    shadowFactor = shadowFactor * shadowFactor; // Quadratic falloff
                    
                    // Scale to 0-30% opacity as in CSS
                    float shadowIntensity = shadowFactor * 0.3f;
                    
                    // Create inner shadow color
                    RGBA innerShadowColor = RGBA(
                        shadowColor.r,
                        shadowColor.g,
                        shadowColor.b,
                        static_cast<unsigned char>(shadowColor.a * shadowIntensity)
                    );
                    
                    // Add inner shadow effect
                    shapes.blendPixel(drawX, drawY, innerShadowColor, innerShadowColor.a / 255.0f);
                }
            }
        }
    }
}

void ChartStyler::drawCentroids(
    const std::vector<std::vector<double> >& centroids,
    const std::vector<RGBA>& clusterColors,
    const DataMapper::AxisRange& xRange,
    const DataMapper::AxisRange& yRange)
{
    // Check for valid data
    if (centroids.empty() || centroids[0].size() < 2) {
        printf("drawCentroids: No valid centroid data found\n");
        return;
    }
    
    printf("Drawing %zu centroids. X range: [%.2f, %.2f], Y range: [%.2f, %.2f]\n", 
           centroids.size(), xRange.min, xRange.max, yRange.min, yRange.max);
    
    // Draw each centroid using the style from plotter.cpp
    for (size_t i = 0; i < centroids.size() && i < clusterColors.size(); ++i) {
        if (centroids[i].size() < 2) {
            printf("Warning: Centroid %zu doesn't have enough coordinates (needed 2, has %zu)\n", 
                   i, centroids[i].size());
            continue;
        }
        
        // Get the centroid coordinates in data space
        double dataX = centroids[i][0];
        double dataY = centroids[i][1];
        
        printf("Centroid %zu data coordinates: (%.2f, %.2f)\n", i, dataX, dataY);
        
        // Map centroid from data space to screen space
        DataMapper::Point p = dataMapper.mapDataToScreen(dataX, dataY, xRange, yRange);
        
        printf("Centroid %zu screen coordinates: (%.2f, %.2f)\n", i, p.x, p.y);
        
        // Verify that mapped centroid coordinates are within the valid plot area
        int plotLeft = layout.getMarginLeft();
        int plotRight = layout.getWidth() - layout.getMarginRight();
        int plotTop = layout.getMarginTop();
        int plotBottom = layout.getHeight() - layout.getMarginBottom();
        
        if (p.x < plotLeft || p.x > plotRight || p.y < plotTop || p.y > plotBottom) {
            printf("Warning: Mapped centroid coordinates (%.2f, %.2f) are outside the plot area [%d,%d,%d,%d]\n", 
                   p.x, p.y, plotLeft, plotTop, plotRight, plotBottom);
            
            // Clamp coordinates to the plot area to ensure visibility
            p.x = std::max(std::min(p.x, (double)plotRight), (double)plotLeft);
            p.y = std::max(std::min(p.y, (double)plotBottom), (double)plotTop);
            
            printf("Clamped centroid coordinates to: (%.2f, %.2f)\n", p.x, p.y);
        }
        
        // Scale coordinates for supersampling
        int ssaaX = layout.getSsaaFactor() * p.x;
        int ssaaY = layout.getSsaaFactor() * p.y;
        
        // First draw shadow with offset - exactly as in plotter.cpp
        RGBA shadowColor(0x00, 0x00, 0x00, 0x66); // 40% opacity black shadow
        int shadowOffset = layout.getSsaaFactor() * 3; // 3px offset as in plotter.cpp
        
        // Draw larger soft shadow first (matches plotter.cpp box-shadow effect)
        int shadowRadius = layout.getSsaaFactor() * 12;
        for (int dy = -shadowRadius; dy <= shadowRadius; dy++) {
            for (int dx = -shadowRadius; dx <= shadowRadius; dx++) {
                int distSqr = dx*dx + dy*dy;
                if (distSqr > shadowRadius*shadowRadius) continue; // Only within radius
                
                // Calculate shadow intensity - fade out toward edges (Gaussian-like)
                float shadowDistance = std::sqrt(distSqr);
                float shadowIntensity = 0.4f * std::exp(-shadowDistance / (shadowRadius / 2.0f));
                
                int drawX = ssaaX + dx + shadowOffset;
                int drawY = ssaaY + dy + shadowOffset;
                
                if (drawX >= 0 && drawX < static_cast<int>(layout.getSsaaImage().getWidth()) &&
                    drawY >= 0 && drawY < static_cast<int>(layout.getSsaaImage().getHeight())) {
                    RGBA pixelShadow = shadowColor;
                    pixelShadow.a = static_cast<unsigned char>(shadowColor.a * shadowIntensity);
                    shapes.blendPixel(drawX, drawY, pixelShadow, shadowIntensity);
                }
            }
        }
        
        // Draw the white circle on top of the shadow
        RGBA whiteFill(0xFF, 0xFF, 0xFF, 0xFF); // Solid white
        int circleRadius = layout.getSsaaFactor() * 10; // 10px radius as in plotter.cpp
        
        // Draw solid white circle
        shapes.drawCircle(ssaaX, ssaaY, circleRadius, whiteFill, true);
        
        // Get the appropriate cross color - matching plotter.cpp exactly
        RGBA crossColor;
        
        // Try to find a dark version of this cluster color
        char darkColorKeyBuffer[32];
        std::sprintf(darkColorKeyBuffer, "cluster%dDark", (int)(i+1));
        std::string darkColorKey(darkColorKeyBuffer);
        
        // Check if we have a predefined dark color variant for any cluster
        if (colors.hasElementColor(darkColorKey)) {
            // Use the predefined dark color
            crossColor = colors.getElementColor(darkColorKey);
        } else {
            // Create a darkened version of the cluster color as fallback
            crossColor = RGBA(
                static_cast<unsigned char>(clusterColors[i].r * 0.6f),
                static_cast<unsigned char>(clusterColors[i].g * 0.6f),
                static_cast<unsigned char>(clusterColors[i].b * 0.6f),
                0xFF
            );
        }
        
        // Draw horizontal line of cross - exact styling from plotter.cpp
        int crossThickness = layout.getSsaaFactor() * 1;
        int crossLength = layout.getSsaaFactor() * 8;
        
        // Draw horizontal line
        for (int y = -crossThickness; y <= crossThickness; ++y) {
            shapes.drawLine(
                ssaaX - crossLength, ssaaY + y,
                ssaaX + crossLength, ssaaY + y,
                crossColor
            );
        }
        
        // Draw vertical line
        for (int x = -crossThickness; x <= crossThickness; ++x) {
            shapes.drawLine(
                ssaaX + x, ssaaY - crossLength,
                ssaaX + x, ssaaY + crossLength,
                crossColor
            );
        }
    }
}

void ChartStyler::drawClusterLabels(
    const std::vector<DataMapper::Point>& clusterCenters,
    const std::vector<int>& clusterRadii,
    const std::vector<std::vector<std::pair<int, int> > >& clusterPoints,
    const std::vector<RGBA>& clusterColors)
{
    // Check if we have valid data
    if (clusterCenters.empty() || clusterRadii.empty() || 
        clusterPoints.empty() || clusterColors.empty()) {
        return;
    }
    
    // Draw a label for each cluster
    for (size_t i = 0; i < clusterCenters.size() && i < clusterColors.size(); ++i) {
        // Create label text
        char labelText[32];
        std::sprintf(labelText, "Cluster %zu", i);
        
        // Format label for text dimensions
        int labelWidth = layout.estimateTextWidth(labelText, 22);
        int labelHeight = 30; // Approximate height for 22px font
        
        // Background padding
        int padX = 12;
        int padY = 8;
        int bgWidth = labelWidth + padX*2;
        int bgHeight = labelHeight + padY*2;
        
        // Setup for label placement
        int plotCenterX = layout.getMarginLeft() + layout.getPlotWidth() / 2;
        int plotCenterY = layout.getMarginTop() + layout.getPlotHeight() / 2;
        
        // Variables for final label position
        int posX, posY;
        
        // Calculate safe boundaries for label placement
        int safeLeftBound = static_cast<int>(layout.getMarginLeft()) + bgWidth/2 + 10;
        int safeRightBound = static_cast<int>(layout.getWidth() - layout.getMarginRight()) - bgWidth/2 - 10;
        int safeTopBound = static_cast<int>(layout.getMarginTop()) + bgHeight/2 + 10;
        int safeBottomBound = static_cast<int>(layout.getHeight() - layout.getMarginBottom()) - bgHeight/2 - 10;
        
        // Calculate the vector from plot center to cluster center
        double dx = clusterCenters[i].x - plotCenterX;
        double dy = clusterCenters[i].y - plotCenterY;
        
        // Calculate the angle based on the cluster's position in the plot
        double angle = std::atan2(dy, dx);
        
        // Calculate the point on the circle edge in this direction
        int touchX = clusterCenters[i].x + static_cast<int>(clusterRadii[i] * std::cos(angle));
        int touchY = clusterCenters[i].y + static_cast<int>(clusterRadii[i] * std::sin(angle));
        
        // Position the label based on the angle
        // This ensures the appropriate edge of the label touches the circle
        if (angle >= -3.14159265358979323846/4 && angle < 3.14159265358979323846/4) {
            // Right side of circle (label to the right)
            posX = touchX + bgWidth/2; // Left edge of label touches circle
            posY = touchY;             // Vertically centered
        } 
        else if (angle >= 3.14159265358979323846/4 && angle < 3*3.14159265358979323846/4) {
            // Bottom side of circle (label below)
            posX = touchX;             // Horizontally centered
            posY = touchY + bgHeight/2; // Top edge of label touches circle
        }
        else if ((angle >= 3*3.14159265358979323846/4 && angle <= 3.14159265358979323846) ||
                 (angle >= -3.14159265358979323846 && angle < -3*3.14159265358979323846/4)) {
            // Left side of circle (label to the left)
            posX = touchX - bgWidth/2; // Right edge of label touches circle
            posY = touchY;             // Vertically centered
        }
        else {
            // Top side of circle (label above)
            posX = touchX;             // Horizontally centered
            posY = touchY - bgHeight/2; // Bottom edge of label touches circle
        }
        
        // Check if the label would be outside chart boundaries
        int leftEdge = posX - bgWidth/2;
        int rightEdge = posX + bgWidth/2;
        int topEdge = posY - bgHeight/2;
        int bottomEdge = posY + bgHeight/2;
        
        // If outside boundaries, try standard positions (0°, 90°, 180°, 270°)
        if (leftEdge < static_cast<int>(layout.getMarginLeft()) || 
            rightEdge > static_cast<int>(layout.getWidth() - layout.getMarginRight()) || 
            topEdge < static_cast<int>(layout.getMarginTop()) || 
            bottomEdge > static_cast<int>(layout.getHeight() - layout.getMarginBottom())) {
            
            // Try standard angles to find a good position
            bool found = false;
            double testAngles[4] = {0, 3.14159265358979323846/2, 3.14159265358979323846, -3.14159265358979323846/2}; // 0°, 90°, 180°, 270°
            
            for (int j = 0; j < 4 && !found; j++) {
                int circleEdgeX = clusterCenters[i].x + static_cast<int>(clusterRadii[i] * std::cos(testAngles[j]));
                int circleEdgeY = clusterCenters[i].y + static_cast<int>(clusterRadii[i] * std::sin(testAngles[j]));
                
                // Position label based on angle
                int testX, testY;
                
                // Determine label position based on direction
                if (j == 0) { // Right
                    testX = circleEdgeX + bgWidth/2;
                    testY = circleEdgeY;
                } else if (j == 1) { // Down
                    testX = circleEdgeX;
                    testY = circleEdgeY + bgHeight/2;
                } else if (j == 2) { // Left
                    testX = circleEdgeX - bgWidth/2;
                    testY = circleEdgeY;
                } else { // Up
                    testX = circleEdgeX;
                    testY = circleEdgeY - bgHeight/2;
                }
                
                // Check if this position would be within boundaries
                int testLeftEdge = testX - bgWidth/2;
                int testRightEdge = testX + bgWidth/2;
                int testTopEdge = testY - bgHeight/2;
                int testBottomEdge = testY + bgHeight/2;
                
                if (testLeftEdge >= static_cast<int>(layout.getMarginLeft()) && 
                    testRightEdge <= static_cast<int>(layout.getWidth() - layout.getMarginRight()) && 
                    testTopEdge >= static_cast<int>(layout.getMarginTop()) && 
                    testBottomEdge <= static_cast<int>(layout.getHeight() - layout.getMarginBottom())) {
                    posX = testX;
                    posY = testY;
                    found = true;
                }
            }
            
            // If still not found, force within boundaries
            if (!found) {
                posX = std::max(safeLeftBound, std::min(safeRightBound, posX));
                posY = std::max(safeTopBound, std::min(safeBottomBound, posY));
            }
        }
        
        // Calculate background rectangle position
        int bgX = posX - bgWidth/2;
        int bgY = posY - bgHeight/2;
        
        // Position for text centered in the background
        int textX = posX;
        int textY = posY;
        
        // Get the cluster color for the background
        RGBA bgColor = clusterColors[i % clusterColors.size()];
        
        // Set background opacity
        bgColor.a = 0xE6; // 90% opacity
        
        // First, draw a shadow to make the label stand out better
        RGBA shadowColor(0, 0, 0, 0x80); // 50% opacity black
        shapes.drawSolidRoundedRect(
            layout.getSsaaFactor() * (bgX + 2),  // Offset by 2px for shadow effect
            layout.getSsaaFactor() * (bgY + 2),
            layout.getSsaaFactor() * bgWidth,
            layout.getSsaaFactor() * bgHeight,
            layout.getSsaaFactor() * 8,  // 8px corner radius
            shadowColor
        );
        
        // Then draw the main background using the solid rounded rectangle drawer
        // This ensures perfect corner clipping and proper alpha blending
        shapes.drawSolidRoundedRect(
            layout.getSsaaFactor() * bgX,
            layout.getSsaaFactor() * bgY, 
            layout.getSsaaFactor() * bgWidth,
            layout.getSsaaFactor() * bgHeight,
            layout.getSsaaFactor() * 8,  // 8px corner radius
            bgColor
        );
        
        // Create a dark text color for better contrast
        RGBA textColor = RGBA(0x10, 0x10, 0x10, 0xFF); // Almost black, fully opaque
        
        // Dark colors need light text
        if ((bgColor.r + bgColor.g + bgColor.b) / 3 < 128) {
            textColor = RGBA(0xF0, 0xF0, 0xF0, 0xFF); // Almost white, fully opaque
        }
        
        // Draw the label text - centered on the background
        text.drawText(textX, textY, labelText, textColor, 22, true);
        
        // Draw count label below cluster name (optional enhancement)
        if (!clusterPoints[i].empty()) {
            char countText[32];
            std::sprintf(countText, "%zu points", clusterPoints[i].size());
            
            // Draw smaller count text below main label
            text.drawText(textX, textY + 20, countText, textColor, 18, true);
        }
    }
}

int ChartStyler::calculateInfoBoxHeight(const std::vector<std::string>& labels, unsigned int fontSize) {
    // For horizontal layout, height is always the same regardless of number of items
    int itemHeight = fontSize + 6;
    
    // Calculate total height including top and bottom padding
    return 24 + itemHeight;  // 12px padding top and bottom + single row height
}

} // namespace shmea 
