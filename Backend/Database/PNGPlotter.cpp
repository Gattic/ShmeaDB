//PNGPlotter.cpp
#include "PNGPlotter.h"
#include "png-helper.h"

using namespace shmea;

PNGPlotter::PNGPlotter(unsigned width, unsigned height, int max_candles, double max_price, double low_price, int lines)
	: image(), width(width), height(height), 
	min_price(low_price),
	max_price(max_price),
	old_min_price(min_price),
	old_max_price(max_price),
	last_candle_pos(0),
	last_timestamp(0),
	total_candles_drawn(0),
	max_candles(max_candles),
	margin_x(20),
	margin_y(20),
	candle_width(width / max_candles),
	lines(lines),
	first_line_point(lines, true),
	last_price_pos(lines, 0),
	last_line_drawn(0),
	line_colors(),
	line_color_names(),
	color_bullish(0, 255, 0, 255), 
	color_bearish(255, 0, 0, 255)
{
	image.Allocate(width, height);
	RGBA white(255, 255, 255, 255);
	image.SetAllPixels(white);


/*	if(lines != 0)
	    generateUniqueColors(lines);*/

	initialize_colors(line_colors, line_color_names);
}

void PNGPlotter::initialize_colors(std::vector<RGBA>& lines_colors, std::vector<std::string>& lines_colors_name)
{

    lines_colors.push_back(RGBA(0, 0, 255, 255));    // Blue
    lines_colors_name.push_back("Blue");

    lines_colors.push_back(RGBA(255, 165, 0, 255));  // Orange
    lines_colors_name.push_back("Orange");

    lines_colors.push_back(RGBA(128, 0, 128, 255));  // Purple
    lines_colors_name.push_back("Purple");

    lines_colors.push_back(RGBA(0, 255, 255, 255));  // Cyan
    lines_colors_name.push_back("Cyan");

    lines_colors.push_back(RGBA(255, 255, 0, 255));  // Yellow
    lines_colors_name.push_back("Yellow");

    lines_colors.push_back(RGBA(255, 0, 255, 255));  // Magenta
    lines_colors_name.push_back("Magenta");

    lines_colors.push_back(RGBA(0, 128, 128, 255));  // Teal
    lines_colors_name.push_back("Teal");

    lines_colors.push_back(RGBA(255, 140, 0, 255));  // Dark Orange
    lines_colors_name.push_back("Dark Orange");

    lines_colors.push_back(RGBA(255, 105, 180, 255));  // Pink
    lines_colors_name.push_back("Pink");

    lines_colors.push_back(RGBA(173, 216, 230, 255));  // Light Blue
    lines_colors_name.push_back("Light Blue");
	

}
/* 

Comment these out because it might be useful in the future, but for now it's easier to hardcode colors


//Helper function to convert from hue to RGB
float hueToRGB(float p, float q, float t)
{
    if (t < 0) t += 1;
    if (t > 1) t -= 1;
    if (t < 1.0f / 6) return p + (q - p) * 6 * t;
    if (t < 1.0f / 2) return q;
    if (t < 2.0f / 3) return p + (q - p) * (2.0f / 3 - t) * 6;

    return p;
}
PNGPlotter::RGB PNGPlotter::HSLToRGB(float h, float s, float l)
{
    float r, g, b;

    if (s == 0)
    {
	r = g = b = 1; //Achromatic gray
    }
    else
    {
	float q = l < 0.5f ? l * (1 + s) : l + s - l * s;
	float p = 2 * l - q;
	r = hueToRGB(p, q, h + 1.0f / 3);
	g = hueToRGB(p, q, h);
	b = hueToRGB(p, q, h - 1.0f / 3);
    }

    //Convert to RGB [0, 255]
    PNGPlotter::RGB rgb;
    rgb.r = static_cast<int>(r * 255);
    rgb.g = static_cast<int>(g * 255);
    rgb.b = static_cast<int>(b * 255);

    return rgb;
}

void PNGPlotter::generateUniqueColors(int x)
{
    float saturation = 0.7f; //Fixed saturation
    float lightness = 0.5f;

    for(int i = 0; i < x; ++i)
    {
	//Spread hue values evenly across the color wheel
	float hue = static_cast<float>(i) / x;

	//convert HSL to RGB
	PNGPlotter::RGB rgb = HSLToRGB(hue, saturation, lightness);
	line_colors.push_back(RGBA(rgb.r, rgb.g, rgb.b, 255));
	
    }
} */  

