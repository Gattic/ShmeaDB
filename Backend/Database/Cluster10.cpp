// Cluster10.cpp
#include "Cluster10.h"
#include "png-helper.h"
#include <algorithm>
#include <limits>
#include <string>  // for std::to_string
#include <cmath>   // for M_PI

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
    
    // Initialize yAxisLabel to empty
    yAxisLabel = "";
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
    elementColors["bgGradientTop"] = RGBA(0x02, 0x13, 0x31, 0xFF);    // #021331 from histogram CSS
    elementColors["bgGradientBottom"] = RGBA(0x00, 0x0B, 0x1E, 0xFF); // #000B1E darker tone for gradient
    
    // Grid and axes colors from CSS 
    elementColors["majorGrid"] = RGBA(0x19, 0x23, 0x35, 0x80);        // --chart-lines with transparency
    elementColors["minorGrid"] = RGBA(0x19, 0x23, 0x35, 0x40);        // Lighter grid lines
    elementColors["axes"] = RGBA(0xFF, 0xFF, 0xFF, 0xCC);             // White axes with slight transparency
    elementColors["border"] = RGBA(0xCD, 0xD5, 0xE5, 0x26);           // Border with transparency
    
    // Text colors from CSS
    elementColors["title"] = RGBA(0xFF, 0xFF, 0xFF, 0xFF);            // Pure white titles
    elementColors["axisLabel"] = RGBA(0xCD, 0xD5, 0xE5, 0xCC);        // Light gray for labels
    elementColors["legend"] = RGBA(0xFF, 0xFF, 0xFF, 0xEE);           // Nearly white for legend text
    
    // Info box colors from CSS
    elementColors["legendBgTop"] = RGBA(0x23, 0x0B, 0x6A, 0xF0);      // Purple gradient top
    elementColors["legendBgBottom"] = RGBA(0x15, 0x21, 0x56, 0xF0);   // Blue gradient bottom
    
    // Theme colors from CSS semantic colors - EXACT matches from histogram_with_labels.css
    themeColors.clear(); // Clear any existing colors
    themeColors.push_back(RGBA(0x5C, 0xE9, 0xFF, 0xFF));              // #5CE9FF - Bright cyan
    themeColors.push_back(RGBA(0x5C, 0xFF, 0xB3, 0xFF));              // #5CFFB3 - Bright aquamarine
    themeColors.push_back(RGBA(0xBA, 0xB1, 0xFF, 0xFF));              // #BAB1FF - Light purple
    themeColors.push_back(RGBA(0xD6, 0xFF, 0xC7, 0xFF));              // #D6FFC7 - Light green
    themeColors.push_back(RGBA(0x51, 0xE3, 0xAD, 0xFF));              // #51E3AD - Medium aquamarine
    themeColors.push_back(RGBA(0x42, 0xCD, 0xFF, 0xFF));              // #42CDFF - Medium blue
    themeColors.push_back(RGBA(0x8C, 0xFF, 0xF9, 0xFF));              // #8CFFF9 - Light cyan
    themeColors.push_back(RGBA(0x8B, 0xF4, 0xB8, 0xFF));              // #8BF4B8 - Light green
    
    // Special chart colors with more vibrance
    elementColors["bullish"] = RGBA(0x33, 0xF5, 0x9B, 0xFF);          // Bright green for bullish candles
    elementColors["bearish"] = RGBA(0xFF, 0x5C, 0x74, 0xFF);          // Bright pink/red for bearish candles
    
    // Dark versions of colors for contrast and accents
    elementColors["cluster1Dark"] = RGBA(0x0A, 0x71, 0x43, 0xFF);     // Dark green
    elementColors["cluster2Dark"] = RGBA(0x11, 0x5E, 0x79, 0xFF);     // Dark cyan
    elementColors["cluster3Dark"] = RGBA(0x47, 0x18, 0xBF, 0xFF);     // Deep purple
    elementColors["cluster4Dark"] = RGBA(0x17, 0x69, 0x0B, 0xFF);     // Dark green
    
    // Additional accent colors
    elementColors["highlight"] = RGBA(0xFF, 0xFF, 0xFF, 0x80);        // White highlight 50% opacity
    elementColors["shadow"] = RGBA(0x00, 0x00, 0x00, 0x80);           // Black shadow 50% opacity
    elementColors["innerShadowBg"] = RGBA(0x19, 0x23, 0x35, 0x03);    // Very slight inner shadow
    
    // Colors for cluster shadows from CSS files
    elementColors["cluster1Shadow"] = RGBA(0x5C, 0xFF, 0xB3, 0x80);   // Aquamarine shadow
    elementColors["cluster2Shadow"] = RGBA(0x5C, 0xE9, 0xFF, 0x80);   // Cyan shadow
    elementColors["cluster3Shadow"] = RGBA(0xBA, 0xB1, 0xFF, 0x80);   // Purple shadow
    elementColors["cluster4Shadow"] = RGBA(0xD6, 0xFF, 0xC7, 0x80);   // Green shadow
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
    // Draw the dark gradient background from CSS styles
    // Using radial gradient similar to histogram_with_labels.css: 
    // radial-gradient(164.63% 83.5% at 54.69% 50%, #021331 0%, #000B1E 100%)
    
    // Define the gradient center point (at 54.69% 50% as specified in CSS)
    float centerX = width * 0.5469f;
    float centerY = height * 0.5f;
    
    // Define the gradient radius (164.63% width and 83.5% height elliptical gradient)
    float radiusX = width * 1.6463f;
    float radiusY = height * 0.835f;
    
    // Get colors from our pre-defined palette
    RGBA centerColor = elementColors["bgGradientTop"];    // #021331
    RGBA edgeColor = elementColors["bgGradientBottom"];   // #000B1E
    
    // Render the radial gradient
    for (unsigned int y = 0; y < height; ++y) {
        for (unsigned int x = 0; x < width; ++x) {
            // Calculate distance from center (normalize based on elliptical radiuses)
            float dx = (x - centerX) / radiusX;
            float dy = (y - centerY) / radiusY;
            
            // Calculate normalized distance (0.0 to 1.0)
            float dist = std::sqrt(dx*dx + dy*dy);
            dist = std::min(1.0f, dist); // Clamp to maximum 1.0
            
            // Use cubic easing for more dramatic gradient falloff (matching CSS)
            dist = dist * dist * dist; // Cubic falloff
            
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
    
    // Add subtle vignette effect at the top (like in histogram_with_labels.css)
    // CSS: linear-gradient(180deg, #021331 0%, rgba(2, 19, 49, 0) 100%)
    int vignetteFadeHeight = height * 0.2f; // Top 20% has vignette
    
    for (unsigned int y = 0; y < vignetteFadeHeight; ++y) {
        // Calculate fade factor (1.0 at top, 0.0 at bottom of fade)
        float fadeFactor = 1.0f - (static_cast<float>(y) / vignetteFadeHeight);
        fadeFactor = fadeFactor * fadeFactor * 0.5f; // Square it and adjust intensity
        
        for (unsigned int x = 0; x < width; ++x) {
            // Get current pixel and darken it slightly
            RGBA currentColor = image.GetPixel(x, y);
            RGBA fadeColor(
                static_cast<unsigned char>(currentColor.r * (1.0f - fadeFactor)),
                static_cast<unsigned char>(currentColor.g * (1.0f - fadeFactor)),
                static_cast<unsigned char>(currentColor.b * (1.0f - fadeFactor)),
                0xFF
            );
            
            // Set the darkened pixel
            image.SetPixel(x, y, fadeColor);
        }
    }
    
    // Draw grid if enabled
    if (showGrid) {
        drawGrid();
    }
}

void Cluster10::drawGrid()
{
    // Calculate the effective plotting area considering the margins
    int effectiveWidth = width - margin_left - margin_right;
    int effectiveHeight = height - margin_top - margin_bottom;
    
    // Use exact grid pattern from CSS files in concepts/
    // Matching the fig and pdf files' grid styling
    int gridDivisionsX = 8;  // 8 vertical gridlines for better spacing
    int gridDivisionsY = 5;  // 5 horizontal gridlines like in the design
    
    // Get colors for the grid lines with exact opacity from CSS
    RGBA gridColor = elementColors["majorGrid"];
    gridColor.a = 0x66; // Increased to 40% opacity for better visibility
    
    // Draw X-axis grid lines (vertical lines)
    for (int i = 1; i < gridDivisionsX; i++) {
        float percentage = static_cast<float>(i) / gridDivisionsX;
        int x = margin_left + static_cast<int>(percentage * effectiveWidth);
        
        // Draw grid line with exact 1px width - ensure it's visible
        for (int y = margin_top; y <= height - margin_bottom; y++) {
            if (x >= 0 && x < width && y >= 0 && y < height) {
                RGBA currentPixel = image.GetPixel(x, y);
                RGBA blendedColor = blendRGBA(currentPixel, gridColor);
                image.SetPixel(x, y, blendedColor);
            }
        }
    }
    
    // Draw Y-axis grid lines (horizontal lines)
    for (int i = 1; i < gridDivisionsY; i++) {
        float percentage = static_cast<float>(i) / gridDivisionsY;
        int y = height - margin_bottom - static_cast<int>(percentage * effectiveHeight);
        
        // Draw grid line with exact 1px width - ensure it's visible
        for (int x = margin_left; x <= width - margin_right; x++) {
            if (x >= 0 && x < width && y >= 0 && y < height) {
                RGBA currentPixel = image.GetPixel(x, y);
                RGBA blendedColor = blendRGBA(currentPixel, gridColor);
                image.SetPixel(x, y, blendedColor);
            }
        }
    }
    
    // Draw outer border with exact styling from CSS
    RGBA borderColor = elementColors["border"];
    borderColor.a = 0x66; // Increased to 40% opacity to match grid lines
    int borderWidth = 1;  // 1px border as in design
    
    // Draw border with proper rounded corners
    // Top border
    drawLine(margin_left + cornerRadius, margin_top, 
             width - margin_right - cornerRadius, margin_top, 
             borderColor, borderWidth);
             
    // Bottom border
    drawLine(margin_left + cornerRadius, height - margin_bottom, 
             width - margin_right - cornerRadius, height - margin_bottom, 
             borderColor, borderWidth);
             
    // Left border
    drawLine(margin_left, margin_top + cornerRadius, 
             margin_left, height - margin_bottom - cornerRadius, 
             borderColor, borderWidth);
             
    // Right border
    drawLine(width - margin_right, margin_top + cornerRadius, 
             width - margin_right, height - margin_bottom - cornerRadius, 
             borderColor, borderWidth);
    
    // Add subtle rounded corners for the border - exactly matching CSS
    drawRoundedCorners(margin_left, margin_top, width - margin_right, height - margin_bottom, 
                      cornerRadius, borderColor);
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
                    
                    // For semi-transparent colors, blend with background
                    if (lineColor.a < 255) {
                        RGBA currentPixel = image.GetPixel(drawX, drawY);
                        RGBA blendedColor = blendRGBA(currentPixel, lineColor);
                        image.SetPixel(drawX, drawY, blendedColor);
                    } else {
                        // Fully opaque - just set the pixel
                    image.SetPixel(drawX, drawY, lineColor);
                    }
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
    // Get text color from CSS design
    RGBA textColor = elementColors["axisLabel"];
    textColor.a = 0xCC; // 80% opacity for readability
    
    // Set and draw Y-axis label using proper rotation
    setYAxisLabel(yLabel);
    
    // Draw X-axis label centered below the X-axis
    int xLabelX = margin_left + getPlotWidth() / 2;
    int xLabelY = height - margin_bottom / 2;
    drawText(xLabelX, xLabelY, xLabel, textColor, fontSize, true);
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
    // Draw any additional elements that should be rendered last
    
    // Draw axis ticks (placeholder example)
    if (showAxes) {
        // Draw Y-axis label if it's not empty
        if (!yAxisLabel.empty()) {
            // Get text color from CSS design
            RGBA textColor = elementColors["axisLabel"];
            textColor.a = 0xCC; // 80% opacity for readability
            
            // Position for the Y-axis label - adjusted for rotated text
            int labelX = margin_left / 3; // Positioned closer to the left edge
            int labelY = margin_top + getPlotHeight() / 2; // Center vertically
            
            // Draw rotated label
            drawVerticalText(yAxisLabel, labelX, labelY, 16, textColor);
        }
    }
    
    // Make sure the directory exists
    std::string filenameWithPath = folder + "/" + filename;
    
    // Save PNG file - SavePNG is a void method, not a bool
    image.SavePNG(filenameWithPath.c_str());
    printf("Saved chart as: %s\n", filenameWithPath.c_str());
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
    // Draw each cluster circle with the exact styling from CSS
    for (size_t cluster = 0; cluster < clusterCenters.size(); ++cluster) {
        if (clusterRadii[cluster] == 0) continue; // Skip empty clusters
        
        // Get the cluster color
        RGBA circleColor = clusterColors[cluster % clusterColors.size()];
        
        // Get the appropriate shadow color exactly as in CSS
        RGBA shadowColor;
        if (cluster < elementColors.size()) {
        char shadowKeyBuffer[32];
        std::sprintf(shadowKeyBuffer, "cluster%dShadow", (int)(cluster+1));
        std::string shadowKey(shadowKeyBuffer);
        
            // Use the specific cluster shadow color if defined
        if (elementColors.find(shadowKey) != elementColors.end()) {
            shadowColor = elementColors[shadowKey];
        } else {
                // Otherwise use the semi-transparent cluster color
            shadowColor = RGBA(
                circleColor.r,
                circleColor.g,
                circleColor.b,
                0x80 // 50% opacity
                );
            }
        } else {
            // Fallback semi-transparent shadow
            shadowColor = RGBA(
                circleColor.r,
                circleColor.g,
                circleColor.b,
                0x80
            );
        }
        
        int radius = clusterRadii[cluster];
        Point center = clusterCenters[cluster];
        
        // Draw filled circle with subtle gradient effect - exactly as in CSS
        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                // Calculate exact distance from center
                float dist = std::sqrt(dx*dx + dy*dy);
                
                // Skip pixels outside the circle
                if (dist > radius) continue;
                
                // Calculate the normalized distance (0 at center, 1 at edge)
                float normDist = dist / radius;
                
                int drawX = center.x + dx;
                int drawY = center.y + dy;
                
                // Skip pixels outside plot area
                if (drawX < static_cast<int>(margin_left) || 
                    drawX >= static_cast<int>(width - margin_right) ||
                    drawY < static_cast<int>(margin_top) || 
                    drawY >= static_cast<int>(height - margin_bottom)) {
                    continue;
                }
                
                // Get existing pixel color (the background gradient)
                RGBA existingColor = image.GetPixel(drawX, drawY);
                
                // Prepare the fill color with appropriate opacity - matching CSS
                RGBA fillColor = RGBA(
                        circleColor.r,
                        circleColor.g,
                        circleColor.b,
                    38  // 15% opacity, exactly as in CSS
                );
                
                // Prepare border color - smaller border (thinner than 1px)
                RGBA borderColor;
                if (normDist > 0.985f) { // Increased from 0.97f for thinner border
                    borderColor = RGBA(
                        circleColor.r, 
                        circleColor.g, 
                        circleColor.b, 
                        184  // 72% opacity (reduced from 80%)
                    );
                    
                    // Blend with existing background
                    RGBA resultColor = blendRGBA(existingColor, borderColor);
                    image.SetPixel(drawX, drawY, resultColor);
                    continue;
                }
                
                // Blend fill color with existing background
                RGBA resultColor = blendRGBA(existingColor, fillColor);
                
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
                        resultColor = blendRGBA(resultColor, innerShadowColor);
                }
                
                // Set the final pixel
                image.SetPixel(drawX, drawY, resultColor);
            }
        }
    }
}

