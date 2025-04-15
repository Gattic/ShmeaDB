#include "plotter.h"
#include "../Database/png-helper.h"
#include <algorithm>
#include <vector>
#include <limits>
#include <string>  // for std::to_string
#include <cmath>   // for M_PI

using namespace shmea;

Plotter::Plotter(unsigned int width, unsigned int height, 
                    unsigned int margin_top, unsigned int margin_right, 
                    unsigned int margin_bottom, unsigned int margin_left,
                    unsigned int ssaa_factor)
    : width(width),
      height(height),
      margin_top(margin_top),
      margin_right(margin_right),
      margin_bottom(margin_bottom),
      margin_left(margin_left),
      ssaaFactor(ssaa_factor),
      showGrid(true),
      showAxes(true),
      cornerRadius(12),
      hasLogo(false)
{
    // Calculate supersampled dimensions
    ssaaWidth = width * ssaaFactor;
    ssaaHeight = height * ssaaFactor;
    
    // Initialize the visualization
    initialize();
}

// Helper method for constructor initialization
void Plotter::initialize()
{
    // Allocate the supersampled image first
    initializeSuperSampling();
    
    // Initialize final output image
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

// Initialize supersampling buffer
void Plotter::initializeSuperSampling()
{
    // Allocate larger buffer for supersampled rendering
    ssaaImage.Allocate(ssaaWidth, ssaaHeight);
    
    // Fill with black to start
    for (unsigned int y = 0; y < ssaaHeight; ++y) {
        for (unsigned int x = 0; x < ssaaWidth; ++x) {
            ssaaImage.SetPixel(x, y, RGBA(0, 0, 0, 0xFF));
        }
    }
}

// Downsample the supersampled image to the final output resolution
void Plotter::downsampleToOutput()
{
    // For each pixel in the output image
    for (unsigned int y = 0; y < height; ++y) {
        for (unsigned int x = 0; x < width; ++x) {
            // Accumulate color values from the supersampled region
            unsigned int r = 0, g = 0, b = 0, a = 0;
            
            // Sample the NxN region from the supersampled image
            for (unsigned int sy = 0; sy < ssaaFactor; ++sy) {
                for (unsigned int sx = 0; sx < ssaaFactor; ++sx) {
                    unsigned int ssaaX = x * ssaaFactor + sx;
                    unsigned int ssaaY = y * ssaaFactor + sy;
                    
                    // Ensure we're within bounds of the supersampled image
                    if (ssaaX < ssaaWidth && ssaaY < ssaaHeight) {
                    RGBA pixel = ssaaImage.GetPixel(ssaaX, ssaaY);
                    
                        // Pre-multiply alpha for more accurate blending
                        float alphaFactor = pixel.a / 255.0f;
                        r += static_cast<unsigned int>(pixel.r * alphaFactor);
                        g += static_cast<unsigned int>(pixel.g * alphaFactor);
                        b += static_cast<unsigned int>(pixel.b * alphaFactor);
                    a += pixel.a;
                    }
                }
            }
            
            // Calculate average color
            unsigned int totalSamples = ssaaFactor * ssaaFactor;
            
            // Handle case where alpha is zero to avoid division by zero
            if (a == 0) {
                image.SetPixel(x, y, RGBA(0, 0, 0, 0));
                continue;
            }
            
            // Normalize alpha channel
            float avgAlpha = a / static_cast<float>(totalSamples);
            
            // For better accuracy with very transparent areas, ensure we don't divide by zero
            float avgAlphaFactor = avgAlpha / 255.0f;
            if (avgAlphaFactor < 0.001f) {
                avgAlphaFactor = 0.001f;
            }
            
            // Unpremultiply alpha
            unsigned char finalR = static_cast<unsigned char>(std::min(255.0f, r / static_cast<float>(totalSamples) / avgAlphaFactor));
            unsigned char finalG = static_cast<unsigned char>(std::min(255.0f, g / static_cast<float>(totalSamples) / avgAlphaFactor));
            unsigned char finalB = static_cast<unsigned char>(std::min(255.0f, b / static_cast<float>(totalSamples) / avgAlphaFactor));
            unsigned char finalA = static_cast<unsigned char>(avgAlpha);
            
            // Set the downsampled pixel in the output image
            image.SetPixel(x, y, RGBA(finalR, finalG, finalB, finalA));
        }
    }
}

void Plotter::setSuperSamplingFactor(unsigned int factor)
{
    if (factor < 1) factor = 1; // Ensure factor is at least 1
    
    // Only reinitialize if the factor has changed
    if (factor != ssaaFactor) {
        ssaaFactor = factor;
        ssaaWidth = width * ssaaFactor;
        ssaaHeight = height * ssaaFactor;
        
        // Reinitialize supersampling buffer
        initializeSuperSampling();
        
        // Redraw the background
        drawBackground();
    }
}

Plotter::~Plotter()
{
    // Clean up FreeType resources
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

void Plotter::initialize_colors()
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

void Plotter::initialize_font(const std::string fontPath)
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

void Plotter::drawBackground()
{
    // Draw the dark gradient background from CSS styles
    // Using radial gradient similar to histogram_with_labels.css: 
    // radial-gradient(164.63% 83.5% at 54.69% 50%, #021331 0%, #000B1E 100%)
    
    // Define the gradient center point (at 54.69% 50% as specified in CSS)
    float centerX = ssaaWidth * 0.5469f;
    float centerY = ssaaHeight * 0.5f;
    
    // Define the gradient radius (164.63% width and 83.5% height elliptical gradient)
    float radiusX = ssaaWidth * 1.6463f;
    float radiusY = ssaaHeight * 0.835f;
    
    // Get colors from our pre-defined palette
    RGBA centerColor = elementColors["bgGradientTop"];    // #021331
    RGBA edgeColor = elementColors["bgGradientBottom"];   // #000B1E
    
    // Render the radial gradient
    for (unsigned int y = 0; y < ssaaHeight; ++y) {
        for (unsigned int x = 0; x < ssaaWidth; ++x) {
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
            
            // Set the pixel in the supersampled image
            ssaaImage.SetPixel(x, y, pixelColor);
        }
    }
    
    // Add subtle vignette effect at the top (like in histogram_with_labels.css)
    // CSS: linear-gradient(180deg, #021331 0%, rgba(2, 19, 49, 0) 100%)
    int vignetteFadeHeight = ssaaHeight * 0.2f; // Top 20% has vignette
    
    for (unsigned int y = 0; y < vignetteFadeHeight; ++y) {
        // Calculate fade factor (1.0 at top, 0.0 at bottom of fade)
        float fadeFactor = 1.0f - (static_cast<float>(y) / vignetteFadeHeight);
        fadeFactor = fadeFactor * fadeFactor * 0.5f; // Square it and adjust intensity
        
        for (unsigned int x = 0; x < ssaaWidth; ++x) {
            // Get current pixel and darken it slightly
            RGBA currentColor = ssaaImage.GetPixel(x, y);
            RGBA fadeColor(
                static_cast<unsigned char>(currentColor.r * (1.0f - fadeFactor)),
                static_cast<unsigned char>(currentColor.g * (1.0f - fadeFactor)),
                static_cast<unsigned char>(currentColor.b * (1.0f - fadeFactor)),
                0xFF
            );
            
            // Set the darkened pixel in the supersampled image
            ssaaImage.SetPixel(x, y, fadeColor);
        }
    }
    
    // Draw grid if enabled
    if (showGrid) {
        drawGrid();
    }
}

void Plotter::drawGrid()
{
    // Calculate the effective plotting area considering the margins
    int effectiveWidth = ssaaWidth - scaleX(margin_left) - scaleX(margin_right);
    int effectiveHeight = ssaaHeight - scaleY(margin_top) - scaleY(margin_bottom);
    
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
        int x = scaleX(margin_left) + static_cast<int>(percentage * effectiveWidth);
        
        // Draw grid line with exact 1px width (scaled for supersampling)
        int lineWidth = ssaaFactor; // Scale line width with supersampling
        for (int y = scaleY(margin_top); y <= ssaaHeight - scaleY(margin_bottom); y++) {
            for (int dx = 0; dx < lineWidth; dx++) {
                int drawX = x + dx;
                if (drawX >= 0 && drawX < ssaaWidth && y >= 0 && y < ssaaHeight) {
                    RGBA currentPixel = ssaaImage.GetPixel(drawX, y);
                    RGBA blendedColor = blendRGBA(currentPixel, gridColor);
                    ssaaImage.SetPixel(drawX, y, blendedColor);
                }
            }
        }
    }
    
    // Draw Y-axis grid lines (horizontal lines)
    for (int i = 1; i < gridDivisionsY; i++) {
        float percentage = static_cast<float>(i) / gridDivisionsY;
        int y = ssaaHeight - scaleY(margin_bottom) - static_cast<int>(percentage * effectiveHeight);
        
        // Draw grid line with exact 1px width (scaled for supersampling)
        int lineWidth = ssaaFactor; // Scale line width with supersampling
        for (int x = scaleX(margin_left); x <= ssaaWidth - scaleX(margin_right); x++) {
            for (int dy = 0; dy < lineWidth; dy++) {
                int drawY = y + dy;
                if (x >= 0 && x < ssaaWidth && drawY >= 0 && drawY < ssaaHeight) {
                    RGBA currentPixel = ssaaImage.GetPixel(x, drawY);
                    RGBA blendedColor = blendRGBA(currentPixel, gridColor);
                    ssaaImage.SetPixel(x, drawY, blendedColor);
                }
            }
        }
    }
    
    // Draw outer border with exact styling from CSS
    RGBA borderColor = elementColors["border"];
    borderColor.a = 0x66; // Increased to 40% opacity to match grid lines
    int borderWidth = ssaaFactor;  // Scale border width with supersampling
    
    // Calculate border bounds with proper rounded corners
    int scaledCornerRadius = scaleSize(cornerRadius);
    int leftX = scaleX(margin_left);
    int topY = scaleY(margin_top);
    int rightX = ssaaWidth - scaleX(margin_right);
    int bottomY = ssaaHeight - scaleY(margin_bottom);
    
    // Draw straight border segments (avoiding the corner regions)
    
    // Top border (from left corner to right corner)
    drawLine(leftX + scaledCornerRadius, topY, 
             rightX - scaledCornerRadius, topY, 
             borderColor, borderWidth);
             
    // Bottom border (from left corner to right corner)
    drawLine(leftX + scaledCornerRadius, bottomY, 
             rightX - scaledCornerRadius, bottomY, 
             borderColor, borderWidth);
             
    // Left border (from top corner to bottom corner)
    drawLine(leftX, topY + scaledCornerRadius, 
             leftX, bottomY - scaledCornerRadius, 
             borderColor, borderWidth);
             
    // Right border (from top corner to bottom corner)
    drawLine(rightX, topY + scaledCornerRadius, 
             rightX, bottomY - scaledCornerRadius, 
             borderColor, borderWidth);
    
    // Add rounded corners with proper supersampling and anti-aliasing
    drawRoundedCorners(leftX, topY, 
                       rightX, bottomY, 
                       scaledCornerRadius, borderColor);
}

void Plotter::drawAxes()
{
    // Calculate the center (origin) of the plot
    int centerX = scaleX(margin_left + (width - margin_left - margin_right) / 2);
    int centerY = scaleY(margin_top + (height - margin_top - margin_bottom) / 2);
    
    // We will NOT draw any default axis labels here
    // Each visualization method will handle its own specific labels
    
    // Draw axis lines at center if needed (for charts that need origin axes)
    if (showAxes) {
        RGBA axisColor = elementColors["axes"];
        
        // Draw X-axis (if needed)
        drawLine(scaleX(margin_left), centerY, ssaaWidth - scaleX(margin_right), centerY, axisColor, 2);
        
        // Draw Y-axis (if needed)
        drawLine(centerX, scaleY(margin_top), centerX, ssaaHeight - scaleY(margin_bottom), axisColor, 2);
    }
}

// Helper function to clamp a value between min and max
inline int Plotter::clamp(int value, int min, int max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

void Plotter::drawLine(int x1, int y1, int x2, int y2, const RGBA& lineColor, int lineWidth)
{
    // Clamp coordinates to stay within image bounds
    x1 = clamp(x1, 0, ssaaWidth - 1);
    x2 = clamp(x2, 0, ssaaWidth - 1);
    y1 = clamp(y1, 0, ssaaHeight - 1);
    y2 = clamp(y2, 0, ssaaHeight - 1);
    
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
                
                if (drawX >= 0 && drawX < static_cast<int>(ssaaWidth) && 
                    drawY >= 0 && drawY < static_cast<int>(ssaaHeight)) {
                    
                    // For semi-transparent colors, blend with background
                    if (lineColor.a < 255) {
                        RGBA currentPixel = ssaaImage.GetPixel(drawX, drawY);
                        RGBA blendedColor = blendRGBA(currentPixel, lineColor);
                        ssaaImage.SetPixel(drawX, drawY, blendedColor);
                    } else {
                        // Fully opaque - just set the pixel
                        ssaaImage.SetPixel(drawX, drawY, lineColor);
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

void Plotter::drawPoint(int x, int y, int size, const RGBA& color)
{
    // Draw a filled circle for the point with anti-aliasing
    drawCircle(x, y, size, color, true, 0);
}

void Plotter::drawCircle(int x, int y, int radius, const RGBA& color, bool filled, int borderWidth)
{
    // Note: x, y, radius are already in supersampled space
    // To improve quality, use anti-aliasing for circles
    float radiusSquared = radius * radius;
    float outerRadiusSquared = (radius + 0.5f) * (radius + 0.5f);
    float innerRadiusSquared = (filled) ? 0 : (radius - borderWidth) * (radius - borderWidth);
    
    for (int dy = -radius - 1; dy <= radius + 1; dy++) {
        for (int dx = -radius - 1; dx <= radius + 1; dx++) {
            // Calculate exact distance squared from center
            float distSquared = dx * dx + dy * dy;
            
            // Skip pixels definitely outside the circle
            if (distSquared > outerRadiusSquared) {
                continue;
            }
            
            // Skip pixels definitely inside the inner border for non-filled circles
            if (!filled && distSquared < innerRadiusSquared) {
                continue;
            }
            
                    int drawX = x + dx;
                    int drawY = y + dy;
                    
            // Skip pixels outside the image
            if (drawX < 0 || drawX >= static_cast<int>(ssaaWidth) ||
                drawY < 0 || drawY >= static_cast<int>(ssaaHeight)) {
                continue;
            }
            
            // Apply anti-aliasing at the edges
            float alpha = 1.0f;
            
            if (filled) {
                // For filled circle, apply anti-aliasing only at the outer edge
                if (distSquared > radiusSquared) {
                    // Calculate alpha based on distance from the edge
                    alpha = 1.0f - (std::sqrt(distSquared) - radius);
                    alpha = std::max(0.0f, std::min(1.0f, alpha));
                }
            } else {
                // For outline circle, apply anti-aliasing at both inner and outer edges
                float innerRadius = radius - borderWidth;
                float outerRadius = radius;
                float distance = std::sqrt(distSquared);
                
                if (distance > outerRadius) {
                    // Outer edge
                    alpha = 1.0f - (distance - outerRadius);
                    alpha = std::max(0.0f, std::min(1.0f, alpha));
                } else if (distance < innerRadius) {
                    // Inner edge
                    alpha = 1.0f - (innerRadius - distance);
                    alpha = std::max(0.0f, std::min(1.0f, alpha));
                }
            }
            
            // Apply alpha to the color
            if (alpha < 1.0f) {
                RGBA adjustedColor = color;
                adjustedColor.a = static_cast<unsigned char>(color.a * alpha);
                
                // Blend with existing pixel
                RGBA currentPixel = ssaaImage.GetPixel(drawX, drawY);
                RGBA blendedColor = blendRGBA(currentPixel, adjustedColor);
                ssaaImage.SetPixel(drawX, drawY, blendedColor);
            } else {
                // Full opacity or middle of the circle
                if (color.a < 255) {
                    // If original color is semi-transparent, still need to blend
                    RGBA currentPixel = ssaaImage.GetPixel(drawX, drawY);
                    RGBA blendedColor = blendRGBA(currentPixel, color);
                    ssaaImage.SetPixel(drawX, drawY, blendedColor);
                } else {
                    // Fully opaque
                        ssaaImage.SetPixel(drawX, drawY, color);
                }
            }
        }
    }
}

void Plotter::drawRect(int x, int y, int rectWidth, int rectHeight, const RGBA& color, bool filled, int borderWidth)
{
    // Note: x, y, rectWidth, rectHeight are already in supersampled space
    if (filled) {
        // Draw filled rectangle
        for (int dy = 0; dy < rectHeight; dy++) {
            for (int dx = 0; dx < rectWidth; dx++) {
                int drawX = x + dx;
                int drawY = y + dy;
                
                if (drawX >= 0 && drawX < static_cast<int>(ssaaWidth) &&
                    drawY >= 0 && drawY < static_cast<int>(ssaaHeight)) {
                    // Handle alpha blending for semi-transparent rectangles
                    if (color.a < 255) {
                        RGBA currentPixel = ssaaImage.GetPixel(drawX, drawY);
                        RGBA blendedColor = blendRGBA(currentPixel, color);
                        ssaaImage.SetPixel(drawX, drawY, blendedColor);
                    } else {
                        // Fully opaque - just set the pixel
                    ssaaImage.SetPixel(drawX, drawY, color);
                    }
                }
            }
        }
    } else {
        // Draw top border
        for (int dx = 0; dx < rectWidth; dx++) {
            for (int b = 0; b < borderWidth; b++) {
                int drawX = x + dx;
                int drawY = y + b;
                
                if (drawX >= 0 && drawX < static_cast<int>(ssaaWidth) &&
                    drawY >= 0 && drawY < static_cast<int>(ssaaHeight)) {
                    // Handle alpha blending
                    if (color.a < 255) {
                        RGBA currentPixel = ssaaImage.GetPixel(drawX, drawY);
                        RGBA blendedColor = blendRGBA(currentPixel, color);
                        ssaaImage.SetPixel(drawX, drawY, blendedColor);
                    } else {
                    ssaaImage.SetPixel(drawX, drawY, color);
                    }
                }
            }
        }
        
        // Draw bottom border
        for (int dx = 0; dx < rectWidth; dx++) {
            for (int b = 0; b < borderWidth; b++) {
                int drawX = x + dx;
                int drawY = y + rectHeight - b - 1;
                
                if (drawX >= 0 && drawX < static_cast<int>(ssaaWidth) &&
                    drawY >= 0 && drawY < static_cast<int>(ssaaHeight)) {
                    // Handle alpha blending
                    if (color.a < 255) {
                        RGBA currentPixel = ssaaImage.GetPixel(drawX, drawY);
                        RGBA blendedColor = blendRGBA(currentPixel, color);
                        ssaaImage.SetPixel(drawX, drawY, blendedColor);
                    } else {
                    ssaaImage.SetPixel(drawX, drawY, color);
                    }
                }
            }
        }
        
        // Draw left border
        for (int dy = 0; dy < rectHeight; dy++) {
            for (int b = 0; b < borderWidth; b++) {
                int drawX = x + b;
                int drawY = y + dy;
                
                if (drawX >= 0 && drawX < static_cast<int>(ssaaWidth) &&
                    drawY >= 0 && drawY < static_cast<int>(ssaaHeight)) {
                    // Handle alpha blending
                    if (color.a < 255) {
                        RGBA currentPixel = ssaaImage.GetPixel(drawX, drawY);
                        RGBA blendedColor = blendRGBA(currentPixel, color);
                        ssaaImage.SetPixel(drawX, drawY, blendedColor);
                    } else {
                    ssaaImage.SetPixel(drawX, drawY, color);
                    }
                }
            }
        }
        
        // Draw right border
        for (int dy = 0; dy < rectHeight; dy++) {
            for (int b = 0; b < borderWidth; b++) {
                int drawX = x + rectWidth - b - 1;
                int drawY = y + dy;
                
                if (drawX >= 0 && drawX < static_cast<int>(ssaaWidth) &&
                    drawY >= 0 && drawY < static_cast<int>(ssaaHeight)) {
                    // Handle alpha blending
                    if (color.a < 255) {
                        RGBA currentPixel = ssaaImage.GetPixel(drawX, drawY);
                        RGBA blendedColor = blendRGBA(currentPixel, color);
                        ssaaImage.SetPixel(drawX, drawY, blendedColor);
                    } else {
                    ssaaImage.SetPixel(drawX, drawY, color);
                    }
                }
            }
        }
    }
}

void Plotter::drawText(int x, int y, const std::string& text, const RGBA& color, unsigned int fontSize, bool centerAligned)
{
    // Scale coordinates and font size for supersampling
    int ssaaX = scaleX(x);
    int ssaaY = scaleY(y);
    unsigned int ssaaFontSize = fontSize * ssaaFactor;
    
    // Set the font size
    if (FT_Set_Pixel_Sizes(face, 0, ssaaFontSize)) {
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
        ssaaX -= textWidth / 2;
    }
    
    // Compute baseline using font metrics
    int baseline = face->size->metrics.ascender / 64; // Convert from 26.6 fixed-point to pixels
    
    // Adjusted spacing for better readability
    unsigned int extraSpacing = ssaaFontSize / 10; // Spacing between characters
    
    // Draw each character
    unsigned int penX = ssaaX;
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
        unsigned int drawY = ssaaY - glyph->bitmap_top;
        
        // Draw the glyph bitmap
        for (unsigned int row = 0; row < glyphHeight; ++row) {
            for (unsigned int col = 0; col < glyphWidth; ++col) {
                // Get pixel value from glyph bitmap
                unsigned char value = glyph->bitmap.buffer[row * glyphWidth + col];
                
                if (value > 0) { // Only draw if the glyph pixel is not empty
                    unsigned int imgX = drawX + col;
                    unsigned int imgY = drawY + row;
                    
                    if (imgX < ssaaWidth && imgY < ssaaHeight) {
                        // Calculate alpha-blended color
                        float alpha = value / 255.0f;
                        RGBA blendedColor;
                        blendedColor.r = static_cast<unsigned char>(color.r * alpha);
                        blendedColor.g = static_cast<unsigned char>(color.g * alpha);
                        blendedColor.b = static_cast<unsigned char>(color.b * alpha);
                        blendedColor.a = static_cast<unsigned char>(color.a * alpha);
                        
                        // Get current pixel and blend with the glyph
                        RGBA currentPixel = ssaaImage.GetPixel(imgX, imgY);
                        RGBA finalColor = blendRGBA(currentPixel, blendedColor);
                        ssaaImage.SetPixel(imgX, imgY, finalColor);
                    }
                }
            }
        }
        
        // Advance cursor position
        penX += (glyph->advance.x >> 6) + extraSpacing;
    }
}

void Plotter::addTitle(const std::string& text, unsigned int fontSize)
{
    // Position title at the top left of the image with some padding
    int x = margin_left;
    int y = margin_top / 2;
    
    // Use left alignment (false for centerAligned parameter)
    drawText(x, y, text, elementColors["title"], fontSize, false);
}

// Common method for drawing axis labels with consistent positioning
void Plotter::drawAxisLabels(const std::string& xLabel, const std::string& yLabel, unsigned int fontSize, bool centerX) {
    // Get text color from CSS design
    RGBA textColor = elementColors["axisLabel"];
    textColor.a = 0xCC; // 80% opacity for readability
    
    // Draw X-axis label with proper styling
    int xLabelX = centerX ? width / 2 : margin_left;
    int xLabelY = height - margin_bottom / 3;
    drawText(xLabelX, xLabelY, xLabel, textColor, fontSize, centerX);
    
    // Draw Y-axis label (vertical text) with proper styling
    // Use consistent position at margin_left/2 to prevent cutoff
    drawVerticalText(yLabel, margin_left / 2, height / 2, fontSize, textColor);
}

// Add this helper method after drawInfoBox to calculate heights of elements
int Plotter::calculateInfoBoxHeight(const std::vector<std::string>& labels, unsigned int fontSize) {
    // For horizontal layout, height is always the same regardless of number of items
    int itemHeight = fontSize + 6;
    
    // Calculate total height including top and bottom padding
    return 24 + itemHeight;  // 12px padding top and bottom + single row height
}

// Helper function to estimate text width based on string length and font size
int Plotter::estimateTextWidth(const std::string& text, unsigned int fontSize) {
    // Use a more accurate character width estimation
    // Different characters have different widths, so we'll use an average factor
    return static_cast<int>(text.length() * fontSize * 0.75) + 10; // Add padding for safety
}

// Update addLegend method to use dynamic spacing calculation
int Plotter::addLegend(const std::vector<std::string>& labels, const std::vector<RGBA>& colors, int x, int y, unsigned int fontSize)
{
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
        int textWidth = estimateTextWidth(labels[i], fontSize);
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
    drawInfoBox(x, y, legendWidth, legendHeight, "", fontSize);
    
    // Draw each legend item with exact positioning - now horizontally aligned
    int currentX = x + sidePadding; // Start with left padding
    
    for (size_t i = 0; i < labels.size(); ++i) {
        // Draw color indicator dot
        int dotX = currentX;
        int dotY = y + legendHeight/2; // Centered vertically
        
        // Scale dot size and use supersampling for the dot
        drawCircle(scaleX(dotX), scaleY(dotY), scaleSize(colorIndicatorSize/2), colors[i], true);
        
        // Draw label text
        drawText(dotX + colorTextPadding, dotY, labels[i], elementColors["legend"], fontSize, false);
        
        // Move to next item position based on actual width
        currentX += colorIndicatorSize + colorTextPadding + textWidths[i] + minItemSpacing;
    }
    
    // Return the height of the legend box
    return legendHeight;
}

// Helper method to prepare legend labels and colors
void Plotter::prepareLegendColors(const std::vector<std::string>& labels, const std::vector<RGBA>& colors,
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

void Plotter::plotPoints(const std::vector<Point>& points, const RGBA& color, int pointSize)
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
        
        // Convert to supersampled coordinates
        int ssX = scaleX(p.x);
        int ssY = scaleY(p.y);
        int ssPointSize = scaleSize(pointSize);
        
        // Draw the point in supersampled space
        drawPoint(ssX, ssY, ssPointSize, color);
    }
}

void Plotter::plotLine(const std::vector<Point>& points, const RGBA& color, int lineWidth)
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
        
        // Convert to supersampled coordinates for drawing
        int ssX1 = scaleX(p1.x);
        int ssY1 = scaleY(p1.y);
        int ssX2 = scaleX(p2.x);
        int ssY2 = scaleY(p2.y);
        int ssLineWidth = scaleSize(lineWidth);
        
        // Draw the line segment in supersampled space
        drawLine(ssX1, ssY1, ssX2, ssY2, color, ssLineWidth);
    }
}

void Plotter::saveAsPNG(const std::string& filename, const std::string& folder)
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
    
    // Draw the logo if available
    if (hasLogo) {
        drawLogo();
    }
    
    // Downsample the supersampled image to the final output image
    downsampleToOutput();
    
    // Make sure the directory exists
    std::string filenameWithPath = folder + "/" + filename;
    
    // Save PNG file - SavePNG is a void method, not a bool
    image.SavePNG(filenameWithPath.c_str());
    printf("Saved chart as: %s\n", filenameWithPath.c_str());
}

