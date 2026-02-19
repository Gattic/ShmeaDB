#include "GridRenderer.h"
#include "TextRenderer.h"
#include <cmath>
#include <cstdio>

namespace shmea {

GridRenderer::GridRenderer(SuperSamplingManager& ssaaManager, ColorManager& colorManager, 
                       ChartLayout& chartLayout, ShapeRenderer& shapeRenderer,
			FT_Library& newFT, FT_Face& newFace)
    : ssaa(ssaaManager),
      colors(colorManager),
      layout(chartLayout),
      shapes(shapeRenderer),
      ft(newFT),
      face(newFace)
{
}

GridRenderer::~GridRenderer() {
    // Nothing to clean up
}

void GridRenderer::drawBackground() {
    // Draw the dark gradient background from CSS styles
    // Using radial gradient similar to histogram_with_labels.css:
    // radial-gradient(164.63% 83.5% at 54.69% 50%, #021331 0%, #000B1E 100%)

    // Cache dimensions and image reference to avoid repeated getter calls
    unsigned int w = ssaa.getWidth();
    unsigned int h = ssaa.getHeight();
    Image& img = ssaa.getImage();
    RGBA* pixels = img.getData();

    // Define the gradient center point (at 54.69% 50% as specified in CSS)
    float centerX = w * 0.5469f;
    float centerY = h * 0.5f;

    // Define the gradient radius (164.63% width and 83.5% height elliptical gradient)
    float radiusX = w * 1.6463f;
    float radiusY = h * 0.835f;

    // Precompute reciprocals to replace division with multiplication
    float invRadiusX = 1.0f / radiusX;
    float invRadiusY = 1.0f / radiusY;

    // Get colors from our pre-defined palette
    RGBA centerColor = colors.getElementColor("bgGradientTop");    // #021331
    RGBA edgeColor = colors.getElementColor("bgGradientBottom");   // #000B1E

    // Precompute 256-entry gradient lookup table indexed by quantized dist value
    // This replaces per-pixel float interpolation (6 multiplies + 3 adds + 3 casts)
    // with a single table lookup per pixel
    static const unsigned int LUT_SIZE = 256;
    RGBA gradientLUT[LUT_SIZE];
    for (unsigned int i = 0; i < LUT_SIZE; ++i) {
        float dist = static_cast<float>(i) / (LUT_SIZE - 1);
        float oneMinusDist = 1.0f - dist;
        gradientLUT[i].r = static_cast<unsigned char>(centerColor.r * oneMinusDist + edgeColor.r * dist);
        gradientLUT[i].g = static_cast<unsigned char>(centerColor.g * oneMinusDist + edgeColor.g * dist);
        gradientLUT[i].b = static_cast<unsigned char>(centerColor.b * oneMinusDist + edgeColor.b * dist);
        gradientLUT[i].a = 0xFF;
    }

    // Precompute vignette boundary and per-row fade factors
    unsigned int vignetteFadeHeight = static_cast<unsigned int>(h * 0.2f);

    // Render gradient + vignette in a single pass using LUT
    float lutScale = static_cast<float>(LUT_SIZE - 1);
    for (unsigned int y = 0; y < h; ++y) {
        // Hoist dy calculation out of the inner loop - constant per row
        float dy = (y - centerY) * invRadiusY;
        float dySq = dy * dy;
        RGBA* row = pixels + y * w;

        // Precompute vignette factor for this row (applied only to top 20%)
        float vignetteScale = 1.0f;
        if (y < vignetteFadeHeight) {
            float fadeFactor = 1.0f - (static_cast<float>(y) / vignetteFadeHeight);
            fadeFactor = fadeFactor * fadeFactor * 0.5f;
            vignetteScale = 1.0f - fadeFactor;
        }

        if (y < vignetteFadeHeight) {
            // Vignette rows: apply darkening during the LUT write (single pass)
            for (unsigned int x = 0; x < w; ++x) {
                float dx = (x - centerX) * invRadiusX;
                float distSq = dx*dx + dySq;
                if (distSq > 1.0f) distSq = 1.0f;
                float dist = distSq * distSq;
                unsigned int lutIdx = static_cast<unsigned int>(dist * lutScale);
                const RGBA& c = gradientLUT[lutIdx];
                row[x].r = static_cast<unsigned char>(c.r * vignetteScale);
                row[x].g = static_cast<unsigned char>(c.g * vignetteScale);
                row[x].b = static_cast<unsigned char>(c.b * vignetteScale);
                row[x].a = 0xFF;
            }
        } else {
            // Non-vignette rows: pure LUT copy
            for (unsigned int x = 0; x < w; ++x) {
                float dx = (x - centerX) * invRadiusX;
                float distSq = dx*dx + dySq;
                if (distSq > 1.0f) distSq = 1.0f;
                float dist = distSq * distSq;
                unsigned int lutIdx = static_cast<unsigned int>(dist * lutScale);
                row[x] = gradientLUT[lutIdx];
            }
        }
    }

    // Draw grid if enabled
    if (layout.isGridVisible()) {
        drawGrid();
    }
}

void GridRenderer::drawGrid() {
    // Cache dimensions and image reference
    unsigned int w = ssaa.getWidth();
    unsigned int h = ssaa.getHeight();
    Image& img = ssaa.getImage();
    int samplingFactor = ssaa.getSamplingFactor();

    // Calculate the effective plotting area considering the margins
    int effectiveWidth = w - ssaa.scaleX(layout.getMarginLeft()) - ssaa.scaleX(layout.getMarginRight());
    int effectiveHeight = h - ssaa.scaleY(layout.getMarginTop()) - ssaa.scaleY(layout.getMarginBottom());

    // Use exact grid pattern from CSS files in concepts/
    // Matching the fig and pdf files' grid styling
    int gridDivisionsX = 8;  // 8 vertical gridlines for better spacing
    int gridDivisionsY = 5;  // 5 horizontal gridlines like in the design

    // Get colors for the grid lines with exact opacity from CSS
    RGBA gridColor = colors.getElementColor("majorGrid");
    gridColor.a = 0x66; // Increased to 40% opacity for better visibility

    // Draw X-axis grid lines (vertical lines)
    for (int i = 1; i < gridDivisionsX; i++) {
        float percentage = static_cast<float>(i) / gridDivisionsX;
        int x = ssaa.scaleX(layout.getMarginLeft()) + static_cast<int>(percentage * effectiveWidth);

        // Draw grid line with exact 1px width (scaled for supersampling)
        int lineWidth = samplingFactor;
        for (int y = ssaa.scaleY(layout.getMarginTop()); y <= static_cast<int>(h - ssaa.scaleY(layout.getMarginBottom())); y++) {
            for (int dx = 0; dx < lineWidth; dx++) {
                int drawX = x + dx;
                if (drawX >= 0 && drawX < static_cast<int>(w) && y >= 0 && y < static_cast<int>(h)) {
                    RGBA currentPixel = img.GetPixel(drawX, y);
                    RGBA blendedColor = colors.blendRGBA(currentPixel, gridColor);
                    img.SetPixel(drawX, y, blendedColor);
                }
            }
        }
    }

    // Draw Y-axis grid lines (horizontal lines)
    for (int i = 1; i < gridDivisionsY; i++) {
        float percentage = static_cast<float>(i) / gridDivisionsY;
        int y = h - ssaa.scaleY(layout.getMarginBottom()) - static_cast<int>(percentage * effectiveHeight);

        // Draw grid line with exact 1px width (scaled for supersampling)
        int lineWidth = samplingFactor;
        for (int x = ssaa.scaleX(layout.getMarginLeft()); x <= static_cast<int>(w - ssaa.scaleX(layout.getMarginRight())); x++) {
            for (int dy = 0; dy < lineWidth; dy++) {
                int drawY = y + dy;
                if (x >= 0 && x < static_cast<int>(w) && drawY >= 0 && drawY < static_cast<int>(h)) {
                    RGBA currentPixel = img.GetPixel(x, drawY);
                    RGBA blendedColor = colors.blendRGBA(currentPixel, gridColor);
                    img.SetPixel(x, drawY, blendedColor);
                }
            }
        }
    }
    
    // Draw outer border with exact styling from CSS
    RGBA borderColor = colors.getElementColor("border");
    borderColor.a = 0x66; // Increased to 40% opacity to match grid lines
    int borderWidth = samplingFactor;  // Scale border width with supersampling

    // Calculate border bounds with proper rounded corners
    int scaledCornerRadius = ssaa.scaleSize(layout.getCornerRadius());
    int leftX = ssaa.scaleX(layout.getMarginLeft());
    int topY = ssaa.scaleY(layout.getMarginTop());
    int rightX = w - ssaa.scaleX(layout.getMarginRight());
    int bottomY = h - ssaa.scaleY(layout.getMarginBottom());
    
    // Draw straight border segments (avoiding the corner regions)
    
    // Top border (from left corner to right corner)
    shapes.drawLine(leftX + scaledCornerRadius, topY, 
               rightX - scaledCornerRadius, topY, 
               borderColor, borderWidth);
             
    // Bottom border (from left corner to right corner)
    shapes.drawLine(leftX + scaledCornerRadius, bottomY, 
               rightX - scaledCornerRadius, bottomY, 
               borderColor, borderWidth);
             
    // Left border (from top corner to bottom corner)
    shapes.drawLine(leftX, topY + scaledCornerRadius, 
               leftX, bottomY - scaledCornerRadius, 
               borderColor, borderWidth);
             
    // Right border (from top corner to bottom corner)
    shapes.drawLine(rightX, topY + scaledCornerRadius, 
               rightX, bottomY - scaledCornerRadius, 
               borderColor, borderWidth);
    
    // Add rounded corners with proper supersampling and anti-aliasing
    shapes.drawRoundedCorners(leftX, topY, 
                       rightX, bottomY, 
                       scaledCornerRadius, borderColor);
}

void GridRenderer::drawAxes() {
    // Get axis color from CSS-like design
    RGBA axisColor = colors.getElementColor("axes");
    axisColor.a = 0xDD; // 85% opacity for better contrast
    
    int width = layout.getWidth();
    int height = layout.getHeight();
    int marginLeft = layout.getMarginLeft();
    int marginRight = layout.getMarginRight();
    int marginTop = layout.getMarginTop();
    int marginBottom = layout.getMarginBottom();
    
    // Calculate the effective plotting area
    int plotWidth = layout.getPlotWidth();
    int plotHeight = layout.getPlotHeight();
    
    // Calculate axis positions
    int xAxisY = height - marginBottom;
    int yAxisX = marginLeft;
    
    // Draw X-axis line (horizontal line)
    shapes.drawLine(
        ssaa.scaleX(marginLeft),
        ssaa.scaleY(xAxisY),
        ssaa.scaleX(marginLeft + plotWidth),
        ssaa.scaleY(xAxisY),
        axisColor,
        ssaa.scaleSize(2) // 2px width like in CSS
    );
    
    // Draw Y-axis line (vertical line)
    shapes.drawLine(
        ssaa.scaleX(yAxisX),
        ssaa.scaleY(marginTop),
        ssaa.scaleX(yAxisX),
        ssaa.scaleY(marginTop + plotHeight),
        axisColor,
        ssaa.scaleSize(2) // 2px width like in CSS
    );
    
    // Draw origin indicator (small circle at origin point)
    shapes.drawCircle(
        ssaa.scaleX(marginLeft),
        ssaa.scaleY(height - marginBottom),
        ssaa.scaleSize(4), // 4px radius
        axisColor,
        true // filled
    );
}

void GridRenderer::drawOriginAxes(double xMin, double xMax, double yMin, double yMax) {
    // Get axis color from CSS-like design with increased contrast for four-quadrant view
    RGBA axisColor = colors.getElementColor("axes");
    axisColor.a = 0xFF; // 100% opacity for better contrast
    
    // Calculate plot area dimensions
    int width = layout.getWidth();
    int height = layout.getHeight();
    int marginLeft = layout.getMarginLeft();
    int marginRight = layout.getMarginRight();
    int marginTop = layout.getMarginTop();
    int marginBottom = layout.getMarginBottom();
    int plotWidth = layout.getPlotWidth();
    int plotHeight = layout.getPlotHeight();
    
    // Calculate where the origin (0,0) should be in screen coordinates
    // This calculation must be precise for proper alignment with data points
    double originXPercent = -xMin / (xMax - xMin);
    double originYPercent = 1.0 - (-yMin / (yMax - yMin)); // Y-axis is inverted
    
    // Make sure origin percentage is within valid range [0,1]
    originXPercent = std::max(0.0, std::min(1.0, originXPercent));
    originYPercent = std::max(0.0, std::min(1.0, originYPercent));
    
    // Calculate origin coordinates in screen space
    int originX = marginLeft + static_cast<int>(originXPercent * plotWidth + 0.5); // Round to nearest pixel
    int originY = marginTop + static_cast<int>(originYPercent * plotHeight + 0.5); // Round to nearest pixel
    
    printf("Drawing origin axes at screen coordinates: (%d, %d)\n", originX, originY);
    printf("Data ranges - X: [%.2f, %.2f], Y: [%.2f, %.2f]\n", xMin, xMax, yMin, yMax);
    
    // Draw X-axis line (horizontal line through origin)
    shapes.drawLine(
        ssaa.scaleX(marginLeft),
        ssaa.scaleY(originY),
        ssaa.scaleX(marginLeft + plotWidth),
        ssaa.scaleY(originY),
        axisColor,
        ssaa.scaleSize(2) // 2px width
    );
    
    // Draw Y-axis line (vertical line through origin)
    shapes.drawLine(
        ssaa.scaleX(originX),
        ssaa.scaleY(marginTop),
        ssaa.scaleX(originX),
        ssaa.scaleY(marginTop + plotHeight),
        axisColor,
        ssaa.scaleSize(2) // 2px width
    );
    
    // Draw origin indicator (small circle at origin point)
    shapes.drawCircle(
        ssaa.scaleX(originX),
        ssaa.scaleY(originY),
        ssaa.scaleSize(5), // 5px radius - slightly larger for emphasis
        axisColor,
        true // filled
    );
    
    // Create TextRenderer for the axis labels
    TextRenderer textRenderer(ssaa, colors, layout);
    textRenderer.initialize(ft, face);
    
    // Draw quadrant indicators
    RGBA labelColor = colors.getElementColor("axisLabel");
    labelColor.a = 0xCC; // 80% opacity
    
    // Quadrant I (top-right)
    if (xMax > 0 && yMax > 0) {
        int labelX = originX + 20;
        int labelY = originY - 20;
        if (labelX < width - marginRight - 30 && labelY > marginTop + 30) {
            textRenderer.drawText(labelX, labelY, "I", labelColor, 18, true);
        }
    }
    
    // Quadrant II (top-left)
    if (xMin < 0 && yMax > 0) {
        int labelX = originX - 20;
        int labelY = originY - 20;
        if (labelX > marginLeft + 30 && labelY > marginTop + 30) {
            textRenderer.drawText(labelX, labelY, "II", labelColor, 18, true);
        }
    }
    
    // Quadrant III (bottom-left)
    if (xMin < 0 && yMin < 0) {
        int labelX = originX - 20;
        int labelY = originY + 20;
        if (labelX > marginLeft + 30 && labelY < height - marginBottom - 30) {
            textRenderer.drawText(labelX, labelY, "III", labelColor, 18, true);
        }
    }
    
    // Quadrant IV (bottom-right)
    if (xMax > 0 && yMin < 0) {
        int labelX = originX + 20;
        int labelY = originY + 20;
        if (labelX < width - marginRight - 30 && labelY < height - marginBottom - 30) {
            textRenderer.drawText(labelX, labelY, "IV", labelColor, 18, true);
        }
    }
    
    // If X range spans zero, add "0" label on the y-axis
    if (xMin < 0 && xMax > 0) {
        textRenderer.drawText(originX, originY + 15, "0", labelColor, 16, true);
    }
}

void GridRenderer::drawInfoBox(int x, int y, int boxWidth, int boxHeight, const std::string& text, unsigned int fontSize) {
    // Cache dimensions and image reference
    unsigned int w = ssaa.getWidth();
    unsigned int h = ssaa.getHeight();
    Image& img = ssaa.getImage();

    // Scale coordinates and dimensions for supersampling
    int ssaaX = ssaa.scaleX(x);
    int ssaaY = ssaa.scaleY(y);
    int ssaaBoxWidth = ssaa.scaleSize(boxWidth);
    int ssaaBoxHeight = ssaa.scaleSize(boxHeight);
    int ssaaCornerRadius = ssaa.scaleSize(8); // Common corner radius for all info boxes, scaled
    float cornerRadiusSq = static_cast<float>(ssaaCornerRadius) * ssaaCornerRadius;

    // Get gradient colors from element colors - exact colors from CSS
    RGBA bgTopColor = colors.getElementColor("legendBgTop");
    RGBA bgBottomColor = colors.getElementColor("legendBgBottom");

    // Precompute reciprocal for gradient interpolation
    float invGradTotal = 1.0f / static_cast<float>(ssaaBoxWidth + ssaaBoxHeight);

    // Draw gradient background with proper alpha blending
    for (int dy = 0; dy < ssaaBoxHeight; dy++) {
        // Calculate gradient interpolation - diagonal gradient to match the histogram stats box
        for (int dx = 0; dx < ssaaBoxWidth; dx++) {
            float gradPos = (dx + dy) * invGradTotal;
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
                float cdx = ssaaCornerRadius - dx;
                float cdy = ssaaCornerRadius - dy;
                inCorner = (cdx * cdx + cdy * cdy) > cornerRadiusSq;
            }
            // Check top-right corner
            else if (dx >= ssaaBoxWidth - ssaaCornerRadius && dy < ssaaCornerRadius) {
                float cdx = dx - (ssaaBoxWidth - ssaaCornerRadius);
                float cdy = ssaaCornerRadius - dy;
                inCorner = (cdx * cdx + cdy * cdy) > cornerRadiusSq;
            }
            // Check bottom-left corner
            else if (dx < ssaaCornerRadius && dy >= ssaaBoxHeight - ssaaCornerRadius) {
                float cdx = ssaaCornerRadius - dx;
                float cdy = dy - (ssaaBoxHeight - ssaaCornerRadius);
                inCorner = (cdx * cdx + cdy * cdy) > cornerRadiusSq;
            }
            // Check bottom-right corner
            else if (dx >= ssaaBoxWidth - ssaaCornerRadius && dy >= ssaaBoxHeight - ssaaCornerRadius) {
                float cdx = dx - (ssaaBoxWidth - ssaaCornerRadius);
                float cdy = dy - (ssaaBoxHeight - ssaaCornerRadius);
                inCorner = (cdx * cdx + cdy * cdy) > cornerRadiusSq;
            }

            // Only draw pixel if it's within the rounded rectangle
            if (!inCorner) {
                int drawX = ssaaX + dx;
                int drawY = ssaaY + dy;

                if (drawX >= 0 && drawX < static_cast<int>(w) &&
                    drawY >= 0 && drawY < static_cast<int>(h)) {
                    // Use proper alpha blending
                    RGBA existingPixel = img.GetPixel(drawX, drawY);
                    RGBA blendedColor = colors.blendRGBA(existingPixel, currentBgColor);
                    img.SetPixel(drawX, drawY, blendedColor);
                }
            }
        }
    }

