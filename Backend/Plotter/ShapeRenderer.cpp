#include "ShapeRenderer.h"
#include <algorithm>
#include <cmath>

namespace shmea {

ShapeRenderer::ShapeRenderer(SuperSamplingManager& ssaaManager, ColorManager& colorManager, ChartLayout& chartLayout)
    : ssaa(ssaaManager),
      colors(colorManager),
      layout(chartLayout)
{
}

ShapeRenderer::~ShapeRenderer() {
    // Nothing to clean up
}

void ShapeRenderer::blendPixel(int x, int y, const RGBA& color, float alpha) {
    if (x >= 0 && x < static_cast<int>(ssaa.getWidth()) && 
        y >= 0 && y < static_cast<int>(ssaa.getHeight())) {
        RGBA baseColor = ssaa.getImage().GetPixel(x, y);
        RGBA blendedColor = colors.blendColors(baseColor, color, alpha);
        ssaa.getImage().SetPixel(x, y, blendedColor);
    }
}

void ShapeRenderer::drawLine(int x1, int y1, int x2, int y2, const RGBA& lineColor, int lineWidth) {
    // Clamp coordinates to stay within image bounds
    x1 = ChartLayout::clamp(x1, 0, ssaa.getWidth() - 1);
    x2 = ChartLayout::clamp(x2, 0, ssaa.getWidth() - 1);
    y1 = ChartLayout::clamp(y1, 0, ssaa.getHeight() - 1);
    y2 = ChartLayout::clamp(y2, 0, ssaa.getHeight() - 1);
    
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
                
                if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) && 
                    drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
                    
                    // For semi-transparent colors, blend with background
                    if (lineColor.a < 255) {
                        RGBA currentPixel = ssaa.getImage().GetPixel(drawX, drawY);
                        RGBA blendedColor = colors.blendRGBA(currentPixel, lineColor);
                        ssaa.getImage().SetPixel(drawX, drawY, blendedColor);
                    } else {
                        // Fully opaque - just set the pixel
                        ssaa.getImage().SetPixel(drawX, drawY, lineColor);
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

void ShapeRenderer::drawPoint(int x, int y, int size, const RGBA& color) {
    // Draw a filled circle for the point with anti-aliasing
    drawCircle(x, y, size, color, true, 0);
}

void ShapeRenderer::drawCircle(int x, int y, int radius, const RGBA& color, bool filled, int borderWidth) {
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
            if (drawX < 0 || drawX >= static_cast<int>(ssaa.getWidth()) ||
                drawY < 0 || drawY >= static_cast<int>(ssaa.getHeight())) {
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
                RGBA currentPixel = ssaa.getImage().GetPixel(drawX, drawY);
                RGBA blendedColor = colors.blendRGBA(currentPixel, adjustedColor);
                ssaa.getImage().SetPixel(drawX, drawY, blendedColor);
            } else {
                // Full opacity or middle of the circle
                if (color.a < 255) {
                    // If original color is semi-transparent, still need to blend
                    RGBA currentPixel = ssaa.getImage().GetPixel(drawX, drawY);
                    RGBA blendedColor = colors.blendRGBA(currentPixel, color);
                    ssaa.getImage().SetPixel(drawX, drawY, blendedColor);
                } else {
                    // Fully opaque
                    ssaa.getImage().SetPixel(drawX, drawY, color);
                }
            }
        }
    }
}

