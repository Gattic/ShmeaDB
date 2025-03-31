#include "BaseDrawer.h"

namespace shmea {

BaseDrawer::BaseDrawer(Image& image, unsigned int width, unsigned int height, 
                       int margin_top, int margin_right, int margin_bottom, int margin_left)
    : image(image), width(width), height(height), 
      margin_top(margin_top), margin_right(margin_right), 
      margin_bottom(margin_bottom), margin_left(margin_left) {
}

int BaseDrawer::clamp(int value, int min, int max) const {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

void BaseDrawer::drawPoint(int x, int y, int thickness, const RGBA& pointColor) {
    // Check if the main point is within the plotting area bounds
    if (x >= margin_left && x < (width - margin_right) &&
        y >= margin_top && y < (height - margin_bottom)) {
        
        // Draw the main point
        image.SetPixel(x, y, pointColor);

        // Draw additional pixels for thickness with bounds checking to make it round
        for (int dx = -thickness; dx <= thickness; ++dx) {
            for (int dy = -thickness; dy <= thickness; ++dy) {
                // Check if the pixel falls within the circle defined by the thickness
                if (dx * dx + dy * dy <= thickness * thickness) {
                    int newX = x + dx;
                    int newY = y + dy;
                    if (newX >= margin_left && newX < (width - margin_right) &&
                        newY >= margin_top && newY < (height - margin_bottom)) {
                        image.SetPixel(newX, newY, pointColor);
                    }
                }
            }
        }
    }
}

void BaseDrawer::drawLine(int x1, int y1, int x2, int y2, const RGBA& lineColor, int lineWidth) {
    x1 = clamp(x1, margin_left, width - margin_right);
    x2 = clamp(x2, margin_left, width - margin_right);
    y1 = clamp(y1, margin_top, height - margin_bottom);
    y2 = clamp(y2, margin_top, height - margin_bottom);

    //Bresenham's Line algorithm (or close to it)
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);

    bool steep = dy > dx;
    
    //Swap if the line is steep (more vertical than horizontal)
    if(steep) {
        //swap x1 and y1
        int temp = x1;
        x1 = y1;
        y1 = temp;
        
        //Swap x2 and y2
        temp = x2;
        x2 = y2;
        y2 = temp;
        
        //Swap dx and dy
        temp = dx;
        dx = dy;
        dy = temp;
    }

    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    
    double gradient = static_cast<double>(dy) / static_cast<double>(dx);
    double err = 0.0;

    while (true) {
        // Draw a filled circle or rectangle for each point to ensure consistent thickness
        for (int i = -lineWidth / 2; i <= lineWidth / 2; ++i) {
            for (int j = -lineWidth / 2; j <= lineWidth / 2; ++j) {
                int newX = steep ? y1 + i : x1 + i;
                int newY = steep ? x1 + j : y1 + j;

                if (newX >= margin_left && newX < (width - margin_right) &&
                    newY >= margin_top && newY < (height - margin_bottom)) {
                    image.SetPixel(newX, newY, lineColor);
                }
            }
        }
        
        //Check if the end has been reached
        if (x1 == x2 && y1 == y2)
            break;
        
        err += gradient;
        if(err >= 0.5) {
            y1 += sy;
            err -= 1.0;
        }
        x1 += sx;
    }
}

void BaseDrawer::drawCircle(int x, int y, int radius, const RGBA& color) {
    // Draw a circle using the Midpoint Circle Algorithm
    int x0 = 0;
    int y0 = radius;
    int d = 3 - 2 * radius;
    
    // Draw the initial points
    drawCirclePoints(x, y, x0, y0, color);
    
    // Iterate to draw the complete circle
    while (x0 <= y0) {
        x0++;
        if (d > 0) {
            y0--;
            d = d + 4 * (x0 - y0) + 10;
        } else {
            d = d + 4 * x0 + 6;
        }
        drawCirclePoints(x, y, x0, y0, color);
    }
    
    // Draw additional circles with slightly different radii for thickness
    for (int thickness = 1; thickness <= 3; thickness++) {
        // Draw inner circle
        if (radius - thickness > 0) {
            int innerX0 = 0;
            int innerY0 = radius - thickness;
            int innerD = 3 - 2 * (radius - thickness);
            
            drawCirclePoints(x, y, innerX0, innerY0, color);
            
            while (innerX0 <= innerY0) {
                innerX0++;
                if (innerD > 0) {
                    innerY0--;
                    innerD = innerD + 4 * (innerX0 - innerY0) + 10;
                } else {
                    innerD = innerD + 4 * innerX0 + 6;
                }
                drawCirclePoints(x, y, innerX0, innerY0, color);
            }
        }
        
        // Draw outer circle
        int outerX0 = 0;
        int outerY0 = radius + thickness;
        int outerD = 3 - 2 * (radius + thickness);
        
        drawCirclePoints(x, y, outerX0, outerY0, color);
        
        while (outerX0 <= outerY0) {
            outerX0++;
            if (outerD > 0) {
                outerY0--;
                outerD = outerD + 4 * (outerX0 - outerY0) + 10;
            } else {
                outerD = outerD + 4 * outerX0 + 6;
            }
            drawCirclePoints(x, y, outerX0, outerY0, color);
        }
    }
}

void BaseDrawer::drawCirclePoints(int x, int y, int x0, int y0, const RGBA& color) {
    // Draw points with additional pixels for thickness
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            // Check if points are within the plotting area bounds
            if (x + x0 + dx >= margin_left && x + x0 + dx < width - margin_right &&
                y + y0 + dy >= margin_top && y + y0 + dy < height - margin_bottom) {
                image.SetPixel(x + x0 + dx, y + y0 + dy, color);
            }
            if (x - x0 + dx >= margin_left && x - x0 + dx < width - margin_right &&
                y + y0 + dy >= margin_top && y + y0 + dy < height - margin_bottom) {
                image.SetPixel(x - x0 + dx, y + y0 + dy, color);
            }
            if (x + x0 + dx >= margin_left && x + x0 + dx < width - margin_right &&
                y - y0 + dy >= margin_top && y - y0 + dy < height - margin_bottom) {
                image.SetPixel(x + x0 + dx, y - y0 + dy, color);
            }
            if (x - x0 + dx >= margin_left && x - x0 + dx < width - margin_right &&
                y - y0 + dy >= margin_top && y - y0 + dy < height - margin_bottom) {
                image.SetPixel(x - x0 + dx, y - y0 + dy, color);
            }
            if (x + y0 + dx >= margin_left && x + y0 + dx < width - margin_right &&
                y + x0 + dy >= margin_top && y + x0 + dy < height - margin_bottom) {
                image.SetPixel(x + y0 + dx, y + x0 + dy, color);
            }
            if (x - y0 + dx >= margin_left && x - y0 + dx < width - margin_right &&
                y + x0 + dy >= margin_top && y + x0 + dy < height - margin_bottom) {
                image.SetPixel(x - y0 + dx, y + x0 + dy, color);
            }
            if (x + y0 + dx >= margin_left && x + y0 + dx < width - margin_right &&
                y - x0 + dy >= margin_top && y - x0 + dy < height - margin_bottom) {
                image.SetPixel(x + y0 + dx, y - x0 + dy, color);
            }
            if (x - y0 + dx >= margin_left && x - y0 + dx < width - margin_right &&
                y - x0 + dy >= margin_top && y - x0 + dy < height - margin_bottom) {
                image.SetPixel(x - y0 + dx, y - x0 + dy, color);
            }
        }
    }
}

}  // namespace shmea