// Helper function for proper alpha blending that preserves the background
RGBA Cluster10::blendRGBA(const RGBA& base, const RGBA& over) {
    // If the overlay is fully transparent, return the base unchanged
    if (over.a == 0) return base;
    
    // If the overlay is fully opaque, return it directly
    if (over.a == 255) return over;
    
    // Calculate alpha values for blending
    float alphaOver = over.a / 255.0f;
    float alphaBase = base.a / 255.0f;
    float alphaOut = alphaOver + alphaBase * (1.0f - alphaOver);
    
    // If the resulting alpha is zero, return transparent black
    if (alphaOut < 0.001f) return RGBA(0, 0, 0, 0);
    
    // Blend the colors properly considering the alpha channels
    unsigned char r = static_cast<unsigned char>((over.r * alphaOver + base.r * alphaBase * (1.0f - alphaOver)) / alphaOut);
    unsigned char g = static_cast<unsigned char>((over.g * alphaOver + base.g * alphaBase * (1.0f - alphaOver)) / alphaOut);
    unsigned char b = static_cast<unsigned char>((over.b * alphaOver + base.b * alphaBase * (1.0f - alphaOver)) / alphaOut);
    unsigned char a = static_cast<unsigned char>(alphaOut * 255.0f);
    
    return RGBA(r, g, b, a);
}