void ShapeRenderer::drawRect(int x, int y, int rectWidth, int rectHeight, const RGBA& color, bool filled, int borderWidth) {
    // Note: x, y, rectWidth, rectHeight are already in supersampled space
    if (filled) {
        // Draw filled rectangle
        for (int dy = 0; dy < rectHeight; dy++) {
            for (int dx = 0; dx < rectWidth; dx++) {
                int drawX = x + dx;
                int drawY = y + dy;
                
                if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) &&
                    drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
                    // Handle alpha blending for semi-transparent rectangles
                    if (color.a < 255) {
                        RGBA currentPixel = ssaa.getImage().GetPixel(drawX, drawY);
                        RGBA blendedColor = colors.blendRGBA(currentPixel, color);
                        ssaa.getImage().SetPixel(drawX, drawY, blendedColor);
                    } else {
                        // Fully opaque - just set the pixel
                        ssaa.getImage().SetPixel(drawX, drawY, color);
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
                
                if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) &&
                    drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
                    // Handle alpha blending
                    if (color.a < 255) {
                        RGBA currentPixel = ssaa.getImage().GetPixel(drawX, drawY);
                        RGBA blendedColor = colors.blendRGBA(currentPixel, color);
                        ssaa.getImage().SetPixel(drawX, drawY, blendedColor);
                    } else {
                        ssaa.getImage().SetPixel(drawX, drawY, color);
                    }
                }
            }
        }
        
        // Draw bottom border
        for (int dx = 0; dx < rectWidth; dx++) {
            for (int b = 0; b < borderWidth; b++) {
                int drawX = x + dx;
                int drawY = y + rectHeight - b - 1;
                
                if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) &&
                    drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
                    // Handle alpha blending
                    if (color.a < 255) {
                        RGBA currentPixel = ssaa.getImage().GetPixel(drawX, drawY);
                        RGBA blendedColor = colors.blendRGBA(currentPixel, color);
                        ssaa.getImage().SetPixel(drawX, drawY, blendedColor);
                    } else {
                        ssaa.getImage().SetPixel(drawX, drawY, color);
                    }
                }
            }
        }
        
        // Draw left border
        for (int dy = 0; dy < rectHeight; dy++) {
            for (int b = 0; b < borderWidth; b++) {
                int drawX = x + b;
                int drawY = y + dy;
                
                if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) &&
                    drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
                    // Handle alpha blending
                    if (color.a < 255) {
                        RGBA currentPixel = ssaa.getImage().GetPixel(drawX, drawY);
                        RGBA blendedColor = colors.blendRGBA(currentPixel, color);
                        ssaa.getImage().SetPixel(drawX, drawY, blendedColor);
                    } else {
                        ssaa.getImage().SetPixel(drawX, drawY, color);
                    }
                }
            }
        }
        
        // Draw right border
        for (int dy = 0; dy < rectHeight; dy++) {
            for (int b = 0; b < borderWidth; b++) {
                int drawX = x + rectWidth - b - 1;
                int drawY = y + dy;
                
                if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) &&
                    drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
                    // Handle alpha blending
                    if (color.a < 255) {
                        RGBA currentPixel = ssaa.getImage().GetPixel(drawX, drawY);
                        RGBA blendedColor = colors.blendRGBA(currentPixel, color);
                        ssaa.getImage().SetPixel(drawX, drawY, blendedColor);
                    } else {
                        ssaa.getImage().SetPixel(drawX, drawY, color);
                    }
                }
            }
        }
    }
}

