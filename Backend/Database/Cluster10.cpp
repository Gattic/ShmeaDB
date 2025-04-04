// Cluster10.cpp
#include "Cluster10.h"
#include "png-helper.h"
#include <algorithm>
#include <limits>
#include <string>  // for std::to_string

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
      cornerRadius(12)
{
    // Initialize the visualization
    initialize();
}

// Helper method for constructor initialization
void Cluster10::initialize()
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
    // Initialize the color palette based on exact values from the CSS file
    
    // Background colors from CSS dark theme
    // Using a dark blue background similar to the app theme
    elementColors["bgGradientTop"] = RGBA(0x19, 0x23, 0x35, 0xFF);    // --chart-lines: #192335
    elementColors["bgGradientBottom"] = RGBA(0x00, 0x0B, 0x1E, 0xFF); // --text-inverted: #000b1e (darker for gradient effect)
    
    // Grid and axes colors from CSS 
    elementColors["majorGrid"] = RGBA(0x19, 0x23, 0x35, 0x80);        // --chart-lines with transparency
    elementColors["minorGrid"] = RGBA(0x19, 0x23, 0x35, 0x40);        // Lighter grid lines
    elementColors["axes"] = RGBA(0xFF, 0xFF, 0xFF, 0xCC);             // --text-primary: #ffffff with slight transparency
    elementColors["border"] = RGBA(0xCD, 0xD5, 0xE5, 0x26);           // --divider: rgba(205 213 229 / 0.15)
    
    // Text colors from CSS
    elementColors["title"] = RGBA(0xFF, 0xFF, 0xFF, 0xFF);            // --text-primary: #ffffff
    elementColors["axisLabel"] = RGBA(0xCD, 0xD5, 0xE5, 0xCC);        // Based on --divider but more opaque
    elementColors["legend"] = RGBA(0xFF, 0xFF, 0xFF, 0xEE);           // --text-primary with slight transparency
    
    // Info box colors from CSS
    elementColors["legendBgTop"] = RGBA(0x23, 0x0B, 0x6A, 0xF0);      // --blue-marguerite-950: #230b6a
    elementColors["legendBgBottom"] = RGBA(0x15, 0x21, 0x56, 0xF0);   // --cornflower-blue-950: #152156
    
    // Theme colors from CSS semantic colors - EXACT matches
    themeColors.push_back(RGBA(0x5C, 0xFF, 0xB3, 0xFF));              // --charts-group-1-fill: var(--aquamarine-300): #5cffb3
    themeColors.push_back(RGBA(0x5C, 0xE9, 0xFF, 0xFF));              // --charts-group-2-fill: var(--spray-300): #5ce9ff
    themeColors.push_back(RGBA(0xBA, 0xB1, 0xFF, 0xFF));              // --charts-group-3-fill: var(--blue-marguerite-300): #bab1ff
    themeColors.push_back(RGBA(0xD6, 0xFF, 0xC7, 0xFF));              // --charts-group-4-fill: var(--screamin-green-100): #d6ffc7
    themeColors.push_back(RGBA(0x5C, 0x9D, 0xFF, 0xFF));              // --charts-group-5-fill: var(--cornflower-blue-400): #5c9dff
    themeColors.push_back(RGBA(0x01, 0xB8, 0x63, 0xFF));              // --charts-group-6-fill: var(--aquamarine-600): #01b863
    themeColors.push_back(RGBA(0x8C, 0x4A, 0x72, 0xFF));              // --charts-group-7-fill: var(--wine-berry-700): #8c4a72
    themeColors.push_back(RGBA(0x01, 0x92, 0xB9, 0xFF));              // --charts-group-8-fill: var(--spray-600): #0192b9
    
    // Special chart colors - EXACT matches from CSS
    elementColors["bullish"] = RGBA(0x33, 0xF5, 0x9B, 0xFF);          // --price-buy: var(--aquamarine-400): #33f59b
    elementColors["bearish"] = RGBA(0xFF, 0x5C, 0x74, 0xFF);          // --price-sell: var(--wild-watermelon-400): #ff5c74
    
    // Dark versions of colors for contrast and accents - from CSS
    elementColors["cluster1Dark"] = RGBA(0x0A, 0x71, 0x43, 0xFF);     // --charts-group-1-dark: var(--aquamarine-800): #0a7143
    elementColors["cluster2Dark"] = RGBA(0x11, 0x5E, 0x79, 0xFF);     // --charts-group-2-dark: var(--spray-800): #115e79
    elementColors["cluster3Dark"] = RGBA(0x47, 0x18, 0xBF, 0xFF);     // --charts-group-3-dark: var(--blue-marguerite-800): #4718bf
    elementColors["cluster4Dark"] = RGBA(0x17, 0x69, 0x0B, 0xFF);     // --charts-group-4-dark: var(--screamin-green-800): #17690b
    
    // Additional accent colors
    elementColors["highlight"] = RGBA(0xFF, 0xFF, 0xFF, 0x80);        // White highlight with 50% opacity
    elementColors["shadow"] = RGBA(0x00, 0x00, 0x00, 0x80);           // Black shadow with 50% opacity
    elementColors["innerShadowBg"] = RGBA(0x19, 0x23, 0x35, 0x03);    // --inner-shadow-bg: rgba(25 35 53 / 0.01) but slightly more visible
    
    // Colors for cluster shadows from cluster10.css
    // From box-shadow: inset 0px 0px 248.9px -140px #BAB1FF;
    elementColors["cluster1Shadow"] = RGBA(0x5C, 0xFF, 0xB3, 0x80);   // Cluster 1 shadow color (#5CFFB3 with 50% opacity)
    elementColors["cluster2Shadow"] = RGBA(0x5C, 0xE9, 0xFF, 0x80);   // Cluster 2 shadow color (#5CE9FF with 50% opacity)
    elementColors["cluster3Shadow"] = RGBA(0xBA, 0xB1, 0xFF, 0x80);   // Cluster 3 shadow color (#BAB1FF with 50% opacity)
    elementColors["cluster4Shadow"] = RGBA(0xD6, 0xFF, 0xC7, 0x80);   // Cluster 4 shadow color (#D6FFC7 with 50% opacity)
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
    // Draw the dark gradient background from the cluster10.css design
    // CSS: background: radial-gradient(164.63% 83.5% at 54.69% 50%, #021331 0%, #000B1E 100%);
    
    // Define the gradient center point (at 54.69% 50% as specified in CSS)
    float centerX = width * 0.5469f;
    float centerY = height * 0.5f;
    
    // Define the gradient radius (164.63% width and 83.5% height elliptical gradient)
    float radiusX = width * 1.6463f;
    float radiusY = height * 0.835f;
    
    // Define the colors from CSS
    RGBA centerColor(0x02, 0x13, 0x31, 0xFF); // #021331
    RGBA edgeColor(0x00, 0x0B, 0x1E, 0xFF);   // #000B1E
    
    // Render the radial gradient
    for (unsigned int y = 0; y < height; ++y) {
        for (unsigned int x = 0; x < width; ++x) {
            // Calculate distance from center (normalize based on elliptical radiuses)
            float dx = (x - centerX) / radiusX;
            float dy = (y - centerY) / radiusY;
            
            // Calculate normalized distance (0.0 to 1.0) - squared for smoother gradient
            float dist = std::sqrt(dx*dx + dy*dy);
            dist = std::min(1.0f, dist); // Clamp to maximum 1.0
            
            // Interpolate between the two colors
            RGBA pixelColor(
                static_cast<unsigned char>(centerColor.r * (1.0f - dist) + edgeColor.r * dist),
                static_cast<unsigned char>(centerColor.g * (1.0f - dist) + edgeColor.g * dist),
                static_cast<unsigned char>(centerColor.b * (1.0f - dist) + edgeColor.b * dist),
                0xFF
            );
            
            // Set the pixel
            image.SetPixel(x, y, pixelColor);
        }
    }
    
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
    
    // Use significantly fewer grid divisions - only major grid lines, no minor lines
    int gridDivisionsX = 4;  // 4 evenly spaced vertical gridlines
    int gridDivisionsY = 3;  // 3 evenly spaced horizontal gridlines
    
    // Get colors for the grid lines - use CSS-aligned colors
    RGBA gridColor = elementColors["majorGrid"];
    
    // Draw X-axis grid lines (vertical lines)
    for (int i = 1; i < gridDivisionsX; i++) {
        float percentage = static_cast<float>(i) / gridDivisionsX;
        int x = margin_left + static_cast<int>(percentage * effectiveWidth);
        
        // Draw vertical grid line
        drawLine(x, margin_top, x, height - margin_bottom, gridColor, 1);
    }
    
    // Draw Y-axis grid lines (horizontal lines)
    for (int i = 1; i < gridDivisionsY; i++) {
        float percentage = static_cast<float>(i) / gridDivisionsY;
        int y = height - margin_bottom - static_cast<int>(percentage * effectiveHeight);
        
        // Draw horizontal grid line
        drawLine(margin_left, y, width - margin_right, y, gridColor, 1);
    }
    
    // Draw plot border - using theme border color
    RGBA borderColor = elementColors["border"];
    
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
    
    // Calculate dimensions based on content
    int itemHeight = fontSize + 6;
    int itemSpacing = 10;
    int colorIndicatorSize = fontSize - 2;
    int colorTextPadding = 12;
    
    // Calculate legend width based on text length
    int maxTextWidth = 0;
    for (size_t i = 0; i < labels.size(); ++i) {
        int textWidth = static_cast<int>(labels[i].length() * fontSize * 0.6);
        maxTextWidth = std::max(maxTextWidth, textWidth);
    }
    
    int legendWidth = maxTextWidth + colorIndicatorSize + colorTextPadding + 36;
    int legendHeight = labels.size() * itemHeight + (labels.size() - 1) * itemSpacing + 24;
    
    // Draw the box using our common method (no text initially)
    drawInfoBox(x, y, legendWidth, legendHeight, "", fontSize);
    
    // Draw each legend item with exact positioning
    for (size_t i = 0; i < labels.size(); ++i) {
        int itemY = y + 10 + i * (itemHeight + itemSpacing);
        
        // Draw color indicator dot
        int dotX = x + 15;
        int dotY = itemY + itemHeight/2;
        drawCircle(dotX, dotY, colorIndicatorSize/2, colors[i], true);
        
        // Draw label text
        drawText(dotX + colorTextPadding, dotY, labels[i], elementColors["legend"], fontSize, false);
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

// Helper for creating cluster legends
void Cluster10::createClusterLegend(const std::vector<RGBA>& clusterColors, int numClusters, int x, int y)
{
    std::vector<std::string> legendLabels;
    std::vector<RGBA> legendColors;
    
    // Add cluster entries
    for (int i = 0; i < numClusters; ++i) {
        char label[32];
        std::sprintf(label, "Cluster %d", i);
        legendLabels.push_back(label);
        
        // Use fully opaque colors for the legend dots
        RGBA legendColor = clusterColors[i];
        legendColor.a = 0xFF; // Full opacity for legend dots
        legendColors.push_back(legendColor);
    }
    
    // Add the centroid legend item
    legendLabels.push_back("Centroid");
    legendColors.push_back(RGBA(0xFF, 0xFF, 0xFF, 0xFF)); // Fully opaque
    
    // Add the legend to the visualization
    addLegend(legendLabels, legendColors, x, y, 16);
}

// Plot clusters with centroids
void Cluster10::plotClusters(const std::vector<std::vector<double> >& data, const std::vector<int>& labels, 
                           const std::vector<std::vector<double> >& centroids)
{
    if (data.empty() || data[0].size() < 2 || data.size() != labels.size()) {
        return;
    }
    
    // Create a chart configuration
    ChartConfig config("Cluster Analysis", 36, "Feature X", "Feature Y");
    
    // Initialize chart with background, grid, etc.
    initializeChart(config.title, config.titleFontSize);
    
    // Find number of unique clusters
    int maxCluster = -1;
    for (size_t i = 0; i < labels.size(); ++i) {
        maxCluster = std::max(maxCluster, labels[i]);
    }
    
    // Total number of clusters
    int numClusters = maxCluster + 1;
    
    // Prepare colors for each cluster - use colors from our theme
    std::vector<RGBA> clusterColors;
    
    // Use colors from themeColors
    for (int i = 0; i < numClusters && i < static_cast<int>(themeColors.size()); ++i) {
        clusterColors.push_back(themeColors[i]);
    }
    
    // If there are more clusters than theme colors, cycle through them
    for (int i = themeColors.size(); i < numClusters; ++i) {
        clusterColors.push_back(themeColors[i % themeColors.size()]);
    }
    
    // Calculate axis ranges for X and Y dimensions, including centroids in the calculation
    AxisRange xRange = calculateXRange(data);
    AxisRange yRange = calculateYRange(data);
    
    // Also consider centroids for scaling
    for (size_t i = 0; i < centroids.size(); ++i) {
        const std::vector<double>& centroid = centroids[i];
        if (centroid.size() >= 2) {
            // Update ranges if centroid extends beyond current range
            xRange.min = std::min(xRange.min, centroid[0] - xRange.padding);
            xRange.max = std::max(xRange.max, centroid[0] + xRange.padding);
            yRange.min = std::min(yRange.min, centroid[1] - yRange.padding);
            yRange.max = std::max(yRange.max, centroid[1] + yRange.padding);
        }
    }
    
    // Add axis labels
    drawText(width / 2, height - margin_bottom / 3, config.xAxisLabel, elementColors["axisLabel"], config.axisFontSize, true);
    drawVerticalText(config.yAxisLabel, margin_left/4, height/2 - 70, config.axisFontSize, elementColors["axisLabel"]);
    
    // Draw axis ticks using our helper methods
    drawXAxisTicks(xRange.min, xRange.max, 4, 1);
    drawYAxisTicks(yRange.min, yRange.max, 3, false, 1, 25);
    
    // Structure to store points for each cluster
    std::vector<std::vector<std::pair<int, int> > > clusterPoints;
    for (int i = 0; i < numClusters; ++i) {
        std::vector<std::pair<int, int> > empty;
        clusterPoints.push_back(empty);
    }
    
    // Calculate optimal point size based on data density
    int pointSize = 6;  // Default size
    if (data.size() < 50) {
        pointSize = 8;  // Larger points for small datasets
    } else if (data.size() > 200) {
        pointSize = 4;  // Smaller points for large datasets
    }
    
    // First, prepare cluster boundaries using data coordinates
    std::vector<Point> clusterCenters;
    std::vector<int> clusterRadii;
    
    for (int cluster = 0; cluster < numClusters; ++cluster) {
        // Calculate cluster bounds and center
        calculateClusterBounds(data, labels, cluster, clusterCenters, clusterRadii, xRange, yRange);
    }

    // Draw cluster circles with proper transparency
    drawClusterCircles(clusterCenters, clusterRadii, clusterColors);

    // Draw each data point on top of the circles
    for (size_t i = 0; i < data.size(); ++i) {
        if (data[i].size() < 2) continue;
        
        // Get the cluster label
        int cluster = labels[i];
        if (cluster < 0 || cluster >= numClusters) {
            continue;
        }
        
        // Map data point to screen coordinates
        Point screenPoint = mapDataToScreen(data[i][0], data[i][1], xRange, yRange);
        
        // Store point coordinates for later use
        clusterPoints[cluster].push_back(std::make_pair(screenPoint.x, screenPoint.y));
        
        // Draw the point with the cluster color
        drawPoint(screenPoint.x, screenPoint.y, pointSize, clusterColors[cluster]);
    }
    
    // Draw centroids as white circles with colored crosses
    drawCentroids(centroids, clusterColors, xRange, yRange);
    
    // Draw cluster labels
    drawClusterLabels(clusterCenters, clusterPoints, clusterColors);
    
    // Add the legend
    createClusterLegend(clusterColors, numClusters, width - margin_right - 150, margin_top + 15);
}

// Helper method to calculate cluster bounds
void Cluster10::calculateClusterBounds(
    const std::vector<std::vector<double> >& data,
    const std::vector<int>& labels,
    int cluster,
    std::vector<Point>& clusterCenters,
    std::vector<int>& clusterRadii,
    const AxisRange& xRange,
    const AxisRange& yRange)
{
        // Calculate cluster bounds
        double minX = std::numeric_limits<double>::max();
        double maxX = -std::numeric_limits<double>::max();
        double minY = std::numeric_limits<double>::max();
        double maxY = -std::numeric_limits<double>::max();
        double sumX = 0.0, sumY = 0.0;
        int pointCount = 0;
        
        // Calculate cluster centroid and bounds from all points
        for (size_t i = 0; i < data.size(); ++i) {
            if (labels[i] == cluster && data[i].size() >= 2) {
                pointCount++;
                sumX += data[i][0];
                sumY += data[i][1];
                minX = std::min(minX, data[i][0]);
                maxX = std::max(maxX, data[i][0]);
                minY = std::min(minY, data[i][1]);
                maxY = std::max(maxY, data[i][1]);
            }
        }
        
        if (pointCount == 0) {
            // Add placeholder values for empty clusters
            clusterCenters.push_back(Point(0, 0));
            clusterRadii.push_back(0);
        return;
        }
        
        // Calculate true centroid (average of all points)
        double centerX = sumX / pointCount;
        double centerY = sumY / pointCount;
        
        // Find the maximum distance from any point to the centroid
        double maxDist = 0.0;
        for (size_t i = 0; i < data.size(); ++i) {
            if (labels[i] == cluster && data[i].size() >= 2) {
                double dx = data[i][0] - centerX;
                double dy = data[i][1] - centerY;
                double dist = std::sqrt(dx*dx + dy*dy);
                maxDist = std::max(maxDist, dist);
            }
        }
        
        // Map to screen coordinates
        Point screenCenter = mapDataToScreen(centerX, centerY, xRange, yRange);
        Point edgePoint = mapDataToScreen(centerX + maxDist, centerY, xRange, yRange);
    int radius = std::abs(edgePoint.x - screenCenter.x) + 15; // Add padding for better visibility
        
        clusterCenters.push_back(screenCenter);
        clusterRadii.push_back(radius);
    }
    
// Helper method to draw cluster circles with transparency
void Cluster10::drawClusterCircles(
    const std::vector<Point>& clusterCenters,
    const std::vector<int>& clusterRadii,
    const std::vector<RGBA>& clusterColors)
{
    // Draw each cluster circle with proper transparency
    for (size_t cluster = 0; cluster < clusterCenters.size(); ++cluster) {
        if (clusterRadii[cluster] == 0) continue; // Skip empty clusters
        
        // Get the cluster color
        RGBA circleColor = clusterColors[cluster % clusterColors.size()];
        
        // Get the appropriate shadow color based on cluster index (matching CSS)
        RGBA shadowColor;
        char shadowKeyBuffer[32];
        std::sprintf(shadowKeyBuffer, "cluster%dShadow", (int)(cluster+1));
        std::string shadowKey(shadowKeyBuffer);
        
        // Use the specific cluster shadow if defined, otherwise fallback to the cluster color
        if (elementColors.find(shadowKey) != elementColors.end()) {
            shadowColor = elementColors[shadowKey];
        } else {
            // If not found, create a semi-transparent version of the cluster color
            shadowColor = circleColor;
            shadowColor.a = 0x80; // 50% opacity
        }
        
        // CSS styling from cluster10.css
        // box-sizing: border-box;
        // background: rgba(25, 35, 53, 0.01);
        // box-shadow: inset 0px 0px 248.9px -140px #BAB1FF;
        // border: 1px solid clusterColor;
        
        // Create fill and border colors with proper opacity
        RGBA fillColor = circleColor;
        fillColor.a = 38; // 15% opacity (38/255 ≈ 0.15)
        
        RGBA borderColor = circleColor;
        borderColor.a = 0xCC; // 80% opacity (0xCC ≈ 204/255 ≈ 0.8)
        
        int radius = clusterRadii[cluster];
        Point center = clusterCenters[cluster];
        
        // Draw the cluster circle in a single pass
        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                // Calculate exact distance from center
                int distSqr = dx*dx + dy*dy;
                float dist = std::sqrt(static_cast<float>(distSqr));
                
                // Skip pixels outside the circle
                if (dist > radius) continue;
                
                int drawX = center.x + dx;
                int drawY = center.y + dy;
                
                // Check plot area bounds
                if (drawX < static_cast<int>(margin_left) || 
                    drawX >= static_cast<int>(width - margin_right) ||
                    drawY < static_cast<int>(margin_top) || 
                    drawY >= static_cast<int>(height - margin_bottom)) {
                    continue; // Skip pixels outside plot area
                }
                
                // Get the existing pixel color to blend with
                RGBA existingColor = image.GetPixel(drawX, drawY);
                
                // Layer 1: Base fill color (main circle fill)
                // All pixels within the circle get this base fill color
                RGBA baseColor = fillColor;
                image.SetPixel(drawX, drawY, blendColors(existingColor, baseColor, 0.15f));
                
                // Layer 2: Apply inner shadow if within shadow distance
                float maxShadowDist = radius * 0.65f; // From CSS box-shadow radius
                if (dist <= maxShadowDist) {
                    // Shadow intensity follows a natural curve - stronger near the edge
                    float shadowFactor = 1.0f - (dist / maxShadowDist);
                    float innerShadowIntensity = 0.3f * shadowFactor * shadowFactor;
                    
                    // Apply shadow with blending (gets current color and blends shadow on top)
                    RGBA currentColor = image.GetPixel(drawX, drawY);
                    RGBA withShadow = blendColors(currentColor, shadowColor, innerShadowIntensity);
                    image.SetPixel(drawX, drawY, withShadow);
                }
                
                // Layer 3: Add border if pixel is at the edge
                float distFromEdge = radius - dist;
                if (distFromEdge < 1.0f) {
                    // This is a border pixel - use higher opacity border color
                    RGBA currentColor = image.GetPixel(drawX, drawY);
                    RGBA withBorder = blendColors(currentColor, borderColor, 0.8f);
                    image.SetPixel(drawX, drawY, withBorder);
                }
            }
        }
    }
}

