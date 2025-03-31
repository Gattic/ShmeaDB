#include "LabelsDrawer.h"
#include <iostream>
#include <cstring>
#include <cstdlib> // For getenv

namespace shmea {

LabelsDrawer::LabelsDrawer(Image& image, unsigned int width, unsigned int height, 
                          int margin_top, int margin_right, int margin_bottom, int margin_left)
    : BaseDrawer(image, width, height, margin_top, margin_right, margin_bottom, margin_left),
      headerPenXStarting(500), headerPenYStarting(75), headerXSpacing(25), headerYSpacing(150),
      freetype_available(false) {
    
    initialize_font();
}

LabelsDrawer::~LabelsDrawer() {
    if (freetype_available) {
        FT_Done_Face(face);
        FT_Done_FreeType(ft);
    }
}

void LabelsDrawer::initialize_font(const std::string& fontPath) {
    // Initialize FreeType
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "Could not initialize FreeType Library." << std::endl;
        freetype_available = false;
        return;
    }
    
    // Try multiple font paths to increase chances of finding a font
    std::vector<std::string> possibleFonts;
    possibleFonts.push_back(fontPath);  // Try the provided path first
    
    // Try system fonts
#ifdef _WIN32
    possibleFonts.push_back("C:/Windows/Fonts/arial.ttf");
    possibleFonts.push_back("C:/Windows/Fonts/verdana.ttf");
#elif defined(__APPLE__)
    possibleFonts.push_back("/System/Library/Fonts/Helvetica.ttc");
    possibleFonts.push_back("/System/Library/Fonts/Geneva.ttf");
#else // Linux and others
    possibleFonts.push_back("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
    possibleFonts.push_back("/usr/share/fonts/TTF/DejaVuSans.ttf");
    possibleFonts.push_back("/usr/share/fonts/truetype/freefont/FreeSans.ttf");
    possibleFonts.push_back("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf");
#endif

    // Try to load a font from the list
    freetype_available = false;
    for (size_t i = 0; i < possibleFonts.size(); ++i) {
        if (FT_New_Face(ft, possibleFonts[i].c_str(), 0, &face) == 0) {
            std::cout << "Successfully loaded font: " << possibleFonts[i] << std::endl;
            freetype_available = true;
            break;
        }
    }
    
    if (!freetype_available) {
        std::cerr << "Failed to load any font. Text rendering will be disabled." << std::endl;
        FT_Done_FreeType(ft);
    }
}

void LabelsDrawer::drawLabel(unsigned int penX, unsigned int penY, const std::string& text, 
                           unsigned int fontSize, unsigned int xOffset, unsigned int yOffset,
                           bool hasBox, RGBA labelColor, RGBA textColor) {
    if (!freetype_available) {
        // Simple fallback when FreeType font couldn't be loaded
        if (hasBox) {
            unsigned int boxWidth = text.length() * fontSize / 2;
            unsigned int boxHeight = fontSize;
            unsigned int boxX = penX + xOffset;
            unsigned int boxY = penY - yOffset - boxHeight / 2;
            
            for (unsigned int y = 0; y < boxHeight; ++y) {
                for (unsigned int x = 0; x < boxWidth; ++x) {
                    unsigned imgX = boxX + x;
                    unsigned imgY = boxY + y;

                    if (imgX < width && imgY < height) {
                        image.SetPixel(imgX, imgY, labelColor);
                    }
                }
            }
        }
        return;
    }

    if (FT_Set_Pixel_Sizes(face, 0, fontSize)) {
        std::cerr << "Error: Could not set pixel sizes" << std::endl;
        return;
    }
    
    penX += xOffset;
    penY -= yOffset;
   
    // Compute baseline using font metrics
    int ascender = face->size->metrics.ascender / 64; // Convert from 26.6 fixed-point to pixels
    int descender = face->size->metrics.descender / 64; // Convert to pixels
    
    float heightScale = 0.5;
    
    // Compute a common baseline using font metrics
    int baseline = face->size->metrics.ascender / 64; 
    
    // Adjusted spacing for better readability
    unsigned int extraSpacing = fontSize / 6;

    // Calculate the bounding box for the text
    unsigned int boxWidth = 0;
    unsigned int boxHeight = static_cast<unsigned int>((ascender - descender) * heightScale);

    // Measure the total width of the text with reduced spacing
    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "Warning: Could not load character " << c << std::endl;
            continue;
        }

        FT_GlyphSlot glyph = face->glyph;

        // Add glyph width and reduced extra spacing
        boxWidth += (glyph->advance.x >> 6) + extraSpacing;

