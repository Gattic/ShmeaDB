//PNGPlotter.h
#ifndef PNGPLOTTER_H
#define PNGPLOTTER_H

#include "image.h"
#include <string>
#include <limits>
#include <vector>
#include <algorithm>
#include <iostream>

namespace shmea{

class PNGPlotter
{

	struct RGB
	{
	    int r;
	    int g;
	    int b;
	};
	private:
		Image image;
		unsigned width;
		unsigned height;
		float min_price, max_price;
		float old_min_price, old_max_price;
		int last_candle_pos;
		long last_timestamp;
		int total_candles_drawn;
		const int max_candles;
		const int margin_x;
		const int margin_y;
		int candle_width;
		int lines;
		std::vector<bool> first_line_point;
		std::vector<int> last_price_pos;
		int last_line_drawn;
		std::vector<RGBA> line_colors;

		RGBA color_bullish;
		RGBA color_bearish;


		RGB HSLToRGB(float, float, float);
		void generateUniqueColors(int);
		void drawLine(int, int, int, int, int);
		void drawCandleStick(Image&, int, int, int, int, int, RGBA&);
	public:

		PNGPlotter(unsigned, unsigned, int, double, double, int = 0);
		void addDataPoint(double, int = 0, bool = true);
		void drawNewCandle(long, float, float, float, float);
		void SavePNG(const std::string&, const std::string&);
	
};
};
#endif