    // Add subtle inner highlight to top edge - exact 10% opacity from CSS
    RGBA highlightColor(0xFF, 0xFF, 0xFF, 0x1A); // 10% white (0.1 * 255 = 26 ≈ 0x1A)
    for (int dx = ssaaCornerRadius; dx < ssaaBoxWidth - ssaaCornerRadius; dx++) {
        int pixelX = ssaaX + dx;
        int pixelY = ssaaY;

        if (pixelX >= 0 && pixelX < static_cast<int>(w) &&
            pixelY >= 0 && pixelY < static_cast<int>(h)) {
            // Blend highlight with existing pixel
            RGBA existingPixel = img.GetPixel(pixelX, pixelY);
            RGBA blendedColor = colors.blendRGBA(existingPixel, highlightColor);
            img.SetPixel(pixelX, pixelY, blendedColor);
        }
    }

    // Add subtle drop shadow to bottom edge - exact 15% opacity from CSS
    RGBA shadowColor(0x00, 0x00, 0x00, 0x26); // 15% black (0.15 * 255 = 38 ≈ 0x26)
    for (int dx = ssaaCornerRadius; dx < ssaaBoxWidth - ssaaCornerRadius; dx++) {
        int pixelX = ssaaX + dx;
        int pixelY = ssaaY + ssaaBoxHeight - 1;

        if (pixelX >= 0 && pixelX < static_cast<int>(w) &&
            pixelY >= 0 && pixelY < static_cast<int>(h)) {
            // Blend shadow with existing pixel
            RGBA existingPixel = img.GetPixel(pixelX, pixelY);
            RGBA blendedColor = colors.blendRGBA(existingPixel, shadowColor);
            img.SetPixel(pixelX, pixelY, blendedColor);
        }
    }
    