void ShapeRenderer::drawRoundedCorners(int left, int top, int right, int bottom, int radius, const RGBA& color) {
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
                    
                    if (px >= 0 && px < static_cast<int>(ssaa.getWidth()) && 
                        py >= 0 && py < static_cast<int>(ssaa.getHeight())) {
                        
                        // Create color with calculated alpha
                        RGBA pixelColor = color;
                        pixelColor.a = static_cast<unsigned char>(255.0f * alpha);
                        
                        // Blend with existing pixel
                        RGBA existingColor = ssaa.getImage().GetPixel(px, py);
                        RGBA blendedColor = colors.blendRGBA(existingColor, pixelColor);
                        ssaa.getImage().SetPixel(px, py, blendedColor);
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
                    
                    if (px >= 0 && px < static_cast<int>(ssaa.getWidth()) && 
                        py >= 0 && py < static_cast<int>(ssaa.getHeight())) {
                        
                        RGBA pixelColor = color;
                        pixelColor.a = static_cast<unsigned char>(255.0f * alpha);
                        RGBA existingColor = ssaa.getImage().GetPixel(px, py);
                        RGBA blendedColor = colors.blendRGBA(existingColor, pixelColor);
                        ssaa.getImage().SetPixel(px, py, blendedColor);
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
                    
                    if (px >= 0 && px < static_cast<int>(ssaa.getWidth()) && 
                        py >= 0 && py < static_cast<int>(ssaa.getHeight())) {
                        
                        RGBA pixelColor = color;
                        pixelColor.a = static_cast<unsigned char>(255.0f * alpha);
                        RGBA existingColor = ssaa.getImage().GetPixel(px, py);
                        RGBA blendedColor = colors.blendRGBA(existingColor, pixelColor);
                        ssaa.getImage().SetPixel(px, py, blendedColor);
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
                    
                    if (px >= 0 && px < static_cast<int>(ssaa.getWidth()) && 
                        py >= 0 && py < static_cast<int>(ssaa.getHeight())) {
                        
                        RGBA pixelColor = color;
                        pixelColor.a = static_cast<unsigned char>(255.0f * alpha);
                        RGBA existingColor = ssaa.getImage().GetPixel(px, py);
                        RGBA blendedColor = colors.blendRGBA(existingColor, pixelColor);
                        ssaa.getImage().SetPixel(px, py, blendedColor);
                    }
                }
            }
        }
    }
}

void ShapeRenderer::drawCandlestick(int x, int y_open, int y_close, int y_high, int y_low, const RGBA& color, int customBodyWidth) {
    // Use the provided body width if available, otherwise use the default
    int bodyWidth = (customBodyWidth > 0) ? 
                    customBodyWidth * ssaa.getSamplingFactor() : 
                    std::max(static_cast<int>(4 * ssaa.getSamplingFactor()), 
                         static_cast<int>(ssaa.getSamplingFactor() * 12));
    
    // Wick thickness should be proportional to body width but not too thin
    int wickThickness = std::max(static_cast<int>(1 * ssaa.getSamplingFactor()),
                             static_cast<int>(bodyWidth / 6));
    
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
    if (bodyBottom - bodyTop < static_cast<int>(4 * ssaa.getSamplingFactor())) {
        bodyBottom = bodyTop + 4 * ssaa.getSamplingFactor();
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
            
            if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) && 
                drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
                ssaa.getImage().SetPixel(drawX, drawY, gradientColor);
            }
        }
    }
    
    // Add sophisticated 3D effects with highlights and shadows - matching CSS
    
    // Left edge highlight with gradient fade - brighter near edge
    RGBA highlightColor(0xFF, 0xFF, 0xFF, 0x99);  // 60% opacity white
    int edgeWidth = 5 * ssaa.getSamplingFactor(); // Highlight width (scaled)
    
    for (int dy = bodyTop; dy <= bodyBottom; ++dy) {
        for (int dx = 0; dx < edgeWidth; ++dx) {
            // Calculate fade strength - more pronounced edge highlight
            float alpha = (edgeWidth - dx) / static_cast<float>(edgeWidth) * 0.6f;
            int drawX = x - bodyWidth/2 + dx;
            int drawY = dy;
            
            if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) && 
                drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
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
            
            if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) && 
                drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
                RGBA pixelShadow = shadowColor;
                pixelShadow.a = static_cast<unsigned char>(255 * alpha);
                blendPixel(drawX, drawY, pixelShadow, alpha);
            }
        }
    }
    
    // Top edge highlight for 3D effect - subtle rounded top
    float topHighlightAlpha = 0.5f;
    int topEdgeHeight = std::max(static_cast<int>(2 * ssaa.getSamplingFactor()), (bodyBottom - bodyTop) / 20);
    int cornerRadius = 3 * ssaa.getSamplingFactor(); // Corner radius matching CSS, scaled
    
    for (int dx = -bodyWidth/2 + cornerRadius; dx <= bodyWidth/2 - cornerRadius; ++dx) {
        int drawX = x + dx;
        int drawY = bodyTop;
        
        if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) && 
            drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
            blendPixel(drawX, drawY, highlightColor, topHighlightAlpha);
        }
    }
    
    // Bottom edge shadow for 3D effect - subtle rounded bottom
    float bottomShadowAlpha = 0.5f;
    
    for (int dx = -bodyWidth/2 + cornerRadius; dx <= bodyWidth/2 - cornerRadius; ++dx) {
        int drawX = x + dx;
        int drawY = bodyBottom;
        
        if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) && 
            drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
            blendPixel(drawX, drawY, shadowColor, bottomShadowAlpha);
        }
    }
    
    // Add subtle highlight to corners for rounded appearance
    // Top-left corner extra highlight
    for (int i = 0; i < cornerRadius; ++i) {
        for (int j = 0; j < cornerRadius; ++j) {
            float distance = std::sqrt(i*i + j*j);
            if (distance <= cornerRadius) {
                int drawX = x - bodyWidth/2 + i;
                int drawY = bodyTop + j;
                
                if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) && 
                    drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
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
                int drawX = x + bodyWidth/2 - i - 1;
                int drawY = bodyBottom - j;
                
                if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) && 
                    drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
                    blendPixel(drawX, drawY, shadowColor, bottomShadowAlpha * 0.7f);
                }
            }
        }
    }
}

