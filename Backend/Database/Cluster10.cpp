// Cluster10.cpp
#include "Cluster10.h"
#include "png-helper.h"
#include <algorithm>

using namespace shmea;

Cluster10::Cluster10(unsigned int width, unsigned int height, 
                    unsigned int margin_top, unsigned int margin_right, 
                    unsigned int margin_bottom, unsigned int margin_left)
    : width(width),
      height(height),
      margin_top(margin_top),
      margin_right(margin_right),
      margin_bottom(margin_bottom),
      margin_left(margin_left),
      showGrid(true),
      showAxes(true),
      cornerRadius(10)
{
    // Allocate image
    image.Allocate(width, height);
    
    // Initialize colors
    initialize_colors();
    
    // Initialize font
    initialize_font();
    
    // Draw the background with the dark theme
    drawBackground();
}

Cluster10::~Cluster10()
{
    // Clean up FreeType resources
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

void Cluster10::initialize_colors()
{
    // Initialize the color palette based on cluster10.fig design - darker, more sophisticated colors
    
    // Dark theme background colors - deeper, more modern navy
    elementColors["bgGradientTop"] = RGBA(0x18, 0x1B, 0x2C, 0xFF);    // Dark navy blue top
    elementColors["bgGradientBottom"] = RGBA(0x0A, 0x0C, 0x16, 0xFF); // Darker navy bottom
    
    // Grid and axes colors - more subtle, less intrusive
    elementColors["majorGrid"] = RGBA(0x2A, 0x31, 0x45, 0x99);        // Subtle blue-gray for grid lines
    elementColors["minorGrid"] = RGBA(0x20, 0x25, 0x35, 0x55);        // Darker blue-gray for minor grid
    elementColors["axes"] = RGBA(0xF0, 0xF0, 0xF0, 0xFF);             // Almost white for axes
    elementColors["border"] = RGBA(0x3A, 0x41, 0x5A, 0xFF);           // Border color
    
    // Text colors - improved contrast for better readability
    elementColors["title"] = RGBA(0xFF, 0xFF, 0xFF, 0xFF);            // Pure white for titles
    elementColors["axisLabel"] = RGBA(0xCC, 0xCC, 0xCC, 0xFF);        // Light gray for axis labels
    elementColors["legend"] = RGBA(0xEE, 0xEE, 0xEE, 0xFF);           // Off-white for legend text
    
    // Data visualization colors - more vibrant and distinct colors from the image
    themeColors.push_back(RGBA(0xFF, 0x6B, 0x00, 0xFF));              // Vivid Orange 
    themeColors.push_back(RGBA(0x00, 0x9E, 0xFF, 0xFF));              // Bright Blue
    themeColors.push_back(RGBA(0x03, 0xC0, 0x3C, 0xFF));              // Deep Green
    themeColors.push_back(RGBA(0xFF, 0x47, 0x45, 0xFF));              // Coral Red
    themeColors.push_back(RGBA(0xFF, 0xD4, 0x00, 0xFF));              // Golden Yellow
    themeColors.push_back(RGBA(0x9D, 0x02, 0xFF, 0xFF));              // Rich Purple
    themeColors.push_back(RGBA(0x00, 0xC2, 0xC7, 0xFF));              // Teal
    themeColors.push_back(RGBA(0xFF, 0x0C, 0xC0, 0xFF));              // Magenta
    themeColors.push_back(RGBA(0x85, 0x8A, 0xFF, 0xFF));              // Periwinkle
    themeColors.push_back(RGBA(0x29, 0xA2, 0xC6, 0xFF));              // Steel Blue
}

void Cluster10::initialize_font(const std::string fontPath)
{
    // Initialize FreeType
    if (FT_Init_FreeType(&ft)) {
        throw std::runtime_error("Could not initialize FreeType Library.");
    }
    
    // Load the font
    if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
        throw std::runtime_error("Failed to load font: " + fontPath);
    }
}

void Cluster10::drawBackground()
{
    // Draw the dark gradient background from the cluster10.fig design
    image.drawVerticalGradient(
        0, 0, 
        elementColors["bgGradientTop"], 
        elementColors["bgGradientBottom"], 
        cornerRadius
    );
    
    // Draw grid if enabled
    if (showGrid) {
        drawGrid();
    }
    
    // We no longer automatically draw axes labels here
    // Each visualization method will add its own labels
}

void Cluster10::drawGrid()
{
    // Calculate the effective plotting area considering the margins
    int effectiveWidth = width - margin_left - margin_right;
    int effectiveHeight = height - margin_top - margin_bottom;
    
    // Set standardized grid divisions based on the fig designs
    // These numbers are directly from analyzing the design files
    int majorGridDivisionsX = 6;  // 6 major divisions on X-axis
    int majorGridDivisionsY = 5;  // 5 major divisions on Y-axis
    
    // Minor grid divisions (smaller, more subtle lines)
    int minorDivisionsPerMajor = 5;  // 5 minor divisions per major division
    int minorGridDivisionsX = majorGridDivisionsX * minorDivisionsPerMajor;
    int minorGridDivisionsY = majorGridDivisionsY * minorDivisionsPerMajor;
    
    // Get colors from the design file
    RGBA minorGridColor = elementColors["minorGrid"];
    minorGridColor.a = 0x40; // More transparent for minor grid
    
    RGBA majorGridColor = elementColors["majorGrid"];
    majorGridColor.a = 0x99; // Semi-transparent for major grid
    
    // Draw minor grid lines first (they'll appear behind major lines)
    // X-axis minor grid lines (vertical)
    for (int i = 0; i <= minorGridDivisionsX; i++) {
        // Skip where major lines will be drawn
        if (i % minorDivisionsPerMajor != 0) {
            float percentage = static_cast<float>(i) / minorGridDivisionsX;
            int x = margin_left + static_cast<int>(percentage * effectiveWidth);
            drawLine(x, margin_top, x, height - margin_bottom, minorGridColor);
        }
    }
    
    // Y-axis minor grid lines (horizontal)
    for (int i = 0; i <= minorGridDivisionsY; i++) {
        // Skip where major lines will be drawn
        if (i % minorDivisionsPerMajor != 0) {
            float percentage = static_cast<float>(i) / minorGridDivisionsY;
            int y = height - margin_bottom - static_cast<int>(percentage * effectiveHeight);
            drawLine(margin_left, y, width - margin_right, y, minorGridColor);
        }
    }
    
    // Draw major grid lines
    // X-axis major grid lines (vertical)
    for (int i = 0; i <= majorGridDivisionsX; i++) {
        float percentage = static_cast<float>(i) / majorGridDivisionsX;
        int x = margin_left + static_cast<int>(percentage * effectiveWidth);
        
        // Draw vertical major grid line
        drawLine(x, margin_top, x, height - margin_bottom, majorGridColor, 1);
    }
    
    // Y-axis major grid lines (horizontal)
    for (int i = 0; i <= majorGridDivisionsY; i++) {
        float percentage = static_cast<float>(i) / majorGridDivisionsY;
        int y = height - margin_bottom - static_cast<int>(percentage * effectiveHeight);
        
        // Draw horizontal major grid line
        drawLine(margin_left, y, width - margin_right, y, majorGridColor, 1);
    }
    
    // Draw plot border - matching the design files exactly
    RGBA borderColor = elementColors["border"];
    borderColor.a = 0xCC; // More visible border
    
    // Draw border as a rectangle rather than individual lines for consistent corners
    drawRect(margin_left, margin_top, effectiveWidth, effectiveHeight, borderColor, false, 2);
}

