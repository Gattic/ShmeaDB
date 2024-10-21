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
		float last_candle_pos;
		long last_timestamp;
		int total_candles_drawn;
		const int max_candles;
		const float margin_x;
		const float margin_y;
		float candle_width;
		int lines;
		std::vector<bool> first_line_point;
		std::vector<float> last_price_pos;
		float last_line_drawn;
		std::vector<RGBA> line_colors;
		std::vector<std::string> line_color_names;
		RGBA color_bullish;
		RGBA color_bearish;


//		RGB HSLToRGB(float, float, float);
//		void generateUniqueColors(int);
		void initialize_colors(std::vector<RGBA>&, std::vector<std::string>&);
		Image downsampleToTargetSize();
			
		void drawLine(float, float, float, float, int);
		void plot(float, float, float, int);
		void drawCandleStick(Image&, float, float, float, float, float, RGBA&);
	public:

		

		static const int TARGET_WIDTH = 1800;
		static const int TARGET_HEIGHT = 1200;
		static const int SUPERSAMPLE_SCALE = 4;
		static const int SUPERSAMPLE_WIDTH = TARGET_WIDTH * SUPERSAMPLE_SCALE;
		static const int SUPERSAMPLE_HEIGHT = TARGET_HEIGHT * SUPERSAMPLE_SCALE;
		
		PNGPlotter(unsigned, unsigned, int, double, double, int = 0);
		void addDataPoint(double, int = 0, bool = true);
		void drawNewCandle(long, float, float, float, float);
		void SavePNG(const std::string&, const std::string&);
	
};
};
#endif
