//PNGPlotter.cpp
#include "PNGPlotter.h"
#include "png-helper.h"

using namespace shmea;

PNGPlotter::PNGPlotter(unsigned width, unsigned height)
	: image(), width(width), height(height), color_bullish(0, 255, 0, 255), color_bearish(255, 0, 0, 255)
{
	image.Allocate(width, height);
	RGBA white(255, 255, 255, 255);
	image.SetAllPixels(white);

	//Init scaling bounds
	min_time = std::numeric_limits<long>::max();
	max_time = std::numeric_limits<long>::min();
	min_price = std::numeric_limits<float>::max();
	max_price = std::numeric_limits<float>::min();

}

void PNGPlotter::addDataPoint(long timestamp, float raw_open, float raw_close, float raw_high, float raw_low)
{
    // Update scaling based on new data point
    min_time = std::min(min_time, timestamp);
    max_time = std::max(max_time, timestamp);
    min_price = std::min(min_price, raw_low);
    max_price = std::max(max_price, raw_high);


    RGBA white(255, 255, 255, 255);
    image.SetAllPixels(white);

    redraw();

 }

void PNGPlotter::redraw()
{

    // Calculate ranges for scaling
    for(unsigned i = 0; i < dataPoints.size(); ++i)
    {
	/*long timestamp;
	float raw_open, raw_close, raw_high, raw_low;
	long time_range = max_time - min_time;
    	float price_range = max_price - min_price;

    // Ensure time_range and price_range are not zero
	if (time_range == 0) time_range = 1; // Prevent division by zero
    	if (price_range == 0) price_range = 1; // Prevent division by zero

    // Scale x position (time) for subsequent data points
    	int x = static_cast<int>((static_cast<float>(timestamp - min_time) / time_range) * (width - 40)) + 20;

    // Scale y positions (price)
    	int y_open = static_cast<int>(height - ((raw_open - min_price) / price_range * (height - 40)) - 20);
    	int y_close = static_cast<int>(height - ((raw_close - min_price) / price_range * (height - 40)) - 20);
    	int y_high = static_cast<int>(height - ((raw_high - min_price) / price_range * (height - 40)) - 20);
    	int y_low = static_cast<int>(height - ((raw_low - min_price) / price_range * (height - 40)) - 20);

	drawCandleStick(image, x, y_open, y_close, y_high, y_low, raw_close >= raw_open ? color_bullish : color_bearish);*/

    }
}


void PNGPlotter::drawCandleStick(Image& img, int x, int y_open, int y_close, int y_high, int y_low, RGBA& color)
{
	//Define the width of the candlestick body and the wick
	const int body_width = 4;
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

void PNGPlotter::SavePNG(const std::string& filename)
{
	image.SavePNG(filename.c_str());
}