void Plotter::setShowGrid(bool show)
{
    showGrid = show;
    
    // Redraw the background
    drawBackground();
}

void Plotter::setShowAxes(bool show)
{
    showAxes = show;
    
    // Redraw the background
    drawBackground();
}

void Plotter::setCornerRadius(int radius)
{
    cornerRadius = radius;
    
    // Redraw the background
    drawBackground();
}

// Plot clusters with centroids
void Plotter::plotClusters(const std::vector<std::vector<double> >& data, const std::vector<int>& labels, 
                           const std::vector<std::vector<double> >& centroids)
{
    if (data.empty() || data[0].size() < 2 || data.size() != labels.size()) {
        return;
    }
    
    // Create a chart configuration with increased font size for mobile
    ChartConfig config("Cluster Analysis", 36, "Feature X", "Feature Y", 32);
    
    // Variables for margin storage and positioning
    unsigned int originalTopMargin, originalRightMargin;
    int titleY, legendY;
    
    // Use common setup for chart initialization
    setupChart(config, &originalTopMargin, &originalRightMargin, 180, &titleY, &legendY);
    
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
    
    // Estimate legend height before drawing it
    std::vector<std::string> tempLegendLabels;
    for (int i = 0; i < numClusters; ++i) {
        char labelBuffer[32];
        std::sprintf(labelBuffer, "Cluster %d", i);
        tempLegendLabels.push_back(labelBuffer);
    }
    tempLegendLabels.push_back("Centroid");
    
    int estimatedLegendHeight = calculateInfoBoxHeight(tempLegendLabels, 16);
    
    // Dynamically set the top margin to accommodate title and legend with a small buffer
    margin_top = legendY + estimatedLegendHeight + 15; // 15px buffer
    
    // Redraw with proper margins
    prepareCanvas();
    
    // Redraw title after prepareCanvas
    drawText(margin_left, titleY, config.title, elementColors["title"], config.titleFontSize, false);
    
    // Draw the legend
    int actualLegendHeight = createClusterLegend(clusterColors, numClusters, margin_left, legendY);
    
    // If actual height differs significantly from estimate, adjust margin (not redrawing for performance)
    if (std::abs(actualLegendHeight - estimatedLegendHeight) > 20) {
        margin_top = legendY + actualLegendHeight + 15;
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
    
    // Draw axis labels and ticks
    drawAxisLabels(config.xAxisLabel, config.yAxisLabel, config.axisFontSize);
    drawXAxisTicks(xRange.min, xRange.max, 4, 1);
    drawYAxisTicks(yRange.min, yRange.max, 3, false, 1, 50);
    
    // Structure to store points for each cluster
    std::vector<std::vector<std::pair<int, int> > > clusterPoints;
    for (int i = 0; i < numClusters; ++i) {
        std::vector<std::pair<int, int> > empty;
        clusterPoints.push_back(empty);
    }
    
    // Calculate optimal point size based on data density
    int pointSize = 6;  // Default size
    if (data.size() < 50) {
        pointSize = 10;  // Larger points for small datasets and better mobile visibility
    } else if (data.size() > 200) {
        pointSize = 5;  // Smaller points for large datasets
    } else {
        pointSize = 8;  // Medium datasets get slightly larger points for mobile
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
        
        // Store point coordinates in output space for later use
        clusterPoints[cluster].push_back(std::make_pair(screenPoint.x, screenPoint.y));
        
        // Draw the point with the cluster color - scaled for supersampling
        drawPoint(scaleX(screenPoint.x), scaleY(screenPoint.y), 
                  scaleSize(pointSize), clusterColors[cluster]);
    }
    
    // Draw centroids as white circles with colored crosses
    drawCentroids(centroids, clusterColors, xRange, yRange);
    
    // Draw cluster labels
    drawClusterLabels(clusterCenters, clusterRadii, clusterPoints, clusterColors);
    
    // Restore original margins
    margin_right = originalRightMargin;
    margin_top = originalTopMargin;
}

// Helper method to calculate cluster bounds
void Plotter::calculateClusterBounds(
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
    
    // Map to screen coordinates - use unscaled coordinates as these will be scaled later
    Point screenCenter = mapDataToScreen(centerX, centerY, xRange, yRange);
    Point edgePoint = mapDataToScreen(centerX + maxDist, centerY, xRange, yRange);
    
    // Calculate radius in output space (not supersampled yet)
    int radius = std::abs(edgePoint.x - screenCenter.x) + 15; // Add padding for better visibility
    
    clusterCenters.push_back(screenCenter);
    clusterRadii.push_back(radius);
}

// Helper method to draw cluster circles with transparency
void Plotter::drawClusterCircles(
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
        
        // Scale the coordinates and dimensions for supersampling
        int radius = scaleSize(clusterRadii[cluster]);
        Point center;
        center.x = scaleX(clusterCenters[cluster].x);
        center.y = scaleY(clusterCenters[cluster].y);
        
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
                
                // Skip pixels outside supersampled image bounds
                if (drawX < 0 || drawX >= static_cast<int>(ssaaWidth) ||
                    drawY < 0 || drawY >= static_cast<int>(ssaaHeight)) {
                    continue;
                }
                
                // Get existing pixel color (the background gradient)
                RGBA existingColor = ssaaImage.GetPixel(drawX, drawY);
                
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
                    ssaaImage.SetPixel(drawX, drawY, resultColor);
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
                ssaaImage.SetPixel(drawX, drawY, resultColor);
            }
        }
    }
}

