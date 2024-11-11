//PNGPlotter.cpp
#include "PNGPlotter.h"
#include "png-helper.h"

using namespace shmea;
//Just as a reminder, graphSize is the amount of data points that are appearing across the graph. This can be candles, data points, bars, etc
PNGPlotter::PNGPlotter(unsigned width, unsigned height, int graphSize, double max_price, double low_price, int lines, int margin_top, int margin_right, int margin_bottom, int margin_left, bool fourQuadrants)
	: image(), width(width), height(height), 
	min_price(low_price),
	max_price(max_price),
	margin_top(margin_top),
	margin_right(margin_right),
	margin_bottom(margin_bottom),
	margin_left(margin_left),
	fourQuadrants(fourQuadrants),
	last_timestamp(0),
	total_candles_drawn(0),
	graphSize(graphSize),
	candle_width(static_cast<int>(graphSize != 0 ? (width - margin_left - margin_right) / graphSize : 1)),
	last_candle_pos(static_cast<int>(candle_width / 2)),
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
	RGBA DarkGray(0x40, 0x40, 0x40, 0xFF);
	RGBA Black(0x00, 0x00, 0x00, 0x7F);
	image.drawVerticalGradient(0, 0, DarkGray, Black, 0);

	if(fourQuadrants)
	{
		drawFourQuadrants();
	}
	
	initialize_colors(line_colors, line_color_names);
//	drawGrid();
}

int PNGPlotter::getWidth()
{
	return width;
}

int PNGPlotter::getHeight()
{
	return height;
}

void PNGPlotter::drawFourQuadrants()
{
    RGBA lineColor(0xC8, 0xC8, 0xC8, 0xC8); // Light gray for the quadrant lines

    // Calculate positions for the middle lines
    int midX = (width - margin_left - margin_right) / 2 + margin_left;
    int midY = (height - margin_top - margin_bottom) / 2 + margin_top;

    // Draw the vertical middle line
    drawLine(midX, margin_top, midX, height - margin_bottom, lineColor);

    // Draw the horizontal middle line
    drawLine(margin_left, midY, width - margin_right, midY, lineColor);
}

void PNGPlotter::drawGrid(int numHorizontalLines, int numVerticalLines) {
    RGBA gridColor(200, 200, 200, 200); // Light gray for the grid lines

    // Draw horizontal grid lines and y-axis labels
    for (int i = 0; i <= numHorizontalLines; ++i) {
        int y = i * (height - margin_top - margin_bottom) / numHorizontalLines;
        drawLine(margin_left, y, width - margin_right, y, gridColor);

/*        // Draw y-axis labels if provided
        if (!yLabels.empty() && i < yLabels.size()) {
            drawText(margin_x - 30, y, yLabels[i]); // Adjust x-position for label placement
        }*/
    }

    // Draw vertical grid lines and x-axis labels
    for (int i = 0; i <= numVerticalLines; ++i) {
        int x = i * (width - margin_left - margin_right) / numVerticalLines;
        drawLine(x, 0, x, height - margin_top - margin_bottom, gridColor);

/*        // Draw x-axis labels if provided
        if (!xLabels.empty() && i < xLabels.size()) {
            drawText(x, height - margin_y + 15, xLabels[i]); // Adjust y-position for label placement
        }*/
    }
}