    // Draw the text if provided
    if (!text.empty()) {
        // Create a TextRenderer instance for text rendering
        TextRenderer textRenderer(ssaa, colors, layout);
	textRenderer.initialize(ft, face);
        
        // Text position using original (non-supersampled) coordinates
        RGBA textColor = colors.getElementColor("legend"); // White text
        int textY = y + boxHeight/2;
        int textX = x + 15; // 15px padding from left edge
        textRenderer.drawText(textX, textY, text, textColor, fontSize, false);
    }
}

//I am forgoing the labels and numTicsk comparison. The number of labels determines how many labels are going on the x-axis
void GridRenderer::drawXAxisTicks(const std::vector<DataMapper::CandleXAxis>& labels, const Series& series )
{
    // Get text color from CSS design
    RGBA textColor = colors.getElementColor("axisLabel");
    textColor.a = 0xCC; // 80% opacity - exactly like in plotter.cpp
    
    // Create TextRenderer instance for text rendering
    TextRenderer textRenderer(ssaa, colors, layout);
    textRenderer.initialize(ft, face);
    
    // Define labelY here to make it available throughout the function
    int labelY = layout.getHeight() - layout.getMarginBottom() + 25; // Exact position from plotter.cpp
    int startX = layout.getMarginLeft();
    int tickIndex = 0;
    // Draw ticks at specific intervals - exactly like in plotter.cpp
    // This creates tick marks at positions 0, tickInterval, 2*tickInterval, etc.
    for(int i = 0; i < series.data.size(); ++i)
    {
	double serieTimestamp = series.data[i].x;
        double labelTimestamp = labels[tickIndex].timestamp;

	if(i == 0 || tickIndex < labels.size() && labelTimestamp <= serieTimestamp)
	{
		int indexPos = i;
		if(labelTimestamp != serieTimestamp)
		{
			--indexPos;
		}

		int x = startX + indexPos;
        
        	// Draw tick mark - exactly like plotter.cpp
	        shapes.drawLine(
           	 ssaa.scaleX(x), 
            	ssaa.scaleY(layout.getHeight() - layout.getMarginBottom()), 
            	ssaa.scaleX(x), 
            	ssaa.scaleY(layout.getHeight() - layout.getMarginBottom() + 6), 
            	textColor, 
            	ssaa.getSamplingFactor());
       
//TODO: Labels will need its own index 
        	textRenderer.drawText(x, labelY, labels[tickIndex].timeLabel, textColor, 22, true); // Center-aligned with 22px font
		++tickIndex;
	}
    }
    
    // Note: We don't add a special case for the last label here because in plotter.cpp,
    // the specific histogram implementation in plotHistogram() handles drawing labels
    // for index positions 0, 4, 8, etc. and the last index directly in its loop.
	return;
}
//I am forgoing the labels and numTicsk comparison. The number of labels determines how many labels are going on the x-axis
void GridRenderer::drawCandleXAxisTicks(const std::vector<DataMapper::CandleData>& candles, const std::vector<DataMapper::CandleXAxis>& labels, int startingOffset, int candleOffset)
{
    // Get text color from CSS design
    RGBA textColor = colors.getElementColor("axisLabel");
    textColor.a = 0xCC; // 80% opacity - exactly like in plotter.cpp
    
    // Create TextRenderer instance for text rendering
    TextRenderer textRenderer(ssaa, colors, layout);
    textRenderer.initialize(ft, face);
    
    // Define labelY here to make it available throughout the function
    int labelY = layout.getHeight() - layout.getMarginBottom() + 25; // Exact position from plotter.cpp
    int startX = layout.getMarginLeft() + startingOffset / 2;
    int tickIndex = 0;
    // Draw ticks at specific intervals - exactly like in plotter.cpp
    // This creates tick marks at positions 0, tickInterval, 2*tickInterval, etc.
    for (int i = 0; i < candles.size(); ++i) {

	double candleTimestamp = candles[i].timestamp;
	double labelTimestamp = labels[tickIndex].timestamp;
	
	if(i == 0 || (tickIndex < labels.size() && (labelTimestamp <= candleTimestamp)))
	{
		int indexPos = i;
		if (labelTimestamp != candleTimestamp)
		{
			--indexPos;
		}
		int x = startX + indexPos * candleOffset + candleOffset/2;
        
        	// Draw tick mark - exactly like plotter.cpp
	        shapes.drawLine(
           	 ssaa.scaleX(x), 
            	ssaa.scaleY(layout.getHeight() - layout.getMarginBottom()), 
            	ssaa.scaleX(x), 
            	ssaa.scaleY(layout.getHeight() - layout.getMarginBottom() + 6), 
            	textColor, 
            	ssaa.getSamplingFactor());
       
//TODO: Labels will need its own index 
        	textRenderer.drawText(x, labelY, labels[tickIndex].timeLabel, textColor, 22, true); // Center-aligned with 22px font
		++tickIndex;
	}
    }
    
    // Note: We don't add a special case for the last label here because in plotter.cpp,
    // the specific histogram implementation in plotHistogram() handles drawing labels
    // for index positions 0, 4, 8, etc. and the last index directly in its loop.
	return;
}