// Helper method to draw centroids
void Cluster10::drawCentroids(
    const std::vector<std::vector<double> >& centroids,
    const std::vector<RGBA>& clusterColors,
    const AxisRange& xRange,
    const AxisRange& yRange)
{
    // Draw centroids
    for (size_t i = 0; i < centroids.size() && i < clusterColors.size(); ++i) {
        if (centroids[i].size() < 2) continue;
        
        // Map centroid to screen coordinates
        Point centroidPoint = mapDataToScreen(centroids[i][0], centroids[i][1], xRange, yRange);
        
        // From cluster10.css - centroids are solid circles with drop shadows
        // First draw shadow - offset by 1px and 40% opacity
        RGBA shadowColor(0x00, 0x00, 0x00, 0x66); // 40% opacity black shadow (0.4 * 255 = 102)
        
        // Draw larger soft shadow first (matches CSS box-shadow effect)
        for (int dy = -10; dy <= 10; dy++) {
            for (int dx = -10; dx <= 10; dx++) {
                int distSqr = dx*dx + dy*dy;
                if (distSqr > 100) continue; // Only pixels within 10px radius
                
                // Calculate shadow intensity - fade out toward edges
                float shadowIntensity = 0.3f * (1.0f - std::sqrt(distSqr) / 10.0f);
                
                int drawX = centroidPoint.x + dx + 1; // +1 for shadow offset
                int drawY = centroidPoint.y + dy + 1; // +1 for shadow offset
                
                if (drawX >= 0 && drawX < static_cast<int>(width) &&
                    drawY >= 0 && drawY < static_cast<int>(height)) {
                    RGBA pixelShadow = shadowColor;
                    pixelShadow.a = static_cast<unsigned char>(shadowColor.a * shadowIntensity);
                    blendPixel(drawX, drawY, pixelShadow, shadowIntensity);
                }
            }
        }
        
        // Draw the white circle on top of the shadow (matches CSS)
        RGBA whiteFill(0xFF, 0xFF, 0xFF, 0xFF); // Solid white
        
        // Draw solid white circle (8px radius as in CSS)
        for (int dy = -8; dy <= 8; dy++) {
            for (int dx = -8; dx <= 8; dx++) {
                int distSqr = dx*dx + dy*dy;
                if (distSqr > 64) continue; // Only pixels within 8px radius
                
                int drawX = centroidPoint.x + dx;
                int drawY = centroidPoint.y + dy;
                
                if (drawX >= 0 && drawX < static_cast<int>(width) &&
                    drawY >= 0 && drawY < static_cast<int>(height)) {
                    image.SetPixel(drawX, drawY, whiteFill);
                }
            }
        }
        
        // Get the appropriate cross color (dark version of the cluster color)
        RGBA crossColor;
        
        // Use the dark version of the color if available, otherwise darken the cluster color
        char darkColorKeyBuffer[32];
        std::sprintf(darkColorKeyBuffer, "cluster%dDark", (int)(i+1));
        std::string darkColorKey(darkColorKeyBuffer);
        
        if (i < 4 && elementColors.find(darkColorKey) != elementColors.end()) {
            // Use the exact CSS dark color for better match with the design system
            crossColor = elementColors[darkColorKey];
        } else {
            // For higher indices, darken the cluster color
            crossColor = clusterColors[i];
            // Make it darker for better visibility against white
            crossColor.r = static_cast<unsigned char>(crossColor.r * 0.6f);
            crossColor.g = static_cast<unsigned char>(crossColor.g * 0.6f);
            crossColor.b = static_cast<unsigned char>(crossColor.b * 0.6f);
        }
        crossColor.a = 0xFF; // Fully opaque
        
        // Draw horizontal line of cross - exactly as in CSS (3px thick line)
        for (int y = -1; y <= 1; ++y) {
            drawLine(centroidPoint.x - 7, centroidPoint.y + y, centroidPoint.x + 7, centroidPoint.y + y, crossColor);
        }
        
        // Draw vertical line of cross - exactly as in CSS (3px thick line)
        for (int x = -1; x <= 1; ++x) {
            drawLine(centroidPoint.x + x, centroidPoint.y - 7, centroidPoint.x + x, centroidPoint.y + 7, crossColor);
        }
    }
}