void PNGPlotter::initialize_colors(std::vector<RGBA>& lines_colors, std::vector<std::string>& lines_colors_name)
{
    // TODO: Find a way to auto generate these, for now there are just 10
    lines_colors.push_back(RGBA(0x00, 0x00, 0xFF, 0xFF));    // Blue
    lines_colors_name.push_back("Blue");

    lines_colors.push_back(RGBA(0xFF, 0xA5, 0x00, 0xFF));  // Orange
    lines_colors_name.push_back("Orange");

    lines_colors.push_back(RGBA(0x80, 0x00, 0x80, 0xFF));  // Purple
    lines_colors_name.push_back("Purple");

    lines_colors.push_back(RGBA(0x00, 0xFF, 0xFF, 0xFF));  // Cyan
    lines_colors_name.push_back("Cyan");

    lines_colors.push_back(RGBA(0xFF, 0xFF, 0x00, 0xFF));  // Yellow
    lines_colors_name.push_back("Yellow");

    lines_colors.push_back(RGBA(0xFF, 0x00, 0xFF, 0xFF));  // Magenta
    lines_colors_name.push_back("Magenta");

    lines_colors.push_back(RGBA(0x00, 0x80, 0x80, 0xFF));  // Teal
    lines_colors_name.push_back("Teal");

    lines_colors.push_back(RGBA(0xFF, 0x8C, 0x00, 0xFF));  // Dark Orange
    lines_colors_name.push_back("Dark Orange");

    lines_colors.push_back(RGBA(0xFF, 0x69, 0xB4, 0xFF));  // Pink
    lines_colors_name.push_back("Pink");

    lines_colors.push_back(RGBA(0xAD, 0xD8, 0xE6, 0xFF));  // Light Blue
    lines_colors_name.push_back("Light Blue");

    // Remember to add colour entries of indicator when adding new indicators
    indicatorColors["BBLow"] = RGBA(0x00, 0x00, 0xFF, 0xFF); // Blue
    indicatorColors["BBHigh"] = RGBA(0x00, 0x00, 0xFF, 0xFF); // Blue
    indicatorColors["EMA"] = RGBA(0xFF, 0xFF, 0x00, 0xFF); // Yellow
    indicatorColors["SMA"] = RGBA(0x80, 0x00, 0x80, 0xFF); // Purple
} 

void PNGPlotter::addDataPointWithIndicator(double newPrice, int portIndex, std::string indicator, std::string value)
{
	//TODO: ERROR CHECK value when you implement it
	if (indicator.empty()) {
        	printf("Error: You need to define an indicator to draw a line.\n");
        	return;
    	}

	if (indicatorColors.find(indicator) == indicatorColors.end()) 
	{
        	printf("Error: The specified indicator '%s' is not recognized.\n", indicator.c_str());
        	return;
	}

	addDataPoint(newPrice, portIndex, true, &indicatorColors[indicator]); 


} 

inline int clamp(int value, int min, int max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

void PNGPlotter::addDataPoint(double newPrice, int portIndex, bool draw, RGBA* lineColor, int lineWidth)
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

    if(lineColor == NULL)
    {
	lineColor = &line_colors[portIndex];
    }

    int y = height - margin_bottom - static_cast<int>((newPrice - min_price) / (max_price - min_price) * (height - margin_top - margin_bottom));

    y = clamp(y, margin_top, height - margin_bottom);	

    if(draw)
    {
	if(!first_line_point[portIndex])
	{
    		first_line_point[portIndex] = true;
		last_line_drawn = -1 * static_cast<int>(candle_width / 2); //This is done so that when we add by candle_width below, the starting x will be in the middle of the candle stick
	}
	else
	{
		int startX = last_line_drawn + margin_left;
		int endX = last_line_drawn + candle_width + margin_left;

		drawLine(startX, last_price_pos[portIndex], endX, y, *lineColor, lineWidth);
	}

    	//update the previous-y coordinate
    	last_price_pos[portIndex] = y;
    }



    //Only update X once all the lines have been drawn
    if(portIndex == lines - 1)
	last_line_drawn += candle_width;
    
}

void PNGPlotter::addDataPointsPCA(std::vector<std::vector<double> >& data, RGBA& pointColor)
{
    // Calculate the effective plotting area considering the margins
    int effectiveWidth = width - margin_left - margin_right;
    int effectiveHeight = height - margin_top - margin_bottom;

    double spacing = static_cast<double>(effectiveWidth) / graphSize;
    int pointThickness = static_cast<int>(spacing / 3);

    if(pointThickness < 1)
    {
      pointThickness = 1;
    }

    // Calculate the scaling factors for the x and y dimensions
    double xScale = static_cast<double>(effectiveWidth) / graphSize;
    double yScale = static_cast<double>(effectiveHeight) / (max_price - min_price);

    //Calculate the center (origin)
    int centerX = effectiveWidth / 2 + margin_left;
    int centerY = effectiveHeight / 2 + margin_top; 

    for(size_t i = 0; i < data.size(); ++i)
    {
	int xPNG = static_cast<int>((data[i][0] * xScale) + centerX + (pointThickness * 0.5));
	int yPNG = static_cast<int>(centerY - (data[i][1] * yScale));

	//Draw point
	drawPoint(xPNG, yPNG, pointThickness, pointColor);
    }
}

