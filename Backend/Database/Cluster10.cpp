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
    // Initialize the color palette based on cluster10.fig design
    
    // Dark theme background colors
    elementColors["bgGradientTop"] = RGBA(0x18, 0x1B, 0x2C, 0xFF);    // Dark navy blue top
    elementColors["bgGradientBottom"] = RGBA(0x0A, 0x0C, 0x16, 0xFF); // Darker navy bottom
    
    // Grid and axes colors
    elementColors["majorGrid"] = RGBA(0x3A, 0x41, 0x5A, 0x99);        // Subtle blue-gray for grid lines, semi-transparent
    elementColors["minorGrid"] = RGBA(0x2A, 0x30, 0x45, 0x55);        // Darker blue-gray for minor grid lines, more transparent
    elementColors["axes"] = RGBA(0xF0, 0xF0, 0xF0, 0xFF);             // Almost white for axes
    elementColors["border"] = RGBA(0x3A, 0x41, 0x5A, 0xFF);           // Border color
    
    // Text colors
    elementColors["title"] = RGBA(0xFF, 0xFF, 0xFF, 0xFF);            // White for titles
    elementColors["axisLabel"] = RGBA(0xCC, 0xCC, 0xCC, 0xFF);        // Light gray for axis labels
    elementColors["legend"] = RGBA(0xEE, 0xEE, 0xEE, 0xFF);           // Off-white for legend text
    
    // Data visualization colors - vibrant modern palette
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
    
    // Draw axes if enabled
    if (showAxes) {
        drawAxes();
    }
}

void Cluster10::drawGrid()
{
    // Calculate the effective plotting area considering the margins
    int effectiveWidth = width - margin_left - margin_right;
    int effectiveHeight = height - margin_top - margin_bottom;
    
    // Number of grid divisions
    int majorGridDivisions = 4;   // Number of major grid divisions
    int minorGridDivisions = 16;  // Number of minor grid divisions
    
    // Draw minor grid lines first (so they appear behind major lines)
    for (int i = 0; i <= minorGridDivisions; i++) {
        float percentage = static_cast<float>(i) / minorGridDivisions;
        int x = margin_left + static_cast<int>(percentage * effectiveWidth);
        int y = margin_top + static_cast<int>(percentage * effectiveHeight);
        
        // Draw vertical minor grid line
        if (i % (minorGridDivisions / majorGridDivisions) != 0) { // Skip where major lines will be
            drawLine(x, margin_top, x, height - margin_bottom, elementColors["minorGrid"]);
        }
        
        // Draw horizontal minor grid line
        if (i % (minorGridDivisions / majorGridDivisions) != 0) { // Skip where major lines will be
            drawLine(margin_left, y, width - margin_right, y, elementColors["minorGrid"]);
        }
    }
    
    // Draw major grid lines
    for (int i = 0; i <= majorGridDivisions; i++) {
        float percentage = static_cast<float>(i) / majorGridDivisions;
        int x = margin_left + static_cast<int>(percentage * effectiveWidth);
        int y = margin_top + static_cast<int>(percentage * effectiveHeight);
        
        // Draw vertical major grid line
        drawLine(x, margin_top, x, height - margin_bottom, elementColors["majorGrid"]);
        
        // Draw horizontal major grid line
        drawLine(margin_left, y, width - margin_right, y, elementColors["majorGrid"]);
    }
    
    // Draw border around the plot area
    drawLine(margin_left, margin_top, width - margin_right, margin_top, elementColors["border"]);
    drawLine(width - margin_right, margin_top, width - margin_right, height - margin_bottom, elementColors["border"]);
    drawLine(margin_left, height - margin_bottom, width - margin_right, height - margin_bottom, elementColors["border"]);
    drawLine(margin_left, margin_top, margin_left, height - margin_bottom, elementColors["border"]);
}

