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
	color_bullish(0, 255, 0, 255), 
	color_bearish(255, 0, 0, 255)
{
	image.Allocate(width, height);
	RGBA white(255, 255, 255, 255);
	image.SetAllPixels(white);

	if(lines != 0)
	    generateUniqueColors(lines);

}

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
}   

void PNGPlotter::addDataPoint(double newPrice, int portIndex)
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

    if(!first_line_point[portIndex])
	drawLine(last_line_drawn - 1, last_price_pos[portIndex], last_line_drawn, y, portIndex);


    //update the previous-y coordinate
    last_price_pos[portIndex] = y;

    //Only update X once all the lines have been drawn
    if(portIndex == lines - 1)
	last_line_drawn += candle_width;

    //set the first_point for the specific line to false after the first data point is plotted
    first_line_point[portIndex] = false;

}

void PNGPlotter::drawLine(int x1, int y1, int x2, int y2, int portIndex)
{
    //Bresenham's Line algorithm (or close to it)
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;

    while (true)
    {
	//Set pixel at (x1, y1)
	image.SetPixel(x1, y1, line_colors[portIndex]); //Fix draw to get from vector
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

void PNGPlotter::drawNewCandle(long timestamp, float raw_open, float raw_close, float raw_high, float raw_low)
{
    
   int x = last_candle_pos + candle_width;

    int y_open = height - static_cast<int>((raw_open - min_price) / (max_price - min_price) * (height - 2 * margin_y))- margin_y;
    
    int y_close = height - static_cast<int>((raw_close - min_price) / (max_price - min_price) * (height - 2 * margin_y)) - margin_y;

    int y_high = height - static_cast<int>((raw_high - min_price) / (max_price - min_price) * (height - 2 * margin_y)) - margin_y;

    int y_low = height - static_cast<int>((raw_low - min_price) / (max_price - min_price) * (height - 2 * margin_y)) - margin_y;

    //Determine the color based on whether the candlestick is bullish or bearish
    RGBA& color = (raw_close >= raw_open) ? color_bullish : color_bearish;

    //Draw the candlestick at the calculated x position
    drawCandleStick(image, x, y_open, y_close, y_high, y_low, color);
    
    //Update the last drawn x position
    last_candle_pos = x;
}


void PNGPlotter::drawCandleStick(Image& img, int x, int y_open, int y_close, int y_high, int y_low, RGBA& color)
{
	//Define the width of the candlestick body and the wick
	const int body_width = candle_width;
	const int half_body_width = body_width / 2;

	//Draw wick (line between high and low)
	for(int y = y_low; y <= y_high; ++y)
	{
		img.SetPixel(x, y, color);
	}

	//Draw Body (rectangle between open and close)
	int body_top = std::min(y_open, y_close);
	int body_bottom = std::max(y_open, y_close);

	for(int y = body_top; y <= body_bottom; ++y)
	{
		for(int dx = -half_body_width; dx <= half_body_width; ++dx)
		{
			img.SetPixel(x + dx, y, color);
		}
	}
}

void PNGPlotter::SavePNG(const std::string& filename, const std::string& folder)
{
	RGBA test(124, 43, 189, 255);
	image.SetPixel(0, 0, test);
	image.SetPixel(1199, 799, test);
	std::string full_path = folder;
	full_path.append("/");
	full_path.append(filename);
	image.SavePNG(full_path.c_str());
}