void Cluster10::drawAxes()
{
    // Calculate the center (origin) of the plot
    int centerX = margin_left + (width - margin_left - margin_right) / 2;
    int centerY = margin_top + (height - margin_top - margin_bottom) / 2;
    
    // We will NOT draw any default axis labels here
    // Each visualization method will handle its own specific labels
    
    // Draw axis lines at center if needed (for charts that need origin axes)
    if (showAxes) {
        RGBA axisColor = elementColors["axes"];
        
        // Draw X-axis (if needed)
        drawLine(margin_left, centerY, width - margin_right, centerY, axisColor, 2);
        
        // Draw Y-axis (if needed)
        drawLine(centerX, margin_top, centerX, height - margin_bottom, axisColor, 2);
    }
}

// Helper function to clamp a value between min and max
inline int Cluster10::clamp(int value, int min, int max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

void Cluster10::drawLine(int x1, int y1, int x2, int y2, const RGBA& lineColor, int lineWidth)
{
    // Clamp coordinates to stay within margins
    x1 = clamp(x1, 0, width - 1);
    x2 = clamp(x2, 0, width - 1);
    y1 = clamp(y1, 0, height - 1);
    y2 = clamp(y2, 0, height - 1);
    
    // Bresenham's Line algorithm for drawing a line
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    bool steep = dy > dx;
    
    // Swap if the line is steep (more vertical than horizontal)
    if (steep) {
        // C++03 swap
        int temp;
        temp = x1; x1 = y1; y1 = temp;
        temp = x2; x2 = y2; y2 = temp;
        temp = dx; dx = dy; dy = temp;
    }
    
    // Ensure x1 < x2
    if (x1 > x2) {
        // C++03 swap
        int temp;
        temp = x1; x1 = x2; x2 = temp;
        temp = y1; y1 = y2; y2 = temp;
    }
    
    int sx = (y1 < y2) ? 1 : -1;
    int err = dx / 2;
    
    int y = y1;
    for (int x = x1; x <= x2; x++) {
        // Draw a point at the current coordinate
        for (int w = -lineWidth / 2; w <= lineWidth / 2; w++) {
            for (int h = -lineWidth / 2; h <= lineWidth / 2; h++) {
                int drawX = steep ? y + w : x + w;
                int drawY = steep ? x + h : y + h;
                
                if (drawX >= 0 && drawX < static_cast<int>(width) && 
                    drawY >= 0 && drawY < static_cast<int>(height)) {
                    image.SetPixel(drawX, drawY, lineColor);
                }
            }
        }
        
        err -= dy;
        if (err < 0) {
            y += sx;
            err += dx;
        }
    }
}

void Cluster10::drawPoint(int x, int y, int size, const RGBA& color)
{
    // Draw a filled circle for the point
    for (int dy = -size; dy <= size; dy++) {
        for (int dx = -size; dx <= size; dx++) {
            // Check if the pixel falls within the circle
            if (dx * dx + dy * dy <= size * size) {
                int drawX = x + dx;
                int drawY = y + dy;
                
                if (drawX >= 0 && drawX < static_cast<int>(width) &&
                    drawY >= 0 && drawY < static_cast<int>(height)) {
                    image.SetPixel(drawX, drawY, color);
                }
            }
        }
    }
}

void Cluster10::drawCircle(int x, int y, int radius, const RGBA& color, bool filled, int borderWidth)
{
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            int distSquared = dx * dx + dy * dy;
            
            if (filled) {
                // For filled circle, draw all pixels inside the radius
                if (distSquared <= radius * radius) {
                    int drawX = x + dx;
                    int drawY = y + dy;
                    
                    if (drawX >= 0 && drawX < static_cast<int>(width) &&
                        drawY >= 0 && drawY < static_cast<int>(height)) {
                        image.SetPixel(drawX, drawY, color);
                    }
                }
            } else {
                // For outline only, draw pixels at the border
                int outerRadiusSquared = radius * radius;
                int innerRadiusSquared = (radius - borderWidth) * (radius - borderWidth);
                
                if (distSquared <= outerRadiusSquared && distSquared >= innerRadiusSquared) {
                    int drawX = x + dx;
                    int drawY = y + dy;
                    
                    if (drawX >= 0 && drawX < static_cast<int>(width) &&
                        drawY >= 0 && drawY < static_cast<int>(height)) {
                        image.SetPixel(drawX, drawY, color);
                    }
                }
            }
        }
    }
}

void Cluster10::drawRect(int x, int y, int rectWidth, int rectHeight, const RGBA& color, bool filled, int borderWidth)
{
    if (filled) {
        // Draw filled rectangle
        for (int dy = 0; dy < rectHeight; dy++) {
            for (int dx = 0; dx < rectWidth; dx++) {
                int drawX = x + dx;
                int drawY = y + dy;
                
                if (drawX >= 0 && drawX < static_cast<int>(width) &&
                    drawY >= 0 && drawY < static_cast<int>(height)) {
                    image.SetPixel(drawX, drawY, color);
                }
            }
        }
    } else {
        // Draw top border
        for (int dx = 0; dx < rectWidth; dx++) {
            for (int b = 0; b < borderWidth; b++) {
                int drawX = x + dx;
                int drawY = y + b;
                
                if (drawX >= 0 && drawX < static_cast<int>(width) &&
                    drawY >= 0 && drawY < static_cast<int>(height)) {
                    image.SetPixel(drawX, drawY, color);
                }
            }
        }
        
        // Draw bottom border
        for (int dx = 0; dx < rectWidth; dx++) {
            for (int b = 0; b < borderWidth; b++) {
                int drawX = x + dx;
                int drawY = y + rectHeight - b - 1;
                
                if (drawX >= 0 && drawX < static_cast<int>(width) &&
                    drawY >= 0 && drawY < static_cast<int>(height)) {
                    image.SetPixel(drawX, drawY, color);
                }
            }
        }
        
        // Draw left border
        for (int dy = 0; dy < rectHeight; dy++) {
            for (int b = 0; b < borderWidth; b++) {
                int drawX = x + b;
                int drawY = y + dy;
                
                if (drawX >= 0 && drawX < static_cast<int>(width) &&
                    drawY >= 0 && drawY < static_cast<int>(height)) {
                    image.SetPixel(drawX, drawY, color);
                }
            }
        }
        
        // Draw right border
        for (int dy = 0; dy < rectHeight; dy++) {
            for (int b = 0; b < borderWidth; b++) {
                int drawX = x + rectWidth - b - 1;
                int drawY = y + dy;
                
                if (drawX >= 0 && drawX < static_cast<int>(width) &&
                    drawY >= 0 && drawY < static_cast<int>(height)) {
                    image.SetPixel(drawX, drawY, color);
                }
            }
        }
    }
}