// Helper method to draw cluster labels
void Cluster10::drawClusterLabels(
    const std::vector<Point>& clusterCenters,
    const std::vector<std::vector<std::pair<int, int> > >& clusterPoints,
    const std::vector<RGBA>& clusterColors)
{
    // Draw cluster labels
    for (size_t cluster = 0; cluster < clusterPoints.size(); ++cluster) {
        if (clusterPoints[cluster].empty()) continue;
        
        // Calculate optimal label position based on cluster layout
        int offsetX = 0, offsetY = 0;
        
        // Position based on cluster position in reference image
        switch (cluster) {
            case 0: // Orange cluster (bottom left)
                offsetX = 5;
                offsetY = 40;
                break;
            case 1: // Blue cluster (top right)
                offsetX = -5;
                offsetY = -30;
                break;
            case 2: // Green cluster (bottom right)
                offsetX = -30;
                offsetY = 30;
                break;
            default:
                offsetX = (cluster % 2 == 0) ? 30 : -30;
                offsetY = (cluster % 2 == 0) ? 30 : -30;
        }
        
        // Format label
        char label[32];
        std::sprintf(label, "Cluster %d", (int)cluster);
        
        // Draw label at calculated position
        RGBA labelColor = clusterColors[cluster % clusterColors.size()];
        drawText(clusterCenters[cluster].x + offsetX, clusterCenters[cluster].y + offsetY, 
                label, labelColor, 22, true);
    }
}