void PNGPlotter::addDataPoint(double newPrice, int portIndex, bool draw)
{
    if(lines == 0)
    {
	printf("PNG Plotter not initialized with the correct amount of lines to draw");
	return;
    }

    if(lines - 1 == -1 || lines - 1 < portIndex)
    {
	printf("PNG Plotter does not have the index for this line assigned");
	return;
    }
    float y = height - ((newPrice - min_price) / (max_price - min_price) * (height - 2 * margin_y)) - margin_y;

    if(!first_line_point[portIndex] && draw)
	drawLine(last_line_drawn - 1, last_price_pos[portIndex], last_line_drawn, y, portIndex);


    //update the previous-y coordinate
    last_price_pos[portIndex] = y;

    //Only update X once all the lines have been drawn
    if(portIndex == lines - 1)
	last_line_drawn += candle_width;

    //set the first_point for the specific line to false after the first data point is plotted
    first_line_point[portIndex] = false;
    //drawCandleStick(image, x, y_open, y_close, y_high, y_low, raw_close >= raw_open ? color_bullish : color_bearish);

}

inline int roundFloat(float val)
{
    return static_cast<int>(val + (val >= 0 ? 0.5f : -0.5f));
}


float ipart(float x) {
    return std::floor(x);
}

float fpart(float x) {
    return x - std::floor(x);
}

float rfpart(float x) {
    return 1.0f - fpart(x);
}

// Plot function to blend colors based on brightness
void PNGPlotter::plot(float x, float y, float brightness, int portIndex) {
    RGBA color = line_colors[portIndex];
    RGBA pixelColor = image.GetPixel(static_cast<int>(x), static_cast<int>(y));

    unsigned char blendedR = static_cast<unsigned char>(color.r * brightness + pixelColor.r * (1 - brightness));
    unsigned char blendedG = static_cast<unsigned char>(color.g * brightness + pixelColor.g * (1 - brightness));
    unsigned char blendedB = static_cast<unsigned char>(color.b * brightness + pixelColor.b * (1 - brightness));

    image.SetPixel(static_cast<int>(x), static_cast<int>(y), RGBA(blendedR, blendedG, blendedB, 255));
}

// Xiaolin Wu's Line Algorithm with Thickness (C++98 version)
void PNGPlotter::drawLine(float x1, float y1, float x2, float y2, int portIndex) {
    // Local variable for line thickness
    int thickness = candle_width;  // Adjust the thickness value here

    bool steep = std::abs(y2 - y1) > std::abs(x2 - x1);
    
    if (steep) {
        // Swap x and y if the line is steep
        std::swap(x1, y1);
        std::swap(x2, y2);
    }

    if (x1 > x2) {
        // Ensure we always draw from left to right
        std::swap(x1, x2);
        std::swap(y1, y2);
    }

    float dx = x2 - x1;
    float dy = y2 - y1;
    float gradient = (dx == 0) ? 1 : dy / dx;

    // Handle the first endpoint
    float xEnd = roundFloat(x1);

    float yEnd = y1 + gradient * (xEnd - x1);
    float xGap = rfpart(x1 + 0.5f);
    float xPixel1 = xEnd;
    float yPixel1 = ipart(yEnd);

    // Adjust for line thickness: draw additional lines above and below
    for (int t = -thickness / 2; t <= thickness / 2; ++t) {
        if (steep) {
            plot(yPixel1 + t, xPixel1, rfpart(yEnd) * xGap, portIndex);
            plot(yPixel1 + 1 + t, xPixel1, fpart(yEnd) * xGap, portIndex);
        } else {
            plot(xPixel1, yPixel1 + t, rfpart(yEnd) * xGap, portIndex);
            plot(xPixel1, yPixel1 + 1 + t, fpart(yEnd) * xGap, portIndex);
        }
    }

    float intery = yEnd + gradient;

    // Handle the second endpoint
    xEnd = roundFloat(x2);
    yEnd = y2 + gradient * (xEnd - x2);
    xGap = fpart(x2 + 0.5f);
    float xPixel2 = xEnd;
    float yPixel2 = ipart(yEnd);

    // Adjust for line thickness: draw additional lines above and below
    for (int t = -thickness / 2; t <= thickness / 2; ++t) {
        if (steep) {
            plot(yPixel2 + t, xPixel2, rfpart(yEnd) * xGap, portIndex);
            plot(yPixel2 + 1 + t, xPixel2, fpart(yEnd) * xGap, portIndex);
        } else {
            plot(xPixel2, yPixel2 + t, rfpart(yEnd) * xGap, portIndex);
            plot(xPixel2, yPixel2 + 1 + t, fpart(yEnd) * xGap, portIndex);
        }
    }

    // Main loop for drawing the line
    if (steep) {
        for (int x = static_cast<int>(xPixel1 + 1); x < static_cast<int>(xPixel2); ++x) {
            for (int t = -thickness / 2; t <= thickness / 2; ++t) {
                plot(ipart(intery) + t, x, rfpart(intery), portIndex);
                plot(ipart(intery) + 1 + t, x, fpart(intery), portIndex);
            }
            intery += gradient;
        }
    } else {
        for (int x = static_cast<int>(xPixel1 + 1); x < static_cast<int>(xPixel2); ++x) {
            for (int t = -thickness / 2; t <= thickness / 2; ++t) {
                plot(x, ipart(intery) + t, rfpart(intery), portIndex);
                plot(x, ipart(intery) + 1 + t, fpart(intery), portIndex);
            }
            intery += gradient;
        }
    }
}