// Helper method to draw centroids
void Cluster10::drawCentroids(
    const std::vector<std::vector<double> >& centroids,
    const std::vector<RGBA>& clusterColors,
    const AxisRange& xRange,
    const AxisRange& yRange)
{
    // Draw centroids with styling exactly matching the CSS
    for (size_t i = 0; i < centroids.size() && i < clusterColors.size(); ++i) {
        if (centroids[i].size() < 2) continue;
        
        // Map centroid to screen coordinates
        Point centroidPoint = mapDataToScreen(centroids[i][0], centroids[i][1], xRange, yRange);
        
        // CSS uses solid white circle with drop shadow and colored cross
        
        // First draw shadow - 3px offset and 40% opacity (exactly as in CSS)
        RGBA shadowColor(0x00, 0x00, 0x00, 0x66); // 40% opacity black shadow
        int shadowOffset = 3; // 3px offset as in CSS
        
        // Draw larger soft shadow first (matches CSS box-shadow effect)
        for (int dy = -12; dy <= 12; dy++) {
            for (int dx = -12; dx <= 12; dx++) {
                int distSqr = dx*dx + dy*dy;
                if (distSqr > 144) continue; // Only pixels within 12px radius
                
                // Calculate shadow intensity - fade out toward edges (Gaussian-like)
                float shadowDistance = std::sqrt(distSqr);
                float shadowIntensity = 0.4f * std::exp(-shadowDistance / 6.0f);
                
                int drawX = centroidPoint.x + dx + shadowOffset;
                int drawY = centroidPoint.y + dy + shadowOffset;
                
                if (drawX >= 0 && drawX < static_cast<int>(width) &&
                    drawY >= 0 && drawY < static_cast<int>(height)) {
                    RGBA pixelShadow = shadowColor;
                    pixelShadow.a = static_cast<unsigned char>(shadowColor.a * shadowIntensity);
                    blendPixel(drawX, drawY, pixelShadow, shadowIntensity);
                }
            }
        }
        
        // Draw the white circle on top of the shadow - exactly as in CSS design
        RGBA whiteFill(0xFF, 0xFF, 0xFF, 0xFF); // Solid white
        int circleRadius = 10; // 10px radius as in CSS
        
        // Draw solid white circle
        for (int dy = -circleRadius; dy <= circleRadius; dy++) {
            for (int dx = -circleRadius; dx <= circleRadius; dx++) {
                int distSqr = dx*dx + dy*dy;
                if (distSqr > circleRadius * circleRadius) continue;
                
                int drawX = centroidPoint.x + dx;
                int drawY = centroidPoint.y + dy;
                
                if (drawX >= 0 && drawX < static_cast<int>(width) &&
                    drawY >= 0 && drawY < static_cast<int>(height)) {
                    image.SetPixel(drawX, drawY, whiteFill);
                }
            }
        }
        
        // Get the appropriate cross color - matching CSS exactly
        RGBA crossColor;
        char darkColorKeyBuffer[32];
        std::sprintf(darkColorKeyBuffer, "cluster%dDark", (int)(i+1));
        std::string darkColorKey(darkColorKeyBuffer);
        
        if (i < 4 && elementColors.find(darkColorKey) != elementColors.end()) {
            // Use the exact dark color version from CSS
            crossColor = elementColors[darkColorKey];
        } else {
            // For other clusters, create a darkened version
            crossColor = RGBA(
                static_cast<unsigned char>(clusterColors[i].r * 0.6f),
                static_cast<unsigned char>(clusterColors[i].g * 0.6f),
                static_cast<unsigned char>(clusterColors[i].b * 0.6f),
                0xFF
            );
        }
        
        // Draw horizontal line of cross - 3px thickness as in CSS
            for (int y = -1; y <= 1; ++y) {
            drawLine(centroidPoint.x - 8, centroidPoint.y + y, 
                     centroidPoint.x + 8, centroidPoint.y + y, crossColor);
            }
            
        // Draw vertical line of cross - 3px thickness as in CSS
            for (int x = -1; x <= 1; ++x) {
            drawLine(centroidPoint.x + x, centroidPoint.y - 8, 
                     centroidPoint.x + x, centroidPoint.y + 8, crossColor);
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
    // Enhanced candlestick styling to match CSS design exactly
    
    // Define the width of the candlestick body - matching CSS
    int bodyWidth = 18;  // Width of candlestick body in pixels 
    int wickThickness = 3;  // Thickness of wick line
    
    // Draw the wick (line from high to low) with proper styling
    for (int i = -wickThickness/2; i <= wickThickness/2; ++i) {
        // Use semi-transparent color for the wick to match design
        RGBA wickColor = color;
        wickColor.a = 0xE6; // 90% opacity
        drawLine(x + i, y_high, x + i, y_low, wickColor);
    }
    
    // Determine the top and bottom of the body
    int bodyTop = std::min(y_open, y_close);
    int bodyBottom = std::max(y_open, y_close);
    
    // Ensure minimum body height for better visibility - matching CSS
    if (bodyBottom - bodyTop < 4) {
        bodyBottom = bodyTop + 4;
    }
    
    // Draw the body (rectangle between open and close) with gradient
    for (int dy = bodyTop; dy <= bodyBottom; ++dy) {
        // Calculate relative position in the body (0 at top, 1 at bottom)
        float relativePos = (dy - bodyTop) / static_cast<float>(bodyBottom - bodyTop);
        
        // Create a parabolic gradient factor for a more pronounced center highlight
        // This creates the "bulging" effect seen in the CSS design
        float gradientFactor = 1.0f - 4.0f * (relativePos - 0.5f) * (relativePos - 0.5f);
        gradientFactor = 0.85f + gradientFactor * 0.15f; // Scale to 0.85-1.0 range
        
        // Create gradient color with the bulging effect
        RGBA gradientColor(
            static_cast<unsigned char>(std::min(255.0f, color.r * gradientFactor)),
            static_cast<unsigned char>(std::min(255.0f, color.g * gradientFactor)),
            static_cast<unsigned char>(std::min(255.0f, color.b * gradientFactor)),
            0xFF // Fully opaque
        );
        
        for (int dx = -bodyWidth/2; dx <= bodyWidth/2; ++dx) {
            // Set the pixel directly with full opacity
            int drawX = x + dx;
            int drawY = dy;
            
            if (drawX >= margin_left && drawX < static_cast<int>(width - margin_right) && 
                drawY >= margin_top && drawY < static_cast<int>(height - margin_bottom)) {
                image.SetPixel(drawX, drawY, gradientColor);
            }
        }
    }
    
    // Add sophisticated 3D effects with highlights and shadows - matching CSS
    
    // Left edge highlight with gradient fade - brighter near edge
    RGBA highlightColor(0xFF, 0xFF, 0xFF, 0x99);  // 60% opacity white
    int edgeWidth = 5; // Highlight width (slightly wider than before)
    
    for (int dy = bodyTop; dy <= bodyBottom; ++dy) {
        for (int dx = 0; dx < edgeWidth; ++dx) {
            // Calculate fade strength - more pronounced edge highlight
            float alpha = (edgeWidth - dx) / static_cast<float>(edgeWidth) * 0.6f;
            int drawX = x - bodyWidth/2 + dx;
            int drawY = dy;
            
            if (drawX >= margin_left && drawX < static_cast<int>(width - margin_right) && 
                drawY >= margin_top && drawY < static_cast<int>(height - margin_bottom)) {
                RGBA pixelHighlight = highlightColor;
                pixelHighlight.a = static_cast<unsigned char>(255 * alpha);
                blendPixel(drawX, drawY, pixelHighlight, alpha);
            }
        }
    }
    
    // Right edge shadow with gradient fade - darker near edge
    RGBA shadowColor(0x00, 0x00, 0x00, 0x99);  // 60% opacity black
    
    for (int dy = bodyTop; dy <= bodyBottom; ++dy) {
        for (int dx = 0; dx < edgeWidth; ++dx) {
            // Calculate fade strength - more pronounced edge shadow
            float alpha = (edgeWidth - dx) / static_cast<float>(edgeWidth) * 0.6f;
            int drawX = x + bodyWidth/2 - dx - 1;
            int drawY = dy;
            
            if (drawX >= margin_left && drawX < static_cast<int>(width - margin_right) && 
                drawY >= margin_top && drawY < static_cast<int>(height - margin_bottom)) {
                RGBA pixelShadow = shadowColor;
                pixelShadow.a = static_cast<unsigned char>(255 * alpha);
                blendPixel(drawX, drawY, pixelShadow, alpha);
            }
        }
    }
    
    // Add rounded corners as in CSS
    int cornerRadius = 3; // Slightly larger corner radius
    
    // Top rounded edge with highlight
    for (int dx = -bodyWidth/2 + cornerRadius; dx <= bodyWidth/2 - cornerRadius; ++dx) {
        int drawX = x + dx;
        int drawY = bodyTop;
        
        if (drawX >= margin_left && drawX < static_cast<int>(width - margin_right) && 
            drawY >= margin_top && drawY < static_cast<int>(height - margin_bottom)) {
            blendPixel(drawX, drawY, highlightColor, 0.4f);
        }
    }
    
    // Bottom rounded edge with shadow
    for (int dx = -bodyWidth/2 + cornerRadius; dx <= bodyWidth/2 - cornerRadius; ++dx) {
        int drawX = x + dx;
        int drawY = bodyBottom;
        
        if (drawX >= margin_left && drawX < static_cast<int>(width - margin_right) && 
            drawY >= margin_top && drawY < static_cast<int>(height - margin_bottom)) {
            blendPixel(drawX, drawY, shadowColor, 0.4f);
        }
    }
    
    // Top-left and top-right corner pixels for proper CSS-like rounding
    for (int i = 0; i < cornerRadius; ++i) {
        for (int j = 0; j < cornerRadius; ++j) {
            float distance = std::sqrt(i*i + j*j);
            if (distance <= cornerRadius) {
                // Top-left corner
                int drawX = x - bodyWidth/2 + i;
                int drawY = bodyTop + j;
                if (drawX >= margin_left && drawX < static_cast<int>(width - margin_right) && 
                    drawY >= margin_top && drawY < static_cast<int>(height - margin_bottom)) {
                    blendPixel(drawX, drawY, highlightColor, 0.5f);
                }
                
                // Top-right corner
                drawX = x + bodyWidth/2 - i;
                if (drawX >= margin_left && drawX < static_cast<int>(width - margin_right) && 
                    drawY >= margin_top && drawY < static_cast<int>(height - margin_bottom)) {
                    blendPixel(drawX, drawY, highlightColor, 0.3f);
                }
            }
        }
    }
    
    // Bottom-left and bottom-right corner pixels for proper CSS-like rounding
    for (int i = 0; i < cornerRadius; ++i) {
        for (int j = 0; j < cornerRadius; ++j) {
            float distance = std::sqrt(i*i + j*j);
            if (distance <= cornerRadius) {
                // Bottom-left corner
                int drawX = x - bodyWidth/2 + i;
                int drawY = bodyBottom - j;
                if (drawX >= margin_left && drawX < static_cast<int>(width - margin_right) && 
                    drawY >= margin_top && drawY < static_cast<int>(height - margin_bottom)) {
                    blendPixel(drawX, drawY, shadowColor, 0.3f);
                }
                
                // Bottom-right corner
                drawX = x + bodyWidth/2 - i;
                if (drawX >= margin_left && drawX < static_cast<int>(width - margin_right) && 
                    drawY >= margin_top && drawY < static_cast<int>(height - margin_bottom)) {
                    blendPixel(drawX, drawY, shadowColor, 0.5f);
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
    
    // Create stats display with properly aligned text and layout from CSS
    // Position the stats box in the top-right corner with proper margins
    int boxWidth = 200;
    int boxHeight = 120;
    int boxX = width - margin_right - boxWidth - 20;
    int boxY = margin_top + 60;
    
    // CSS uses custom gradient background for info boxes:
    // background: linear-gradient(225deg, #230B6A 0%, #152156 100%);
    // with box-shadow: 0px 8px 24px rgba(0, 11, 30, 0.4);
    
    // Draw box background first - custom gradient exactly as in CSS
    for (int dy = 0; dy < boxHeight; ++dy) {
        for (int dx = 0; dx < boxWidth; ++dx) {
            int drawX = boxX + dx;
            int drawY = boxY + dy;
            
            // Calculate gradient position (0 to 1) - diagonal gradient like CSS
            float gradPos = (dx + dy) / static_cast<float>(boxWidth + boxHeight);
            
            // Create gradient color - blending between two CSS colors
            RGBA gradColor;
            gradColor.r = static_cast<unsigned char>(elementColors["legendBgTop"].r * (1.0f - gradPos) + elementColors["legendBgBottom"].r * gradPos);
            gradColor.g = static_cast<unsigned char>(elementColors["legendBgTop"].g * (1.0f - gradPos) + elementColors["legendBgBottom"].g * gradPos);
            gradColor.b = static_cast<unsigned char>(elementColors["legendBgTop"].b * (1.0f - gradPos) + elementColors["legendBgBottom"].b * gradPos);
            gradColor.a = 0xE6; // 90% opacity (0.9 * 255 = 230 ≈ 0xE6)
            
            // Check if we're inside the rounded corners
            int cornerRadius = 8; // 8px corner radius from CSS
            bool isInCorner = false;
            
            // Top-left corner
            if (dx < cornerRadius && dy < cornerRadius) {
                float dist = std::sqrt((cornerRadius - dx) * (cornerRadius - dx) + (cornerRadius - dy) * (cornerRadius - dy));
                if (dist > cornerRadius) isInCorner = true;
            }
            // Top-right corner
            else if (dx >= boxWidth - cornerRadius && dy < cornerRadius) {
                float dist = std::sqrt((dx - (boxWidth - cornerRadius)) * (dx - (boxWidth - cornerRadius)) + 
                                       (cornerRadius - dy) * (cornerRadius - dy));
                if (dist > cornerRadius) isInCorner = true;
            }
            // Bottom-left corner
            else if (dx < cornerRadius && dy >= boxHeight - cornerRadius) {
                float dist = std::sqrt((cornerRadius - dx) * (cornerRadius - dx) + 
                                       (dy - (boxHeight - cornerRadius)) * (dy - (boxHeight - cornerRadius)));
                if (dist > cornerRadius) isInCorner = true;
            }
            // Bottom-right corner
            else if (dx >= boxWidth - cornerRadius && dy >= boxHeight - cornerRadius) {
                float dist = std::sqrt((dx - (boxWidth - cornerRadius)) * (dx - (boxWidth - cornerRadius)) + 
                                       (dy - (boxHeight - cornerRadius)) * (dy - (boxHeight - cornerRadius)));
                if (dist > cornerRadius) isInCorner = true;
            }
            
            if (!isInCorner && drawX >= 0 && drawX < width && drawY >= 0 && drawY < height) {
                // Use blendPixel instead of SetPixel for proper alpha blending
                blendPixel(drawX, drawY, gradColor, gradColor.a / 255.0f);
            }
        }
    }
    
    // Add drop shadow effect - like CSS box-shadow
    // box-shadow: 0px 8px 24px rgba(0, 11, 30, 0.4);
    for (int dy = 0; dy < 30; ++dy) {
        for (int dx = -20; dx < boxWidth + 20; ++dx) {
            int drawX = boxX + dx;
            int drawY = boxY + boxHeight + dy;
            
            // Skip pixels outside image
            if (drawX < 0 || drawX >= width || drawY < 0 || drawY >= height) continue;
            
            // Calculate shadow intensity
            float shadowFactor = 1.0f - (dy / 30.0f);
            float shadowAlpha = shadowFactor * 0.2f; // 20% max opacity
            
            // Apply shadow effect
            RGBA shadowColor(0, 11, 30, static_cast<unsigned char>(255 * shadowAlpha));
            blendPixel(drawX, drawY, shadowColor, shadowAlpha);
        }
    }
    
    // Draw the text with styling from CSS
    RGBA textColor = elementColors["legend"]; // White text
    
    // Main title
    drawText(boxX + 16, boxY + 20, "Histogram Statistics", textColor, 16, false);
    
    // Horizontal separator
    RGBA lineColor(0xFF, 0xFF, 0xFF, 0x40); // 25% opacity white
    drawLine(boxX + 16, boxY + 32, boxX + boxWidth - 16, boxY + 32, lineColor);
    
    // Format stats values with 1 decimal place and consistent width
    char meanText[64], modeText[64], maxText[64], sumText[64];
    std::sprintf(meanText, "Mean: %.1f", mean);
    std::sprintf(modeText, "Mode: %.1f", mode);
    std::sprintf(maxText, "Max: %d", maxBinValue);
    std::sprintf(sumText, "Sum: %d", sum);
    
    // Draw stats with exact positioning and font sizes from CSS
    drawText(boxX + 16, boxY + 55, meanText, textColor, 14, false);
    drawText(boxX + 16, boxY + 75, modeText, textColor, 14, false);
    drawText(boxX + 16, boxY + 95, maxText, textColor, 14, false);
    
    // Sum on the right - just like in CSS
    int sumWidth = 0; // Placeholder since we don't have text width calculation
    drawText(boxX + boxWidth - 16 - sumWidth, boxY + 95, sumText, textColor, 14, false);
}

void Cluster10::drawVerticalText(const std::string& text, int x, int y, int fontSize, const RGBA& color)
{
    // Set the font size
    if (FT_Set_Pixel_Sizes(face, 0, fontSize)) {
        printf("Error: Could not set pixel sizes for vertical text\n");
        return;
    }
    
    // For properly rotated text, we first need to measure the total width
    int textWidth = 0;
    
    // Calculate the total width of the text
    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            continue;
        }
        
        FT_GlyphSlot glyph = face->glyph;
        textWidth += (glyph->advance.x >> 6);
    }
    
    // We'll rotate the text 270 degrees (or -90 degrees) so the bottom faces left (toward the graph)
    // This means the characters will be drawn upside down compared to the previous rotation
    
    // Start position (center of rotation)
    int drawX = x;
    int drawY = y + textWidth/2; // Center vertically based on text width
    
    // Draw each character rotated
    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            printf("Warning: Could not load character %c\n", c);
            continue;
        }
        
        FT_GlyphSlot glyph = face->glyph;
        
        unsigned int glyphWidth = glyph->bitmap.width;
        unsigned int glyphHeight = glyph->bitmap.rows;
        
        // Calculate the rotated position for the glyph
        // For 270° rotation (or -90°):
        // x' = -y and y' = x
        for (unsigned int row = 0; row < glyphHeight; ++row) {
            for (unsigned int col = 0; col < glyphWidth; ++col) {
                // Get pixel value from glyph bitmap
                unsigned char value = glyph->bitmap.buffer[row * glyphWidth + col];
                
                if (value > 0) { // Only draw if the glyph pixel is not empty
                    // Apply the rotation transformation
                    // For 270° rotation with bottom facing left:
                    int rotatedX = drawX - glyph->bitmap_top + row;
                    int rotatedY = drawY - col - glyph->bitmap_left;
                    
                    if (rotatedX >= 0 && rotatedX < static_cast<int>(width) &&
                        rotatedY >= 0 && rotatedY < static_cast<int>(height)) {
                        // Calculate alpha-blended color
                        float alpha = value / 255.0f;
                        RGBA blendedColor;
                        blendedColor.r = static_cast<unsigned char>(color.r * alpha);
                        blendedColor.g = static_cast<unsigned char>(color.g * alpha);
                        blendedColor.b = static_cast<unsigned char>(color.b * alpha);
                        blendedColor.a = static_cast<unsigned char>(color.a * alpha);
                        
                        image.SetPixel(rotatedX, rotatedY, blendedColor);
                    }
                }
            }
        }
        
        // Move "down" for next character (which is actually moving left in rotated space)
        drawY -= (glyph->advance.x >> 6);
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
    
    // Draw gradient background with proper alpha blending
    for (int dy = 0; dy < boxHeight; dy++) {
        // Calculate gradient interpolation - diagonal gradient to match the histogram stats box
        for (int dx = 0; dx < boxWidth; dx++) {
            float gradPos = (dx + dy) / static_cast<float>(boxWidth + boxHeight);
            RGBA currentBgColor(
                static_cast<unsigned char>(bgTopColor.r * (1.0f - gradPos) + bgBottomColor.r * gradPos),
                static_cast<unsigned char>(bgTopColor.g * (1.0f - gradPos) + bgBottomColor.g * gradPos),
                static_cast<unsigned char>(bgTopColor.b * (1.0f - gradPos) + bgBottomColor.b * gradPos),
                0xE6  // 90% opacity like the histogram stats box
            );
            
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
            
            int drawX = x + dx;
            int drawY = y + dy;
            
            if (!inCorner && drawX >= 0 && drawX < static_cast<int>(width) &&
                drawY >= 0 && drawY < static_cast<int>(height)) {
                // Use blendPixel for proper alpha blending
                blendPixel(drawX, drawY, currentBgColor, currentBgColor.a / 255.0f);
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
    
    // Use the same color as grid lines for consistency with CSS design
    RGBA gridColor = elementColors["majorGrid"];
    gridColor.a = 0x66; // Increased to 40% opacity to match grid lines
    
    // Get text color from CSS design
    RGBA textColor = elementColors["axisLabel"];
    textColor.a = 0xCC; // 80% opacity for better readability
    
    // Draw each tick mark and label
    for (int i = 0; i <= numTicks; ++i) {
        float percentage = static_cast<float>(i) / numTicks;
        int y = height - margin_bottom - static_cast<int>(percentage * plotHeight);
        double value = minValue + percentage * (maxValue - minValue);
        
        // Draw horizontal tick line with consistent styling
        if (i > 0) { // Skip duplicate line at bottom
            // Draw short tick mark with fine-tuned transparency
            drawLine(margin_left - 6, y, margin_left, y, textColor, 1);
            
            // Grid lines are drawn in drawGrid() to ensure visual consistency
        }
        
        // Format the value based on type with CSS-matching precision
        char valueText[32];
        if (isInteger) {
            std::sprintf(valueText, "%d", static_cast<int>(value));
        } else {
            char formatStr[10];
            std::sprintf(formatStr, "%%.%df", precision);
            std::sprintf(valueText, formatStr, value);
        }
        
        // Draw value label with proper spacing from CSS
        drawText(margin_left - labelOffset, y, valueText, textColor, 16, true);
    }
}

// Draw X-axis ticks with text labels
void Cluster10::drawXAxisTicks(const std::vector<std::string>& labels, int numTicks) {
    int plotWidth = getPlotWidth();
    
    // Use the same colors as other grid elements for CSS consistency
    RGBA gridColor = elementColors["majorGrid"];
    gridColor.a = 0x66; // Increased to 40% opacity to match grid lines
    
    // Get text color from CSS design
    RGBA textColor = elementColors["axisLabel"];
    textColor.a = 0xCC; // 80% opacity for readability
    
    int totalLabels = static_cast<int>(labels.size());
    int tickInterval = std::max(1, totalLabels / numTicks);
    
    for (int i = 0; i < totalLabels; i += tickInterval) {
        float percentage = static_cast<float>(i) / totalLabels;
        int x = margin_left + static_cast<int>(percentage * plotWidth);
        
        // Draw tick mark with consistent styling (grid lines drawn in drawGrid)
        drawLine(x, height - margin_bottom, x, height - margin_bottom + 6, textColor, 1);
        
        // Draw label with proper spacing and alignment from CSS
        int labelY = height - margin_bottom + 25; // More spacing as in CSS
        drawText(x, labelY, labels[i], textColor, 14, true);
    }
}

// Draw X-axis ticks with numeric values
void Cluster10::drawXAxisTicks(double minValue, double maxValue, int numTicks, int precision) {
    int plotWidth = getPlotWidth();
    
    // Use the same colors as other grid elements for CSS consistency
    RGBA gridColor = elementColors["majorGrid"];
    gridColor.a = 0x66; // Increased to 40% opacity to match grid lines
    
    // Get text color from CSS design
    RGBA textColor = elementColors["axisLabel"];
    textColor.a = 0xCC; // 80% opacity for readability
    
    for (int i = 0; i < numTicks; ++i) {
        float percentage = static_cast<float>(i) / (numTicks - 1);
        int x = margin_left + static_cast<int>(percentage * plotWidth);
        double value = minValue + percentage * (maxValue - minValue);
        
        // Draw tick mark with consistent styling (grid lines are drawn in drawGrid)
        drawLine(x, height - margin_bottom, x, height - margin_bottom + 6, textColor, 1);
        
        // Format the value with consistent precision from CSS
        char valueText[32];
        char formatStr[10];
        std::sprintf(formatStr, "%%.%df", precision);
        std::sprintf(valueText, formatStr, value);
        
        // Draw value label with proper spacing and alignment from CSS
        int labelY = height - margin_bottom + 25; // More spacing as in CSS
        drawText(x, labelY, valueText, textColor, 14, true);
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

void Cluster10::plotHistogram(const std::vector<int>& bins, const RGBA& color)
{
    if (bins.empty()) {
        return;
    }
    
    // Use themeColors[0] if custom color not provided
    RGBA useColor = color;
    if (color.r == 0 && color.g == 0 && color.b == 0 && color.a == 0) {
        useColor = themeColors[0]; // #5CE9FF - Bright cyan
    }
    
    // Create a chart configuration with histogram styling
    ChartConfig config("Data Distribution", 42, "Value", "Frequency");
    
    // Initialize chart with background, grid, etc.
    initializeChart(config.title, config.titleFontSize);
    
    // Calculate the effective plotting area
    int plotWidth = getPlotWidth();
    int plotHeight = getPlotHeight();
    
    // Find the maximum value in bins for scaling
    int maxBinValue = *std::max_element(bins.begin(), bins.end());
    if (maxBinValue == 0) maxBinValue = 1; // Avoid division by zero
    
    // Calculate optimal bar width and spacing based on the CSS design
    // Bars are narrower with more spacing than default
    int totalBars = bins.size();
    float barWidthPercentage = 0.6f; // Bar takes 60% of available space
    float spacingPercentage = 0.4f; // 40% for spacing
    
    int totalBarSpace = plotWidth / totalBars;
    int barWidth = static_cast<int>(totalBarSpace * barWidthPercentage);
    int barSpacing = static_cast<int>(totalBarSpace * spacingPercentage);
    
    // Ensure minimum size and spacing for readability
    barWidth = std::max(barWidth, 40); // Match the 40px width from CSS 
    barSpacing = std::max(barSpacing, 30); // Generous spacing as in CSS
    
    // Draw Y-axis with value labels - more ticks for better scale
    drawYAxisTicks(0, maxBinValue, 5, true, 0, 25);
    
    // Draw histogram bars
    drawHistogramBars(bins, maxBinValue, totalBars, barWidth, barSpacing, useColor);
    
    // Draw X-axis label with proper styling
    drawText(width / 2, height - margin_bottom / 3, config.xAxisLabel, elementColors["axisLabel"], config.axisFontSize, true);
    
    // Draw Y-axis label (vertical text) with proper styling
    drawVerticalText(config.yAxisLabel, margin_left / 4, height / 2 - 60, config.axisFontSize, elementColors["axisLabel"]);
    
    // Draw statistics info box with matching CSS styling
    drawHistogramStats(bins, maxBinValue);
    
    // Add a legend with updated styling
    std::vector<std::string> legendLabels;
    legendLabels.push_back("Frequency");
    
    std::vector<RGBA> legendColors;
    legendColors.push_back(useColor);
    
    // Position the legend in the top-right corner
    addLegend(legendLabels, legendColors, width - margin_right - 180, margin_top + 20, 18);
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

void Cluster10::drawHistogramYAxis(int maxValue, int numTicks)
{
    // Use our common Y-axis method with integer values
    drawYAxisTicks(0, maxValue, numTicks, true, 0, 25);
}

void Cluster10::drawHistogramBars(const std::vector<int>& bins, int maxBinValue, int totalBars, 
                               int barWidth, int barSpacing, const RGBA& color)
{
    int plotHeight = getPlotHeight();
    
    // Calculate start X position to center the bars - match histogram_with_labels.css
    int startX = margin_left + (getPlotWidth() - (totalBars * (barWidth + barSpacing) - barSpacing)) / 2;
    
    // Use the exact color from histogram_with_labels.css - bright cyan
    RGBA barColor = themeColors[0]; // #5CE9FF - Bright cyan from CSS
    if (color.r != 0 || color.g != 0 || color.b != 0) {
        barColor = color; // Use provided color if specified
    }
    
    // Draw each bar with styling from CSS
    for (size_t i = 0; i < bins.size(); ++i) {
        // Calculate bar height based on bin value with minimum height
        float ratio = static_cast<float>(bins[i]) / maxBinValue;
        int barHeight = static_cast<int>(ratio * plotHeight);
        
        // Ensure minimum height for visibility (exactly like histogram_with_labels.css)
        barHeight = std::max(barHeight, 8);
        
        // Calculate bar position
        int x = startX + i * (barWidth + barSpacing);
        int y = height - margin_bottom - barHeight;
        
        // Draw the bar with a vertical gradient - matching CSS
        for (int dy = 0; dy < barHeight; ++dy) {
            // Calculate gradient position (0 at top, 1 at bottom)
            float gradientPos = static_cast<float>(dy) / barHeight;
            
            // Adjust gradient to match the CSS: brighter at top, slightly darker at bottom
            // Using the non-linear gradient like in histogram_with_labels.css
            float gradientFactor = 1.0f - gradientPos * 0.3f; // Top 30% brighter
            
            // Create gradient color
            RGBA gradientColor(
                static_cast<unsigned char>(std::min(255.0f, barColor.r * gradientFactor)),
                static_cast<unsigned char>(std::min(255.0f, barColor.g * gradientFactor)),
                static_cast<unsigned char>(std::min(255.0f, barColor.b * gradientFactor)),
                barColor.a
            );
            
            // Draw this row of the bar
            for (int dx = 0; dx < barWidth; ++dx) {
                int drawX = x + dx;
                int drawY = y + dy;
                
                if (drawX >= margin_left && drawX < width - margin_right &&
                    drawY >= margin_top && drawY < height - margin_bottom) {
                    image.SetPixel(drawX, drawY, gradientColor);
                }
            }
        }
        
        // Add 3D effect with highlights and shadows (exactly like CSS)
        drawHistogramBarHighlights(x, y, barWidth, barHeight);
        
        // Add bar value label on top of taller bars - similar to histogram_with_labels.css
        if (barHeight > 45) { // Only for sufficiently tall bars
            char valueText[16];
            std::sprintf(valueText, "%d", bins[i]);
            
            // Position above the bar with proper spacing
            drawText(x + barWidth / 2, y - 20, valueText, elementColors["legend"], 16, true);
        }
        
        // Add bottom label for bars - exactly like histogram_with_labels.css
        // Only add labels for some bars to prevent crowding
        if (i % 4 == 0 || i == bins.size() - 1) {
            char labelText[16];
            std::sprintf(labelText, "%zu", i);
            RGBA labelColor = elementColors["axisLabel"];
            
            // Exact positioning from CSS
            drawText(x + barWidth / 2, height - margin_bottom + 20, labelText, labelColor, 14, true);
        }
    }
}

void Cluster10::drawHistogramBarHighlights(int x, int y, int barWidth, int barHeight)
{
    // Enhanced styling for histogram bars to match CSS
    
    // Colors for highlights and shadows - more pronounced
    RGBA highlightColor(0xFF, 0xFF, 0xFF, 0x60); // Semi-transparent white (38% opacity)
    RGBA shadowColor(0x00, 0x00, 0x00, 0x60);    // Semi-transparent black (38% opacity)
    
    // Calculate highlight/shadow widths proportional to bar width
    int edgeWidth = std::max(3, barWidth / 10);
    
    // Left edge highlight with gradient fade - brighter near edge
    for (int dy = 0; dy < barHeight; ++dy) {
        for (int dx = 0; dx < edgeWidth; ++dx) {
            // Calculate fade strength - more intense at edge, fades toward center
            float alpha = (edgeWidth - dx) / static_cast<float>(edgeWidth) * 0.6f;
            int drawX = x + dx;
            int drawY = y + dy;
            
            if (drawX >= margin_left && drawX < width - margin_right &&
                drawY >= margin_top && drawY < height - margin_bottom) {
                RGBA pixelHighlight = highlightColor;
                pixelHighlight.a = static_cast<unsigned char>(255 * alpha);
                blendPixel(drawX, drawY, pixelHighlight, alpha);
            }
        }
    }
    
    // Right edge shadow with gradient fade - darker near edge
    for (int dy = 0; dy < barHeight; ++dy) {
        for (int dx = 0; dx < edgeWidth; ++dx) {
            // Calculate fade strength - more intense at edge, fades toward center
            float alpha = (edgeWidth - dx) / static_cast<float>(edgeWidth) * 0.6f;
            int drawX = x + barWidth - dx - 1;
            int drawY = y + dy;
            
            if (drawX >= margin_left && drawX < width - margin_right &&
                drawY >= margin_top && drawY < height - margin_bottom) {
                RGBA pixelShadow = shadowColor;
                pixelShadow.a = static_cast<unsigned char>(255 * alpha);
                blendPixel(drawX, drawY, pixelShadow, alpha);
            }
        }
    }
    
    // Top edge highlight for 3D effect - subtle rounded top
    float topHighlightAlpha = 0.5f;
    int topEdgeHeight = std::max(2, barHeight / 20);
    
    for (int dx = edgeWidth; dx < barWidth - edgeWidth; ++dx) {
        for (int dy = 0; dy < topEdgeHeight; ++dy) {
            // Fade from top edge downward
            float alpha = (topEdgeHeight - dy) / static_cast<float>(topEdgeHeight) * topHighlightAlpha;
            int drawX = x + dx;
            int drawY = y + dy;
            
            if (drawX >= margin_left && drawX < width - margin_right &&
                drawY >= margin_top && drawY < height - margin_bottom) {
                RGBA pixelHighlight = highlightColor;
                pixelHighlight.a = static_cast<unsigned char>(255 * alpha);
                blendPixel(drawX, drawY, pixelHighlight, alpha);
            }
        }
    }
    
    // Bottom edge shadow for 3D effect - subtle rounded bottom
    float bottomShadowAlpha = 0.5f;
    int bottomEdgeHeight = std::max(2, barHeight / 20);
    
    for (int dx = edgeWidth; dx < barWidth - edgeWidth; ++dx) {
        for (int dy = 0; dy < bottomEdgeHeight; ++dy) {
            // Fade from bottom edge upward
            float alpha = (bottomEdgeHeight - dy) / static_cast<float>(bottomEdgeHeight) * bottomShadowAlpha;
            int drawX = x + dx;
            int drawY = y + barHeight - dy - 1;
            
            if (drawX >= margin_left && drawX < width - margin_right &&
                drawY >= margin_top && drawY < height - margin_bottom) {
                RGBA pixelShadow = shadowColor;
                pixelShadow.a = static_cast<unsigned char>(255 * alpha);
                blendPixel(drawX, drawY, pixelShadow, alpha);
            }
        }
    }
    
    // Add subtle highlight to corners for rounded appearance
    // Top-left corner extra highlight
    for (int i = 0; i < edgeWidth; ++i) {
        for (int j = 0; j < topEdgeHeight; ++j) {
            float alphaX = (edgeWidth - i) / static_cast<float>(edgeWidth);
            float alphaY = (topEdgeHeight - j) / static_cast<float>(topEdgeHeight);
            float alpha = alphaX * alphaY * 0.7f; // Stronger corner highlight
            
            int drawX = x + i;
            int drawY = y + j;
            
            if (drawX >= margin_left && drawX < width - margin_right &&
                drawY >= margin_top && drawY < height - margin_bottom) {
                RGBA pixelHighlight = highlightColor;
                pixelHighlight.a = static_cast<unsigned char>(255 * alpha);
                blendPixel(drawX, drawY, pixelHighlight, alpha);
            }
        }
    }
    
    // Bottom-right corner extra shadow
    for (int i = 0; i < edgeWidth; ++i) {
        for (int j = 0; j < bottomEdgeHeight; ++j) {
            float alphaX = (edgeWidth - i) / static_cast<float>(edgeWidth);
            float alphaY = (bottomEdgeHeight - j) / static_cast<float>(bottomEdgeHeight);
            float alpha = alphaX * alphaY * 0.7f; // Stronger corner shadow
            
            int drawX = x + barWidth - i - 1;
            int drawY = y + barHeight - j - 1;
            
            if (drawX >= margin_left && drawX < width - margin_right &&
                drawY >= margin_top && drawY < height - margin_bottom) {
                RGBA pixelShadow = shadowColor;
                pixelShadow.a = static_cast<unsigned char>(255 * alpha);
                blendPixel(drawX, drawY, pixelShadow, alpha);
            }
        }
    }
}

// Helper method to draw rounded corners for the grid border with anti-aliasing
void Cluster10::drawRoundedCorners(int left, int top, int right, int bottom, int radius, const RGBA& color)
{
    // Generate arc points for a quarter circle with high precision
    std::vector<std::pair<float, float> > arcPoints;
    
    // Use higher precision for smoother corners - generate at 0.01 radian steps
    for (float angle = 0.0f; angle <= M_PI/2; angle += 0.01f) {
        float x = radius * std::cos(angle);
        float y = radius * std::sin(angle);
        arcPoints.push_back(std::make_pair(x, y));
    }
    
    // Pre-compute the full alpha value to ensure consistent transparency
    float fullAlpha = color.a / 255.0f;
    
    // Draw top-left corner arc with anti-aliasing
    for (size_t i = 0; i < arcPoints.size(); i++) {
        float fx = left + radius - arcPoints[i].first;
        float fy = top + radius - arcPoints[i].second;
        
        // Draw with anti-aliasing - blend pixels at the borders
        int ix = static_cast<int>(fx);
        int iy = static_cast<int>(fy);
        
        // Only if within plot area
        if (ix >= 0 && ix < static_cast<int>(width) && iy >= 0 && iy < static_cast<int>(height)) {
            // Use consistent alpha with original color alpha channel
            RGBA pixelColor = color;
            blendPixel(ix, iy, pixelColor, fullAlpha);
        }
    }
    
    // Draw top-right corner arc with anti-aliasing
    for (size_t i = 0; i < arcPoints.size(); i++) {
        float fx = right - radius + arcPoints[i].first;
        float fy = top + radius - arcPoints[i].second;
        
        int ix = static_cast<int>(fx);
        int iy = static_cast<int>(fy);
        
        if (ix >= 0 && ix < static_cast<int>(width) && iy >= 0 && iy < static_cast<int>(height)) {
            RGBA pixelColor = color;
            blendPixel(ix, iy, pixelColor, fullAlpha);
        }
    }
    
    // Draw bottom-left corner arc with anti-aliasing
    for (size_t i = 0; i < arcPoints.size(); i++) {
        float fx = left + radius - arcPoints[i].first;
        float fy = bottom - radius + arcPoints[i].second;
        
        int ix = static_cast<int>(fx);
        int iy = static_cast<int>(fy);
        
        if (ix >= 0 && ix < static_cast<int>(width) && iy >= 0 && iy < static_cast<int>(height)) {
            RGBA pixelColor = color;
            blendPixel(ix, iy, pixelColor, fullAlpha);
        }
    }
    
    // Draw bottom-right corner arc with anti-aliasing
    for (size_t i = 0; i < arcPoints.size(); i++) {
        float fx = right - radius + arcPoints[i].first;
        float fy = bottom - radius + arcPoints[i].second;
        
        int ix = static_cast<int>(fx);
        int iy = static_cast<int>(fy);
        
        if (ix >= 0 && ix < static_cast<int>(width) && iy >= 0 && iy < static_cast<int>(height)) {
            RGBA pixelColor = color;
            blendPixel(ix, iy, pixelColor, fullAlpha);
        }
    }
}

void Cluster10::setYAxisLabel(const std::string& label)
{
    yAxisLabel = label;
    
    // Get text color from CSS design
    RGBA textColor = elementColors["axisLabel"];
    textColor.a = 0xCC; // 80% opacity for readability
    
    // Position for the Y-axis label - adjusted for rotated text
    int labelX = margin_left / 3; // Positioned closer to the left edge
    int labelY = margin_top + getPlotHeight() / 2; // Center vertically
    
    // Draw rotated label
    drawVerticalText(yAxisLabel, labelX, labelY, 16, textColor);
}

// End of Cluster10.cpp
// End of Cluster10.cpp