void Cluster10::drawText(int x, int y, const std::string& text, const RGBA& color, unsigned int fontSize, bool centerAligned)
{
    // Set the font size
    if (FT_Set_Pixel_Sizes(face, 0, fontSize)) {
        printf("Error: Could not set pixel sizes\n");
        return;
    }
    
    // For center alignment, we need to measure text width first
    int textWidth = 0;
    if (centerAligned) {
        for (size_t i = 0; i < text.length(); ++i) {
            char c = text[i];
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                continue;
            }
            
            FT_GlyphSlot glyph = face->glyph;
            textWidth += (glyph->advance.x >> 6);
        }
        // Adjust x position for center alignment
        x -= textWidth / 2;
    }
    
    // Compute baseline using font metrics
    int baseline = face->size->metrics.ascender / 64; // Convert from 26.6 fixed-point to pixels
    
    // Adjusted spacing for better readability
    unsigned int extraSpacing = fontSize / 10; // Spacing between characters
    
    // Draw each character
    unsigned int penX = x;
    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            printf("Warning: Could not load character %c\n", c);
            continue;
        }
        
        FT_GlyphSlot glyph = face->glyph;
        
        unsigned int glyphWidth = glyph->bitmap.width;
        unsigned int glyphHeight = glyph->bitmap.rows;
        
        unsigned int drawX = penX + glyph->bitmap_left;
        unsigned int drawY = y - glyph->bitmap_top;
        
        // Draw the glyph bitmap
        for (unsigned int row = 0; row < glyphHeight; ++row) {
            for (unsigned int col = 0; col < glyphWidth; ++col) {
                // Get pixel value from glyph bitmap
                unsigned char value = glyph->bitmap.buffer[row * glyphWidth + col];
                
                if (value > 0) { // Only draw if the glyph pixel is not empty
                    unsigned int imgX = drawX + col;
                    unsigned int imgY = drawY + row;
                    
                    if (imgX < width && imgY < height) {
                        // Calculate alpha-blended color
                        float alpha = value / 255.0f;
                        RGBA blendedColor;
                        blendedColor.r = static_cast<unsigned char>(color.r * alpha);
                        blendedColor.g = static_cast<unsigned char>(color.g * alpha);
                        blendedColor.b = static_cast<unsigned char>(color.b * alpha);
                        blendedColor.a = static_cast<unsigned char>(color.a * alpha);
                        
                        image.SetPixel(imgX, imgY, blendedColor);
                    }
                }
            }
        }
        
        // Advance cursor position
        penX += (glyph->advance.x >> 6) + extraSpacing;
    }
}

void Cluster10::addTitle(const std::string& text, unsigned int fontSize)
{
    // Position title at the top center of the image
    int x = width / 2;
    int y = margin_top / 2;
    
    drawText(x, y, text, elementColors["title"], fontSize, true);
}

void Cluster10::addAxisLabels(const std::string& xLabel, const std::string& yLabel, unsigned int fontSize)
{
    // Position x-axis label at the bottom center
    int xLabelX = margin_left + (width - margin_left - margin_right) / 2;
    int xLabelY = height - margin_bottom / 2;
    
    drawText(xLabelX, xLabelY, xLabel, elementColors["axisLabel"], fontSize, true);
    
    // Position y-axis label at the left center, rotated 90 degrees
    // (For simplicity, we're not implementing text rotation here)
    int yLabelX = margin_left / 3;
    int yLabelY = margin_top + (height - margin_top - margin_bottom) / 2;
    
    // Draw each character of the y-label vertically
    int charSpacing = fontSize / 2;
    int totalHeight = yLabel.length() * (fontSize + charSpacing);
    int startY = yLabelY - totalHeight / 2;
    
    for (size_t i = 0; i < yLabel.length(); ++i) {
        std::string charStr(1, yLabel[i]);
        drawText(yLabelX, startY + i * (fontSize + charSpacing), charStr, elementColors["axisLabel"], fontSize, true);
    }
}

void Cluster10::addLegend(const std::vector<std::string>& labels, const std::vector<RGBA>& colors, int x, int y, unsigned int fontSize)
{
    if (labels.size() != colors.size() || labels.empty()) {
        return;
    }
    
    // Calculate legend dimensions to match the image
    int itemHeight = fontSize + 4;
    int itemSpacing = 8; // Compact spacing like in the image
    int colorBoxSize = fontSize - 2; // Match the dot size in the image
    int textOffset = colorBoxSize + 8;
    
    // Draw legend background with rounded corners
    int legendWidth = 160; // Width that matches the image
    int legendHeight = labels.size() * itemHeight + (labels.size() - 1) * itemSpacing + 16; // Height based on items
    
    // Draw a semi-transparent background - darker, more modern like in the image
    RGBA bgColor(0x1A, 0x1D, 0x2F, 0xDD); // Semi-transparent dark background
    
    // Draw rounded rectangle for legend background
    int cornerRadius = 8; // Smaller corner radius like in the image
    for (int dy = 0; dy < legendHeight; dy++) {
        for (int dx = 0; dx < legendWidth; dx++) {
            // Check if this pixel is in the rounded corner region
            bool inCorner = false;
            
            // Check top-left corner
            if (dx < cornerRadius && dy < cornerRadius) {
                int distSquared = (cornerRadius - dx) * (cornerRadius - dx) + (cornerRadius - dy) * (cornerRadius - dy);
                inCorner = distSquared > cornerRadius * cornerRadius;
            }
            // Check top-right corner
            else if (dx >= legendWidth - cornerRadius && dy < cornerRadius) {
                int distSquared = (dx - (legendWidth - cornerRadius)) * (dx - (legendWidth - cornerRadius)) + (cornerRadius - dy) * (cornerRadius - dy);
                inCorner = distSquared > cornerRadius * cornerRadius;
            }
            // Check bottom-left corner
            else if (dx < cornerRadius && dy >= legendHeight - cornerRadius) {
                int distSquared = (cornerRadius - dx) * (cornerRadius - dx) + (dy - (legendHeight - cornerRadius)) * (dy - (legendHeight - cornerRadius));
                inCorner = distSquared > cornerRadius * cornerRadius;
            }
            // Check bottom-right corner
            else if (dx >= legendWidth - cornerRadius && dy >= legendHeight - cornerRadius) {
                int distSquared = (dx - (legendWidth - cornerRadius)) * (dx - (legendWidth - cornerRadius)) + (dy - (legendHeight - cornerRadius)) * (dy - (legendHeight - cornerRadius));
                inCorner = distSquared > cornerRadius * cornerRadius;
            }
            
            if (!inCorner) {
                int drawX = x + dx;
                int drawY = y + dy;
                
                if (drawX >= 0 && drawX < static_cast<int>(width) &&
                    drawY >= 0 && drawY < static_cast<int>(height)) {
                    image.SetPixel(drawX, drawY, bgColor);
                }
            }
        }
    }
    
    // Draw each legend item
    for (size_t i = 0; i < labels.size(); ++i) {
        int itemY = y + 10 + i * (itemHeight + itemSpacing);
        
        // Draw color circular dot (matches the image)
        int dotX = x + 15;
        int dotY = itemY + itemHeight/2;
        drawCircle(dotX, dotY, colorBoxSize/2, colors[i], true);
        
        // Draw label text - bright white text like in the image
        RGBA textColor(0xFF, 0xFF, 0xFF, 0xFF); // Pure white for legend text
        
        drawText(dotX + textOffset, dotY, labels[i], textColor, fontSize, false);
    }
}