void Cluster10::drawCandlestick(int x, int y_open, int y_close, int y_high, int y_low, const RGBA& color)
{
    // Define the width of the candlestick body
    int bodyWidth = 14;  // Width of candlestick body in pixels
    int wickThickness = 3;  // Thickness of wick line
    
    // Draw the wick (line from high to low) with proper transparency handling
    for (int i = -wickThickness/2; i <= wickThickness/2; ++i) {
        // Use semi-transparent color for the wick to match design
        RGBA wickColor = color;
        wickColor.a = 0xFF; // Make wick fully opaque
        drawLine(x + i, y_high, x + i, y_low, wickColor);
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
            // Set the pixel directly with full opacity
            int drawX = x + dx;
            int drawY = dy;
            
            if (drawX >= 0 && drawX < static_cast<int>(width) && 
                drawY >= 0 && drawY < static_cast<int>(height)) {
                RGBA bodyColor = color;
                bodyColor.a = 0xFF; // Make body fully opaque
                image.SetPixel(drawX, drawY, bodyColor);
            }
        }
    }
    
    // Add highlight/shadow for 3D effect with proper alpha blending
    RGBA highlightColor(0xFF, 0xFF, 0xFF, 0x80);  // Semi-transparent white (50% opacity)
    RGBA shadowColor(0x00, 0x00, 0x00, 0x80);     // Semi-transparent black (50% opacity)
    
    // Left edge highlight - with proper alpha blending
    for (int dy = bodyTop; dy <= bodyBottom; ++dy) {
        for (int dx = 0; dx < 3; ++dx) { // 3px highlight width
            // Calculate fade strength based on position
            float alpha = 0x80 * (3.0f - dx) / 3.0f / 255.0f;
            int drawX = x - bodyWidth/2 + dx;
            blendPixel(drawX, dy, highlightColor, alpha);
        }
    }
    
    // Right edge shadow - with proper alpha blending
    for (int dy = bodyTop; dy <= bodyBottom; ++dy) {
        for (int dx = 0; dx < 3; ++dx) { // 3px shadow width
            // Calculate fade strength based on position
            float alpha = 0x80 * (3.0f - dx) / 3.0f / 255.0f;
            int drawX = x + bodyWidth/2 - dx - 1;
            blendPixel(drawX, dy, shadowColor, alpha);
        }
    }
    
    // Subtle top highlight (rounded top effect)
    float topBottomAlpha = 0x60 / 255.0f; // 38% opacity
        
    for (int dx = -bodyWidth/2 + 1; dx <= bodyWidth/2 - 1; ++dx) {
        int drawX = x + dx;
        blendPixel(drawX, bodyTop, highlightColor, topBottomAlpha);
    }
    
    // Subtle bottom shadow (rounded bottom effect)
    for (int dx = -bodyWidth/2 + 1; dx <= bodyWidth/2 - 1; ++dx) {
        int drawX = x + dx;
        blendPixel(drawX, bodyBottom, shadowColor, topBottomAlpha);
    }
}