//TODO: We are just guessing where the tickers should go, instead we should be smarter about it
void GridRenderer::drawXAxisTicks(const std::vector<std::string>& labels, int numTicks, XPositionMode xMode, int startingOffset) {
    int plotWidth = layout.getPlotWidth();
    
    // Use the same colors as other grid elements for CSS consistency
    // RGBA gridColor = colors.getElementColor("majorGrid");
    // gridColor.a = 0x66; // 40% opacity to match grid lines - exactly like plotter.cpp
    
    // Get text color from CSS design
    RGBA textColor = colors.getElementColor("axisLabel");
    textColor.a = 0xCC; // 80% opacity - exactly like in plotter.cpp
    
    int totalLabels = static_cast<int>(labels.size());
    
    // Use the exact calculation from plotter.cpp for tick interval
    int tickInterval = std::max(1, totalLabels / numTicks);
    
    // Create TextRenderer instance for text rendering
    TextRenderer textRenderer(ssaa, colors, layout);
    textRenderer.initialize(ft, face);
    
    // Define labelY here to make it available throughout the function
    int labelY = layout.getHeight() - layout.getMarginBottom() + 25; // Exact position from plotter.cpp
    int startX = layout.getMarginLeft() + startingOffset / 2; 
    // Draw ticks at specific intervals - exactly like in plotter.cpp
    // This creates tick marks at positions 0, tickInterval, 2*tickInterval, etc.
    for (int i = 0; i < totalLabels; i += tickInterval) {
        // Calculate percentage along X-axis - exact formula from plotter.cpp
	float percentage = static_cast<float>(i) / (totalLabels - 1);
	int x = startX + static_cast<int>(percentage * (plotWidth - startingOffset));
        
        // Draw tick mark - exactly like plotter.cpp
        shapes.drawLine(
            ssaa.scaleX(x), 
            ssaa.scaleY(layout.getHeight() - layout.getMarginBottom()), 
            ssaa.scaleX(x), 
            ssaa.scaleY(layout.getHeight() - layout.getMarginBottom() + 6), 
            textColor, 
            ssaa.getSamplingFactor());
        
        // Draw label - exact positioning from plotter.cpp
        textRenderer.drawText(x, labelY, labels[i], textColor, 22, true); // Center-aligned with 22px font
    }
    
    // Note: We don't add a special case for the last label here because in plotter.cpp,
    // the specific histogram implementation in plotHistogram() handles drawing labels
    // for index positions 0, 4, 8, etc. and the last index directly in its loop.
}