// Helper function for proper alpha blending that preserves the background
RGBA Plotter::blendRGBA(const RGBA& base, const RGBA& over) {
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
void Plotter::drawCentroids(
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
        
        // Scale for supersampling
        int ssaaX = scaleX(centroidPoint.x);
        int ssaaY = scaleY(centroidPoint.y);
        
        // CSS uses solid white circle with drop shadow and colored cross
        
        // First draw shadow - 3px offset and 40% opacity (exactly as in CSS)
        RGBA shadowColor(0x00, 0x00, 0x00, 0x66); // 40% opacity black shadow
        int shadowOffset = scaleSize(3); // 3px offset as in CSS, scaled
        
        // Draw larger soft shadow first (matches CSS box-shadow effect)
        int shadowRadius = scaleSize(12);
        for (int dy = -shadowRadius; dy <= shadowRadius; dy++) {
            for (int dx = -shadowRadius; dx <= shadowRadius; dx++) {
                int distSqr = dx*dx + dy*dy;
                if (distSqr > shadowRadius*shadowRadius) continue; // Only pixels within 12px radius
                
                // Calculate shadow intensity - fade out toward edges (Gaussian-like)
                float shadowDistance = std::sqrt(distSqr);
                float shadowIntensity = 0.4f * std::exp(-shadowDistance / (shadowRadius / 2.0f));
                
                int drawX = ssaaX + dx + shadowOffset;
                int drawY = ssaaY + dy + shadowOffset;
                
                if (drawX >= 0 && drawX < static_cast<int>(ssaaWidth) &&
                    drawY >= 0 && drawY < static_cast<int>(ssaaHeight)) {
                    RGBA pixelShadow = shadowColor;
                    pixelShadow.a = static_cast<unsigned char>(shadowColor.a * shadowIntensity);
                    blendPixel(drawX, drawY, pixelShadow, shadowIntensity);
                }
            }
        }
        
        // Draw the white circle on top of the shadow - exactly as in CSS design
        RGBA whiteFill(0xFF, 0xFF, 0xFF, 0xFF); // Solid white
        int circleRadius = scaleSize(10); // 10px radius as in CSS, scaled
        
        // Draw solid white circle
        for (int dy = -circleRadius; dy <= circleRadius; dy++) {
            for (int dx = -circleRadius; dx <= circleRadius; dx++) {
                int distSqr = dx*dx + dy*dy;
                if (distSqr > circleRadius * circleRadius) continue;
                
                int drawX = ssaaX + dx;
                int drawY = ssaaY + dy;
                
                if (drawX >= 0 && drawX < static_cast<int>(ssaaWidth) &&
                    drawY >= 0 && drawY < static_cast<int>(ssaaHeight)) {
                    ssaaImage.SetPixel(drawX, drawY, whiteFill);
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
        
        // Draw horizontal line of cross - 3px thickness as in CSS (scaled)
        int crossThickness = scaleSize(1);
        int crossLength = scaleSize(8);
        for (int y = -crossThickness; y <= crossThickness; ++y) {
            drawLine(ssaaX - crossLength, ssaaY + y, 
                     ssaaX + crossLength, ssaaY + y, crossColor);
        }
        
        // Draw vertical line of cross - 3px thickness as in CSS (scaled)
        for (int x = -crossThickness; x <= crossThickness; ++x) {
            drawLine(ssaaX + x, ssaaY - crossLength, 
                     ssaaX + x, ssaaY + crossLength, crossColor);
        }
    }
}

// Helper method to draw cluster labels
void Plotter::drawClusterLabels(
    const std::vector<Point>& clusterCenters,
    const std::vector<int>& clusterRadii,
    const std::vector<std::vector<std::pair<int, int> > >& clusterPoints,
    const std::vector<RGBA>& clusterColors)
{
    // Draw cluster labels
    for (size_t cluster = 0; cluster < clusterPoints.size(); ++cluster) {
        if (clusterPoints[cluster].empty() || cluster >= clusterCenters.size() || cluster >= clusterRadii.size()) continue;
        
        // Get the center and radius of this cluster
        Point center = clusterCenters[cluster];
        int radius = clusterRadii[cluster];
        
        // Format label
        char label[32];
        std::sprintf(label, "Cluster %d", (int)cluster);
        
        // Get text dimensions
        int labelWidth = estimateTextWidth(label, 22);
        int labelHeight = 30; // Approximate height for 22px font
        
        // Background padding
        int padX = 12;
        int padY = 8;
        int bgWidth = labelWidth + padX*2;
        int bgHeight = labelHeight + padY*2;
        
        // Setup for label placement
        int plotCenterX = margin_left + getPlotWidth() / 2;
        int plotCenterY = margin_top + getPlotHeight() / 2;
        
        // Variables for final label position
        int posX, posY;
        
        // Calculate safe boundaries for label placement
        int safeLeftBound = static_cast<int>(margin_left) + bgWidth/2 + 10;
        int safeRightBound = static_cast<int>(width - margin_right) - bgWidth/2 - 10;
        int safeTopBound = static_cast<int>(margin_top) + bgHeight/2 + 10;
        int safeBottomBound = static_cast<int>(height - margin_bottom) - bgHeight/2 - 10;
        
        // Special handling for cluster 1
        if (cluster == 1) {
            // For cluster 1, place label to the bottom-left of the cluster
            // This is a fixed position relative to the cluster that we know works well
            double fixedAngle = 3.0 * M_PI / 4.0; // 135 degrees - towards bottom-left
            
            // Reduced offset to bring the label closer to the cluster circle
            // Use just 1.2x the radius plus a small fixed margin
            int fixedOffset = static_cast<int>(radius * 1.2) + 15;
            
            posX = center.x + static_cast<int>(fixedOffset * std::cos(fixedAngle));
            posY = center.y + static_cast<int>(fixedOffset * std::sin(fixedAngle));
            
            // Ensure the label stays within boundaries
            posX = std::max(safeLeftBound, std::min(safeRightBound, posX));
            posY = std::max(safeTopBound, std::min(safeBottomBound, posY));
        }
        // Different approach for clusters 0 and 2 (and any others)
        else {
            // Calculate angle from plot center to cluster center
            double dx = center.x - plotCenterX;
            double dy = center.y - plotCenterY;
            double angle = std::atan2(dy, dx);
            
            // For non-cluster-1 labels, calculate diagonal of label for proper offset
            double labelDiagonal = std::sqrt(bgWidth * bgWidth + bgHeight * bgHeight) / 2.0;
            int offsetDistance = static_cast<int>(labelDiagonal) + 10; // Extra margin
            
            // Calculate initial position at the edge of the circle plus offset
            posX = center.x + static_cast<int>((radius + offsetDistance) * std::cos(angle));
            posY = center.y + static_cast<int>((radius + offsetDistance) * std::sin(angle));
            
            // Check if the label would be outside chart boundaries
            int leftEdge = posX - bgWidth/2;
            int rightEdge = posX + bgWidth/2;
            int topEdge = posY - bgHeight/2;
            int bottomEdge = posY + bgHeight/2;
            
            // If outside boundaries, adjust position
            if (leftEdge < margin_left || rightEdge > width - margin_right || 
                topEdge < margin_top || bottomEdge > height - margin_bottom) {
                
                // Try a few simple angles to find a good position
                bool found = false;
                double testAngles[4] = {0, M_PI/2, M_PI, -M_PI/2}; // 0°, 90°, 180°, 270°
                
                for (int i = 0; i < 4 && !found; i++) {
                    int testX = center.x + static_cast<int>((radius + offsetDistance) * std::cos(testAngles[i]));
                    int testY = center.y + static_cast<int>((radius + offsetDistance) * std::sin(testAngles[i]));
                    
                    // Check if this position would be within boundaries
                    int testLeftEdge = testX - bgWidth/2;
                    int testRightEdge = testX + bgWidth/2;
                    int testTopEdge = testY - bgHeight/2;
                    int testBottomEdge = testY + bgHeight/2;
                    
                    if (testLeftEdge >= margin_left && testRightEdge <= width - margin_right && 
                        testTopEdge >= margin_top && testBottomEdge <= height - margin_bottom) {
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
        }
        
        // Calculate background rectangle position
        int bgX = posX - bgWidth/2;
        int bgY = posY - bgHeight/2;
        
        // Position for text centered in the background
        int textX = bgX + padX;
        int textY = bgY + padY + labelHeight/2;
        
        // Get the cluster color for the background
        RGBA bgColor = clusterColors[cluster % clusterColors.size()];
        
        // Define corner radius for the label background
        int cornerRadius = 8; // Increased from 6 to 8 for more visible rounded corners
        
        // Draw background with rounded corners using scaled coordinates
        bgColor.a = 0xE6; // 90% opacity
        
        // New approach: Draw the rounded rectangle manually with proper clipping
        // Convert all coordinates to supersampled space
        int ssX = scaleX(bgX);
        int ssY = scaleY(bgY);
        int ssWidth = scaleSize(bgWidth);
        int ssHeight = scaleSize(bgHeight);
        int ssCornerRadius = scaleSize(cornerRadius);
        
        // For each pixel in the rounded rectangle
        for (int y = 0; y < ssHeight; y++) {
            for (int x = 0; x < ssWidth; x++) {
                // Determine which region of the rounded rectangle this pixel falls in
                bool inCorner = false;
                bool drawPixel = true;
                
                // Check if we're in a corner region
                if (x < ssCornerRadius && y < ssCornerRadius) {
                    // Top-left corner
                    float dist = std::sqrt(std::pow(ssCornerRadius - x, 2) + std::pow(ssCornerRadius - y, 2));
                    if (dist > ssCornerRadius) {
                        drawPixel = false; // Outside rounded corner
                    }
                }
                else if (x >= ssWidth - ssCornerRadius && y < ssCornerRadius) {
                    // Top-right corner
                    float dist = std::sqrt(std::pow(x - (ssWidth - ssCornerRadius), 2) + std::pow(ssCornerRadius - y, 2));
                    if (dist > ssCornerRadius) {
                        drawPixel = false; // Outside rounded corner
                    }
                }
                else if (x < ssCornerRadius && y >= ssHeight - ssCornerRadius) {
                    // Bottom-left corner
                    float dist = std::sqrt(std::pow(ssCornerRadius - x, 2) + std::pow(y - (ssHeight - ssCornerRadius), 2));
                    if (dist > ssCornerRadius) {
                        drawPixel = false; // Outside rounded corner
                    }
                }
                else if (x >= ssWidth - ssCornerRadius && y >= ssHeight - ssCornerRadius) {
                    // Bottom-right corner
                    float dist = std::sqrt(std::pow(x - (ssWidth - ssCornerRadius), 2) + std::pow(y - (ssHeight - ssCornerRadius), 2));
                    if (dist > ssCornerRadius) {
                        drawPixel = false; // Outside rounded corner
                    }
                }
                
                // Draw the pixel if it's inside the rounded rectangle
                if (drawPixel) {
                    int drawX = ssX + x;
                    int drawY = ssY + y;
                    
                    if (drawX >= 0 && drawX < static_cast<int>(ssaaWidth) && 
                        drawY >= 0 && drawY < static_cast<int>(ssaaHeight)) {
                        // For semi-transparent colors, blend with background
                        if (bgColor.a < 255) {
                            RGBA currentPixel = ssaaImage.GetPixel(drawX, drawY);
                            RGBA blendedColor = blendRGBA(currentPixel, bgColor);
                            ssaaImage.SetPixel(drawX, drawY, blendedColor);
                        } else {
                            // Fully opaque - just set the pixel
                            ssaaImage.SetPixel(drawX, drawY, bgColor);
                        }
                    }
                }
            }
        }
        
        // Create a dark text color for better contrast
        RGBA textColor = RGBA(
            static_cast<unsigned char>(bgColor.r * 0.2f),  // Very dark version of the cluster color
            static_cast<unsigned char>(bgColor.g * 0.2f),
            static_cast<unsigned char>(bgColor.b * 0.2f),
            0xFF // Fully opaque
        );
        
        // Ensure minimum darkness for readability
        if ((textColor.r + textColor.g + textColor.b) / 3 > 60) {
            textColor = RGBA(0x20, 0x20, 0x20, 0xFF); // Default to very dark gray if too light
        }
        
        // Draw the label text
        drawText(textX, textY, label, textColor, 22, false);
    }
}

void Plotter::drawCandlestick(int x, int y_open, int y_close, int y_high, int y_low, const RGBA& color)
{
    // Enhanced candlestick styling to match CSS design exactly
    
    // Apply supersampling scaling to coordinates
    int ssaaX = scaleX(x);
    int ssaaYOpen = scaleY(y_open);
    int ssaaYClose = scaleY(y_close);
    int ssaaYHigh = scaleY(y_high);
    int ssaaYLow = scaleY(y_low);
    
    // Define the width of the candlestick body - matching CSS
    int bodyWidth = scaleSize(18);  // Width of candlestick body in pixels, scaled
    int wickThickness = scaleSize(3);  // Thickness of wick line, scaled
    
    // Draw the wick (line from high to low) with proper styling
    for (int i = -wickThickness/2; i <= wickThickness/2; ++i) {
        // Use semi-transparent color for the wick to match design
        RGBA wickColor = color;
        wickColor.a = 0xE6; // 90% opacity
        drawLine(ssaaX + i, ssaaYHigh, ssaaX + i, ssaaYLow, wickColor);
    }
    
    // Determine the top and bottom of the body
    int bodyTop = std::min(ssaaYOpen, ssaaYClose);
    int bodyBottom = std::max(ssaaYOpen, ssaaYClose);
    
    // Ensure minimum body height for better visibility - matching CSS
    if (bodyBottom - bodyTop < scaleSize(4)) {
        bodyBottom = bodyTop + scaleSize(4);
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
            int drawX = ssaaX + dx;
            int drawY = dy;
            
            if (drawX >= 0 && drawX < static_cast<int>(ssaaWidth) && 
                drawY >= 0 && drawY < static_cast<int>(ssaaHeight)) {
                ssaaImage.SetPixel(drawX, drawY, gradientColor);
            }
        }
    }
    
    // Add sophisticated 3D effects with highlights and shadows - matching CSS
    
    // Left edge highlight with gradient fade - brighter near edge
    RGBA highlightColor(0xFF, 0xFF, 0xFF, 0x99);  // 60% opacity white
    int edgeWidth = scaleSize(5); // Highlight width (slightly wider than before), scaled
    
    for (int dy = bodyTop; dy <= bodyBottom; ++dy) {
        for (int dx = 0; dx < edgeWidth; ++dx) {
            // Calculate fade strength - more pronounced edge highlight
            float alpha = (edgeWidth - dx) / static_cast<float>(edgeWidth) * 0.6f;
            int drawX = ssaaX - bodyWidth/2 + dx;
            int drawY = dy;
            
            if (drawX >= 0 && drawX < static_cast<int>(ssaaWidth) && 
                drawY >= 0 && drawY < static_cast<int>(ssaaHeight)) {
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
            int drawX = ssaaX + bodyWidth/2 - dx - 1;
            int drawY = dy;
            
            if (drawX >= 0 && drawX < static_cast<int>(ssaaWidth) && 
                drawY >= 0 && drawY < static_cast<int>(ssaaHeight)) {
                RGBA pixelShadow = shadowColor;
                pixelShadow.a = static_cast<unsigned char>(255 * alpha);
                blendPixel(drawX, drawY, pixelShadow, alpha);
            }
        }
    }
    
    // Top edge highlight for 3D effect - subtle rounded top
    float topHighlightAlpha = 0.5f;
    int topEdgeHeight = std::max(scaleSize(2), (bodyBottom - bodyTop) / 20);
    int cornerRadius = scaleSize(3); // Corner radius matching CSS, scaled
    
    for (int dx = -bodyWidth/2 + cornerRadius; dx <= bodyWidth/2 - cornerRadius; ++dx) {
        int drawX = ssaaX + dx;
        int drawY = bodyTop;
        
        if (drawX >= 0 && drawX < static_cast<int>(ssaaWidth) && 
            drawY >= 0 && drawY < static_cast<int>(ssaaHeight)) {
            blendPixel(drawX, drawY, highlightColor, topHighlightAlpha);
        }
    }
    
    // Bottom edge shadow for 3D effect - subtle rounded bottom
    float bottomShadowAlpha = 0.5f;
    
    for (int dx = -bodyWidth/2 + cornerRadius; dx <= bodyWidth/2 - cornerRadius; ++dx) {
        int drawX = ssaaX + dx;
        int drawY = bodyBottom;
        
        if (drawX >= 0 && drawX < static_cast<int>(ssaaWidth) && 
            drawY >= 0 && drawY < static_cast<int>(ssaaHeight)) {
            blendPixel(drawX, drawY, shadowColor, bottomShadowAlpha);
        }
    }
    
    // Add subtle highlight to corners for rounded appearance
    // Top-left corner extra highlight
    for (int i = 0; i < cornerRadius; ++i) {
        for (int j = 0; j < cornerRadius; ++j) {
            float distance = std::sqrt(i*i + j*j);
            if (distance <= cornerRadius) {
                int drawX = ssaaX - bodyWidth/2 + i;
                int drawY = bodyTop + j;
                
                if (drawX >= 0 && drawX < static_cast<int>(ssaaWidth) && 
                    drawY >= 0 && drawY < static_cast<int>(ssaaHeight)) {
                    blendPixel(drawX, drawY, highlightColor, topHighlightAlpha * 0.7f);
                }
            }
        }
    }
    
    // Bottom-right corner extra shadow
    for (int i = 0; i < cornerRadius; ++i) {
        for (int j = 0; j < cornerRadius; ++j) {
            float distance = std::sqrt(i*i + j*j);
            if (distance <= cornerRadius) {
                int drawX = ssaaX + bodyWidth/2 - i - 1;
                int drawY = bodyBottom - j;
                
                if (drawX >= 0 && drawX < static_cast<int>(ssaaWidth) && 
                    drawY >= 0 && drawY < static_cast<int>(ssaaHeight)) {
                    blendPixel(drawX, drawY, shadowColor, bottomShadowAlpha * 0.7f);
                }
            }
        }
    }
}

// Helper method to get theme color by index with proper bounds checking
RGBA Plotter::getThemeColor(int index) {
    if (themeColors.empty()) {
        // Return default color if theme colors are empty
        return RGBA(0xFF, 0xFF, 0xFF, 0xFF);
    }
    
    // Use modulo to wrap around if index is out of bounds
    return themeColors[index % themeColors.size()];
}

// Update drawHistogramStats to use dynamic spacing
void Plotter::drawHistogramStats(const std::vector<int>& bins, int maxBinValue, int legendY, unsigned int fontSize)
{
    // Print debug info
    printf("oldplotter: Drawing histogram stats box at legendY=%d\n", legendY);
    
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
    
    // Format stats values with 1 decimal place
    char meanText[64], modeText[64], maxText[64], sumText[64];
    std::sprintf(meanText, "Mean: %.1f", mean);
    std::sprintf(modeText, "Mode: %.1f", mode);
    std::sprintf(maxText, "Max: %d", maxBinValue);
    std::sprintf(sumText, "Sum: %d", sum);
    
    // Calculate required widths
    int titleWidth = estimateTextWidth("Histogram Statistics", 22);
    int meanWidth = estimateTextWidth(meanText, 20);
    int modeWidth = estimateTextWidth(modeText, 20);
    int maxWidth = estimateTextWidth(maxText, 20);
    int sumWidth = estimateTextWidth(sumText, 20);
    
    // Calculate minimum spacing between stats
    int minStatSpacing = 30;
    
    // Calculate box dimensions
    int sidePadding = 20;
    int boxWidth = sidePadding * 2 + meanWidth + modeWidth + maxWidth + sumWidth + minStatSpacing * 3;
    boxWidth = std::max(boxWidth, titleWidth + sidePadding * 2); // Ensure box is wide enough for title
    int boxHeight = 90; // Single row height + padding
    
    // Position the box aligned with the right edge of the chart
    int boxX = width - margin_right - boxWidth;
    
    // Debug the initial position
    printf("oldplotter: Initial boxX=%d, width=%d\n", boxX, boxWidth);
    
    // Check if we need to shift the box left to avoid overlapping with the logo
    if (hasLogo && logoImage.getWidth() > 0 && logoImage.getHeight() > 0) {
        // Get the scaled logo dimensions
        int scaledLogoWidth = logoImage.getWidth();
        int scaledLogoHeight = logoImage.getHeight();
        
        // Apply scaling logic as in drawLogo
        const int maxLogoSize = 200;
        if (scaledLogoWidth > maxLogoSize || scaledLogoHeight > maxLogoSize) {
            float scale = maxLogoSize / static_cast<float>(std::max(scaledLogoWidth, scaledLogoHeight));
            scaledLogoWidth = static_cast<int>(scaledLogoWidth * scale);
            scaledLogoHeight = static_cast<int>(scaledLogoHeight * scale);
        }
        
        printf("oldplotter: Logo dimensions: %d x %d\n", scaledLogoWidth, scaledLogoHeight);
        
        // Logo is positioned at: (width - logoWidth - 20, 20)
        int logoLeft = width - scaledLogoWidth - 20;
        int logoBottom = 20 + scaledLogoHeight;
        
        printf("oldplotter: Logo position: left=%d, bottom=%d\n", logoLeft, logoBottom);
        
        // If the stats box would overlap with the logo, shift it left
        if (boxX + boxWidth >= logoLeft && legendY <= logoBottom) {
            // Allow 20px padding between logo and stats box
            int newBoxX = logoLeft - boxWidth - 20;
            // Ensure box is not positioned off-screen
            newBoxX = std::max(newBoxX, margin_left);
            printf("oldplotter: Moving box from x=%d to x=%d to avoid logo\n", boxX, newBoxX);
            boxX = newBoxX;
        }
    }
    
    // Safety check - ensure the box is within the chart area
    if (boxX < margin_left) {
        boxX = margin_left;
    }
    if (boxX + boxWidth > width - margin_right) {
        boxX = width - margin_right - boxWidth;
    }
    
    int boxY = legendY; // Same position as legend
    
    // Final box position
    printf("oldplotter: Final histogram stats box: x=%d, y=%d\n", boxX, boxY);
    
    // Use our common info box method for consistent styling
    drawInfoBox(boxX, boxY, boxWidth, boxHeight, "", fontSize);
    
    // Draw the text with styling
    RGBA textColor = elementColors["legend"]; // White text
    
    // Main title
    drawText(boxX + sidePadding, boxY + 20, "Histogram Statistics", textColor, 22, false);
    
    // Horizontal separator
    RGBA lineColor(0xFF, 0xFF, 0xFF, 0x40); // 25% opacity white
    drawLine(scaleX(boxX + sidePadding), scaleY(boxY + 32), 
             scaleX(boxX + boxWidth - sidePadding), scaleY(boxY + 32), lineColor, ssaaFactor);
    
    // Calculate positions for each stat to evenly distribute them
    int statY = boxY + 55;
    int contentWidth = boxWidth - (sidePadding * 2);
    int usedWidth = meanWidth + modeWidth + maxWidth + sumWidth;
    int extraSpace = contentWidth - usedWidth;
    int spacing = extraSpace / 3; // Three spaces between four stats
    
    // Draw stats with dynamically calculated positions
    int statX = boxX + sidePadding;
    drawText(statX, statY, meanText, textColor, 20, false);
    
    statX += meanWidth + spacing;
    drawText(statX, statY, modeText, textColor, 20, false);
    
    statX += modeWidth + spacing;
    drawText(statX, statY, maxText, textColor, 20, false);
    
    statX += maxWidth + spacing;
    drawText(statX, statY, sumText, textColor, 20, false);
}

// Overload for backward compatibility
void Plotter::drawHistogramStats(const std::vector<int>& bins, int maxBinValue, unsigned int fontSize)
{
    drawHistogramStats(bins, maxBinValue, margin_top, fontSize);
}

// Modify vertical text rendering method for supersampling
void Plotter::drawVerticalText(const std::string& text, int x, int y, int fontSize, const RGBA& color)
{
    // Scale coordinates and font size for supersampling
    int ssaaX = scaleX(x);
    int ssaaY = scaleY(y);
    int ssaaFontSize = fontSize * ssaaFactor;
    
    // Set the font size
    if (FT_Set_Pixel_Sizes(face, 0, ssaaFontSize)) {
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
    int drawX = ssaaX;
    int drawY = ssaaY + textWidth/2; // Center vertically based on text width
    
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
                    
                    if (rotatedX >= 0 && rotatedX < static_cast<int>(ssaaWidth) &&
                        rotatedY >= 0 && rotatedY < static_cast<int>(ssaaHeight)) {
                        // Calculate alpha-blended color
                        float alpha = value / 255.0f;
                        RGBA blendedColor;
                        blendedColor.r = static_cast<unsigned char>(color.r * alpha);
                        blendedColor.g = static_cast<unsigned char>(color.g * alpha);
                        blendedColor.b = static_cast<unsigned char>(color.b * alpha);
                        blendedColor.a = static_cast<unsigned char>(color.a * alpha);
                        
                        // Get current pixel and blend with the glyph
                        RGBA currentPixel = ssaaImage.GetPixel(rotatedX, rotatedY);
                        RGBA finalColor = blendRGBA(currentPixel, blendedColor);
                        ssaaImage.SetPixel(rotatedX, rotatedY, finalColor);
                    }
                }
            }
        }
        
        // Move "down" for next character (which is actually moving left in rotated space)
        drawY -= (glyph->advance.x >> 6);
    }
}

// Generic range calculation template
template<typename DataType, typename ValueFunction>
Plotter::AxisRange Plotter::calculateRange(const DataType& data, ValueFunction valueFunc)
{
    AxisRange range;
    
    // Return default range if data is empty or doesn't meet conditions
    if (valueFunc.isEmptyData(data)) {
        return range;
    }
    
    // Initialize min and max values from first element
    range.min = range.max = valueFunc.getValue(data, 0);
    
    // Find min and max values in the data
    for (size_t i = 1; i < valueFunc.getSize(data); ++i) {
        if (valueFunc.isValidIndex(data, i)) {
            double value = valueFunc.getValue(data, i);
            range.min = std::min(range.min, value);
            range.max = std::max(range.max, value);
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

// Implement the functor methods for Point X values
bool Plotter::PointXValueFunctor::isEmptyData(const std::vector<Point>& data) const {
    return data.empty();
}

size_t Plotter::PointXValueFunctor::getSize(const std::vector<Point>& data) const {
    return data.size();
}

bool Plotter::PointXValueFunctor::isValidIndex(const std::vector<Point>& data, size_t i) const {
    return i < data.size(); // Always true for vector
}

double Plotter::PointXValueFunctor::getValue(const std::vector<Point>& data, size_t i) const {
    return data[i].x;
}

// Implement the functor methods for Point Y values
bool Plotter::PointYValueFunctor::isEmptyData(const std::vector<Point>& data) const {
    return data.empty();
}

size_t Plotter::PointYValueFunctor::getSize(const std::vector<Point>& data) const {
    return data.size();
}

bool Plotter::PointYValueFunctor::isValidIndex(const std::vector<Point>& data, size_t i) const {
    return i < data.size(); // Always true for vector
}

double Plotter::PointYValueFunctor::getValue(const std::vector<Point>& data, size_t i) const {
    return data[i].y;
}

// Implement the functor methods for Matrix X values
bool Plotter::MatrixXValueFunctor::isEmptyData(const std::vector<std::vector<double> >& data) const {
    return data.empty() || data[0].empty();
}

size_t Plotter::MatrixXValueFunctor::getSize(const std::vector<std::vector<double> >& data) const {
    return data.size();
}

bool Plotter::MatrixXValueFunctor::isValidIndex(const std::vector<std::vector<double> >& data, size_t i) const {
    return i < data.size() && !data[i].empty();
}

double Plotter::MatrixXValueFunctor::getValue(const std::vector<std::vector<double> >& data, size_t i) const {
    return data[i][0];
}

// Implement the functor methods for Matrix Y values
bool Plotter::MatrixYValueFunctor::isEmptyData(const std::vector<std::vector<double> >& data) const {
    return data.empty() || data[0].size() < 2;
}

size_t Plotter::MatrixYValueFunctor::getSize(const std::vector<std::vector<double> >& data) const {
    return data.size();
}

bool Plotter::MatrixYValueFunctor::isValidIndex(const std::vector<std::vector<double> >& data, size_t i) const {
    return i < data.size() && data[i].size() > 1;
}

double Plotter::MatrixYValueFunctor::getValue(const std::vector<std::vector<double> >& data, size_t i) const {
    return data[i][1];
}

Plotter::AxisRange Plotter::calculateXRange(const std::vector<Point>& points)
{
    // Use the class-level functor
    PointXValueFunctor func;
    return calculateRange(points, func);
}

Plotter::AxisRange Plotter::calculateYRange(const std::vector<Point>& points)
{
    // Use the class-level functor
    PointYValueFunctor func;
    return calculateRange(points, func);
}

Plotter::AxisRange Plotter::calculateXRange(const std::vector<std::vector<double> >& data)
{
    // Use the class-level functor
    MatrixXValueFunctor func;
    return calculateRange(data, func);
}

Plotter::AxisRange Plotter::calculateYRange(const std::vector<std::vector<double> >& data)
{
    // Use the class-level functor
    MatrixYValueFunctor func;
    return calculateRange(data, func);
}

Plotter::Point Plotter::mapDataToScreen(double x, double y, const AxisRange& xRange, const AxisRange& yRange)
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
    
    // Ensure the point is within the plot bounds in output space (not supersampled)
    screenX = clamp(screenX, margin_left, width - margin_right);
    screenY = clamp(screenY, margin_top, height - margin_bottom);
    
    return Point(screenX, screenY);
}

// Helper for blending colors with alpha
RGBA Plotter::blendColors(const RGBA& baseColor, const RGBA& overlayColor, float alpha) {
    return RGBA(
        static_cast<unsigned char>(baseColor.r * (1.0f - alpha) + overlayColor.r * alpha),
        static_cast<unsigned char>(baseColor.g * (1.0f - alpha) + overlayColor.g * alpha),
        static_cast<unsigned char>(baseColor.b * (1.0f - alpha) + overlayColor.b * alpha),
        baseColor.a  // Keep the original alpha
    );
}

// Helper for blending a pixel with bounds checking
void Plotter::blendPixel(int x, int y, const RGBA& color, float alpha) {
    if (x >= 0 && x < static_cast<int>(ssaaWidth) && y >= 0 && y < static_cast<int>(ssaaHeight)) {
        RGBA baseColor = ssaaImage.GetPixel(x, y);
        RGBA blendedColor = blendColors(baseColor, color, alpha);
        ssaaImage.SetPixel(x, y, blendedColor);
    }
}

// Common method for drawing info boxes (legend, stats, price info)
void Plotter::drawInfoBox(int x, int y, int boxWidth, int boxHeight, const std::string& text, unsigned int fontSize) {
    // Scale coordinates and dimensions for supersampling
    int ssaaX = scaleX(x);
    int ssaaY = scaleY(y);
    int ssaaBoxWidth = scaleSize(boxWidth);
    int ssaaBoxHeight = scaleSize(boxHeight);
    int ssaaCornerRadius = scaleSize(8); // Common corner radius for all info boxes, scaled 
    
    // Get gradient colors from element colors - exact colors from CSS
    RGBA bgTopColor = elementColors["legendBgTop"];
    RGBA bgBottomColor = elementColors["legendBgBottom"];
    
    // Draw gradient background with proper alpha blending
    for (int dy = 0; dy < ssaaBoxHeight; dy++) {
        // Calculate gradient interpolation - diagonal gradient to match the histogram stats box
        for (int dx = 0; dx < ssaaBoxWidth; dx++) {
            float gradPos = (dx + dy) / static_cast<float>(ssaaBoxWidth + ssaaBoxHeight);
            RGBA currentBgColor(
                static_cast<unsigned char>(bgTopColor.r * (1.0f - gradPos) + bgBottomColor.r * gradPos),
                static_cast<unsigned char>(bgTopColor.g * (1.0f - gradPos) + bgBottomColor.g * gradPos),
                static_cast<unsigned char>(bgTopColor.b * (1.0f - gradPos) + bgBottomColor.b * gradPos),
                0xE6  // 90% opacity like the histogram stats box
            );
            
            // Check if this pixel is in the rounded corner region
            bool inCorner = false;
            
            // Check top-left corner
            if (dx < ssaaCornerRadius && dy < ssaaCornerRadius) {
                float distSquared = std::pow(ssaaCornerRadius - dx, 2) + std::pow(ssaaCornerRadius - dy, 2);
                inCorner = distSquared > std::pow(ssaaCornerRadius, 2);
            }
            // Check top-right corner
            else if (dx >= ssaaBoxWidth - ssaaCornerRadius && dy < ssaaCornerRadius) {
                float distSquared = std::pow(dx - (ssaaBoxWidth - ssaaCornerRadius), 2) + std::pow(ssaaCornerRadius - dy, 2);
                inCorner = distSquared > std::pow(ssaaCornerRadius, 2);
            }
            // Check bottom-left corner
            else if (dx < ssaaCornerRadius && dy >= ssaaBoxHeight - ssaaCornerRadius) {
                float distSquared = std::pow(ssaaCornerRadius - dx, 2) + std::pow(dy - (ssaaBoxHeight - ssaaCornerRadius), 2);
                inCorner = distSquared > std::pow(ssaaCornerRadius, 2);
            }
            // Check bottom-right corner
            else if (dx >= ssaaBoxWidth - ssaaCornerRadius && dy >= ssaaBoxHeight - ssaaCornerRadius) {
                float distSquared = std::pow(dx - (ssaaBoxWidth - ssaaCornerRadius), 2) + std::pow(dy - (ssaaBoxHeight - ssaaCornerRadius), 2);
                inCorner = distSquared > std::pow(ssaaCornerRadius, 2);
            }
            
            int drawX = ssaaX + dx;
            int drawY = ssaaY + dy;
            
            if (!inCorner && drawX >= 0 && drawX < static_cast<int>(ssaaWidth) &&
                drawY >= 0 && drawY < static_cast<int>(ssaaHeight)) {
                // Use blendPixel for proper alpha blending
                blendPixel(drawX, drawY, currentBgColor, currentBgColor.a / 255.0f);
            }
        }
    }
    
    // Add subtle inner highlight to top edge - exact 10% opacity from CSS
    RGBA highlightColor(0xFF, 0xFF, 0xFF, 0x1A); // 10% white (0.1 * 255 = 26 ≈ 0x1A)
    for (int dx = ssaaCornerRadius; dx < ssaaBoxWidth - ssaaCornerRadius; dx++) {
        int pixelX = ssaaX + dx;
        int pixelY = ssaaY;
        blendPixel(pixelX, pixelY, highlightColor, highlightColor.a / 255.0f);
    }
    
    // Add subtle drop shadow to bottom edge - exact 15% opacity from CSS
    RGBA shadowColor(0x00, 0x00, 0x00, 0x26); // 15% black (0.15 * 255 = 38 ≈ 0x26)
    for (int dx = ssaaCornerRadius; dx < ssaaBoxWidth - ssaaCornerRadius; dx++) {
        int pixelX = ssaaX + dx;
        int pixelY = ssaaY + ssaaBoxHeight - 1;
        blendPixel(pixelX, pixelY, shadowColor, shadowColor.a / 255.0f);
    }
    
    // Draw the text if provided
    if (!text.empty()) {
        // Text position using original (non-supersampled) coordinates
        drawText(x + 15, y + boxHeight/2, text, elementColors["legend"], fontSize, false);
    }
}

// Common method for drawing Y-axis ticks and labels
void Plotter::drawYAxisTicks(double minValue, double maxValue, int numTicks, bool isInteger, 
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
            // Draw tick mark at right side of the graph
            drawLine(scaleX(width - margin_right), scaleY(y), 
                     scaleX(width - margin_right + 6), scaleY(y), 
                     textColor, ssaaFactor);
            
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
        
        // Draw value label on the right side of the graph
        // Increased font size for better mobile readability
        drawText(width - margin_right + labelOffset, y, valueText, textColor, 24, false);
    }
}

// Draw X-axis ticks with text labels
void Plotter::drawXAxisTicks(const std::vector<std::string>& labels, int numTicks) {
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
        drawLine(scaleX(x), scaleY(height - margin_bottom), 
                 scaleX(x), scaleY(height - margin_bottom + 6), 
                 textColor, ssaaFactor);
        
        // Draw label with proper spacing and alignment from CSS
        int labelY = height - margin_bottom + 25; // More spacing as in CSS
        // Increased font size from 14 to 22 for better mobile readability
        drawText(x, labelY, labels[i], textColor, 22, true);
    }
}

// Draw X-axis ticks with numeric values
void Plotter::drawXAxisTicks(double minValue, double maxValue, int numTicks, int precision) {
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
        drawLine(scaleX(x), scaleY(height - margin_bottom), 
                 scaleX(x), scaleY(height - margin_bottom + 6), 
                 textColor, ssaaFactor);
        
        // Format the value with consistent precision from CSS
        char valueText[32];
        char formatStr[10];
        std::sprintf(formatStr, "%%.%df", precision);
        std::sprintf(valueText, formatStr, value);
        
        // Draw value label with proper spacing and alignment from CSS
        int labelY = height - margin_bottom + 25; // More spacing as in CSS
        // Increased font size from 14 to 22 for better mobile readability
        drawText(x, labelY, valueText, textColor, 22, true);
    }
}

// Helper for common chart initialization steps
void Plotter::initializeChart(const std::string& title, unsigned int titleFontSize) {
    // Draw the complete background first to ensure proper layering
    drawBackground();
    
    // Add title if provided and fontSize > 0
    if (!title.empty() && titleFontSize > 0) {
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

// Common setup for all chart types - reduces redundancy in plotting functions
void Plotter::setupChart(const ChartConfig& config, unsigned int* originalTopMargin, unsigned int* originalRightMargin, 
                         unsigned int newRightMargin, int* titleY, int* legendY) {
    // Store original margins
    if (originalTopMargin)
        *originalTopMargin = margin_top;
    if (originalRightMargin)
        *originalRightMargin = margin_right;
    
    // Adjust right margin for axis labels
    margin_right = newRightMargin;
    
    // Initialize the canvas
    prepareCanvas();
    
    // Set default titleY if needed and provided
    if (titleY) {
        *titleY = 30; // Standard position below top margin
        
        // Add title
        drawText(margin_left, *titleY, config.title, elementColors["title"], config.titleFontSize, false);
        
        // Calculate legendY position if requested
        if (legendY) {
            int titleHeight = config.titleFontSize - 10;
            *legendY = *titleY + titleHeight - 20; // Position legend closely below title
        }
    }
}

void Plotter::plotHistogram(const std::vector<int>& bins, const RGBA& color)
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
    ChartConfig config("Data Distribution", 42, "Value", "Frequency", 32);
    
    // Variables for margin storage and positioning
    unsigned int originalTopMargin, originalRightMargin;
    int titleY, legendY;
    
    // Use common setup for chart initialization
    setupChart(config, &originalTopMargin, &originalRightMargin, 180, &titleY, &legendY);
    
    // Find the maximum value in bins for scaling
    int maxBinValue = *std::max_element(bins.begin(), bins.end());
    if (maxBinValue == 0) maxBinValue = 1; // Avoid division by zero
    
    // Create our legend labels
    std::vector<std::string> legendLabels;
    legendLabels.push_back("Frequency");
    
    std::vector<RGBA> legendColors;
    legendColors.push_back(useColor);
    
    // Process legend colors
    std::vector<std::string> processedLabels;
    std::vector<RGBA> processedColors;
    prepareLegendColors(legendLabels, legendColors, processedLabels, processedColors);
    
    // Estimate legend height
    int estimatedLegendHeight = calculateInfoBoxHeight(processedLabels, 18);
    
    // Estimate stats box height (histogram stats has a fixed height of about 120px)
    int statsBoxHeight = 120;
    int statsBoxMaxY = legendY + statsBoxHeight;
    
    // Dynamically set the top margin to accommodate title, legend, and stats box with a small buffer
    margin_top = std::max(legendY + estimatedLegendHeight, statsBoxMaxY) + 15; // 15px buffer
    
    // Redraw with proper margins
    prepareCanvas();
    
    // Redraw title after prepareCanvas
    drawText(margin_left, titleY, config.title, elementColors["title"], config.titleFontSize, false);
    
    // Draw the legend
    int actualLegendHeight = addLegend(processedLabels, processedColors, margin_left, legendY, 18);
    
    // Draw statistics info box with matching CSS styling - positioned right of the legend
    drawHistogramStats(bins, maxBinValue, legendY, 16);
    
    // Calculate optimal bar width and spacing based on the CSS design
    // Bars are narrower with more spacing than default
    int totalBars = bins.size();
    float barWidthPercentage = 0.6f; // Bar takes 60% of available space
    float spacingPercentage = 0.4f; // 40% for spacing
    
    int totalBarSpace = getPlotWidth() / totalBars;
    int barWidth = static_cast<int>(totalBarSpace * barWidthPercentage);
    int barSpacing = static_cast<int>(totalBarSpace * spacingPercentage);
    
    // Ensure minimum size and spacing for readability
    barWidth = std::max(barWidth, 40); // Match the 40px width from CSS 
    barSpacing = std::max(barSpacing, 30); // Generous spacing as in CSS
    
    // Draw Y-axis with value labels - more ticks for better scale
    drawYAxisTicks(0, maxBinValue, 5, true, 0, 50);
    
    // Draw histogram bars
    drawHistogramBars(bins, maxBinValue, totalBars, barWidth, barSpacing, useColor);
    
    // Draw axis labels using our common method
    drawAxisLabels(config.xAxisLabel, config.yAxisLabel, config.axisFontSize);
    
    // Restore original margins
    margin_right = originalRightMargin;
    margin_top = originalTopMargin;
}

void Plotter::plotCandlestickChart(const std::vector<CandleData>& candles, 
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
    
    // Create a chart configuration with increased font size for mobile
    ChartConfig config("Financial Data Analysis", 36, "Date", "Price", 32);
    
    // Variables for margin storage and positioning
    unsigned int originalTopMargin, originalRightMargin;
    int titleY, legendY;
    
    // Use common setup for chart initialization
    setupChart(config, &originalTopMargin, &originalRightMargin, 220, &titleY, &legendY);
    
    // Create legend labels and colors
    std::vector<std::string> legendLabels;
    legendLabels.push_back("Bullish Candle");
    legendLabels.push_back("Bearish Candle");
    
    std::vector<RGBA> legendColors;
    legendColors.push_back(useBullishColor);
    legendColors.push_back(useBearishColor);
    
    // Process legend colors
    std::vector<std::string> processedLabels;
    std::vector<RGBA> processedColors;
    prepareLegendColors(legendLabels, legendColors, processedLabels, processedColors);
    
    // Estimate legend height
    int estimatedLegendHeight = calculateInfoBoxHeight(processedLabels, 16);
    
    // Estimate price info box height (typically around 40px height)
    int infoBoxHeight = 40;
    int infoBoxMaxY = legendY + infoBoxHeight;
    
    // Dynamically set the top margin to accommodate title, legend, and price info box with a buffer
    margin_top = std::max(legendY + estimatedLegendHeight, infoBoxMaxY) + 15; // 15px buffer
    
    // Redraw with proper margins
    prepareCanvas();
    
    // Redraw title after prepareCanvas
    drawText(margin_left, titleY, config.title, elementColors["title"], config.titleFontSize, false);
    
    // Draw the legend
    int actualLegendHeight = addLegend(processedLabels, processedColors, margin_left, legendY, 16);
    
    // Add price movement indicators and current price display - positioned next to legend
    drawCandlestickPriceInfo(candles, useBullishColor, useBearishColor, legendY);
    
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
    drawCandlestickYAxis(minPrice, maxPrice, 5); // Increased from 3 to 5 ticks
    
    // Draw X-axis with date labels
    drawCandlestickXAxis(candles, maxVisibleCandles, totalCandles, firstTimestamp, lastTimestamp);
    
    // Draw axis labels using our common method
    drawAxisLabels(config.xAxisLabel, config.yAxisLabel, config.axisFontSize);
    
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
    
    // Restore original margins
    margin_right = originalRightMargin;
    margin_top = originalTopMargin;
}

void Plotter::drawCandlestickYAxis(double minPrice, double maxPrice, int numTicks)
{
    // Use exactly 5 ticks with consistent spacing for better distribution 
    numTicks = 5;
    
    // Use our common Y-axis method with 2 decimal places and increased font size and offset
    drawYAxisTicks(minPrice, maxPrice, numTicks, false, 2, 70);
}

void Plotter::drawCandlestickXAxis(const std::vector<CandleData>& candles, int maxVisibleCandles, 
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
        drawLine(scaleX(x), scaleY(margin_top), 
                scaleX(x), scaleY(height - margin_bottom), 
                gridColor, ssaaFactor);
        
        // Format timestamp into readable date
        char dateText[32];
        std::time_t time = static_cast<std::time_t>(timestamp);
        struct tm* timeinfo = std::localtime(&time);
        std::strftime(dateText, sizeof(dateText), "%m/%d", timeinfo);
        
        // Draw date label with increased font size for mobile readability
        drawText(x, height - margin_bottom + 20, dateText, elementColors["axisLabel"], 24, true);
    }
}

// Update candlestick price info to use dynamic spacing
void Plotter::drawCandlestickPriceInfo(const std::vector<CandleData>& candles,
                                       const RGBA& bullishColor, const RGBA& bearishColor,
                                       int legendY)
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
    
    // Format price info components
    char closeText[64], changeText[64], percentText[64];
    std::sprintf(closeText, "Close: %.2f", latestCandle.close);
    std::sprintf(changeText, "Change: %.2f", priceChange);
    std::sprintf(percentText, "(%.2f%%)", percentChange);
    
    // Calculate required widths
    int closeWidth = estimateTextWidth(closeText, 22);
    int changeWidth = estimateTextWidth(changeText, 22);
    int percentWidth = estimateTextWidth(percentText, 22);
    
    // Calculate minimum spacing between components
    int minComponentSpacing = 30;
    int indicatorSize = 8;
    int indicatorSpace = 40; // Space for indicator including padding
    
    // Calculate box dimensions
    int sidePadding = 20;
    int boxWidth = sidePadding * 2 + closeWidth + changeWidth + percentWidth + indicatorSpace + 
                 minComponentSpacing * 2; // Two spaces between three components plus indicator
    int boxHeight = 40;
    
    int boxX = margin_left + 250; // Left-aligned with the chart
    int boxY = legendY; // Same position as legend
    
    // Draw the info box
    drawInfoBox(boxX, boxY, boxWidth, boxHeight, "", 22);
    
    // Calculate positions for each component to evenly distribute them
    int textY = boxY + (boxHeight / 2);
    int contentWidth = boxWidth - (sidePadding * 2) - indicatorSpace;
    int usedWidth = closeWidth + changeWidth + percentWidth;
    int extraSpace = contentWidth - usedWidth;
    int spacing = extraSpace / 2; // Two spaces between three components
    
    // Draw components with dynamically calculated positions
    int textX = boxX + sidePadding;
    drawText(textX, textY, closeText, elementColors["legend"], 22, false);
    
    textX += closeWidth + spacing;
    drawText(textX, textY, changeText, trendColor, 22, false);
    
    textX += changeWidth + spacing;
    drawText(textX, textY, percentText, trendColor, 22, false);
    
    // Add a small colored indicator box to show trend
    int indicatorX = boxX + boxWidth - sidePadding - indicatorSize;
    int indicatorY = boxY + (boxHeight - indicatorSize) / 2;
    
    // Draw filled rectangle with the appropriate color
    drawRect(scaleX(indicatorX), scaleY(indicatorY), 
             scaleSize(indicatorSize), scaleSize(indicatorSize), 
             trendColor, true);
}

// Overload for backward compatibility
void Plotter::drawCandlestickPriceInfo(const std::vector<CandleData>& candles,
                                        const RGBA& bullishColor, const RGBA& bearishColor)
{
    drawCandlestickPriceInfo(candles, bullishColor, bearishColor, margin_top);
}

void Plotter::drawHistogramYAxis(int maxValue, int numTicks)
{
    // Use our common Y-axis method with integer values and increased font size
    drawYAxisTicks(0, maxValue, numTicks, true, 0, 35);
}

void Plotter::drawHistogramBars(const std::vector<int>& bins, int maxBinValue, int totalBars, 
                               int barWidth, int barSpacing, const RGBA& color)
{
    int plotHeight = getPlotHeight();
    
    // Calculate start X position to center the bars - match histogram_with_labels.css
    int startX = margin_left + (getPlotWidth() - (totalBars * (barWidth + barSpacing) - barSpacing)) / 2;
    
    // Scale dimensions for supersampling
    int ssaaBarWidth = scaleSize(barWidth);
    int ssaaBarSpacing = scaleSize(barSpacing);
    int ssaaStartX = scaleX(startX);
    
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
        
        // Scale bar height for supersampling
        int ssaaBarHeight = scaleSize(barHeight);
        
        // Calculate bar position (in supersampled coordinates)
        int x = ssaaStartX + i * (ssaaBarWidth + ssaaBarSpacing);
        int y = ssaaHeight - scaleY(margin_bottom) - ssaaBarHeight;
        
        // Draw the bar with a vertical gradient - matching CSS
        for (int dy = 0; dy < ssaaBarHeight; ++dy) {
            // Calculate gradient position (0 at top, 1 at bottom)
            float gradientPos = static_cast<float>(dy) / ssaaBarHeight;
            
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
            for (int dx = 0; dx < ssaaBarWidth; ++dx) {
                int drawX = x + dx;
                int drawY = y + dy;
                
                if (drawX >= 0 && drawX < static_cast<int>(ssaaWidth) &&
                    drawY >= 0 && drawY < static_cast<int>(ssaaHeight)) {
                    ssaaImage.SetPixel(drawX, drawY, gradientColor);
                }
            }
        }
        
        // Add 3D effect with highlights and shadows (exactly like CSS)
        drawHistogramBarHighlights(x, y, ssaaBarWidth, ssaaBarHeight);
        
        // Add bar value label on top of taller bars - similar to histogram_with_labels.css
        if (barHeight > 45) { // Only for sufficiently tall bars
            char valueText[16];
            std::sprintf(valueText, "%d", bins[i]);
            
            // Position above the bar with proper spacing (use unscaled coordinates as drawText will scale)
            // Increased font size from 16 to 22 for better mobile readability
            drawText(startX + i * (barWidth + barSpacing) + barWidth / 2, 
                    height - margin_bottom - barHeight - 20, 
                    valueText, elementColors["legend"], 22, true);
        }
        
        // Add bottom label for bars - exactly like histogram_with_labels.css
        // Only add labels for some bars to prevent crowding
        if (i % 4 == 0 || i == bins.size() - 1) {
            char labelText[16];
            std::sprintf(labelText, "%zu", i);
            RGBA labelColor = elementColors["axisLabel"];
            
            // Exact positioning from CSS (use unscaled coordinates as drawText will scale)
            // Increased font size from 14 to 24 for better mobile readability
            drawText(startX + i * (barWidth + barSpacing) + barWidth / 2, 
                    height - margin_bottom + 20, 
                    labelText, labelColor, 24, true);
        }
    }
}

