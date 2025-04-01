#include "LegendDrawer.h"
#include <algorithm>

namespace shmea {

LegendDrawer::LegendDrawer(Image& image, unsigned int width, unsigned int height)
    : BaseDrawer(image, width, height),
      entry_height(40), entry_spacing(10), box_size(30), 
      font_size(500), padding(20),
      background_color(0x20, 0x20, 0x20, 0xD0),
      text_color(0xFF, 0xFF, 0xFF, 0xFF),
      border_color(0xA0, 0xA0, 0xA0, 0xFF),
      max_height(0) {
}

void LegendDrawer::addEntry(const std::string& label, const RGBA& color) {
    entries.push_back(LegendEntry(label, color));
}

void LegendDrawer::drawLegendBox(int x, int y, int width, int height) {
    // Draw background box with semi-transparent background
    for (int dy = 0; dy < height; ++dy) {
        for (int dx = 0; dx < width; ++dx) {
            int px = x + dx;
            int py = y + dy;
            
            if (px >= 0 && px < this->width && py >= 0 && py < this->height) {
                image.SetPixel(px, py, background_color);
            }
        }
    }
    
    // Draw border around the box
    for (int dx = 0; dx < width; ++dx) {
        if (x + dx >= 0 && x + dx < this->width) {
            // Top border
            if (y >= 0 && y < this->height) {
                image.SetPixel(x + dx, y, border_color);
            }
            
            // Bottom border
            if (y + height - 1 >= 0 && y + height - 1 < this->height) {
                image.SetPixel(x + dx, y + height - 1, border_color);
            }
        }
    }
    
    for (int dy = 0; dy < height; ++dy) {
        if (y + dy >= 0 && y + dy < this->height) {
            // Left border
            if (x >= 0 && x < this->width) {
                image.SetPixel(x, y + dy, border_color);
            }
            
            // Right border
            if (x + width - 1 >= 0 && x + width - 1 < this->width) {
                image.SetPixel(x + width - 1, y + dy, border_color);
            }
        }
    }
}

void LegendDrawer::drawEntryBox(int x, int y, const RGBA& color) {
    // Use a smaller box size if needed to fit in the space
    int actual_size = std::min(static_cast<int>(box_size), static_cast<int>(entry_height) - 4);
    
    for (int dy = 0; dy < actual_size; ++dy) {
        for (int dx = 0; dx < actual_size; ++dx) {
            int px = x + dx;
            int py = y + dy;
            
            if (px >= 0 && px < this->width && py >= 0 && py < this->height) {
                image.SetPixel(px, py, color);
            }
        }
    }
}

void LegendDrawer::drawEntryLabel(int x, int y, const std::string& label) {
    // Since we don't have direct access to the LabelsDrawer from here,
    // we'll use a simple text rendering method
    
    unsigned int char_width = font_size / 10;
    unsigned int char_height = font_size / 8;
    
    // Draw each character as a simple rectangle (this is just a basic representation)
    // In a real implementation, you would use proper text rendering from LabelsDrawer
    for (size_t i = 0; i < label.size(); ++i) {
        unsigned int start_x = x + i * char_width;
        
        // Create a simple representation of the character
        for (unsigned int dy = 0; dy < char_height; ++dy) {
            for (unsigned int dx = 0; dx < char_width - 1; ++dx) {
                // Skip some pixels to create a simple letterform shape
                if ((dy == 0 || dy == char_height-1) && dx % 2 == 0) continue;
                if ((dx == 0 || dx == char_width-2) && dy % 2 == 0) continue;
                
                int px = start_x + dx;
                int py = y + dy;
                
                if (px >= 0 && px < width && py >= 0 && py < height) {
                    image.SetPixel(px, py, text_color);
                }
            }
        }
    }
}

void LegendDrawer::draw(unsigned int x, unsigned int y) {
    // Calculate margins for context
    int margin_top = height * 0.1;
    
    if (entries.empty()) {
        return;  // Nothing to draw
    }
    
    // Calculate the dimensions of the legend box
    unsigned int legend_width = 0;
    for (size_t i = 0; i < entries.size(); ++i) {
        // For each entry, we need space for the box, some padding, and the text
        // For simplicity, we'll just assume each character takes a fixed width
        unsigned int entry_width = box_size + padding + entries[i].label.size() * (font_size / 10);
        legend_width = std::max(legend_width, entry_width);
    }
    
    legend_width += 2 * padding;  // Add padding on both sides
    
    unsigned int legend_height = padding + entries.size() * (entry_height + entry_spacing) - entry_spacing + padding;
    
    // Apply max_height constraint if set
    if (max_height > 0 && legend_height > max_height) {
        // Scale down entry height to fit within max_height
        entry_height = (max_height - 2*padding) / entries.size() - entry_spacing;
        if (entry_height < 10) entry_height = 10; // Minimum entry height
        
        // Recalculate the height
        legend_height = padding + entries.size() * (entry_height + entry_spacing) - entry_spacing + padding;
        
        // Further reduce box size if needed
        box_size = std::min(box_size, entry_height - 4);
    }
    
    // Calculate actual box size based on entry height
    int actual_box_size = std::min(static_cast<int>(box_size), static_cast<int>(entry_height) - 4);
    
    // Ensure the legend stays within bounds
    if (x + legend_width > width) {
        x = width - legend_width;
    }
    
    if (y + legend_height > margin_top) {
        // If too tall, try to make it fit horizontally instead
        if (legend_height > margin_top) {
            // Reorganize entries to be in columns instead of rows if too tall
            int entries_per_column = std::max(1, static_cast<int>(margin_top / (entry_height + entry_spacing)));
            int columns = (entries.size() + entries_per_column - 1) / entries_per_column;
            
            // Adjust entry height for available space
            entry_height = (margin_top - 2*padding) / entries_per_column;
            if (entry_height < 10) entry_height = 10;
            
            // Recalculate dimensions for column layout
            actual_box_size = std::min(static_cast<int>(box_size), static_cast<int>(entry_height) - 4);
            legend_width = columns * (legend_width + padding) - padding;
            legend_height = padding + entries_per_column * (entry_height + entry_spacing) - entry_spacing + padding;
            
            // Draw the legend box for column layout
            drawLegendBox(x, y, legend_width, legend_height);
            
            // Draw entries in columns
            for (size_t i = 0; i < entries.size(); ++i) {
                int column = i / entries_per_column;
                int row = i % entries_per_column;
                
                unsigned int entry_x = x + padding + column * (legend_width / columns);
                unsigned int entry_y = y + padding + row * (entry_height + entry_spacing);
                
                // Draw color box
                int box_y_offset = (entry_height - actual_box_size) / 2;
                drawEntryBox(entry_x, entry_y + box_y_offset, entries[i].color);
                
                // Draw label
                drawEntryLabel(entry_x + actual_box_size + padding/2, 
                             entry_y + (entry_height - font_size/8)/2, 
                             entries[i].label);
            }
            
            return;
        }
    }
    
    // Regular layout (single column) if it fits
    drawLegendBox(x, y, legend_width, legend_height);
    
    for (size_t i = 0; i < entries.size(); ++i) {
        unsigned int entry_y = y + padding + i * (entry_height + entry_spacing);
        
        // Draw the color box
        int box_y_offset = (entry_height - actual_box_size) / 2;
        drawEntryBox(x + padding, entry_y + box_y_offset, entries[i].color);
        
        // Draw the label
        drawEntryLabel(x + padding + actual_box_size + padding/2,
                      entry_y + (entry_height - font_size/8)/2,
                      entries[i].label);
    }
}

void LegendDrawer::setEntryHeight(unsigned int height) {
    entry_height = height;
}

void LegendDrawer::setEntrySpacing(unsigned int spacing) {
    entry_spacing = spacing;
}

void LegendDrawer::setBoxSize(unsigned int size) {
    box_size = size;
}

void LegendDrawer::setFontSize(unsigned int size) {
    font_size = size;
}

void LegendDrawer::setPadding(unsigned int pad) {
    padding = pad;
}

void LegendDrawer::setBackgroundColor(const RGBA& color) {
    background_color = color;
}

void LegendDrawer::setTextColor(const RGBA& color) {
    text_color = color;
}

void LegendDrawer::setBorderColor(const RGBA& color) {
    border_color = color;
}

void LegendDrawer::setMaxHeight(unsigned int height) {
    max_height = height;
}

} // namespace shmea