void ShapeRenderer::drawHistogramBar(int x, int y, int barWidth, int barHeight, const RGBA& color) {
    // Draw the bar with a vertical gradient - matching CSS
    for (int dy = 0; dy < barHeight; ++dy) {
        // Calculate gradient position (0 at top, 1 at bottom)
        float gradientPos = static_cast<float>(dy) / barHeight;
        
        // Adjust gradient to match the CSS: brighter at top, slightly darker at bottom
        // Using the non-linear gradient like in histogram_with_labels.css
        float gradientFactor = 1.0f - gradientPos * 0.3f; // Top 30% brighter
        
        // Create gradient color
        RGBA gradientColor(
            static_cast<unsigned char>(std::min(255.0f, color.r * gradientFactor)),
            static_cast<unsigned char>(std::min(255.0f, color.g * gradientFactor)),
            static_cast<unsigned char>(std::min(255.0f, color.b * gradientFactor)),
            color.a
        );
        
        // Draw this row of the bar
        for (int dx = 0; dx < barWidth; ++dx) {
            int drawX = x + dx;
            int drawY = y + dy;
            
            if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) &&
                drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
                ssaa.getImage().SetPixel(drawX, drawY, gradientColor);
            }
        }
    }
    
    // Add 3D effect with highlights and shadows
    drawHistogramBarHighlights(x, y, barWidth, barHeight);
}