void PNGPlotter::addArrow(std::vector<std::vector<double> >& sorted_eig_vecs, RGBA& arrowColor, int arrowSize)
{

    // Calculate the effective plotting area considering the margins
    int effectiveWidth = width - margin_left - margin_right;
    int effectiveHeight = height - margin_top - margin_bottom;

    // Calculate the scaling factors for the x and y dimensions
    double xScale = static_cast<double>(effectiveWidth) / graphSize;
    double yScale = static_cast<double>(effectiveHeight) / (max_price - min_price);

    //Calculate the center (origin)
    int centerX = effectiveWidth / 2 + margin_left;
    int centerY = effectiveHeight / 2 + margin_top; 

    double scaleFactor = 0.5;

    for(size_t i = 0; i < sorted_eig_vecs.size(); ++i)
    {
	double arrowX1 = centerX;
	double arrowY1 = centerY;

	//Normalize the eigenvectors to fit the screen dimensions
	double normX = sorted_eig_vecs[i][0] * centerX;
	double normY = sorted_eig_vecs[i][1] * centerY;

	//Scale the normalized values by a facotr for visibility
	double scaleFactor =  0.5;
	double arrowX2 = arrowX1 + normX * scaleFactor;
	double arrowY2 = arrowY1 + normY * scaleFactor;

	drawArrow(arrowX1, arrowY1, arrowX2, arrowY2, arrowColor, arrowSize);
		
    }
}

void PNGPlotter::addHistogram(std::vector<int>& bins, RGBA& barColor)
{
    int max_count = *std::max_element(bins.begin(), bins.end());

    int bar_spacing = 25;
    double bin_width = (max_price - min_price) / graphSize;

    int plot_width = width - margin_left - margin_right;
    int plot_height = height - margin_top - margin_bottom;

    int total_spacing = (graphSize - 1) * bar_spacing;
    int bar_total_width = plot_width - total_spacing;

    int bar_width = bar_total_width / graphSize;

    for(int i = 0; i < graphSize; ++i)
    {
	int bar_height = static_cast<int>((static_cast<double>(bins[i]) / max_count) * plot_height);
	int x_start = margin_left + i * (bar_width + bar_spacing);
	int y_start = height - margin_bottom - bar_height;

	drawHistogram(x_start, y_start, bar_width, barColor);
    } 

}

void PNGPlotter::drawHistogram(int x_start, int y_start, int bar_width, RGBA& barColor)
{
    for(int x = x_start; x < x_start + bar_width; ++x)
    {
	for(int y = y_start; y < height - margin_bottom; ++y)
	{
	    image.SetPixel(x, y, barColor);
	}
    }


}