void Cluster10::drawAxes()
{
    // Calculate the center (origin) of the plot
    int centerX = margin_left + (width - margin_left - margin_right) / 2;
    int centerY = margin_top + (height - margin_top - margin_bottom) / 2;
    
    // Draw X-axis (thicker line)
    for (int offset = -1; offset <= 1; offset++) {
        drawLine(margin_left, centerY + offset, width - margin_right, centerY + offset, elementColors["axes"]);
    }
    
    // Draw Y-axis (thicker line)
    for (int offset = -1; offset <= 1; offset++) {
        drawLine(centerX + offset, margin_top, centerX + offset, height - margin_bottom, elementColors["axes"]);
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
    
    // Calculate legend dimensions
    int itemHeight = fontSize + 10;
    int itemSpacing = 20;
    int colorBoxSize = fontSize - 4;
    int textOffset = colorBoxSize + 10;
    
    // Draw legend background with rounded corners
    int legendWidth = 200; // Fixed width
    int legendHeight = labels.size() * itemHeight + (labels.size() - 1) * itemSpacing + 20; // Height based on items
    
    // Draw a semi-transparent background
    RGBA bgColor(0x1A, 0x1D, 0x2F, 0xCC); // Semi-transparent dark background
    for (int dy = 0; dy < legendHeight; dy++) {
        for (int dx = 0; dx < legendWidth; dx++) {
            // Check if this pixel is in the rounded corner region
            bool inCorner = false;
            int cornerRadius = 10;
            
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
        
        // Draw color box
        int boxX = x + 10;
        int boxY = itemY;
        drawRect(boxX, boxY, colorBoxSize, colorBoxSize, colors[i], true);
        
        // Draw label text
        drawText(boxX + textOffset, boxY + fontSize/2, labels[i], elementColors["legend"], fontSize, false);
    }
}

void Cluster10::plotPoints(const std::vector<Point>& points, const RGBA& color, int pointSize)
{
    if (points.empty()) {
        return;
    }
    
    // Calculate the effective plotting area
    int plotWidth = width - margin_left - margin_right;
    int plotHeight = height - margin_top - margin_bottom;
    
    // Find min and max values for scaling
    double minX = points[0].x;
    double maxX = points[0].x;
    double minY = points[0].y;
    double maxY = points[0].y;
    
    // C++03 way to iterate through vector
    for (size_t i = 0; i < points.size(); ++i) {
        const Point& point = points[i];
        minX = std::min(minX, point.x);
        maxX = std::max(maxX, point.x);
        minY = std::min(minY, point.y);
        maxY = std::max(maxY, point.y);
    }
    
    // Add some padding to the ranges
    double xRange = maxX - minX;
    double yRange = maxY - minY;
    
    if (xRange < 1e-10) xRange = 1.0;
    if (yRange < 1e-10) yRange = 1.0;
    
    minX -= xRange * 0.05;
    maxX += xRange * 0.05;
    minY -= yRange * 0.05;
    maxY += yRange * 0.05;
    
    // Draw each point
    // C++03 way to iterate through vector
    for (size_t i = 0; i < points.size(); ++i) {
        const Point& point = points[i];
        // Map point coordinates to screen coordinates
        int x = margin_left + static_cast<int>((point.x - minX) / (maxX - minX) * plotWidth);
        int y = height - margin_bottom - static_cast<int>((point.y - minY) / (maxY - minY) * plotHeight);
        
        // Draw the point
        drawPoint(x, y, pointSize, color);
    }
}

void Cluster10::plotLine(const std::vector<Point>& points, const RGBA& color, int lineWidth)
{
    if (points.size() < 2) {
        return;
    }
    
    // Calculate the effective plotting area
    int plotWidth = width - margin_left - margin_right;
    int plotHeight = height - margin_top - margin_bottom;
    
    // Find min and max values for scaling
    double minX = points[0].x;
    double maxX = points[0].x;
    double minY = points[0].y;
    double maxY = points[0].y;
    
    // C++03 way to iterate through vector
    for (size_t i = 0; i < points.size(); ++i) {
        const Point& point = points[i];
        minX = std::min(minX, point.x);
        maxX = std::max(maxX, point.x);
        minY = std::min(minY, point.y);
        maxY = std::max(maxY, point.y);
    }
    
    // Add some padding to the ranges
    double xRange = maxX - minX;
    double yRange = maxY - minY;
    
    if (xRange < 1e-10) xRange = 1.0;
    if (yRange < 1e-10) yRange = 1.0;
    
    minX -= xRange * 0.05;
    maxX += xRange * 0.05;
    minY -= yRange * 0.05;
    maxY += yRange * 0.05;
    
    // Draw line segments between adjacent points
    for (size_t i = 1; i < points.size(); ++i) {
        // Map point coordinates to screen coordinates
        int x1 = margin_left + static_cast<int>((points[i-1].x - minX) / (maxX - minX) * plotWidth);
        int y1 = height - margin_bottom - static_cast<int>((points[i-1].y - minY) / (maxY - minY) * plotHeight);
        
        int x2 = margin_left + static_cast<int>((points[i].x - minX) / (maxX - minX) * plotWidth);
        int y2 = height - margin_bottom - static_cast<int>((points[i].y - minY) / (maxY - minY) * plotHeight);
        
        // Draw the line segment
        drawLine(x1, y1, x2, y2, color, lineWidth);
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
    
    // Calculate the effective plotting area
    int plotWidth = width - margin_left - margin_right;
    int plotHeight = height - margin_top - margin_bottom;
    
    // Find number of unique clusters
    int maxCluster = -1;
    for (size_t i = 0; i < labels.size(); ++i) {
        maxCluster = std::max(maxCluster, labels[i]);
    }
    
    // Prepare colors for each cluster
    std::vector<RGBA> clusterColors;
    for (int i = 0; i <= maxCluster; ++i) {
        clusterColors.push_back(themeColors[i % themeColors.size()]);
    }
    
    // Find min and max values for each dimension
    double minX = data[0][0];
    double maxX = data[0][0];
    double minY = data[0][1];
    double maxY = data[0][1];
    
    for (size_t i = 0; i < data.size(); ++i) {
        const std::vector<double>& point = data[i];
        minX = std::min(minX, point[0]);
        maxX = std::max(maxX, point[0]);
        minY = std::min(minY, point[1]);
        maxY = std::max(maxY, point[1]);
    }
    
    // Also consider centroids for scaling
    for (size_t i = 0; i < centroids.size(); ++i) {
        const std::vector<double>& centroid = centroids[i];
        if (centroid.size() >= 2) {
            minX = std::min(minX, centroid[0]);
            maxX = std::max(maxX, centroid[0]);
            minY = std::min(minY, centroid[1]);
            maxY = std::max(maxY, centroid[1]);
        }
    }
    
    // Add some padding to the ranges
    double xRange = maxX - minX;
    double yRange = maxY - minY;
    
    if (xRange < 1e-10) xRange = 1.0;
    if (yRange < 1e-10) yRange = 1.0;
    
    minX -= xRange * 0.05;
    maxX += xRange * 0.05;
    minY -= yRange * 0.05;
    maxY += yRange * 0.05;
    
    // Structure to store points for each cluster
    // In C++03, we need to initialize this vector manually
    std::vector<std::vector<std::pair<int, int> > > clusterPoints;
    for (int i = 0; i <= maxCluster; ++i) {
        std::vector<std::pair<int, int> > empty;
        clusterPoints.push_back(empty);
    }
    
    // Draw each data point
    for (size_t i = 0; i < data.size(); ++i) {
        // Map point coordinates to screen coordinates
        int x = margin_left + static_cast<int>((data[i][0] - minX) / (maxX - minX) * plotWidth);
        int y = height - margin_bottom - static_cast<int>((data[i][1] - minY) / (maxY - minY) * plotHeight);
        
        // Get the cluster label
        int cluster = labels[i];
        if (cluster < 0 || cluster > maxCluster) {
            continue;
        }
        
        // Store point coordinates for later use
        std::pair<int, int> point;
        point.first = x;
        point.second = y;
        clusterPoints[cluster].push_back(point);
        
        // Draw the point with the cluster color
        drawPoint(x, y, 6, clusterColors[cluster]);
    }
    
    // Draw circles around each cluster
    for (int cluster = 0; cluster <= maxCluster; ++cluster) {
        const std::vector<std::pair<int, int> >& points = clusterPoints[cluster];
        if (points.empty()) {
            continue;
        }
        
        // Calculate the centroid of the cluster in screen coordinates
        int sumX = 0, sumY = 0;
        for (size_t i = 0; i < points.size(); ++i) {
            sumX += points[i].first;
            sumY += points[i].second;
        }
        int centroidX = sumX / points.size();
        int centroidY = sumY / points.size();
        
        // Find the maximum distance from centroid to any point in the cluster
        int maxDist = 0;
        for (size_t i = 0; i < points.size(); ++i) {
            int dx = points[i].first - centroidX;
            int dy = points[i].second - centroidY;
            int dist = static_cast<int>(std::sqrt(static_cast<double>(dx*dx + dy*dy)));
            maxDist = std::max(maxDist, dist);
        }
        
        // Add some padding to the radius
        int radius = maxDist + 10;
        
        // Make the outline color semi-transparent
        RGBA outlineColor = clusterColors[cluster];
        outlineColor.a = 120; // Semi-transparent
        
        // Draw a circle to encompass all points in the cluster
        drawCircle(centroidX, centroidY, radius, outlineColor, false, 3);
        
        // Add a cluster label
        char buffer[64];
        std::sprintf(buffer, "Cluster %d", cluster);
        std::string clusterLabel = buffer;
        drawText(centroidX, centroidY - radius - 15, clusterLabel, clusterColors[cluster], 24, true);
    }
    
    // Draw centroids with a special marker if available
    for (size_t i = 0; i < centroids.size(); ++i) {
        if (centroids[i].size() >= 2 && i <= static_cast<size_t>(maxCluster)) {
            // Map centroid coordinates to screen coordinates
            int x = margin_left + static_cast<int>((centroids[i][0] - minX) / (maxX - minX) * plotWidth);
            int y = height - margin_bottom - static_cast<int>((centroids[i][1] - minY) / (maxY - minY) * plotHeight);
            
            // Draw a larger white point for the centroid
            drawPoint(x, y, 10, RGBA(0xFF, 0xFF, 0xFF, 0xFF));
            
            // Draw a smaller colored point on top
            drawPoint(x, y, 7, clusterColors[i]);
            
            // Draw crosshairs
            int crossSize = 15;
            drawLine(x - crossSize, y, x + crossSize, y, RGBA(0xFF, 0xFF, 0xFF, 0xFF), 2);
            drawLine(x, y - crossSize, x, y + crossSize, RGBA(0xFF, 0xFF, 0xFF, 0xFF), 2);
        }
    }
    
    // Add a legend for the clusters
    std::vector<std::string> legendLabels;
    std::vector<RGBA> legendColors;
    
    for (int i = 0; i <= maxCluster; ++i) {
        char buffer[64];
        std::sprintf(buffer, "Cluster %d", i);
        legendLabels.push_back(buffer);
        legendColors.push_back(clusterColors[i]);
    }
    
    addLegend(legendLabels, legendColors, width - margin_right - 220, margin_top + 20);
}

void Cluster10::plotHistogram(const std::vector<int>& bins, const RGBA& color)
{
    if (bins.empty()) {
        return;
    }
    
    // Calculate the maximum bin value
    int maxBinValue = bins[0];
    for (size_t i = 1; i < bins.size(); ++i) {
        if (bins[i] > maxBinValue) {
            maxBinValue = bins[i];
        }
    }
    
    if (maxBinValue == 0) {
        maxBinValue = 1; // Avoid division by zero
    }
    
    // Calculate the effective plotting area
    int plotWidth = width - margin_left - margin_right;
    int plotHeight = height - margin_top - margin_bottom;
    
    // Calculate bar width and spacing
    int barSpacing = 5;
    int barWidth = (plotWidth - (bins.size() - 1) * barSpacing) / bins.size();
    
    // Draw each bar
    for (size_t i = 0; i < bins.size(); ++i) {
        int barHeight = static_cast<int>((static_cast<double>(bins[i]) / maxBinValue) * plotHeight);
        int x = margin_left + i * (barWidth + barSpacing);
        int y = height - margin_bottom - barHeight;
        
        // Draw the bar with a gradient effect
        for (int dy = 0; dy < barHeight; ++dy) {
            // Calculate fade factor (1.0 at bottom, 0.7 at top)
            float fadeFactor = 0.7f + 0.3f * (1.0f - static_cast<float>(dy) / barHeight);
            
            RGBA barColor(
                static_cast<unsigned char>(color.r * fadeFactor),
                static_cast<unsigned char>(color.g * fadeFactor),
                static_cast<unsigned char>(color.b * fadeFactor),
                color.a
            );
            
            for (int dx = 0; dx < barWidth; ++dx) {
                image.SetPixel(x + dx, y + dy, barColor);
            }
        }
        
        // Draw value on top of the bar
        char buffer[64];
        std::sprintf(buffer, "%d", bins[i]);
        std::string valueText = buffer;
        drawText(x + barWidth / 2, y - 15, valueText, RGBA(0xFF, 0xFF, 0xFF, 0xFF), 18, true);
    }
} 