void GridRenderer::drawXAxisTicks(double minValue, double maxValue, int numTicks, int precision) {
    int plotWidth = layout.getPlotWidth();
    
    // Use the same colors as other grid elements for CSS consistency
    // RGBA gridColor = colors.getElementColor("majorGrid");
    // gridColor.a = 0x66; // 40% opacity to match grid lines - exactly like plotter.cpp
    
    // Get text color from CSS design
    RGBA textColor = colors.getElementColor("axisLabel");
    textColor.a = 0xCC; // 80% opacity - exactly like in plotter.cpp
    
    // Create TextRenderer instance for text rendering
    TextRenderer textRenderer(ssaa, colors, layout);
    textRenderer.initialize(ft, face);
    
    // Draw ticks at precise intervals - exactly like plotter.cpp
    for (int i = 0; i < numTicks; ++i) {
        float percentage = static_cast<float>(i) / (numTicks - 1);
        int x = layout.getMarginLeft() + static_cast<int>(percentage * plotWidth);
        double value = minValue + percentage * (maxValue - minValue);
        
        // Draw tick mark - exactly like plotter.cpp
        shapes.drawLine(
            ssaa.scaleX(x), 
            ssaa.scaleY(layout.getHeight() - layout.getMarginBottom()), 
            ssaa.scaleX(x), 
            ssaa.scaleY(layout.getHeight() - layout.getMarginBottom() + 6), 
            textColor, 
            ssaa.getSamplingFactor());
        
        // Format the value with consistent precision from plotter.cpp
        char valueText[32];
        char formatStr[10];
	int dPoints = 0;
	double tempV = fabs(value);
	if(tempV > 0 && tempV < 1)
	{
		while(fabs(tempV) < 1 && dPoints < 10)
		{
			tempV *= 10;
			dPoints++;
		}
	}
	const int maxPrecision = 10;
	int finalPrecision = std::min(precision + dPoints, 10); 
        std::sprintf(formatStr, "%%.%df", finalPrecision);
        std::sprintf(valueText, formatStr, value);
        
        // Draw value label - exact positioning from plotter.cpp
        int labelY = layout.getHeight() - layout.getMarginBottom() + 25; // Exact position from plotter.cpp
        textRenderer.drawText(x, labelY, valueText, textColor, 22, true); // Center-aligned with 22px font
    }
}