void PNGPlotter::drawPoint(int x, int y, int thickness, RGBA& pointColor)
{
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

void PNGPlotter::drawLine(int x1, int y1, int x2, int y2, RGBA& lineColor, int lineWidth)
{

    x1 = clamp(x1, margin_left, width - margin_right);
    x2 = clamp(x2, margin_left, width - margin_right);
    y1 = clamp(y1, margin_top, height - margin_bottom);
    y2 = clamp(y2, margin_top, height - margin_bottom);

    //Bresenham's Line algorithm (or close to it)
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);

    bool steep = dy > dx;
    
    //Swap if the line is steep (more vertical than horizontal

    if(steep)
    {
	//swap x1 and y1
	int temp = x1;
	x1 = y1;
	y1 = temp;

	//Swap x2 and y2
	temp = x2;
	x2 = y2;
	y2 = temp;

	temp = dx;
	dx = dy;
	dy = temp;    	
    }


    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    
    double gradient = static_cast<double>(dy) / static_cast<double>(dx);
    double err = 0.0;

    while (true)
    {

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
	if(err >= 0.5)
	{
		y1 += sy;
		err -= 1.0;
	}
	x1 += sx;
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
    int y_open = height - margin_bottom - static_cast<int>(adjusted_open / adjusted_max * (height - margin_top - margin_bottom));
    int y_close = height - margin_bottom - static_cast<int>(adjusted_close / adjusted_max * (height - margin_top - margin_bottom));
    int y_high = height - margin_bottom - static_cast<int>(adjusted_high / adjusted_max * (height - margin_top - margin_bottom));
    int y_low = height - margin_bottom - static_cast<int>(adjusted_low / adjusted_max * (height - margin_top - margin_bottom));

    y_open = clamp(y_open, margin_top, height - margin_bottom);
    y_close = clamp(y_close, margin_top, height - margin_bottom);
    y_high = clamp(y_high, margin_top, height - margin_bottom);
    y_low = clamp(y_low, margin_top, height - margin_bottom);

    // Determine the color based on whether the candlestick is bullish or bearish
    RGBA& color = (raw_close >= raw_open) ? color_bullish : color_bearish;

    // Draw the candlestick at the calculated x position
    drawCandleStick(image, last_candle_pos + margin_left, y_open, y_close, y_high, y_low, color);

    // Update the last drawn x position
    last_candle_pos += candle_width;
}

void PNGPlotter::drawCandleStick(Image& img, int x, int y_open, int y_close, int y_high, int y_low, RGBA& color) {
    const int body_width = static_cast<int>(candle_width);  // Ensure width is an integer
    const int half_body_width = body_width / 2;
    const int wick_thickness = 20;

    // Draw wick (line between high and low)
    int wick_top = std::min(y_low, y_high);
    int wick_bottom = std::max(y_low, y_high);
    for (int y = wick_top; y <= wick_bottom; ++y) {
	for(int i = -wick_thickness; i <= wick_thickness; ++i)
	{
            if (x + i >= margin_left && x + i < static_cast<int>(img.getWidth()) - margin_right && y >= margin_top && y < static_cast<int>(img.getHeight()) - margin_bottom) 
            {
                img.SetPixel(x + i, y, color);
            }
	}
    }

    // Draw Body (rectangle between open and close)
    int body_top = std::min(y_open, y_close);
    int body_bottom = std::max(y_open, y_close);
    for (int y = body_top; y <= body_bottom; ++y) {
        for (int dx = -half_body_width; dx <= half_body_width; ++dx) {
            if (x + dx >= margin_left && x + dx < static_cast<int>(img.getWidth()) - margin_right && y >= margin_top && y < static_cast<int>(img.getHeight()) - margin_bottom) {
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
void PNGPlotter::drawArrow(int x1, int y1, int x2, int y2, RGBA& arrowColor, int arrowSize = 10)
{

    // Draw the main line
    drawLine(x1, y1, x2, y2, arrowColor);

    // Calculate the angle of the arrow
    double angle = std::atan2(static_cast<double>(y2 - y1), static_cast<double>(x2 - x1));

    // Calculate the points for the arrowhead
    int arrowX1 = static_cast<int>(x2 - arrowSize * std::cos(angle + M_PI / 6)); // M_PI / 6 = 30 degrees
    int arrowY1 = static_cast<int>(y2 - arrowSize * std::sin(angle + M_PI / 6));
    int arrowX2 = static_cast<int>(x2 - arrowSize * std::cos(angle - M_PI / 6));
    int arrowY2 = static_cast<int>(y2 - arrowSize * std::sin(angle - M_PI / 6));

    // Draw the arrowhead lines
    drawLine(x2, y2, arrowX1, arrowY1, arrowColor);
    drawLine(x2, y2, arrowX2, arrowY2, arrowColor);
}

Image PNGPlotter::downsampleToTargetSize() {
    Image downsampledImage;
    downsampledImage.Allocate(TARGET_WIDTH, TARGET_HEIGHT);

    // Calculate the size of each block of high-res pixels that corresponds to one low-res pixel
    float scaleX = static_cast<float>(width) / TARGET_WIDTH;
    float scaleY = static_cast<float>(height) / TARGET_HEIGHT;

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
//	image.SavePNG(full_path.c_str());
	downsampleImage.SavePNG(full_path.c_str());
}