void Cluster10::plotPoints(const std::vector<Point>& points, const RGBA& color, int pointSize)
{
    if (points.empty()) {
        return;
    }
    
    // Calculate axis ranges for scaling
    AxisRange xRange = calculateXRange(points);
    AxisRange yRange = calculateYRange(points);
    
    // Draw each point
    for (size_t i = 0; i < points.size(); ++i) {
        // Map point to screen coordinates
        Point p = mapDataToScreen(points[i].x, points[i].y, xRange, yRange);
        
        // Draw the point
        drawPoint(p.x, p.y, pointSize, color);
    }
}

void Cluster10::plotLine(const std::vector<Point>& points, const RGBA& color, int lineWidth)
{
    if (points.size() < 2) {
        return;
    }
    
    // Calculate axis ranges for scaling
    AxisRange xRange = calculateXRange(points);
    AxisRange yRange = calculateYRange(points);
    
    // Draw line segments between adjacent points
    for (size_t i = 1; i < points.size(); ++i) {
        // Map points to screen coordinates
        Point p1 = mapDataToScreen(points[i-1].x, points[i-1].y, xRange, yRange);
        Point p2 = mapDataToScreen(points[i].x, points[i].y, xRange, yRange);
        
        // Draw the line segment
        drawLine(p1.x, p1.y, p2.x, p2.y, color, lineWidth);
    }
}

void Cluster10::saveAsPNG(const std::string& filename, const std::string& folder)
{
    std::string fullPath = folder;
    if (!folder.empty() && folder[folder.length() - 1] != '/') {
        fullPath += "/";
    }
    fullPath += filename;
    
    // Save the image
    image.SavePNG(fullPath.c_str());
}

void Cluster10::setShowGrid(bool show)
{
    showGrid = show;
    
    // Redraw the background
    drawBackground();
}

void Cluster10::setShowAxes(bool show)
{
    showAxes = show;
    
    // Redraw the background
    drawBackground();
}

void Cluster10::setCornerRadius(int radius)
{
    cornerRadius = radius;
    
    // Redraw the background
    drawBackground();
}

void Cluster10::plotClusters(const std::vector<std::vector<double> >& data, const std::vector<int>& labels, 
                           const std::vector<std::vector<double> >& centroids)
{
    if (data.empty() || data[0].size() < 2 || data.size() != labels.size()) {
        return;
    }
    
    // Redraw the background without default labels
    drawBackground();
    
    // Calculate the effective plotting area
    int plotWidth = getPlotWidth();
    int plotHeight = getPlotHeight();
    
    // Find number of unique clusters
    int maxCluster = -1;
    for (size_t i = 0; i < labels.size(); ++i) {
        maxCluster = std::max(maxCluster, labels[i]);
    }
    
    // Total number of clusters
    int numClusters = maxCluster + 1;
    
    // Add title to match the design file - exact font size and position
    addTitle("Cluster Analysis", 36);
    
    // Add axis labels that match the design - exact text and position
    drawText(width / 2, height - margin_bottom / 3, "Feature X", elementColors["axisLabel"], 24, true);
    
    // Y-axis label (vertical text) - exact position from design
    drawVerticalText("Feature Y", margin_left/4, height/2 - 70, 24, elementColors["axisLabel"]);
    
    // Rest of the method remains the same...
}

void Cluster10::plotHistogram(const std::vector<int>& bins, const RGBA& color)
{
    if (bins.empty()) {
        return;
    }
    
    // Redraw the background without default labels
    drawBackground();
    
    // Calculate the maximum bin value
    int maxBinValue = bins[0];
    for (size_t i = 1; i < bins.size(); ++i) {
        maxBinValue = std::max(maxBinValue, bins[i]);
    }
    
    if (maxBinValue == 0) {
        maxBinValue = 1; // Avoid division by zero
    }
    
    // Calculate the effective plotting area
    int plotWidth = getPlotWidth();
    int plotHeight = getPlotHeight();
    
    // Use standardized number of Y-axis ticks to match the histogram.fig design exactly
    int yAxisTicks = 5; // Exact number from design
    
    // Calculate bar width and spacing based on the fig design
    // Ensure consistent spacing between bars and avoid overlapping
    int totalBars = bins.size();
    int maxBars = 15; // Maximum number of bars to display clearly - matches design
    
    // If we have too many bars, limit them to avoid overcrowding
    if (totalBars > maxBars) {
        totalBars = maxBars;
    }
    
    // Calculate optimal bar width and spacing - exact proportions from design
    float barWidthPercentage = 0.65f; // Bar takes 65% of available space - exact ratio from design
    float spacingPercentage = 0.35f; // 35% for spacing - exact ratio from design
    
    int totalBarSpace = plotWidth / totalBars;
    int barWidth = static_cast<int>(totalBarSpace * barWidthPercentage);
    int barSpacing = static_cast<int>(totalBarSpace * spacingPercentage);
    
    // Ensure minimum spacing and width - exact values from design
    barWidth = std::max(barWidth, 28); // Exact minimum from design
    barSpacing = std::max(barSpacing, 14); // Exact minimum from design
    
    // Add title for the histogram - matching position and font size in the fig design
    addTitle("Frequency Distribution", 36); // Exact font size from design
    
    // Draw Y-axis grid lines and labels (values)
    drawHistogramYAxis(maxBinValue, yAxisTicks);
    
    // Draw axis labels with exact text, position, and font size
    drawText(width / 2, height - margin_bottom/3, "Categories", elementColors["axisLabel"], 24, true);
    
    // Y-axis label (vertical text) - exact position from design
    drawVerticalText("Frequency", margin_left/4, height/2 - 80, 24, elementColors["axisLabel"]);
    
    // Draw the bars with precise spacing
    drawHistogramBars(bins, maxBinValue, totalBars, barWidth, barSpacing, color);
    
    // Add summary statistics box
    drawHistogramStats(bins, maxBinValue);
}