void Cluster10::plotCandlestickChart(const std::vector<CandleData>& candles, 
                                   const RGBA& bullishColor, 
                                   const RGBA& bearishColor)
{
    if (candles.empty()) {
        return;
    }
    
    // Use theme colors if custom colors not provided
    RGBA useBullishColor = bullishColor;
    RGBA useBearishColor = bearishColor;
    
    // If default colors are used, replace with theme colors
    if (bullishColor.r == 0x03 && bullishColor.g == 0xC0 && bullishColor.b == 0x3C) {
        useBullishColor = elementColors["bullish"];
    }
    if (bearishColor.r == 0xFF && bearishColor.g == 0x47 && bearishColor.b == 0x45) {
        useBearishColor = elementColors["bearish"];
    }
    
    // Create a chart configuration
    ChartConfig config("Financial Data Analysis", 36, "Date", "Price");
    
    // Initialize chart with background, grid, etc.
    initializeChart(config.title, config.titleFontSize);
    
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
    
    // Draw Y-axis with price labels
    drawCandlestickYAxis(minPrice, maxPrice, 3);
    
    // Draw X-axis with date labels
    drawCandlestickXAxis(candles, maxVisibleCandles, totalCandles, firstTimestamp, lastTimestamp);
    
    // Label axes
    drawText(width/2, height - margin_bottom/3, config.xAxisLabel, elementColors["axisLabel"], config.axisFontSize, true);
    drawVerticalText(config.yAxisLabel, margin_left/3, height/2 - 55, config.axisFontSize, elementColors["axisLabel"]);
    
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
        RGBA candleColor = (candle.close > candle.open) ? useBullishColor : useBearishColor;
        
        // Draw the candlestick
        drawCandlestick(x, y_open, y_close, y_high, y_low, candleColor);
    }
    
    // Add price movement indicators and current price display
    drawCandlestickPriceInfo(candles, useBullishColor, useBearishColor);
    
    // Add a legend for bullish/bearish candles
    std::vector<std::string> legendLabels;
    legendLabels.push_back("Bullish Candle");
    legendLabels.push_back("Bearish Candle");
    
    std::vector<RGBA> legendColors;
    // Ensure colors are fully opaque for the legend dots
    RGBA bullishLegendColor = useBullishColor;
    RGBA bearishLegendColor = useBearishColor;
    bullishLegendColor.a = 0xFF; // Full opacity
    bearishLegendColor.a = 0xFF; // Full opacity
    
    legendColors.push_back(bullishLegendColor);
    legendColors.push_back(bearishLegendColor);
    
    // Position the legend in the top-right corner
    addLegend(legendLabels, legendColors, width - margin_right - 180, margin_top + 15, 16);
}

void Cluster10::drawCandlestickYAxis(double minPrice, double maxPrice, int numTicks)
{
    // Use exactly 3 ticks with consistent spacing
    numTicks = 3;
    
    // Use our common Y-axis method with 2 decimal places
    drawYAxisTicks(minPrice, maxPrice, numTicks, false, 2, 30);
}

