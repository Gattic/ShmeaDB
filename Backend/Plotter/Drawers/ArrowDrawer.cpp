#include "ArrowDrawer.h"
#include <cmath>
#include <algorithm>

namespace shmea {

ArrowDrawer::ArrowDrawer(Image& image, unsigned int width, unsigned int height)
    : BaseDrawer(image, width, height) {
}

void ArrowDrawer::drawArrow(int x1, int y1, int x2, int y2, const RGBA& arrowColor, int arrowSize) {
    // Draw the main line
    drawLine(x1, y1, x2, y2, arrowColor);

    // Add line thickness
    drawLine(x1 - 1, y1, x2 - 1, y2, arrowColor);
    drawLine(x1 + 1, y1, x2 + 1, y2, arrowColor);
    drawLine(x1, y1 - 1, x2, y2 - 1, arrowColor);
    drawLine(x1, y1 + 1, x2 + 1, y2 + 1, arrowColor);

    // Calculate the angle of the arrow
    double angle = std::atan2(static_cast<double>(y2 - y1), static_cast<double>(x2 - x1));

    // Calculate the points for the arrowhead
    int arrowX1 = static_cast<int>(x2 - arrowSize * std::cos(angle + M_PI / 6)); // M_PI / 6 = 30 degrees
    int arrowY1 = static_cast<int>(y2 - arrowSize * std::sin(angle + M_PI / 6));
    int arrowX2 = static_cast<int>(x2 - arrowSize * std::cos(angle - M_PI / 6));
    int arrowY2 = static_cast<int>(y2 - arrowSize * std::sin(angle - M_PI / 6));

    // Draw the arrowhead lines
    drawLine(x2, y2, arrowX1, arrowY1, arrowColor);
    
    // Add line thickness
    drawLine(x2 - 1, y2, arrowX1 - 1, arrowY1, arrowColor);
    drawLine(x2 + 1, y2, arrowX1 + 1, arrowY1, arrowColor);
    drawLine(x2, y2 - 1, arrowX1, arrowY1 - 1, arrowColor);
    drawLine(x2, y2 + 1, arrowX1 + 1, arrowY1 + 1, arrowColor);

    // Draw the arrowhead lines
    drawLine(x2, y2, arrowX2, arrowY2, arrowColor);

    // Add line thickness
    drawLine(x2 - 1, y2, arrowX2 - 1, arrowY2, arrowColor);
    drawLine(x2 + 1, y2, arrowX2 + 1, arrowY2, arrowColor);
    drawLine(x2, y2 - 1, arrowX2, arrowY2 - 1, arrowColor);
    drawLine(x2, y2 + 1, arrowX2 + 1, arrowY2 + 1, arrowColor);
}

void ArrowDrawer::addArrow(const std::vector<std::vector<double> >& sorted_eig_vecs, 
                          const std::vector<double>& variance_explained, const RGBA& arrowColor) {
    int arrowSize = 100;

    // Calculate margins based on image size
    int margin_left = width * 0.15;
    int margin_right = width * 0.1;
    int margin_top = height * 0.1;
    int margin_bottom = height * 0.15;

    // Calculate the effective plotting area considering the margins
    int effectiveWidth = width - margin_left - margin_right;
    int effectiveHeight = height - margin_top - margin_bottom;

    // Calculate the center (origin) of the plot
    int centerX = margin_left + effectiveWidth / 2;
    int centerY = margin_top + effectiveHeight / 2;

    // Determine the scale factor based on the plot size
    double scaleFactor = std::min(effectiveWidth, effectiveHeight) * 0.4;

    // For each eigenvector
    for (size_t i = 0; i < sorted_eig_vecs.size(); ++i) {
        // Skip if this eigenvector doesn't have at least 2 dimensions
        if (sorted_eig_vecs[i].size() < 2) continue;
        
        // Starting point of the arrow is the center of the plot
        int arrowX1 = centerX;
        int arrowY1 = centerY;
        
        // Get the direction from the eigenvector
        double vecX = sorted_eig_vecs[i][0];
        double vecY = sorted_eig_vecs[i][1];
        
        // Calculate the magnitude of the eigenvector for normalization
        double magnitude = std::sqrt(vecX * vecX + vecY * vecY);
        if (magnitude < 1e-10) continue; // Skip if magnitude is too small
        
        // Normalize the vector
        vecX /= magnitude;
        vecY /= magnitude;
        
        // Scale the vector to a visible size and flip Y for screen coordinates
        double arrowX2 = arrowX1 + vecX * scaleFactor;
        double arrowY2 = arrowY1 - vecY * scaleFactor; // Note the minus sign for Y coordinate
        
        // Ensure arrow endpoint stays within plotting area
        arrowX2 = clamp(arrowX2, margin_left + 10, width - margin_right - 10);
        arrowY2 = clamp(arrowY2, margin_top + 10, height - margin_bottom - 10);
        
        // Create a different color for each arrow by varying brightness
        RGBA thisArrowColor = arrowColor;
        float brightnessMultiplier = 0.5f + (static_cast<float>(i) / sorted_eig_vecs.size()) * 0.5f;
        thisArrowColor.r = static_cast<unsigned char>(std::min(255.0f, thisArrowColor.r * brightnessMultiplier));
        thisArrowColor.g = static_cast<unsigned char>(std::min(255.0f, thisArrowColor.g * brightnessMultiplier));
        thisArrowColor.b = static_cast<unsigned char>(std::min(255.0f, thisArrowColor.b * brightnessMultiplier));
        
        // Draw the arrow
        drawArrow(arrowX1, arrowY1, arrowX2, arrowY2, thisArrowColor, arrowSize);
    }
}

}  // namespace shmea