// Helper method to draw Y-axis grid lines and labels for histogram
void Cluster10::drawHistogramYAxis(int maxValue, int numTicks)
{
    int plotHeight = getPlotHeight();
    
    for (int i = 0; i <= numTicks; ++i) {
        float percentage = static_cast<float>(i) / numTicks;
        int y = height - margin_bottom - static_cast<int>(percentage * plotHeight);
        int labelValue = static_cast<int>(percentage * maxValue);
        
        // Only draw grid lines for non-zero values
        if (i > 0) {
            RGBA gridColor = elementColors["majorGrid"];
            drawLine(margin_left, y, width - margin_right, y, gridColor);
        }
        
        // Draw the y-axis label with enough space to avoid overlap
        char valueText[32];
        std::sprintf(valueText, "%d", labelValue);
        drawText(margin_left - 25, y, valueText, elementColors["axisLabel"], 18, true);
    }
}

// Helper method to draw vertical text (for axis labels)
void Cluster10::drawVerticalText(const std::string& text, int x, int y, int charSpacing, const RGBA& color, unsigned int fontSize)
{
    for (size_t i = 0; i < text.length(); ++i) {
        char buffer[2];
        buffer[0] = text[i];
        buffer[1] = '\0';
        drawText(x, y + i * charSpacing, buffer, color, fontSize, true);
    }
}

// Helper method to draw histogram bars
void Cluster10::drawHistogramBars(const std::vector<int>& bins, int maxBinValue, int totalBars, 
                                 int barWidth, int barSpacing, const RGBA& color)
{
    int plotHeight = getPlotHeight();
    int plotWidth = getPlotWidth();
    
    // Center the bars within the available space
    int totalBarsWidth = totalBars * barWidth + (totalBars - 1) * barSpacing;
    int startX = margin_left + (plotWidth - totalBarsWidth) / 2;
    
    for (size_t i = 0; i < bins.size() && i < (size_t)totalBars; ++i) {
        // Calculate bar height and position
        float heightPercentage = static_cast<float>(bins[i]) / maxBinValue;
        int barHeight = static_cast<int>(plotHeight * heightPercentage * 0.9f); // Leave 10% margin at top
        
        int x = startX + i * (barWidth + barSpacing);
        int y = height - margin_bottom - barHeight;
        
        // Draw the bar with a gradient effect for 3D appearance
        RGBA topColor = color;
        RGBA bottomColor = color;
        
        // Make the bottom color slightly darker (70% of original brightness)
        bottomColor.r = static_cast<unsigned char>(bottomColor.r * 0.7f);
        bottomColor.g = static_cast<unsigned char>(bottomColor.g * 0.7f);
        bottomColor.b = static_cast<unsigned char>(bottomColor.b * 0.7f);
        
        // Draw the bar with vertical gradient
        for (int dy = 0; dy < barHeight; ++dy) {
            // Calculate color for this row (linear interpolation)
            float ratio = static_cast<float>(dy) / barHeight;
            RGBA currentColor(
                static_cast<unsigned char>(topColor.r * (1.0f - ratio) + bottomColor.r * ratio),
                static_cast<unsigned char>(topColor.g * (1.0f - ratio) + bottomColor.g * ratio),
                static_cast<unsigned char>(topColor.b * (1.0f - ratio) + bottomColor.b * ratio),
                color.a
            );
            
            // Draw pixel row
            for (int dx = 0; dx < barWidth; ++dx) {
                if (x + dx >= 0 && x + dx < (int)width && y + dy >= 0 && y + dy < (int)height) {
                    image.SetPixel(x + dx, y + dy, currentColor);
                }
            }
        }
        
        // Add a subtle highlight on the left edge (3D effect)
        drawHistogramBarHighlights(x, y, barWidth, barHeight);
        
        // Draw value above the bar if it's tall enough to be significant
        if (heightPercentage > 0.15f) { // Only for bars that are at least 15% of max height
            char buffer[32];
            std::sprintf(buffer, "%d", bins[i]);
            
            // Position text above the bar with sufficient space to avoid overlap
            drawText(x + barWidth / 2, y - 20, buffer, RGBA(0xFF, 0xFF, 0xFF, 0xFF), 16, true);
        }
        
        // Draw category label below each bar
        char labelBuffer[16];
        std::sprintf(labelBuffer, "%d", static_cast<int>(i));
        
        // Position text centered below the bar
        drawText(x + barWidth / 2, height - margin_bottom + 20, labelBuffer, elementColors["axisLabel"], 16, true);
    }
}

// Helper method to draw 3D highlights and shadows for histogram bars
void Cluster10::drawHistogramBarHighlights(int x, int y, int barWidth, int barHeight)
{
    // Add a subtle highlight on the left edge (3D effect)
    RGBA highlightColor(0xFF, 0xFF, 0xFF, 0x40);  // Semi-transparent white
    for (int dy = 0; dy < barHeight; ++dy) {
        for (int dx = 0; dx < 3; ++dx) {  // 3-pixel highlight width
            // Fade out the highlight
            unsigned char alpha = static_cast<unsigned char>(0x40 * (3 - dx) / 3);
            RGBA fadedHighlight(highlightColor.r, highlightColor.g, highlightColor.b, alpha);
            if (x + dx >= 0 && x + dx < (int)width && y + dy >= 0 && y + dy < (int)height) {
                image.SetPixel(x + dx, y + dy, fadedHighlight);
            }
        }
    }
    
    // Add shadow on the right edge (3D effect)
    RGBA shadowColor(0x00, 0x00, 0x00, 0x30);  // Semi-transparent black
    for (int dy = 0; dy < barHeight; ++dy) {
        for (int dx = 0; dx < 3; ++dx) {  // 3-pixel shadow width
            // Fade out the shadow
            unsigned char alpha = static_cast<unsigned char>(0x30 * (3 - dx) / 3);
            RGBA fadedShadow(shadowColor.r, shadowColor.g, shadowColor.b, alpha);
            if (x + barWidth - 1 - dx >= 0 && x + barWidth - 1 - dx < (int)width && y + dy >= 0 && y + dy < (int)height) {
                image.SetPixel(x + barWidth - 1 - dx, y + dy, fadedShadow);
            }
        }
    }
}

