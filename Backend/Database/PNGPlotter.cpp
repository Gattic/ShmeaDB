//PNGPlotter.cpp
#include "PNGPlotter.h"
#include "png-helper.h"

using namespace shmea;

PNGPlotter::PNGPlotter(unsigned width, unsigned height, int max_candles, double max_price, double low_price, int lines)
	: image(), width(width), height(height), 
	min_price(low_price),
	max_price(max_price),
	last_candle_pos(0),
	last_timestamp(0),
	total_candles_drawn(0),
	max_candles(max_candles),
	margin_x(20),
	margin_y(200),
	candle_width(static_cast<int>(max_candles != 0 ? width / max_candles : 1)),
	lines(lines),
	first_line_point(lines, false),
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
    int y = height - static_cast<int>((newPrice - min_price) / (max_price - min_price) * (height - 2 * margin_y)) - margin_y;	

    if(draw)
    {
	if(!first_line_point[portIndex] && y != 0)
    		first_line_point[portIndex] = true;
	else
		drawLine(last_line_drawn, last_price_pos[portIndex], last_line_drawn + candle_width, y, portIndex);


    	//update the previous-y coordinate
    	last_price_pos[portIndex] = y;
    }



    //Only update X once all the lines have been drawn
    if(portIndex == lines - 1)
	last_line_drawn += candle_width;

    
    //drawCandleStick(image, x, y_open, y_close, y_high, y_low, raw_close >= raw_open ? color_bullish : color_bearish);

}

void PNGPlotter::drawLine(int x1, int y1, int x2, int y2, int portIndex)
{

    //Bresenham's Line algorithm (or close to it)
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;

    int half_width = candle_width / 10;
    if (half_width == 0)
    {
	half_width = 1;
    }

    while (true)
    {
        // Draw thickness perpendicular to the line direction
        if (dx > dy)  // Predominantly horizontal line
        {
            for (int i = -half_width; i <= half_width; ++i)
            {
                image.SetPixel(x1, y1 + i, line_colors[portIndex]);  // Offset vertically
            }
        }
        else  // Predominantly vertical or diagonal line
        {
            for (int i = -half_width; i <= half_width; ++i)
            {
                image.SetPixel(x1 + i, y1, line_colors[portIndex]);  // Offset horizontally
            }
        }

	//Check if the end has been reached
	if (x1 == x2 && y1 == y2)
	    break;
	
	int e2 = err * 2;
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
}

void PNGPlotter::drawNewCandle(long timestamp, float raw_open, float raw_close, float raw_high, float raw_low) {
    // Adjust prices by subtracting min_price for normalization
    float adjusted_open = raw_open - min_price;
    float adjusted_close = raw_close - min_price;
    float adjusted_high = raw_high - min_price;
    float adjusted_low = raw_low - min_price;
    float adjusted_max = max_price - min_price;

    // Calculate y-coordinates with adjusted prices
    int y_open = height - static_cast<int>(adjusted_open / adjusted_max * (height - 2 * margin_y)) - margin_y;
    int y_close = height - static_cast<int>(adjusted_close / adjusted_max * (height - 2 * margin_y)) - margin_y;
    int y_high = height - static_cast<int>(adjusted_high / adjusted_max * (height - 2 * margin_y)) - margin_y;
    int y_low = height - static_cast<int>(adjusted_low / adjusted_max * (height - 2 * margin_y)) - margin_y;

    // Determine the color based on whether the candlestick is bullish or bearish
    RGBA& color = (raw_close >= raw_open) ? color_bullish : color_bearish;

    // Draw the candlestick at the calculated x position
    drawCandleStick(image, last_candle_pos + candle_width, y_open, y_close, y_high, y_low, color);

    // Update the last drawn x position
    last_candle_pos += candle_width;
}

void PNGPlotter::drawCandleStick(Image& img, int x, int y_open, int y_close, int y_high, int y_low, RGBA& color) {
    const int body_width = static_cast<int>(candle_width);  // Ensure width is an integer
    const int half_body_width = body_width / 2;

    // Draw wick (line between high and low)
    int wick_top = std::min(y_low, y_high);
    int wick_bottom = std::max(y_low, y_high);
    for (int y = wick_top; y <= wick_bottom; ++y) {
        if (x >= 0 && x < img.getWidth() && y >= 0 && y < img.getHeight()) {
	    img.SetPixel(x-1, y, color);
            img.SetPixel(x, y, color);
	    img.SetPixel(x+1, y, color);
        }
    }

    // Draw Body (rectangle between open and close)
    int body_top = std::min(y_open, y_close);
    int body_bottom = std::max(y_open, y_close);
    for (int y = body_top; y <= body_bottom; ++y) {
        for (int dx = -half_body_width; dx <= half_body_width; ++dx) {
            if (x + dx >= 0 && x + dx < img.getWidth() && y >= 0 && y < img.getHeight()) {
                img.SetPixel(x + dx, y, color);
            }
        }
    }
}


/*
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
*/
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