void ShapeRenderer::drawHistogramBarHighlights(int x, int y, int barWidth, int barHeight) {
    // Enhanced styling for histogram bars to match CSS
    // Note: x, y, barWidth, barHeight are already scaled for supersampling at this point
    
    // Colors for highlights and shadows - more pronounced
    RGBA highlightColor(0xFF, 0xFF, 0xFF, 0x60); // Semi-transparent white (38% opacity)
    RGBA shadowColor(0x00, 0x00, 0x00, 0x60);    // Semi-transparent black (38% opacity)
    
    // Calculate highlight/shadow widths proportional to bar width
    int edgeWidth = std::max(static_cast<int>(3 * ssaa.getSamplingFactor()), barWidth / 10);
    
    // Left edge highlight with gradient fade - brighter near edge
    for (int dy = 0; dy < barHeight; ++dy) {
        for (int dx = 0; dx < edgeWidth; ++dx) {
            // Calculate fade strength - more intense at edge, fades toward center
            float alpha = (edgeWidth - dx) / static_cast<float>(edgeWidth) * 0.6f;
            int drawX = x + dx;
            int drawY = y + dy;
            
            if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) &&
                drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
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
            
            if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) &&
                drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
                RGBA pixelShadow = shadowColor;
                pixelShadow.a = static_cast<unsigned char>(255 * alpha);
                blendPixel(drawX, drawY, pixelShadow, alpha);
            }
        }
    }
    
    // Top edge highlight for 3D effect - subtle rounded top
    float topHighlightAlpha = 0.5f;
    int topEdgeHeight = std::max(static_cast<int>(2 * ssaa.getSamplingFactor()), barHeight / 20);
    
    for (int dx = edgeWidth; dx < barWidth - edgeWidth; ++dx) {
        for (int dy = 0; dy < topEdgeHeight; ++dy) {
            // Fade from top edge downward
            float alpha = (topEdgeHeight - dy) / static_cast<float>(topEdgeHeight) * topHighlightAlpha;
            int drawX = x + dx;
            int drawY = y + dy;
            
            if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) &&
                drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
                RGBA pixelHighlight = highlightColor;
                pixelHighlight.a = static_cast<unsigned char>(255 * alpha);
                blendPixel(drawX, drawY, pixelHighlight, alpha);
            }
        }
    }
    
    // Bottom edge shadow for 3D effect - subtle rounded bottom
    float bottomShadowAlpha = 0.5f;
    int bottomEdgeHeight = std::max(static_cast<int>(2 * ssaa.getSamplingFactor()), barHeight / 20);
    
    for (int dx = edgeWidth; dx < barWidth - edgeWidth; ++dx) {
        for (int dy = 0; dy < bottomEdgeHeight; ++dy) {
            // Fade from bottom edge upward
            float alpha = (bottomEdgeHeight - dy) / static_cast<float>(bottomEdgeHeight) * bottomShadowAlpha;
            int drawX = x + dx;
            int drawY = y + barHeight - dy - 1;
            
            if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) &&
                drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
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
            
            if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) &&
                drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
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
            
            if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) &&
                drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
                RGBA pixelShadow = shadowColor;
                pixelShadow.a = static_cast<unsigned char>(255 * alpha);
                blendPixel(drawX, drawY, pixelShadow, alpha);
            }
        }
    }
}