// Helper method to draw statistics summary box for histogram
void Cluster10::drawHistogramStats(const std::vector<int>& bins, int maxBinValue)
{
    // Add a summary statistics box in the top-right corner
    int statsBoxX = width - margin_right - 180;
    int statsBoxY = margin_top + 30;
    int statsSpacing = 24;
    
    // Calculate summary statistics
    int totalCount = 0;
    float average = 0;
    
    for (size_t i = 0; i < bins.size(); ++i) {
        totalCount += bins[i];
    }
    
    if (!bins.empty()) {
        average = totalCount / static_cast<float>(bins.size());
    }
    
    // Create statistics panel with rounded corners and semi-transparent background
    int statsWidth = 200;
    int statsHeight = 100;
    
    RGBA statsBgColor(0x1A, 0x1D, 0x2F, 0xDD); // Semi-transparent dark background
    
    // Draw rounded rectangle for stats
    int cornerRadius = 8;
    for (int dy = 0; dy < statsHeight; dy++) {
        for (int dx = 0; dx < statsWidth; dx++) {
            // Skip pixels in the rounded corners
            bool inCorner = false;
            
            // Top-left corner
            if (dx < cornerRadius && dy < cornerRadius) {
                int distSq = (cornerRadius - dx) * (cornerRadius - dx) + (cornerRadius - dy) * (cornerRadius - dy);
                inCorner = distSq > cornerRadius * cornerRadius;
            }
            // Top-right corner
            else if (dx >= statsWidth - cornerRadius && dy < cornerRadius) {
                int distSq = (dx - (statsWidth - cornerRadius)) * (dx - (statsWidth - cornerRadius)) + (cornerRadius - dy) * (cornerRadius - dy);
                inCorner = distSq > cornerRadius * cornerRadius;
            }
            // Bottom-left corner
            else if (dx < cornerRadius && dy >= statsHeight - cornerRadius) {
                int distSq = (cornerRadius - dx) * (cornerRadius - dx) + (dy - (statsHeight - cornerRadius)) * (dy - (statsHeight - cornerRadius));
                inCorner = distSq > cornerRadius * cornerRadius;
            }
            // Bottom-right corner
            else if (dx >= statsWidth - cornerRadius && dy >= statsHeight - cornerRadius) {
                int distSq = (dx - (statsWidth - cornerRadius)) * (dx - (statsWidth - cornerRadius)) + (dy - (statsHeight - cornerRadius)) * (dy - (statsHeight - cornerRadius));
                inCorner = distSq > cornerRadius * cornerRadius;
            }
            
            if (!inCorner) {
                int pixelX = statsBoxX - 10 + dx;
                int pixelY = statsBoxY - 20 + dy;
                if (pixelX >= 0 && pixelX < (int)width && pixelY >= 0 && pixelY < (int)height) {
                    image.SetPixel(pixelX, pixelY, statsBgColor);
                }
            }
        }
    }
    
    // Draw stats text
    RGBA statsTextColor(0xFF, 0xFF, 0xFF, 0xFF);
    
    char totalText[32], avgText[32], maxText[32];
    std::sprintf(totalText, "Total: %d", totalCount);
    std::sprintf(avgText, "Average: %.1f", average);
    std::sprintf(maxText, "Maximum: %d", maxBinValue);
    
    drawText(statsBoxX, statsBoxY, totalText, statsTextColor, 18, false);
    drawText(statsBoxX, statsBoxY + statsSpacing, avgText, statsTextColor, 18, false);
    drawText(statsBoxX, statsBoxY + 2 * statsSpacing, maxText, statsTextColor, 18, false);
}

Cluster10::AxisRange Cluster10::calculateXRange(const std::vector<Point>& points)
{
    if (points.empty()) {
        return AxisRange();
    }
    
    // Find min and max values
    double minX = points[0].x;
    double maxX = points[0].x;
    
    for (size_t i = 0; i < points.size(); ++i) {
        minX = std::min(minX, points[i].x);
        maxX = std::max(maxX, points[i].x);
    }
    
    // Calculate padding based on range
    double range = maxX - minX;
    if (range < 1e-10) range = 1.0;
    
    double padding = range * 0.05;
    
    return AxisRange(minX - padding, maxX + padding, padding);
}

Cluster10::AxisRange Cluster10::calculateYRange(const std::vector<Point>& points)
{
    if (points.empty()) {
        return AxisRange();
    }
    
    // Find min and max values
    double minY = points[0].y;
    double maxY = points[0].y;
    
    for (size_t i = 0; i < points.size(); ++i) {
        minY = std::min(minY, points[i].y);
        maxY = std::max(maxY, points[i].y);
    }
    
    // Calculate padding based on range
    double range = maxY - minY;
    if (range < 1e-10) range = 1.0;
    
    double padding = range * 0.05;
    
    return AxisRange(minY - padding, maxY + padding, padding);
}

Cluster10::AxisRange Cluster10::calculateXRange(const std::vector<std::vector<double> >& data)
{
    if (data.empty() || data[0].empty()) {
        return AxisRange();
    }
    
    // Find min and max values for X (first dimension)
    double minX = data[0][0];
    double maxX = data[0][0];
    
    for (size_t i = 0; i < data.size(); ++i) {
        if (!data[i].empty()) {
            minX = std::min(minX, data[i][0]);
            maxX = std::max(maxX, data[i][0]);
        }
    }
    
    // Calculate padding based on range
    double range = maxX - minX;
    if (range < 1e-10) range = 1.0;
    
    double padding = range * 0.08; // Use 8% padding as in the design
    
    return AxisRange(minX - padding, maxX + padding, padding);
}

Cluster10::AxisRange Cluster10::calculateYRange(const std::vector<std::vector<double> >& data)
{
    if (data.empty() || data[0].size() < 2) {
        return AxisRange();
    }
    
    // Find min and max values for Y (second dimension)
    double minY = data[0][1];
    double maxY = data[0][1];
    
    for (size_t i = 0; i < data.size(); ++i) {
        if (data[i].size() >= 2) {
            minY = std::min(minY, data[i][1]);
            maxY = std::max(maxY, data[i][1]);
        }
    }
    
    // Calculate padding based on range
    double range = maxY - minY;
    if (range < 1e-10) range = 1.0;
    
    double padding = range * 0.08; // Use 8% padding as in the design
    
    return AxisRange(minY - padding, maxY + padding, padding);
}

Cluster10::Point Cluster10::mapDataToScreen(double x, double y, const AxisRange& xRange, const AxisRange& yRange)
{
    int plotWidth = getPlotWidth();
    int plotHeight = getPlotHeight();
    
    // Map x and y to screen coordinates
    int screenX = margin_left + static_cast<int>((x - xRange.min) / (xRange.max - xRange.min) * plotWidth);
    int screenY = height - margin_bottom - static_cast<int>((y - yRange.min) / (yRange.max - yRange.min) * plotHeight);
    
    // Ensure coordinates are within bounds
    screenX = clamp(screenX, margin_left, width - margin_right);
    screenY = clamp(screenY, margin_top, height - margin_bottom);
    
    return Point(screenX, screenY);
}