void GridRenderer::drawYAxisTicks(double minValue, double maxValue, int numTicks, bool isInteger, 
                               int precision, int labelOffset) {
    int plotHeight = layout.getPlotHeight();
    
    // Use the same colors as other grid elements for CSS consistency
    // RGBA gridColor = colors.getElementColor("majorGrid");
    // gridColor.a = 0x66; // 40% opacity to match grid lines - exactly like plotter.cpp
    
    // Get text color from CSS design
    RGBA textColor = colors.getElementColor("axisLabel");
    textColor.a = 0xCC; // 80% opacity - exactly like in plotter.cpp
    
    // Create TextRenderer instance for text rendering
    TextRenderer textRenderer(ssaa, colors, layout);
    textRenderer.initialize(ft, face);
    
    // Draw each tick mark and label - exactly like in plotter.cpp
    for (int i = 0; i <= numTicks; ++i) {
        float percentage = static_cast<float>(i) / numTicks;
        int y = layout.getHeight() - layout.getMarginBottom() - static_cast<int>(percentage * plotHeight);
        double value = minValue + percentage * (maxValue - minValue);
        
        // Draw horizontal tick line with consistent styling - exactly like plotter.cpp
        if (i > 0) { // Skip duplicate line at bottom (exact behavior from plotter.cpp)
            // Draw tick mark at right side of the graph
            shapes.drawLine(
                ssaa.scaleX(layout.getWidth() - layout.getMarginRight()), 
                ssaa.scaleY(y), 
                ssaa.scaleX(layout.getWidth() - layout.getMarginRight() + 6), 
                ssaa.scaleY(y), 
                textColor, 
                ssaa.getSamplingFactor());
        }
        
        // Format the value based on type with precision from plotter.cpp
        char valueText[32];
        if (isInteger) {
            std::sprintf(valueText, "%d", static_cast<int>(value));
        } else {

	    int dPoints = 0;
	    double tempV = fabs(value);
	    if(tempV > 0 && tempV < 1)
	    {
		    while(fabs(tempV) < 1 && dPoints < 10)
		    {
			tempV *= 10;
			dPoints++;
		    }
	    }
	    const int maxPrecision = 10;
	    int finalPrecision = std::min(precision + dPoints, 10); 
            char formatStr[10];
	    std::sprintf(formatStr, "%%.%df", finalPrecision);
            std::sprintf(valueText, formatStr, value);
        }
        
        // Draw value label - exact positioning from plotter.cpp
        // The labelOffset parameter is critical for proper spacing of labels with varying width
        // In plotter.cpp, this is typically 70px for price value labels in candlestick charts
        // and 50px for other charts
        int labelX = layout.getWidth() - layout.getMarginRight() + labelOffset;
        
        // Draw the value label
        textRenderer.drawText(
            labelX,                   // Horizontal position with proper offset for readability
            y,                        // Exact vertical position
            valueText,                // Formatted value
            textColor,                // Color with proper opacity
            22,                       // Exact font size from plotter.cpp
            false);                   // Left-aligned as in plotter.cpp
    }
}

} // namespace shmea 