        // Adjust boxHeight if a taller glyph is found (with reduced scaled height)
        unsigned int glyphHeight = static_cast<unsigned int>(glyph->bitmap.rows * heightScale);
        if (glyphHeight > boxHeight) {
            boxHeight = glyphHeight;
        }
    }

    // Add padding to the box
    unsigned int padding = fontSize / 10; // Reduced padding

    // Draw the box if necessary
    if (hasBox) {
        unsigned int boxX = penX; // Adjust box start position
        unsigned int boxY = penY - boxHeight; // Adjust for baseline alignment
        for (unsigned int y = 0; y < boxHeight; ++y) {
            for (unsigned int x = 0; x < boxWidth; ++x) {
                unsigned imgX = boxX + x;
                unsigned imgY = boxY + y + (boxHeight);

                if (imgX < width && imgY < height) {
                    image.SetPixel(imgX, imgY, labelColor); // Draw the background box
                }
            }
        }
    }

    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "Warning: Could not load character " << c << std::endl;
            continue;
        }

        FT_GlyphSlot glyph = face->glyph;

        unsigned glyphWidth = glyph->bitmap.width;
        unsigned glyphHeight = glyph->bitmap.rows;

        unsigned int x0 = penX + glyph->bitmap_left;
        unsigned int y0 = penY + static_cast<unsigned int>(baseline * heightScale) - static_cast<unsigned int>(glyph->bitmap_top * heightScale);
    
        // Draw the glyph bitmap with improved aspect ratio
        for (unsigned int y = 0; y < glyphHeight; ++y) {
            for (unsigned int x = 0; x < glyphWidth; ++x) {
                // Calculate adjusted position with improved aspect ratio
                unsigned imgX = x0 + x;
                
                // Use heightScale to vertically compress the text for better aspect ratio
                unsigned imgY = y0 + static_cast<unsigned int>(y * heightScale);

                if (imgX < width && imgY < height) {
                    unsigned char value = glyph->bitmap.buffer[y * glyphWidth + x];
                    if (value > 0) { // Only draw if the glyph pixel is not empty
                        image.SetPixel(imgX, imgY, textColor); 
                    }
                }
            }
        }

        // Advance cursor position with reduced spacing
        penX += (glyph->advance.x >> 6) + extraSpacing;
    }
}

void LabelsDrawer::drawHeader(const std::string& text, unsigned int fontSize, 
                             unsigned int headerPos, unsigned int rePositionY,
                             RGBA headerTextColor) {
    if (!freetype_available) {
        return; // Skip header rendering when no FreeType
    }
    
    unsigned int headerPenX = 0;
    unsigned int headerPenY = (headerPenYStarting * (headerPos + 1)) + (headerYSpacing * (headerPos));

    if (rePositionY != 0) {
        headerPenY += (headerPenY / rePositionY);
    }

    if (headerPos + 1 > headerSpacings.size()) {
        std::vector<unsigned int> newVector;
        newVector.push_back(headerPenXStarting);
        headerSpacings.push_back(newVector);
    }

    headerPenX = headerSpacings[headerPos][headerSpacings[headerPos].size() - 1];
    
    // Set font size
    FT_Set_Pixel_Sizes(face, 0, fontSize);

    unsigned int extraSpacing = fontSize / 4;

    float heightScale = 0.3;
    // Compute a common baseline using font metrics
    int baseline = face->size->metrics.ascender / 64;

    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "Warning: Could not load character " << c << std::endl;
            continue;
        }

        FT_GlyphSlot glyph = face->glyph;

        unsigned glyphWidth = glyph->bitmap.width;
        unsigned glyphHeight = glyph->bitmap.rows;

        unsigned int x0 = headerPenX + glyph->bitmap_left;
        unsigned int y0 = headerPenY + static_cast<unsigned int>(baseline * heightScale) - static_cast<unsigned int>(glyph->bitmap_top * heightScale);
    
        // Draw the glyph bitmap
        for (unsigned int y = 0; y < glyphHeight; ++y) {
            for (unsigned int x = 0; x < glyphWidth; ++x) {
                unsigned imgX = x0 + x;
                unsigned imgY = y0 + static_cast<unsigned int>(y * heightScale);

                if (imgX < width && imgY < height) {
                    unsigned char value = glyph->bitmap.buffer[y * glyphWidth + x];
                    if (value > 0) { // Only draw if the glyph pixel is not empty
                        image.SetPixel(imgX, imgY, headerTextColor);
                    }
                }
            }
        }

        // Advance cursor position
        headerPenX += (glyph->advance.x >> 6) + extraSpacing;
    }
    headerSpacings[headerPos].push_back(headerPenX + headerXSpacing);
}

} // namespace shmea