/*
void PNGPlotter::drawLine(float x1, float y1, float x2, float y2, int portIndex)
{

    //Digital Differential Analyzer: DDA
    float dx = x2 - x1;
    float dy = y2 - y1;

    //Calculate number of steps
    float steps = std::max(std::abs(dx), std::abs(dy));

    //Calculate the increment per step
    float xIncrement = dx / steps;
    float yIncrement = dy / steps;

    float currentX = x1;
    float currentY = y1;

    for(int i = 0; i <= steps; ++i)
    {
	//Draw pixel
	image.SetPixel(roundFloat(currentX), roundFloat(currentY), line_colors[portIndex]);

	//move pos
	currentX += xIncrement;
	currentY += yIncrement;
    }




    //Bresenham's Line algorithm (or close to it)
    float dx = std::abs(x2 - x1);
    float dy = std::abs(y2 - y1);
    float sx = x1 < x2 ? 1 : -1;
    float sy = y1 < y2 ? 1 : -1;
    float err = dx - dy;

    while (true)
    {
	//Set pixel at (x1, y1)
	image.SetPixel(x1, y1, line_colors[portIndex]); //Fix draw to get from vector
	//Check if the end has been reached
	if (x1 == x2 && y1 == y2)
	    break;
	
	float e2 = err * 2;
	if(e2 > -dy)
	{
	    err -= dy;
	    x1 += sx;
	}
	if (e2 < dx)
	{
	    err += dx;
	    y1 += sy;
	}
	
    }
}*/

void PNGPlotter::drawNewCandle(long timestamp, float raw_open, float raw_close, float raw_high, float raw_low)
{
    
    float x = last_candle_pos + candle_width;

    float y_open = height - ((raw_open - min_price) / (max_price - min_price) * (height - 2 * margin_y))- margin_y;
    
    float y_close = height - ((raw_close - min_price) / (max_price - min_price) * (height - 2 * margin_y)) - margin_y;

    float y_high = height - ((raw_high - min_price) / (max_price - min_price) * (height - 2 * margin_y)) - margin_y;

    float y_low = height - ((raw_low - min_price) / (max_price - min_price) * (height - 2 * margin_y)) - margin_y;

    //Determine the color based on whether the candlestick is bullish or bearish
    RGBA& color = (raw_close >= raw_open) ? color_bullish : color_bearish;

    //Draw the candlestick at the calculated x position
    drawCandleStick(image, x, y_open, y_close, y_high, y_low, color);
    
    //Update the last drawn x position
    last_candle_pos = x;
}


void PNGPlotter::drawCandleStick(Image& img, float x, float y_open, float y_close, float y_high, float y_low, RGBA& color)
{
	//Define the width of the candlestick body and the wick
	const float body_width = candle_width;
	const float half_body_width = body_width / 2;

	//Draw wick (line between high and low)
	for(int y = y_low; y <= y_high; ++y)
	{
		img.SetPixel(x, y, color);
	}

	//Draw Body (rectangle between open and close)
	float body_top = std::min(y_open, y_close);
	float body_bottom = std::max(y_open, y_close);

	for(int y = body_top; y <= body_bottom; ++y)
	{
		for(int dx = -half_body_width; dx <= half_body_width; ++dx)
		{
			img.SetPixel(x + dx, y, color);
		}
	}
}

Image PNGPlotter::downsampleToTargetSize() {
    Image downsampledImage;
    downsampledImage.Allocate(TARGET_WIDTH, TARGET_HEIGHT);

    // Calculate the size of each block of high-res pixels that corresponds to one low-res pixel
    float scaleX = static_cast<float>(SUPERSAMPLE_WIDTH) / TARGET_WIDTH;
    float scaleY = static_cast<float>(SUPERSAMPLE_HEIGHT) / TARGET_HEIGHT;

    for (unsigned int y = 0; y < TARGET_HEIGHT; ++y) {
        for (unsigned int x = 0; x < TARGET_WIDTH; ++x) {
            int startX = static_cast<int>(x * scaleX);
            int startY = static_cast<int>(y * scaleY);
            int blockWidth = static_cast<int>(std::ceil(scaleX));
            int blockHeight = static_cast<int>(std::ceil(scaleY));

            // Average the colors over the block of high-res pixels
            RGBA avgColor = image.averageColor(startX, startY, blockWidth, blockHeight);
            downsampledImage.SetPixel(x, y, avgColor);
        }
    }

    return downsampledImage;
}

void PNGPlotter::SavePNG(const std::string& filename, const std::string& folder)
{

	Image downsampleImage = downsampleToTargetSize();

	std::string full_path = folder;
	full_path.append("/");
	full_path.append(filename);
	//image.SavePNG(full_path.c_str());
	downsampleImage.SavePNG(full_path.c_str());
}
