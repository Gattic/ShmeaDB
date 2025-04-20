#include "TextRenderer.h"
#include <cstdio>

namespace shmea {

TextRenderer::TextRenderer(SuperSamplingManager& ssaaManager, ColorManager& colorManager, ChartLayout& chartLayout)
    : ssaa(ssaaManager),
      colors(colorManager),
      layout(chartLayout),
      yAxisLabel(""),
      fontInitialized(false),
      axisLabelFontSize(18) // Default font size for axis labels
{
    // Initialize FreeType
    if (FT_Init_FreeType(&ft)) {
        printf("Error: Could not initialize FreeType Library\n");
    }
}

TextRenderer::~TextRenderer() {
    // Clean up FreeType resources if initialized
}

void TextRenderer::initialize(FT_Library& newFT, FT_Face& newFace)
{
    ft = newFT;
    face = newFace;

    fontInitialized = true;
}

void TextRenderer::drawText(int x, int y, const std::string& text, const RGBA& color, 
                          unsigned int fontSize, bool centerAligned) {
    if (!fontInitialized) {
        printf("Error: Font not initialized\n");
        return;
    }
    
    // Scale coordinates and font size for supersampling - exact scaling from plotter.cpp
    int ssaaX = ssaa.scaleX(x);
    int ssaaY = ssaa.scaleY(y);
    unsigned int ssaaFontSize = fontSize * ssaa.getSamplingFactor();
    
    // Set the font size - exact same call as in plotter.cpp
    if (FT_Set_Pixel_Sizes(face, 0, ssaaFontSize)) {
        printf("Error: Could not set pixel sizes\n");
        return;
    }
    
    // For center alignment, measure text width first - exactly like plotter.cpp
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
        // Adjust x position for center alignment - exact calculation from plotter.cpp
        ssaaX -= textWidth / 2;
    }
    
    // Exact baseline computation from plotter.cpp
    int baseline = face->size->metrics.ascender / 64; // Convert from 26.6 fixed-point to pixels
    
    // Character spacing - exact value from plotter.cpp
    unsigned int extraSpacing = ssaaFontSize / 10;
    
    // Draw each character - exact rendering logic from plotter.cpp
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
        
        // Position calculation - exact from plotter.cpp
        unsigned int drawX = penX + glyph->bitmap_left;
        unsigned int drawY = ssaaY - glyph->bitmap_top;
        
        // Draw the glyph bitmap - exact alpha blending from plotter.cpp
        for (unsigned int row = 0; row < glyphHeight; ++row) {
            for (unsigned int col = 0; col < glyphWidth; ++col) {
                // Get pixel value from glyph bitmap
                unsigned char value = glyph->bitmap.buffer[row * glyphWidth + col];
                
                if (value > 0) { // Only draw if the glyph pixel is not empty
                    unsigned int imgX = drawX + col;
                    unsigned int imgY = drawY + row;
                    
                    if (imgX < ssaa.getWidth() && imgY < ssaa.getHeight()) {
                        // Alpha blending - exactly like plotter.cpp
                        float alpha = value / 255.0f * (color.a / 255.0f);
                        RGBA blendedColor;
                        blendedColor.r = color.r;
                        blendedColor.g = color.g;
                        blendedColor.b = color.b;
                        blendedColor.a = static_cast<unsigned char>(255.0f * alpha);
                        
                        // Get current pixel and blend with the glyph - exact blend function from plotter.cpp
                        RGBA currentPixel = ssaa.getImage().GetPixel(imgX, imgY);
                        RGBA finalColor = colors.blendRGBA(currentPixel, blendedColor);
                        ssaa.getImage().SetPixel(imgX, imgY, finalColor);
                    }
                }
            }
        }
        
        // Advance cursor position - exactly like plotter.cpp
        penX += (glyph->advance.x >> 6) + extraSpacing;
    }
}