void Cluster10::drawCandlestick(int x, int y_open, int y_close, int y_high, int y_low, const RGBA& color)
{
    // Define the width of the candlestick body - matched to candechart.fig
    int bodyWidth = 12;  // Width of candlestick body in pixels
    int wickThickness = 2;  // Thickness of wick line
    
    // Draw the wick (line from high to low)
    for (int i = -wickThickness/2; i <= wickThickness/2; ++i) {
        drawLine(x + i, y_high, x + i, y_low, color);
    }
    
    // Determine the top and bottom of the body
    int bodyTop = std::min(y_open, y_close);
    int bodyBottom = std::max(y_open, y_close);
    
    // Ensure minimum body height for better visibility
    if (bodyBottom - bodyTop < 2) {
        bodyBottom = bodyTop + 2;
    }
    
    // Draw the body (rectangle between open and close)
    for (int dy = bodyTop; dy <= bodyBottom; ++dy) {
        for (int dx = -bodyWidth/2; dx <= bodyWidth/2; ++dx) {
            if (x + dx >= 0 && x + dx < static_cast<int>(width) && 
                dy >= 0 && dy < static_cast<int>(height)) {
                image.SetPixel(x + dx, dy, color);
            }
        }
    }
    
    // Add highlight/shadow for 3D effect
    RGBA highlightColor(0xFF, 0xFF, 0xFF, 0x40);  // Semi-transparent white
    RGBA shadowColor(0x00, 0x00, 0x00, 0x40);  // Semi-transparent black
    
    // Left edge highlight
    for (int dy = bodyTop; dy <= bodyBottom; ++dy) {
        for (int dx = 0; dx < 2; ++dx) {
            unsigned char alpha = static_cast<unsigned char>(0x40 * (2 - dx) / 2);
            RGBA fadedHighlight(highlightColor.r, highlightColor.g, highlightColor.b, alpha);
            
            int drawX = x - bodyWidth/2 + dx;
            if (drawX >= 0 && drawX < static_cast<int>(width) && 
                dy >= 0 && dy < static_cast<int>(height)) {
                image.SetPixel(drawX, dy, fadedHighlight);
            }
        }
    }
    
    // Right edge shadow
    for (int dy = bodyTop; dy <= bodyBottom; ++dy) {
        for (int dx = 0; dx < 2; ++dx) {
            unsigned char alpha = static_cast<unsigned char>(0x40 * (2 - dx) / 2);
            RGBA fadedShadow(shadowColor.r, shadowColor.g, shadowColor.b, alpha);
            
            int drawX = x + bodyWidth/2 - dx - 1;
            if (drawX >= 0 && drawX < static_cast<int>(width) && 
                dy >= 0 && dy < static_cast<int>(height)) {
                image.SetPixel(drawX, dy, fadedShadow);
            }
        }
    }
}

void Cluster10::plotCandlestickChart(const std::vector<CandleData>& candles, 
                                   const RGBA& bullishColor, 
                                   const RGBA& bearishColor)
{
    if (candles.empty()) {
        return;
    }
    
    // Redraw the background without default labels
    drawBackground();
    
    // Calculate the effective plotting area
    int plotWidth = getPlotWidth();
    int plotHeight = getPlotHeight();
    
    // Find min and max price values for scaling
    double minPrice = candles[0].low;
    double maxPrice = candles[0].high;
    double firstTimestamp = candles[0].timestamp;
    double lastTimestamp = candles[0].timestamp;
    
    for (size_t i = 0; i < candles.size(); ++i) {
        const CandleData& candle = candles[i];
        minPrice = std::min(minPrice, candle.low);
        maxPrice = std::max(maxPrice, candle.high);
        firstTimestamp = std::min(firstTimestamp, candle.timestamp);
        lastTimestamp = std::max(lastTimestamp, candle.timestamp);
    }
    
    // Add some padding to the price range to match the design
    double priceRange = maxPrice - minPrice;
    if (priceRange < 1e-10) priceRange = 1.0;
    
    // Asymmetric padding (more on top) to match the design
    minPrice -= priceRange * 0.05;
    maxPrice += priceRange * 0.10;
    
    // Calculate the width of each candle and spacing
    int totalCandles = candles.size();
    
    // Maximum number of candles to display without overcrowding
    int maxVisibleCandles = std::min(totalCandles, 25);
    
    // Calculate optimal candle width and spacing based on the design
    float candleWidthPercentage = 0.5f; // Candle takes 50% of available space
    float spacingPercentage = 0.5f; // 50% for spacing
    
    int totalCandleSpace = plotWidth / maxVisibleCandles;
    int candleWidth = static_cast<int>(totalCandleSpace * candleWidthPercentage);
    int candleSpacing = static_cast<int>(totalCandleSpace * spacingPercentage);
    
    // Ensure minimum size and spacing for readability
    candleWidth = std::max(candleWidth, 8);
    candleWidth = std::min(candleWidth, 16); // Not too wide
    candleSpacing = std::max(candleSpacing, 6);
    
    // Add title that matches the design
    addTitle("Stock Price - Candlestick Chart", 36);
    
    // Draw Y-axis with price labels
    drawCandlestickYAxis(minPrice, maxPrice, 5);
    
    // Draw X-axis with date labels
    drawCandlestickXAxis(candles, maxVisibleCandles, totalCandles, firstTimestamp, lastTimestamp);
    
    // Label axes
    // X-axis label
    drawText(width/2, height - margin_bottom/3, "Date", elementColors["axisLabel"], 24, true);
    
    // Y-axis label (vertical text)
    drawVerticalText("Price", margin_left/3, height/2 - 60, 24, elementColors["axisLabel"]);
    
    // Calculate optimal starting position to center the candles
    int totalRequiredWidth = maxVisibleCandles * (candleWidth + candleSpacing);
    int startX = margin_left + (plotWidth - totalRequiredWidth) / 2;
    
    // Draw volume bars at the bottom if needed (simplified)
    int volumeHeight = plotHeight / 6; // Use 1/6 of plot height for volume
    int priceHeight = plotHeight - volumeHeight;
    
    // Draw each candlestick
    for (size_t i = 0; i < candles.size() && i < (size_t)maxVisibleCandles; ++i) {
        const CandleData& candle = candles[i];
        
        // Map candle coordinates to screen coordinates
        int x = startX + i * (candleWidth + candleSpacing) + candleWidth / 2;
        
        // Scale prices
        int y_open = height - margin_bottom - static_cast<int>((candle.open - minPrice) / (maxPrice - minPrice) * priceHeight);
        int y_close = height - margin_bottom - static_cast<int>((candle.close - minPrice) / (maxPrice - minPrice) * priceHeight);
        int y_high = height - margin_bottom - static_cast<int>((candle.high - minPrice) / (maxPrice - minPrice) * priceHeight);
        int y_low = height - margin_bottom - static_cast<int>((candle.low - minPrice) / (maxPrice - minPrice) * priceHeight);
        
        // Ensure coordinates are within bounds
        y_open = clamp(y_open, margin_top, height - margin_bottom);
        y_close = clamp(y_close, margin_top, height - margin_bottom);
        y_high = clamp(y_high, margin_top, height - margin_bottom);
        y_low = clamp(y_low, margin_top, height - margin_bottom);
        
        // Determine if bullish (close > open) or bearish (close <= open)
        RGBA candleColor = (candle.close > candle.open) ? bullishColor : bearishColor;
        
        // Draw the candlestick
        drawCandlestick(x, y_open, y_close, y_high, y_low, candleColor);
    }
    
    // Add price movement indicators and current price display
    drawCandlestickPriceInfo(candles, bullishColor, bearishColor);
    
    // Add a legend for bullish/bearish candles
    std::vector<std::string> legendLabels;
    legendLabels.push_back("Bullish");
    legendLabels.push_back("Bearish");
    
    std::vector<RGBA> legendColors;
    legendColors.push_back(bullishColor);
    legendColors.push_back(bearishColor);
    
    // Position the legend in the top-left corner for better visibility
    addLegend(legendLabels, legendColors, margin_left + 15, margin_top + 15, 16);
}