void ShapeRenderer::drawRoundedRect(int x, int y, int width, int height, int radius, const RGBA& color, bool filled) {
    // Ensure radius is not too large for the rectangle
    radius = std::min(radius, std::min(width / 2, height / 2));
    
    if (filled) {
        // Draw the main rectangle (excluding corners)
        drawRect(x + radius, y, width - 2 * radius, height, color, true);
        drawRect(x, y + radius, width, height - 2 * radius, color, true);
        
        // Draw the four corner quadrants
        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                float distSquared = dx*dx + dy*dy;
                
                // Skip pixels outside the circle
                if (distSquared > radius*radius) {
                    continue;
                }
                
                // Calculate pixel positions for each corner
                int drawX, drawY;
                
                // Top-left corner
                drawX = x + radius + dx;
                drawY = y + radius + dy;
                if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) &&
                    drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
                    if (color.a < 255) {
                        RGBA currentPixel = ssaa.getImage().GetPixel(drawX, drawY);
                        RGBA blendedColor = colors.blendRGBA(currentPixel, color);
                        ssaa.getImage().SetPixel(drawX, drawY, blendedColor);
                    } else {
                        ssaa.getImage().SetPixel(drawX, drawY, color);
                    }
                }
                
                // Top-right corner
                drawX = x + width - radius + dx;
                drawY = y + radius + dy;
                if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) &&
                    drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
                    if (color.a < 255) {
                        RGBA currentPixel = ssaa.getImage().GetPixel(drawX, drawY);
                        RGBA blendedColor = colors.blendRGBA(currentPixel, color);
                        ssaa.getImage().SetPixel(drawX, drawY, blendedColor);
                    } else {
                        ssaa.getImage().SetPixel(drawX, drawY, color);
                    }
                }
                
                // Bottom-left corner
                drawX = x + radius + dx;
                drawY = y + height - radius + dy;
                if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) &&
                    drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
                    if (color.a < 255) {
                        RGBA currentPixel = ssaa.getImage().GetPixel(drawX, drawY);
                        RGBA blendedColor = colors.blendRGBA(currentPixel, color);
                        ssaa.getImage().SetPixel(drawX, drawY, blendedColor);
                    } else {
                        ssaa.getImage().SetPixel(drawX, drawY, color);
                    }
                }
                
                // Bottom-right corner
                drawX = x + width - radius + dx;
                drawY = y + height - radius + dy;
                if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) &&
                    drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
                    if (color.a < 255) {
                        RGBA currentPixel = ssaa.getImage().GetPixel(drawX, drawY);
                        RGBA blendedColor = colors.blendRGBA(currentPixel, color);
                        ssaa.getImage().SetPixel(drawX, drawY, blendedColor);
                    } else {
                        ssaa.getImage().SetPixel(drawX, drawY, color);
                    }
                }
            }
        }
    } else {
        // For non-filled rounded rectangle, draw the four straight segments
        
        // Top edge
        drawLine(x + radius, y, x + width - radius, y, color);
        
        // Bottom edge
        drawLine(x + radius, y + height, x + width - radius, y + height, color);
        
        // Left edge
        drawLine(x, y + radius, x, y + height - radius, color);
        
        // Right edge
        drawLine(x + width, y + radius, x + width, y + height - radius, color);
        
        // Draw the four rounded corners with anti-aliasing
        float stepSize = 0.5f;
        for (float angle = 0; angle < 2 * 3.14159265f; angle += stepSize / radius) {
            float dx = cos(angle) * radius;
            float dy = sin(angle) * radius;
            
            // Top-left corner (angle π/2 to π)
            if (angle >= 3.14159265f/2 && angle <= 3.14159265f) {
                int drawX = x + radius - static_cast<int>(dx);
                int drawY = y + radius - static_cast<int>(dy);
                if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) &&
                    drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
                    if (color.a < 255) {
                        RGBA currentPixel = ssaa.getImage().GetPixel(drawX, drawY);
                        RGBA blendedColor = colors.blendRGBA(currentPixel, color);
                        ssaa.getImage().SetPixel(drawX, drawY, blendedColor);
                    } else {
                        ssaa.getImage().SetPixel(drawX, drawY, color);
                    }
                }
            }
            
            // Top-right corner (angle 0 to π/2)
            if (angle >= 0 && angle <= 3.14159265f/2) {
                int drawX = x + width - radius + static_cast<int>(dx);
                int drawY = y + radius - static_cast<int>(dy);
                if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) &&
                    drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
                    if (color.a < 255) {
                        RGBA currentPixel = ssaa.getImage().GetPixel(drawX, drawY);
                        RGBA blendedColor = colors.blendRGBA(currentPixel, color);
                        ssaa.getImage().SetPixel(drawX, drawY, blendedColor);
                    } else {
                        ssaa.getImage().SetPixel(drawX, drawY, color);
                    }
                }
            }
            
            // Bottom-left corner (angle π to 3π/2)
            if (angle >= 3.14159265f && angle <= 3*3.14159265f/2) {
                int drawX = x + radius - static_cast<int>(dx);
                int drawY = y + height - radius + static_cast<int>(dy);
                if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) &&
                    drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
                    if (color.a < 255) {
                        RGBA currentPixel = ssaa.getImage().GetPixel(drawX, drawY);
                        RGBA blendedColor = colors.blendRGBA(currentPixel, color);
                        ssaa.getImage().SetPixel(drawX, drawY, blendedColor);
                    } else {
                        ssaa.getImage().SetPixel(drawX, drawY, color);
                    }
                }
            }
            
            // Bottom-right corner (angle 3π/2 to 2π)
            if (angle >= 3*3.14159265f/2 && angle <= 2*3.14159265f) {
                int drawX = x + width - radius + static_cast<int>(dx);
                int drawY = y + height - radius + static_cast<int>(dy);
                if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) &&
                    drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
                    if (color.a < 255) {
                        RGBA currentPixel = ssaa.getImage().GetPixel(drawX, drawY);
                        RGBA blendedColor = colors.blendRGBA(currentPixel, color);
                        ssaa.getImage().SetPixel(drawX, drawY, blendedColor);
                    } else {
                        ssaa.getImage().SetPixel(drawX, drawY, color);
                    }
                }
            }
        }
    }
}