void Plotter::drawHistogramBarHighlights(int x, int y, int barWidth, int barHeight)
{
    // Enhanced styling for histogram bars to match CSS
    // Note: x, y, barWidth, barHeight are already scaled for supersampling at this point
    
    // Colors for highlights and shadows - more pronounced
    RGBA highlightColor(0xFF, 0xFF, 0xFF, 0x60); // Semi-transparent white (38% opacity)
    RGBA shadowColor(0x00, 0x00, 0x00, 0x60);    // Semi-transparent black (38% opacity)
    
    // Calculate highlight/shadow widths proportional to bar width
    int edgeWidth = std::max(3 * static_cast<int>(ssaaFactor), barWidth / 10);
    
    // Left edge highlight with gradient fade - brighter near edge
    for (int dy = 0; dy < barHeight; ++dy) {
        for (int dx = 0; dx < edgeWidth; ++dx) {
            // Calculate fade strength - more intense at edge, fades toward center
            float alpha = (edgeWidth - dx) / static_cast<float>(edgeWidth) * 0.6f;
            int drawX = x + dx;
            int drawY = y + dy;
            
            if (drawX >= 0 && drawX < static_cast<int>(ssaaWidth) &&
                drawY >= 0 && drawY < static_cast<int>(ssaaHeight)) {
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
            
            if (drawX >= 0 && drawX < static_cast<int>(ssaaWidth) &&
                drawY >= 0 && drawY < static_cast<int>(ssaaHeight)) {
                RGBA pixelShadow = shadowColor;
                pixelShadow.a = static_cast<unsigned char>(255 * alpha);
                blendPixel(drawX, drawY, pixelShadow, alpha);
            }
        }
    }
    
    // Top edge highlight for 3D effect - subtle rounded top
    float topHighlightAlpha = 0.5f;
    int topEdgeHeight = std::max(2 * static_cast<int>(ssaaFactor), barHeight / 20);
    
    for (int dx = edgeWidth; dx < barWidth - edgeWidth; ++dx) {
        for (int dy = 0; dy < topEdgeHeight; ++dy) {
            // Fade from top edge downward
            float alpha = (topEdgeHeight - dy) / static_cast<float>(topEdgeHeight) * topHighlightAlpha;
            int drawX = x + dx;
            int drawY = y + dy;
            
            if (drawX >= 0 && drawX < static_cast<int>(ssaaWidth) &&
                drawY >= 0 && drawY < static_cast<int>(ssaaHeight)) {
                RGBA pixelHighlight = highlightColor;
                pixelHighlight.a = static_cast<unsigned char>(255 * alpha);
                blendPixel(drawX, drawY, pixelHighlight, alpha);
            }
        }
    }
    
    // Bottom edge shadow for 3D effect - subtle rounded bottom
    float bottomShadowAlpha = 0.5f;
    int bottomEdgeHeight = std::max(2 * static_cast<int>(ssaaFactor), barHeight / 20);
    
    for (int dx = edgeWidth; dx < barWidth - edgeWidth; ++dx) {
        for (int dy = 0; dy < bottomEdgeHeight; ++dy) {
            // Fade from bottom edge upward
            float alpha = (bottomEdgeHeight - dy) / static_cast<float>(bottomEdgeHeight) * bottomShadowAlpha;
            int drawX = x + dx;
            int drawY = y + barHeight - dy - 1;
            
            if (drawX >= 0 && drawX < static_cast<int>(ssaaWidth) &&
                drawY >= 0 && drawY < static_cast<int>(ssaaHeight)) {
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
            
            if (drawX >= 0 && drawX < static_cast<int>(ssaaWidth) &&
                drawY >= 0 && drawY < static_cast<int>(ssaaHeight)) {
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
            
            if (drawX >= 0 && drawX < static_cast<int>(ssaaWidth) &&
                drawY >= 0 && drawY < static_cast<int>(ssaaHeight)) {
                RGBA pixelShadow = shadowColor;
                pixelShadow.a = static_cast<unsigned char>(255 * alpha);
                blendPixel(drawX, drawY, pixelShadow, alpha);
            }
        }
    }
}

// Helper method to draw rounded corners for the grid border with anti-aliasing
void Plotter::drawRoundedCorners(int left, int top, int right, int bottom, int radius, const RGBA& color)
{
    // Note: input coordinates are already scaled for supersampling
    
    // Pre-calculate alpha value for consistent transparency
    float baseAlpha = color.a / 255.0f;
    
    // Draw the four corners with sub-pixel precision for better quality
    const float stepSize = 0.2f; // 0.2 pixel steps for accurate anti-aliasing
    
    // Top-left corner
    for (float dy = 0; dy <= radius; dy += stepSize) {
        for (float dx = 0; dx <= radius; dx += stepSize) {
            // Calculate precise distance from corner
            float dist = std::sqrt((radius - dx) * (radius - dx) + (radius - dy) * (radius - dy));
            
            // Apply anti-aliasing at the edge: 1.0 at border, fading over 1 pixel
            float alpha = 0.0f;
            if (dist <= radius + 1) {
                if (dist > radius) {
                    // Fade out over a 1-pixel border for anti-aliasing
                    alpha = 1.0f - (dist - radius);
                } else {
                    // Full opacity inside the border
                    alpha = 1.0f;
                }
                alpha = std::max(0.0f, std::min(1.0f, alpha)) * baseAlpha;
                
                // Only draw if we have some opacity
                if (alpha > 0.01f) {
                    int px = static_cast<int>(left + radius - dx);
                    int py = static_cast<int>(top + radius - dy);
                    
                    if (px >= 0 && px < static_cast<int>(ssaaWidth) && 
                        py >= 0 && py < static_cast<int>(ssaaHeight)) {
                        
                        // Create color with calculated alpha
            RGBA pixelColor = color;
                        pixelColor.a = static_cast<unsigned char>(255.0f * alpha);
                        
                        // Blend with existing pixel
                        RGBA existingColor = ssaaImage.GetPixel(px, py);
                        RGBA blendedColor = blendRGBA(existingColor, pixelColor);
                        ssaaImage.SetPixel(px, py, blendedColor);
                    }
                }
            }
        }
    }
    
    // Top-right corner
    for (float dy = 0; dy <= radius; dy += stepSize) {
        for (float dx = 0; dx <= radius; dx += stepSize) {
            float dist = std::sqrt((radius - dx) * (radius - dx) + (radius - dy) * (radius - dy));
            
            float alpha = 0.0f;
            if (dist <= radius + 1) {
                if (dist > radius) {
                    alpha = 1.0f - (dist - radius);
                } else {
                    alpha = 1.0f;
                }
                alpha = std::max(0.0f, std::min(1.0f, alpha)) * baseAlpha;
                
                if (alpha > 0.01f) {
                    int px = static_cast<int>(right - radius + dx);
                    int py = static_cast<int>(top + radius - dy);
                    
                    if (px >= 0 && px < static_cast<int>(ssaaWidth) && 
                        py >= 0 && py < static_cast<int>(ssaaHeight)) {
                        
            RGBA pixelColor = color;
                        pixelColor.a = static_cast<unsigned char>(255.0f * alpha);
                        RGBA existingColor = ssaaImage.GetPixel(px, py);
                        RGBA blendedColor = blendRGBA(existingColor, pixelColor);
                        ssaaImage.SetPixel(px, py, blendedColor);
                    }
                }
            }
        }
    }
    
    // Bottom-left corner
    for (float dy = 0; dy <= radius; dy += stepSize) {
        for (float dx = 0; dx <= radius; dx += stepSize) {
            float dist = std::sqrt((radius - dx) * (radius - dx) + (radius - dy) * (radius - dy));
            
            float alpha = 0.0f;
            if (dist <= radius + 1) {
                if (dist > radius) {
                    alpha = 1.0f - (dist - radius);
                } else {
                    alpha = 1.0f;
                }
                alpha = std::max(0.0f, std::min(1.0f, alpha)) * baseAlpha;
                
                if (alpha > 0.01f) {
                    int px = static_cast<int>(left + radius - dx);
                    int py = static_cast<int>(bottom - radius + dy);
                    
                    if (px >= 0 && px < static_cast<int>(ssaaWidth) && 
                        py >= 0 && py < static_cast<int>(ssaaHeight)) {
                        
            RGBA pixelColor = color;
                        pixelColor.a = static_cast<unsigned char>(255.0f * alpha);
                        RGBA existingColor = ssaaImage.GetPixel(px, py);
                        RGBA blendedColor = blendRGBA(existingColor, pixelColor);
                        ssaaImage.SetPixel(px, py, blendedColor);
                    }
                }
            }
        }
    }
    
    // Bottom-right corner
    for (float dy = 0; dy <= radius; dy += stepSize) {
        for (float dx = 0; dx <= radius; dx += stepSize) {
            float dist = std::sqrt((radius - dx) * (radius - dx) + (radius - dy) * (radius - dy));
            
            float alpha = 0.0f;
            if (dist <= radius + 1) {
                if (dist > radius) {
                    alpha = 1.0f - (dist - radius);
                } else {
                    alpha = 1.0f;
                }
                alpha = std::max(0.0f, std::min(1.0f, alpha)) * baseAlpha;
                
                if (alpha > 0.01f) {
                    int px = static_cast<int>(right - radius + dx);
                    int py = static_cast<int>(bottom - radius + dy);
                    
                    if (px >= 0 && px < static_cast<int>(ssaaWidth) && 
                        py >= 0 && py < static_cast<int>(ssaaHeight)) {
                        
            RGBA pixelColor = color;
                        pixelColor.a = static_cast<unsigned char>(255.0f * alpha);
                        RGBA existingColor = ssaaImage.GetPixel(px, py);
                        RGBA blendedColor = blendRGBA(existingColor, pixelColor);
                        ssaaImage.SetPixel(px, py, blendedColor);
                    }
                }
            }
        }
    }
}

void Plotter::setYAxisLabel(const std::string& label)
{
    yAxisLabel = label;
    
    // Get text color from CSS design
    RGBA textColor = elementColors["axisLabel"];
    textColor.a = 0xCC; // 80% opacity for readability
    
    // Position for the Y-axis label - adjusted for rotated text
    // Keep the Y-axis label on the left side but shifted right to prevent cutoff
    int labelX = margin_left / 2; // Positioned at half margin instead of one-third
    int labelY = margin_top + getPlotHeight() / 2; // Center vertically
    
    // Draw rotated label with increased font size for mobile readability
    drawVerticalText(yAxisLabel, labelX, labelY, 24, textColor); // Increased from 16 to 24
}

// Load logo image from a file
void Plotter::loadLogo(const std::string& logoPath)
{
    // Try to load the logo image from the file
    try {
        logoImage = Image();
        logoImage.LoadPNG(logoPath.c_str());
        
        if (logoImage.getWidth() > 0 && logoImage.getHeight() > 0) {
            hasLogo = true;
            printf("Logo loaded successfully from: %s\n", logoPath.c_str());
        } else {
            hasLogo = false;
            printf("Failed to load logo from: %s\n", logoPath.c_str());
        }
    } catch (...) {
        hasLogo = false;
        printf("Error loading logo from: %s\n", logoPath.c_str());
    }
}

// Draw the logo in the top right corner
void Plotter::drawLogo()
{
    if (!hasLogo || logoImage.getWidth() == 0 || logoImage.getHeight() == 0) {
        return;  // No logo to draw
    }
    
    // Define position for the logo (top right corner)
    int logoWidth = logoImage.getWidth();
    int logoHeight = logoImage.getHeight();
    
    // Limit logo size to a reasonable maximum (e.g., 200x200)
    const int maxLogoSize = 200;
    if (logoWidth > maxLogoSize || logoHeight > maxLogoSize) {
        // Scale down while maintaining aspect ratio
        float scale = maxLogoSize / static_cast<float>(std::max(static_cast<int>(logoWidth), static_cast<int>(logoHeight)));
        logoWidth = static_cast<int>(logoWidth * scale);
        logoHeight = static_cast<int>(logoHeight * scale);
    }
    
    // Scale logo dimensions for supersampling
    int ssaaLogoWidth = logoWidth * ssaaFactor;
    int ssaaLogoHeight = logoHeight * ssaaFactor;
    
    // Calculate position in top right with padding
    int padX = 20 * ssaaFactor;  // Scale padding for supersampling
    int padY = 20 * ssaaFactor;
    int logoX = ssaaWidth - ssaaLogoWidth - padX;
    int logoY = padY;
    
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
            
            // Clamp source coordinates - handle type mismatch with explicit casting
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
            int dstX = logoX + x;
            int dstY = logoY + y;
            
            // Ensure we're within bounds
            if (dstX >= 0 && dstX < static_cast<int>(ssaaWidth) && 
                dstY >= 0 && dstY < static_cast<int>(ssaaHeight)) {
                // Use alpha blending for proper transparency
                float alpha = logoPixel.a / 255.0f;
                blendPixel(dstX, dstY, logoPixel, alpha);
            }
        }
    }
}

// Public method to initialize the chart with background and grid, but without a title
void Plotter::prepareCanvas() {
    // Draw the background
    drawBackground();
    
    // Draw grid if enabled
    if (showGrid) {
        drawGrid();
    }
    
    // Draw axes if enabled
    if (showAxes) {
        drawAxes();
    }
}

// Helper method to create a cluster legend
int Plotter::createClusterLegend(const std::vector<RGBA>& clusterColors, int numClusters, int x, int y)
{
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
    
    // Add the legend to the visualization using our fixed method
    return addLegend(processedLabels, processedColors, x, y, 16);
}

// Explicit template instantiations required for C++03
template shmea::Plotter::AxisRange shmea::Plotter::calculateRange(const std::vector<shmea::Plotter::Point>&, shmea::Plotter::PointXValueFunctor);
template shmea::Plotter::AxisRange shmea::Plotter::calculateRange(const std::vector<shmea::Plotter::Point>&, shmea::Plotter::PointYValueFunctor);
template shmea::Plotter::AxisRange shmea::Plotter::calculateRange(const std::vector<std::vector<double> >&, shmea::Plotter::MatrixXValueFunctor);
template shmea::Plotter::AxisRange shmea::Plotter::calculateRange(const std::vector<std::vector<double> >&, shmea::Plotter::MatrixYValueFunctor);