void TextRenderer::drawVerticalText(const std::string& text, int x, int y, int fontSize, const RGBA& color) {
    if (!fontInitialized) {
        printf("Error: Font not initialized\n");
        return;
    }
    
    // Scale coordinates and font size for supersampling - exact scaling from plotter.cpp
    int ssaaX = ssaa.scaleX(x);
    int ssaaY = ssaa.scaleY(y);
    int ssaaFontSize = fontSize * ssaa.getSamplingFactor();
    
    // Set the font size - exact same call as in plotter.cpp
    if (FT_Set_Pixel_Sizes(face, 0, ssaaFontSize)) {
        printf("Error: Could not set pixel sizes for vertical text\n");
        return;
    }
    
    // For properly rotated text, first measure total width - exact from plotter.cpp
    int textWidth = 0;
    
    // Calculate the total width of the text - exactly like plotter.cpp
    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            continue;
        }
        
        FT_GlyphSlot glyph = face->glyph;
        textWidth += (glyph->advance.x >> 6);
    }
    
    // Rotation calculations - exact method from plotter.cpp
    // Rotate text 270 degrees so it reads from bottom to top
    
    // Start position (center of rotation) - exact calculation from plotter.cpp
    // Position vertically centered based on text width
    int drawX = ssaaX;
    int drawY = ssaaY + textWidth/2; // Center vertically based on text width
    
    // Draw each character rotated - exact rotation algorithm from plotter.cpp
    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            printf("Warning: Could not load character %c\n", c);
            continue;
        }
        
        FT_GlyphSlot glyph = face->glyph;
        
        unsigned int glyphWidth = glyph->bitmap.width;
        unsigned int glyphHeight = glyph->bitmap.rows;
        
        // Calculate rotated position - exact transformation from plotter.cpp
        // For 270° rotation (or -90°): x' = -y and y' = x
        for (unsigned int row = 0; row < glyphHeight; ++row) {
            for (unsigned int col = 0; col < glyphWidth; ++col) {
                // Get pixel value from glyph bitmap
                unsigned char value = glyph->bitmap.buffer[row * glyphWidth + col];
                
                if (value > 0) { // Only draw if the glyph pixel is not empty
                    // Apply rotation transformation - exact algorithm from plotter.cpp
                    // For 270° rotation (bottom facing left)
                    int rotatedX = drawX - glyph->bitmap_top + row;
                    int rotatedY = drawY - col - glyph->bitmap_left;
                    
                    if (rotatedX >= 0 && rotatedX < static_cast<int>(ssaa.getWidth()) &&
                        rotatedY >= 0 && rotatedY < static_cast<int>(ssaa.getHeight())) {
                        // Alpha blending - exactly like plotter.cpp
                        float alpha = value / 255.0f * (color.a / 255.0f);
                        RGBA blendedColor;
                        blendedColor.r = color.r;
                        blendedColor.g = color.g;
                        blendedColor.b = color.b;
                        blendedColor.a = static_cast<unsigned char>(255.0f * alpha);
                        
                        // Get current pixel and blend with the glyph
                        RGBA currentPixel = ssaa.getImage().GetPixel(rotatedX, rotatedY);
                        RGBA finalColor = colors.blendRGBA(currentPixel, blendedColor);
                        ssaa.getImage().SetPixel(rotatedX, rotatedY, finalColor);
                    }
                }
            }
        }
        
        // Move to next character position - exact positioning from plotter.cpp
        drawY -= (glyph->advance.x >> 6);
    }
}

void TextRenderer::addTitle(const std::string& text, unsigned int fontSize) {
    // Position title at the top left of the image with some padding
    int x = layout.getMarginLeft();
    int y = layout.getMarginTop() / 2;
    
    // Use left alignment (false for centerAligned parameter)
    drawText(x, y, text, colors.getElementColor("title"), fontSize, false);
}

void TextRenderer::drawAxisLabels(const std::string& xLabel, const std::string& yLabel, 
                                unsigned int fontSize, bool centerX) {
    // Get text color from CSS design
    RGBA textColor = colors.getElementColor("axisLabel");
    textColor.a = 0xCC; // 80% opacity for readability
    
    // Calculate positioning
    int width = layout.getWidth();
    int height = layout.getHeight();
    int marginLeft = layout.getMarginLeft();
    int marginRight = layout.getMarginRight();
    int marginTop = layout.getMarginTop();
    int marginBottom = layout.getMarginBottom();
    
    // X-axis label position (centered horizontally at bottom)
    int xLabelX = centerX ? width / 2 : marginLeft;
    int xLabelY = height - marginBottom / 2; // Centered in bottom margin
    
    // Use a slightly larger font size for better visibility
    unsigned int scaledFontSize = fontSize + 4;
    
    // Draw X-axis label with proper styling - center aligned if requested
    drawText(xLabelX, xLabelY, xLabel, textColor, scaledFontSize, centerX);
    
    // Save Y-axis label for use during final rendering
    setYAxisLabel(yLabel);
    
    // Do NOT draw Y-axis label here - it will be drawn in saveAsPNG
}

void TextRenderer::setYAxisLabel(const std::string& label) {
    // Store the Y-axis label for later rendering in saveAsPNG - exactly like plotter.cpp
    yAxisLabel = label;
}

const std::string& TextRenderer::getYAxisLabel() const {
    // Return the stored Y-axis label - exactly like plotter.cpp
    return yAxisLabel;
}

void TextRenderer::setAxisLabelFontSize(unsigned int fontSize)
{
    axisLabelFontSize = fontSize;
}

unsigned int TextRenderer::getAxisLabelFontSize() const
{
    return axisLabelFontSize;
}

} // namespace shmea 