// Helper for drawing a fully filled rounded rectangle with proper corner clipping
void ShapeRenderer::drawSolidRoundedRect(int x, int y, int width, int height, int radius, const RGBA& color) {
    // Ensure radius is not too large for the rectangle
    radius = std::min(radius, std::min(width / 2, height / 2));
    
    // Draw each pixel of the rounded rectangle directly
    for (int dy = 0; dy < height; dy++) {
        for (int dx = 0; dx < width; dx++) {
            bool inCorner = false;
            bool drawPixel = true;
            
            // Check if we're in a corner region
            if (dx < radius && dy < radius) {
                // Top-left corner
                float dist = std::sqrt(std::pow(radius - dx, 2) + std::pow(radius - dy, 2));
                if (dist > radius) {
                    drawPixel = false; // Outside rounded corner
                }
            }
            else if (dx >= width - radius && dy < radius) {
                // Top-right corner
                float dist = std::sqrt(std::pow(dx - (width - radius), 2) + std::pow(radius - dy, 2));
                if (dist > radius) {
                    drawPixel = false; // Outside rounded corner
                }
            }
            else if (dx < radius && dy >= height - radius) {
                // Bottom-left corner
                float dist = std::sqrt(std::pow(radius - dx, 2) + std::pow(dy - (height - radius), 2));
                if (dist > radius) {
                    drawPixel = false; // Outside rounded corner
                }
            }
            else if (dx >= width - radius && dy >= height - radius) {
                // Bottom-right corner
                float dist = std::sqrt(std::pow(dx - (width - radius), 2) + std::pow(dy - (height - radius), 2));
                if (dist > radius) {
                    drawPixel = false; // Outside rounded corner
                }
            }
            
            // Draw the pixel if it's inside the rounded rectangle
            if (drawPixel) {
                int drawX = x + dx;
                int drawY = y + dy;
                
                if (drawX >= 0 && drawX < static_cast<int>(ssaa.getWidth()) && 
                    drawY >= 0 && drawY < static_cast<int>(ssaa.getHeight())) {
                    // Always use proper alpha blending for semi-transparent colors
                    if (color.a < 255) {
                        RGBA currentPixel = ssaa.getImage().GetPixel(drawX, drawY);
                        RGBA blendedColor = colors.blendRGBA(currentPixel, color);
                        ssaa.getImage().SetPixel(drawX, drawY, blendedColor);
                    } else {
                        // Fully opaque - just set the pixel
                        ssaa.getImage().SetPixel(drawX, drawY, color);
                    }
                }
            }
        }
    }
}

void ShapeRenderer::drawArrow(int x1, int y1, int x2, int y2, const RGBA& color, int lineWidth, int arrowheadSize) {
    // Draw the main line from (x1,y1) to (x2,y2)
    drawLine(x1, y1, x2, y2, color, lineWidth);
    
    // Calculate the angle of the main line
    float angle = atan2(y2 - y1, x2 - x1);
    
    // Calculate arrowhead points - two lines at ±30 degrees from the main line
    const float arrowAngle = 30.0f * M_PI / 180.0f; // 30 degrees in radians
    
    // Calculate offsets for first arrowhead line (-30 degrees)
    int x3 = x2 - static_cast<int>(arrowheadSize * cos(angle + arrowAngle));
    int y3 = y2 - static_cast<int>(arrowheadSize * sin(angle + arrowAngle));
    
    // Calculate offsets for second arrowhead line (+30 degrees)
    int x4 = x2 - static_cast<int>(arrowheadSize * cos(angle - arrowAngle));
    int y4 = y2 - static_cast<int>(arrowheadSize * sin(angle - arrowAngle));
    
    // Draw the two arrowhead lines
    drawLine(x2, y2, x3, y3, color, lineWidth);
    drawLine(x2, y2, x4, y4, color, lineWidth);
}

} // namespace shmea 