void Cluster10::drawCandlestickXAxis(const std::vector<CandleData>& candles, int maxVisibleCandles, 
                                   int totalCandles, double firstTimestamp, double lastTimestamp)
{
    int plotWidth = getPlotWidth();
    
    // Use exactly 4 evenly spaced labels
    int dateLabelsCount = 4;
    
    for (int i = 1; i < dateLabelsCount; ++i) {
        float percentage = static_cast<float>(i) / dateLabelsCount;
        int x = margin_left + static_cast<int>(percentage * plotWidth);
        
        // Calculate timestamp for this position
        int candleIndex = static_cast<int>(percentage * (totalCandles - 1));
        candleIndex = std::min(candleIndex, totalCandles - 1);
        candleIndex = std::max(candleIndex, 0);
        
        double timestamp = candles[candleIndex].timestamp;
        
        // Draw vertical grid line at each major tick
        RGBA gridColor = elementColors["majorGrid"];
        gridColor.a = 0x70; // Semi-transparent
        drawLine(x, margin_top, x, height - margin_bottom, gridColor, 1);
        
        // Format timestamp into readable date
        char dateText[32];
        std::time_t time = static_cast<std::time_t>(timestamp);
        struct tm* timeinfo = std::localtime(&time);
        std::strftime(dateText, sizeof(dateText), "%m/%d", timeinfo);
        
        // Draw date label
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
    
    // Determine if overall trend is bullish or bearish
    bool isBullish = priceChange >= 0;
    RGBA trendColor = isBullish ? bullishColor : bearishColor;
    
    // Format price info with colored price change
    char priceInfo[128];
    std::sprintf(priceInfo, "Close: %.2f   Change: %.2f (%.2f%%)", 
               latestCandle.close, priceChange, percentChange);
    
    // Set dimensions and position
    int infoWidth = 350;
    int infoHeight = 40;
    int infoX = width - margin_right - infoWidth - 20;
    int infoY = margin_top + 20;
    
    // Draw the info box
    drawInfoBox(infoX, infoY, infoWidth, infoHeight, priceInfo, 16);
    
    // Add a small colored indicator box to show trend
    int indicatorSize = 8;
    int indicatorX = infoX + infoWidth - 30;
    int indicatorY = infoY + (infoHeight - indicatorSize) / 2;
    
    // Draw filled rectangle with the appropriate color
    drawRect(indicatorX, indicatorY, indicatorSize, indicatorSize, trendColor, true);
}

void Cluster10::plotHistogram(const std::vector<int>& bins, const RGBA& color)
{
    if (bins.empty()) {
        return;
    }
    
    // Use the first theme color if custom color not provided or if it's the default
    RGBA useColor = color;
    if (color.r == 0 && color.g == 0 && color.b == 0 && color.a == 0) {
        useColor = themeColors[0]; // Use the first theme color by default
    }
    
    // Create a chart configuration
    ChartConfig config("Data Distribution", 36, "Values", "Frequency");
    
    // Initialize chart with background, grid, etc.
    initializeChart(config.title, config.titleFontSize);
    
    // Calculate the effective plotting area
    int plotWidth = getPlotWidth();
    int plotHeight = getPlotHeight();
    
    // Find the maximum value in bins for scaling
    int maxBinValue = *std::max_element(bins.begin(), bins.end());
    if (maxBinValue == 0) maxBinValue = 1; // Avoid division by zero
    
    // Calculate optimal bar width and spacing based on the design
    int totalBars = bins.size();
    float barWidthPercentage = 0.7f; // Bar takes 70% of available space
    float spacingPercentage = 0.3f; // 30% for spacing
    
    int totalBarSpace = plotWidth / totalBars;
    int barWidth = static_cast<int>(totalBarSpace * barWidthPercentage);
    int barSpacing = static_cast<int>(totalBarSpace * spacingPercentage);
    
    // Ensure minimum size and spacing for readability
    barWidth = std::max(barWidth, 8);
    barSpacing = std::max(barSpacing, 4);
    
    // Draw Y-axis with value labels
    drawYAxisTicks(0, maxBinValue, 4, true, 0, 25);
    
    // Draw histogram bars
    drawHistogramBars(bins, maxBinValue, totalBars, barWidth, barSpacing, useColor);
    
    // Draw X-axis label
    drawText(width / 2, height - margin_bottom / 3, config.xAxisLabel, elementColors["axisLabel"], config.axisFontSize, true);
    
    // Draw Y-axis label (vertical text)
    drawVerticalText(config.yAxisLabel, margin_left / 4, height / 2 - 60, config.axisFontSize, elementColors["axisLabel"]);
    
    // Draw statistics info box
    drawHistogramStats(bins, maxBinValue);
    
    // Add a simple legend
    std::vector<std::string> legendLabels;
    legendLabels.push_back("Frequency");
    
    std::vector<RGBA> legendColors;
    legendColors.push_back(useColor);
    
    // Position the legend in the top-right corner
    addLegend(legendLabels, legendColors, width - margin_right - 150, margin_top + 15, 16);
}

void Cluster10::drawHistogramYAxis(int maxValue, int numTicks)
{
    // Use our common Y-axis method with integer values
    drawYAxisTicks(0, maxValue, numTicks, true, 0, 25);
}

void Cluster10::drawHistogramBars(const std::vector<int>& bins, int maxBinValue, int totalBars, 
                               int barWidth, int barSpacing, const RGBA& color)
{
    int plotHeight = getPlotHeight();
    
    // Calculate start X position to center the bars
    int startX = margin_left + barSpacing / 2;
    
    // Draw each bar
    for (size_t i = 0; i < bins.size(); ++i) {
        // Calculate bar height based on bin value
        float ratio = static_cast<float>(bins[i]) / maxBinValue;
        int barHeight = static_cast<int>(ratio * plotHeight);
        
        // Ensure minimum height for visibility
        barHeight = std::max(barHeight, 2);
        
        // Calculate bar position
        int x = startX + i * (barWidth + barSpacing);
        int y = height - margin_bottom - barHeight;
        
        // Draw the bar rectangle
        drawRect(x, y, barWidth, barHeight, color, true);
        
        // Add subtle 3D effect with highlights and shadows
        drawHistogramBarHighlights(x, y, barWidth, barHeight);
        
        // Add bar value label if bar is tall enough
        if (barHeight > 40) { // Only label bars with sufficient height
            char valueText[16];
            std::sprintf(valueText, "%d", bins[i]);
            drawText(x + barWidth / 2, y - 10, valueText, RGBA(0xFF, 0xFF, 0xFF, 0xFF), 14, true);
        }
    }
}

void Cluster10::drawHistogramBarHighlights(int x, int y, int barWidth, int barHeight)
{
    // Colors for highlights and shadows
    RGBA highlightColor(0xFF, 0xFF, 0xFF, 0x40); // Semi-transparent white
    RGBA shadowColor(0x00, 0x00, 0x00, 0x40);    // Semi-transparent black
    float highlightAlpha = highlightColor.a / 255.0f;
    float shadowAlpha = shadowColor.a / 255.0f;
    
    // Bar edges to highlight/shadow
    struct BarEdge {
        int startX, startY;
        int deltaX, deltaY;
        int width, height;
        RGBA color;
        float alpha;
    };
    
    // Define the four edges of the bar
    BarEdge edges[] = {
    // Left edge highlight
        {x, y, 0, 1, 2, barHeight, highlightColor, highlightAlpha},
        // Top edge highlight
        {x, y, 1, 0, barWidth, 2, highlightColor, highlightAlpha},
        // Right edge shadow
        {x + barWidth - 2, y, 0, 1, 2, barHeight, shadowColor, shadowAlpha},
        // Bottom edge shadow
        {x, y + barHeight - 2, 1, 0, barWidth, 2, shadowColor, shadowAlpha}
    };
    
    // Plot bounds
    int minX = static_cast<int>(margin_left);
    int maxX = static_cast<int>(width - margin_right);
    int minY = static_cast<int>(margin_top);
    int maxY = static_cast<int>(height - margin_bottom);
    
    // Apply effects to each edge
    for (int e = 0; e < 4; ++e) {
        const BarEdge& edge = edges[e];
        
        for (int d1 = 0; d1 < edge.width; ++d1) {
            for (int d2 = 0; d2 < edge.height; ++d2) {
                int drawX = edge.startX + d1 * edge.deltaX + d2 * (1 - edge.deltaX);
                int drawY = edge.startY + d1 * edge.deltaY + d2 * (1 - edge.deltaY);
                
                // Only draw within the plot area
                if (drawX >= minX && drawX < maxX && drawY >= minY && drawY < maxY) {
                    blendPixel(drawX, drawY, edge.color, edge.alpha);
                }
            }
        }
    }
}

// Helper method to get theme color by index with proper bounds checking
RGBA Cluster10::getThemeColor(int index) {
    if (themeColors.empty()) {
        // Return default color if theme colors are empty
        return RGBA(0xFF, 0xFF, 0xFF, 0xFF);
    }
    
    // Use modulo to wrap around if index is out of bounds
    return themeColors[index % themeColors.size()];
}

void Cluster10::drawHistogramStats(const std::vector<int>& bins, int maxBinValue)
{
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
    double mode = maxIndex; // index with highest frequency
    
    // Create stats display with properly aligned values - matching CSS styling
    char statsText[128];
    std::sprintf(statsText, "Total: %d   Mean: %.1f   Mode: %.1f   Max: %d", 
               count, mean, mode, maxBinValue);
    
    // Set dimensions and position - consistent with CSS UI conventions
    int statsWidth = 350;
    int statsHeight = 40;
    int statsX = margin_left + 20;
    int statsY = margin_top + 20;
    
    // Use our common info box method
    drawInfoBox(statsX, statsY, statsWidth, statsHeight, statsText, 16);
    
    // Add colored indicators for each statistic type - matching CSS colors
    int indicatorSize = 6;
    int indicatorSpacing = 80; // Space between indicators
    int firstIndicatorX = statsX + 65; // Position after "Total:" label
    int indicatorY = statsY + (statsHeight - indicatorSize) / 2;
    
    // Draw colored indicators
    drawRect(firstIndicatorX, indicatorY, indicatorSize, indicatorSize, getThemeColor(0), true);
    drawRect(firstIndicatorX + indicatorSpacing, indicatorY, indicatorSize, indicatorSize, getThemeColor(1), true);
    drawRect(firstIndicatorX + 2*indicatorSpacing, indicatorY, indicatorSize, indicatorSize, getThemeColor(2), true);
    drawRect(firstIndicatorX + 3*indicatorSpacing, indicatorY, indicatorSize, indicatorSize, getThemeColor(3), true);
}

void Cluster10::drawVerticalText(const std::string& text, int x, int y, int fontSize, const RGBA& color)
{
    // Set the font size
    if (FT_Set_Pixel_Sizes(face, 0, fontSize)) {
        printf("Error: Could not set pixel sizes for vertical text\n");
        return;
    }
    
    // Calculate line height for vertical spacing
    int lineHeight = fontSize + fontSize / 4; // Add some extra spacing
    
    // Start at center and go up and down to center the text
    int totalHeight = text.length() * lineHeight;
    int startY = y - totalHeight / 2;
    
    // Draw each character vertically
    for (size_t i = 0; i < text.length(); ++i) {
        std::string charStr(1, text[i]);
        int charY = startY + i * lineHeight;
        
        // Draw character centered horizontally
        drawText(x, charY, charStr, color, fontSize, true);
    }
}

Cluster10::AxisRange Cluster10::calculateXRange(const std::vector<Point>& points)
{
    AxisRange range;
    
    if (points.empty()) {
        return range; // Return default range
    }
    
    // Find min and max X values
    range.min = range.max = points[0].x;
    
    for (size_t i = 1; i < points.size(); ++i) {
        range.min = std::min(range.min, points[i].x);
        range.max = std::max(range.max, points[i].x);
    }
    
    // Add padding
    double padding = (range.max - range.min) * range.padding;
    if (padding < 1e-10) {
        padding = 1.0; // Minimum padding to avoid division by zero
    }
    
    range.min -= padding;
    range.max += padding;
    
    return range;
}

Cluster10::AxisRange Cluster10::calculateYRange(const std::vector<Point>& points)
{
    AxisRange range;
    
    if (points.empty()) {
        return range; // Return default range
    }
    
    // Find min and max Y values
    range.min = range.max = points[0].y;
    
    for (size_t i = 1; i < points.size(); ++i) {
        range.min = std::min(range.min, points[i].y);
        range.max = std::max(range.max, points[i].y);
    }
    
    // Add padding
    double padding = (range.max - range.min) * range.padding;
    if (padding < 1e-10) {
        padding = 1.0; // Minimum padding to avoid division by zero
    }
    
    range.min -= padding;
    range.max += padding;
    
    return range;
}

Cluster10::AxisRange Cluster10::calculateXRange(const std::vector<std::vector<double> >& data)
{
    AxisRange range;
    
    if (data.empty() || data[0].empty()) {
        return range; // Return default range
    }
    
    // Find min and max X values (first dimension)
    range.min = range.max = data[0][0];
    
    for (size_t i = 0; i < data.size(); ++i) {
        if (data[i].size() > 0) {
            range.min = std::min(range.min, data[i][0]);
            range.max = std::max(range.max, data[i][0]);
        }
    }
    
    // Add padding
    double padding = (range.max - range.min) * range.padding;
    if (padding < 1e-10) {
        padding = 1.0; // Minimum padding to avoid division by zero
    }
    
    range.min -= padding;
    range.max += padding;
    
    return range;
}

Cluster10::AxisRange Cluster10::calculateYRange(const std::vector<std::vector<double> >& data)
{
    AxisRange range;
    
    if (data.empty() || data[0].size() < 2) {
        return range; // Return default range
    }
    
    // Find min and max Y values (second dimension)
    range.min = range.max = data[0][1];
    
    for (size_t i = 0; i < data.size(); ++i) {
        if (data[i].size() > 1) {
            range.min = std::min(range.min, data[i][1]);
            range.max = std::max(range.max, data[i][1]);
        }
    }
    
    // Add padding
    double padding = (range.max - range.min) * range.padding;
    if (padding < 1e-10) {
        padding = 1.0; // Minimum padding to avoid division by zero
    }
    
    range.min -= padding;
    range.max += padding;
    
    return range;
}

Cluster10::Point Cluster10::mapDataToScreen(double x, double y, const AxisRange& xRange, const AxisRange& yRange)
{
    // Calculate the effective plotting area
    int plotWidth = width - margin_left - margin_right;
    int plotHeight = height - margin_top - margin_bottom;
    
    // Map X coordinate from data space to screen space
    double xRatio = (x - xRange.min) / (xRange.max - xRange.min);
    int screenX = margin_left + static_cast<int>(xRatio * plotWidth);
    
    // Map Y coordinate from data space to screen space (Y-axis is inverted in screen coordinates)
    double yRatio = (y - yRange.min) / (yRange.max - yRange.min);
    int screenY = height - margin_bottom - static_cast<int>(yRatio * plotHeight);
    
    // Ensure the point is within the plot bounds
    screenX = clamp(screenX, margin_left, width - margin_right);
    screenY = clamp(screenY, margin_top, height - margin_bottom);
    
    return Point(screenX, screenY);
}

// Helper for blending colors with alpha
RGBA Cluster10::blendColors(const RGBA& baseColor, const RGBA& overlayColor, float alpha) {
    return RGBA(
        static_cast<unsigned char>(baseColor.r * (1.0f - alpha) + overlayColor.r * alpha),
        static_cast<unsigned char>(baseColor.g * (1.0f - alpha) + overlayColor.g * alpha),
        static_cast<unsigned char>(baseColor.b * (1.0f - alpha) + overlayColor.b * alpha),
        baseColor.a  // Keep the original alpha
    );
}

// Helper for blending a pixel with bounds checking
void Cluster10::blendPixel(int x, int y, const RGBA& color, float alpha) {
    if (x >= 0 && x < static_cast<int>(width) && y >= 0 && y < static_cast<int>(height)) {
        RGBA baseColor = image.GetPixel(x, y);
        RGBA blendedColor = blendColors(baseColor, color, alpha);
        image.SetPixel(x, y, blendedColor);
    }
}

// Common method for drawing info boxes (legend, stats, price info)
void Cluster10::drawInfoBox(int x, int y, int boxWidth, int boxHeight, const std::string& text, unsigned int fontSize) {
    // Common corner radius for all info boxes - matching CSS border-radius
    int cornerRadius = 8; 
    
    // Get gradient colors from element colors - exact colors from CSS
    RGBA bgTopColor = elementColors["legendBgTop"];
    RGBA bgBottomColor = elementColors["legendBgBottom"];
    
    // Create a temporary buffer for the background
    Image tempBg;
    tempBg.Allocate(boxWidth, boxHeight);
    
    // Draw gradient background
    for (int dy = 0; dy < boxHeight; dy++) {
        // Calculate gradient interpolation
        float ratio = static_cast<float>(dy) / boxHeight;
        RGBA currentBgColor(
            static_cast<unsigned char>(bgTopColor.r * (1.0f - ratio) + bgBottomColor.r * ratio),
            static_cast<unsigned char>(bgTopColor.g * (1.0f - ratio) + bgBottomColor.g * ratio),
            static_cast<unsigned char>(bgTopColor.b * (1.0f - ratio) + bgBottomColor.b * ratio),
            0xFF
        );
        
        for (int dx = 0; dx < boxWidth; dx++) {
            // Check if this pixel is in the rounded corner region
            bool inCorner = false;
            
            // Check top-left corner
            if (dx < cornerRadius && dy < cornerRadius) {
                int distSquared = (cornerRadius - dx) * (cornerRadius - dx) + (cornerRadius - dy) * (cornerRadius - dy);
                inCorner = distSquared > cornerRadius * cornerRadius;
            }
            // Check top-right corner
            else if (dx >= boxWidth - cornerRadius && dy < cornerRadius) {
                int distSquared = (dx - (boxWidth - cornerRadius)) * (dx - (boxWidth - cornerRadius)) + (cornerRadius - dy) * (cornerRadius - dy);
                inCorner = distSquared > cornerRadius * cornerRadius;
            }
            // Check bottom-left corner
            else if (dx < cornerRadius && dy >= boxHeight - cornerRadius) {
                int distSquared = (cornerRadius - dx) * (cornerRadius - dx) + (dy - (boxHeight - cornerRadius)) * (dy - (boxHeight - cornerRadius));
                inCorner = distSquared > cornerRadius * cornerRadius;
            }
            // Check bottom-right corner
            else if (dx >= boxWidth - cornerRadius && dy >= boxHeight - cornerRadius) {
                int distSquared = (dx - (boxWidth - cornerRadius)) * (dx - (boxWidth - cornerRadius)) + (dy - (boxHeight - cornerRadius)) * (dy - (boxHeight - cornerRadius));
                inCorner = distSquared > cornerRadius * cornerRadius;
            }
            
            if (!inCorner) {
                tempBg.SetPixel(dx, dy, currentBgColor);
            }
        }
    }
    
    // Apply the box with transparency (95% opacity for better visibility)
    // CSS often uses rgba with 0.95 opacity for overlays
    float boxAlpha = 0.95f;
    for (int dy = 0; dy < boxHeight; dy++) {
        for (int dx = 0; dx < boxWidth; dx++) {
            int drawX = x + dx;
            int drawY = y + dy;
            
            if (drawX >= 0 && drawX < static_cast<int>(width) &&
                drawY >= 0 && drawY < static_cast<int>(height)) {
                
                // Only blend if pixel exists in temp buffer (handles rounded corners)
                if (tempBg.GetPixel(dx, dy).a != 0) {
                    RGBA bgColor = tempBg.GetPixel(dx, dy);
                    RGBA destColor = image.GetPixel(drawX, drawY);
                    RGBA blendedColor = blendColors(destColor, bgColor, boxAlpha);
                    image.SetPixel(drawX, drawY, blendedColor);
                }
            }
        }
    }
    
    // Add subtle inner highlight to top edge - exact 10% opacity from CSS
    RGBA highlightColor(0xFF, 0xFF, 0xFF, 0x1A); // 10% white (0.1 * 255 = 26 ≈ 0x1A)
    for (int dx = cornerRadius; dx < boxWidth - cornerRadius; dx++) {
        int pixelX = x + dx;
        int pixelY = y;
        blendPixel(pixelX, pixelY, highlightColor, highlightColor.a / 255.0f);
    }
    
    // Add subtle drop shadow to bottom edge - exact 15% opacity from CSS
    RGBA shadowColor(0x00, 0x00, 0x00, 0x26); // 15% black (0.15 * 255 = 38 ≈ 0x26)
    for (int dx = cornerRadius; dx < boxWidth - cornerRadius; dx++) {
        int pixelX = x + dx;
        int pixelY = y + boxHeight - 1;
        blendPixel(pixelX, pixelY, shadowColor, shadowColor.a / 255.0f);
    }
    
    // Draw the text if provided
    if (!text.empty()) {
        drawText(x + 15, y + boxHeight/2, text, elementColors["legend"], fontSize, false);
    }
}

// Common method for drawing Y-axis ticks and labels
void Cluster10::drawYAxisTicks(double minValue, double maxValue, int numTicks, bool isInteger, 
                             int precision, int labelOffset) {
    int plotHeight = getPlotHeight();
    
    // Draw each tick mark and label
    for (int i = 0; i <= numTicks; ++i) {
        float percentage = static_cast<float>(i) / numTicks;
        int y = height - margin_bottom - static_cast<int>(percentage * plotHeight);
        double value = minValue + percentage * (maxValue - minValue);
        
        // Draw horizontal grid line
        RGBA gridColor = elementColors["majorGrid"];
        gridColor.a = 0x70; // Semi-transparent
        drawLine(margin_left, y, width - margin_right, y, gridColor);
        
        // Format the value based on type
        char valueText[32];
        if (isInteger) {
            std::sprintf(valueText, "%d", static_cast<int>(value));
        } else {
            char formatStr[10];
            std::sprintf(formatStr, "%%.%df", precision);
            std::sprintf(valueText, formatStr, value);
        }
        
        // Draw value label
        drawText(margin_left - labelOffset, y, valueText, elementColors["axisLabel"], 16, true);
    }
}

// Draw X-axis ticks with text labels
void Cluster10::drawXAxisTicks(const std::vector<std::string>& labels, int numTicks) {
    int plotWidth = getPlotWidth();
    
    int totalLabels = static_cast<int>(labels.size());
    int tickInterval = std::max(1, totalLabels / numTicks);
    
    for (int i = 0; i < totalLabels; i += tickInterval) {
        float percentage = static_cast<float>(i) / totalLabels;
        int x = margin_left + static_cast<int>(percentage * plotWidth);
        
        // Draw vertical grid line
        RGBA gridColor = elementColors["majorGrid"];
        gridColor.a = 0x70; // Semi-transparent
        drawLine(x, margin_top, x, height - margin_bottom, gridColor, 1);
        
        // Draw label
        drawText(x, height - margin_bottom + 20, labels[i], elementColors["axisLabel"], 14, true);
    }
}

// Draw X-axis ticks with numeric values
void Cluster10::drawXAxisTicks(double minValue, double maxValue, int numTicks, int precision) {
    int plotWidth = getPlotWidth();
    
    for (int i = 0; i < numTicks; ++i) {
        float percentage = static_cast<float>(i) / (numTicks - 1);
        int x = margin_left + static_cast<int>(percentage * plotWidth);
        double value = minValue + percentage * (maxValue - minValue);
        
        // Draw vertical grid line
        RGBA gridColor = elementColors["majorGrid"];
        gridColor.a = 0x70; // Semi-transparent
        drawLine(x, margin_top, x, height - margin_bottom, gridColor, 1);
        
        // Format the value
        char valueText[32];
        char formatStr[10];
        std::sprintf(formatStr, "%%.%df", precision);
        std::sprintf(valueText, formatStr, value);
        
        // Draw value label
        drawText(x, height - margin_bottom + 20, valueText, elementColors["axisLabel"], 14, true);
    }
}

// Helper for common chart initialization steps
void Cluster10::initializeChart(const std::string& title, unsigned int titleFontSize) {
    // Draw the complete background first to ensure proper layering
    image.drawVerticalGradient(
        0, 0, 
        elementColors["bgGradientTop"], 
        elementColors["bgGradientBottom"], 
        cornerRadius
    );
    
    // Add title if provided
    if (!title.empty()) {
        addTitle(title, titleFontSize);
    }
    
    // Draw grid if enabled
    if (showGrid) {
        drawGrid();
    }
    
    // Draw axes if enabled
    if (showAxes) {
        drawAxes();
    }
}

// End of Cluster10.cpp
// End of Cluster10.cpp