void Cluster10::drawCandlestickYAxis(double minPrice, double maxPrice, int numTicks)
{
    int plotHeight = getPlotHeight();
    
    // Y-axis price labels and grid lines
    for (int i = 0; i <= numTicks; ++i) {
        float percentage = static_cast<float>(i) / numTicks;
        int y = height - margin_bottom - static_cast<int>(percentage * plotHeight);
        double priceValue = minPrice + percentage * (maxPrice - minPrice);
        
        // Draw horizontal grid line
        RGBA gridColor = elementColors["majorGrid"];
        // Don't draw grid line at the very bottom
        if (i > 0) {
            drawLine(margin_left, y, width - margin_right, y, gridColor);
        }
        
        // Draw price label with 2 decimal places
        char priceText[32];
        std::sprintf(priceText, "%.2f", priceValue);
        drawText(margin_left - 30, y, priceText, elementColors["axisLabel"], 16, true);
    }
}

void Cluster10::drawCandlestickXAxis(const std::vector<CandleData>& candles, int maxVisibleCandles, 
                                   int totalCandles, double firstTimestamp, double lastTimestamp)
{
    int plotWidth = getPlotWidth();
    
    // Calculate number of date labels to show (one per 3-4 candles)
    int dateLabelsCount = maxVisibleCandles / 3; 
    if (dateLabelsCount < 3) dateLabelsCount = 3;
    if (dateLabelsCount > 8) dateLabelsCount = 8; // Not too many labels
    
    for (int i = 0; i <= dateLabelsCount; ++i) {
        float percentage = static_cast<float>(i) / dateLabelsCount;
        int x = margin_left + static_cast<int>(percentage * plotWidth);
        
        // Calculate timestamp for this position
        int candleIndex = static_cast<int>(percentage * (totalCandles - 1));
        candleIndex = std::min(candleIndex, totalCandles - 1);
        candleIndex = std::max(candleIndex, 0);
        
        double timestamp = candles[candleIndex].timestamp;
        
        // Draw vertical grid line
        if (i > 0 && i < dateLabelsCount) {
            RGBA gridColor = elementColors["majorGrid"];
            drawLine(x, margin_top, x, height - margin_bottom, gridColor, 1);
        }
        
        // Format timestamp into readable date (for this example, use simple numeric format)
        // In a real application, you'd convert the timestamp to a proper date format
        char dateText[32];
        std::time_t time = static_cast<std::time_t>(timestamp);
        struct tm* timeinfo = std::localtime(&time);
        std::strftime(dateText, sizeof(dateText), "%m/%d", timeinfo);
        
        // Draw date label, avoiding overlap
        drawText(x, height - margin_bottom + 20, dateText, elementColors["axisLabel"], 14, true);
    }
}

void Cluster10::drawCandlestickPriceInfo(const std::vector<CandleData>& candles,
                                      const RGBA& bullishColor, const RGBA& bearishColor)
{
    if (candles.empty()) {
        return;
    }
    
    const CandleData& latestCandle = candles[candles.size() - 1];
    const CandleData& firstCandle = candles[0];
    
    // Calculate price change
    double priceChange = latestCandle.close - firstCandle.open;
    double percentChange = (priceChange / firstCandle.open) * 100.0;
    
    // Format price info
    char priceInfo[128];
    std::sprintf(priceInfo, "Close: %.2f  Change: %.2f (%.2f%%)", 
               latestCandle.close, priceChange, percentChange);
    
    // Display price info at the top of the chart
    RGBA priceInfoBg(0x1A, 0x1D, 0x2F, 0xDD); // Semi-transparent background
    int infoWidth = 250;
    int infoHeight = 40;
    int infoX = width - margin_right - infoWidth - 20;
    int infoY = margin_top + 20;
    
    // Draw info box with rounded corners
    int cornerRadius = 6;
    for (int dy = 0; dy < infoHeight; dy++) {
        for (int dx = 0; dx < infoWidth; dx++) {
            // Skip pixels in the rounded corners
            bool inCorner = false;
            
            // Top-left corner
            if (dx < cornerRadius && dy < cornerRadius) {
                int distSq = (cornerRadius - dx) * (cornerRadius - dx) + (cornerRadius - dy) * (cornerRadius - dy);
                inCorner = distSq > cornerRadius * cornerRadius;
            }
            // Top-right corner
            else if (dx >= infoWidth - cornerRadius && dy < cornerRadius) {
                int distSq = (dx - (infoWidth - cornerRadius)) * (dx - (infoWidth - cornerRadius)) + (cornerRadius - dy) * (cornerRadius - dy);
                inCorner = distSq > cornerRadius * cornerRadius;
            }
            // Bottom-left corner
            else if (dx < cornerRadius && dy >= infoHeight - cornerRadius) {
                int distSq = (cornerRadius - dx) * (cornerRadius - dx) + (dy - (infoHeight - cornerRadius)) * (dy - (infoHeight - cornerRadius));
                inCorner = distSq > cornerRadius * cornerRadius;
            }
            // Bottom-right corner
            else if (dx >= infoWidth - cornerRadius && dy >= infoHeight - cornerRadius) {
                int distSq = (dx - (infoWidth - cornerRadius)) * (dx - (infoWidth - cornerRadius)) + (dy - (infoHeight - cornerRadius)) * (dy - (infoHeight - cornerRadius));
                inCorner = distSq > cornerRadius * cornerRadius;
            }
            
            if (!inCorner) {
                int pixelX = infoX + dx;
                int pixelY = infoY + dy;
                if (pixelX >= 0 && pixelX < width && pixelY >= 0 && pixelY < height) {
                    image.SetPixel(pixelX, pixelY, priceInfoBg);
                }
            }
        }
    }
    
    // Set color based on price change
    RGBA priceChangeColor = (priceChange >= 0) ? bullishColor : bearishColor;
    
    // Draw price info text
    drawText(infoX + 15, infoY + infoHeight/2, priceInfo, priceChangeColor, 16, false);
} 