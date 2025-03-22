//PNGPlotter.cpp
#include "PNGPlotter.h"
#include "png-helper.h"

using namespace shmea;
//Just as a reminder, graphSize is the amount of data points that are appearing across the graph. This can be candles, data points, bars, etc
// that is represented by graphSize
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
	color_bullish(0x00, 0xFF, 0x00, 0xFF), 
	color_bearish(0xFF, 0x00, 0x00, 0xFF),
	headerPenXStarting(500),
	headerPenYStarting(75),
	headerXSpacing(25),
	headerYSpacing(150)
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
	initialize_font();

	//TODO: initialize the AGG_SIZE conversion from value to string... might want to put that somewhere more accessible
	AGG_SIZE[1] = "1m";
    	AGG_SIZE[2] = "2m";
    	AGG_SIZE[3] = "3m";
    	AGG_SIZE[5] = "5m";
    	AGG_SIZE[15] = "15m";
    	AGG_SIZE[30] = "30m";
    	AGG_SIZE[60] = "1h";
    	AGG_SIZE[240] = "4h";
    	AGG_SIZE[390] = "1D";
    	AGG_SIZE[1950] = "1W";
    	AGG_SIZE[8190] = "1MO";
	AGG_SIZE[1440] = "1D";
	AGG_SIZE[10080] = "1W";
	AGG_SIZE[43200] = "1MO";

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

inline int clamp(int value, int min, int max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}
inline std::vector<float> get_axis_ticks(float max_price, float min_price, int max_ticks = 8) {
    std::vector<float> ticks;

    // Calculate the range
    float range = max_price - min_price;

    // Handle edge cases
    if (range <= 0 || max_ticks <= 1) {
        ticks.push_back(min_price);
        ticks.push_back(max_price);
        return ticks;
    }

    // Calculate a "nice" step size
    float rough_step = range / (max_ticks - 1);
    float step_magnitude = std::pow(10, std::floor(std::log10(rough_step)));
    float nice_step;

    if (rough_step / step_magnitude < 2) {
        nice_step = step_magnitude;
    } else if (rough_step / step_magnitude < 5) {
        nice_step = 2 * step_magnitude;
    } else {
        nice_step = 5 * step_magnitude;
    }

    // Adjust the start and end to be within the given range
    float start = std::ceil(min_price / nice_step) * nice_step;
    float end = std::floor(max_price / nice_step) * nice_step;

    // Generate the ticks
    for (float tick = start; tick <= end; tick += nice_step) {
        if (tick <= max_price && tick >= min_price) {
            ticks.push_back(tick);
        }
    }

    return ticks;
}

void PNGPlotter::drawYGrid() {
    RGBA gridColor(200, 200, 200, 200); // Light gray for the grid lines

    std::vector<float> horizontalLines = get_axis_ticks(max_price, min_price);
    float adjusted_max = max_price - min_price;
    // Draw horizontal grid lines and y-axis labels
    for (int i = 0; i < horizontalLines.size(); ++i) {
	float adjusted_tick = horizontalLines[i] - min_price;
	int y = height - margin_bottom - static_cast<int>(adjusted_tick / adjusted_max * (height - margin_top - margin_bottom));
	y = clamp(y, margin_top, height - margin_bottom);

        drawLine(margin_left, y, width - margin_right, y, gridColor);

	std::ostringstream oss;
	oss << horizontalLines[i];
	std::string numberY = oss.str();	
	if(i != 0)
	{
	    GraphLabel(width - margin_right, y, numberY, 600, 100, 100);
	}

    }

}

inline std::string dateToString(int64_t timestamp, const char* format= "%m-%d-%Y")
{
    // Ensure the timestamp fits within the range of time_t
    std::time_t time = static_cast<std::time_t>(timestamp);

    // Create a buffer for the formatted date string
    char buffer[64];
    std::memset(buffer, 0, sizeof(buffer));

    // Format the timestamp into a human-readable string
    if (std::strftime(buffer, sizeof(buffer), format, std::localtime(&time))) 
    {
        return std::string(buffer);
    } 
    else 
    {
        return "Invalid Date";
    }
}

inline std::vector<std::string> get_date_labels(int64_t start, int64_t end, int total_candles, int max_ticks = 8) {
    std::vector<std::string> labels;

    // Handle edge cases
    if (total_candles <= 0 || max_ticks <= 1 || start >= end) 
    {
        labels.push_back(dateToString(start, "%m-%d-%Y"));
        labels.push_back(dateToString(end, "%m-%d-%Y"));
        return labels;
    }

    // Calculate the number of candles per tick
    int candles_per_tick = total_candles / (max_ticks - 1);
    if (candles_per_tick < 1) candles_per_tick = 1;

    // Calculate the time interval per candle
    int64_t time_per_candle = (end - start) / total_candles;

    std::string last_month_label = ""; //Track last month label
    std::string last_year_label = ""; //Track last year label
    // Generate labels
    for (int tick = 0; tick < max_ticks; ++tick) 
    {
        int candle_index = tick * candles_per_tick;
        if (candle_index >= total_candles) break;

        // Calculate the timestamp for this tick
        int64_t timestamp = start + candle_index * time_per_candle;

        // Add the appropriate label
        if ((end - start) < 86400) 
        { // Less than a day
            labels.push_back(dateToString(timestamp, "%H:%M"));
        }
        else if ((end - start) < 30 * 86400) 
        { // Less than a month
            labels.push_back(dateToString(timestamp, "%m-%d"));
        } 
        else if ((end - start) < 365 * 86400) 
        { // Less than a year
	    std::string current_month = dateToString(timestamp, "%b");
	    if(current_month == last_month_label)
	    {
	    	labels.push_back(dateToString(timestamp, "%d")); //Use day if month repeats
            }
	    else
	    {
		labels.push_back(current_month);
		last_month_label = current_month;
	    }
        }
        else 
        { // Multiple years
	    std::string current_year = dateToString(timestamp, "%Y");
	    if(current_year == last_year_label)
            {
            	labels.push_back(dateToString(timestamp, "%b"));
            }
	    else
	    {
		labels.push_back(current_year);
		last_year_label = current_year;
	    } 
        }
    }

    return labels;
}

void PNGPlotter::drawXGrid(int64_t start, int64_t end)
{

    RGBA gridColor(200, 200, 200, 200); // Light gray for the grid lines

    std::vector<std::string> verticalLines = get_date_labels(start, end, graphSize);
    
    // Calculate step size for x-axis grid
    int step = (width - margin_left - margin_right) / (verticalLines.size() - 1);

    // Draw vertical grid lines and x-axis labels
    for (int i = 0; i < verticalLines.size(); ++i) 
    {
        int x = margin_left + i * step;
        drawLine(x, 0, x, height - margin_top - margin_bottom, gridColor);

	GraphLabel(x, height - margin_top - margin_bottom, verticalLines[i], 600, -(600/4), 600/8);
    }
}


void PNGPlotter::initialize_font(const std::string fontPath)
{

    //Initialize FreeType
    if(FT_Init_FreeType(&ft))
    {
	throw std::runtime_error("Could not initialize FreeType Library.");
    }

    //Load the font
    if(FT_New_Face(ft, fontPath.c_str(), 0, &face))
    {
	throw std::runtime_error("Failed to load font: " + fontPath);
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
    indicatorTextColor["BBHigh"] = RGBA(0x00, 0x00, 0x50, 0xFF);
    indicatorTextColor["BBLow"] = RGBA(0x00, 0x00, 0x50, 0xFF);

    indicatorColors["EMA"] = RGBA(0xFF, 0xBF, 0x00, 0xFF); // Yellow
    indicatorTextColor["EMA"] = RGBA(0x72, 0x56, 0x00, 0xFF);

    indicatorColors["SMA"] = RGBA(0xFF, 0x5C, 0x00, 0xFF); // Orange
    indicatorTextColor["SMA"] = RGBA(0x72, 0x29, 0x00, 0xFF);
    indicatorPoint["BBLow"] = 0;
    indicatorPoint["BBHigh"] = 0;
    indicatorPoint["EMA"] = 0;
    indicatorPoint["SMA"] = 0;
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
	indicatorPoint[indicator] += 1;

	if(indicatorPoint[indicator] == graphSize)
	{
    	    float adjusted_max = max_price - min_price;
	    float adjusted_tick = newPrice - min_price;
	    int y = height - margin_bottom - static_cast<int>(adjusted_tick / adjusted_max * (height - margin_top - margin_bottom));
	    y = clamp(y, margin_top, height - margin_bottom);

	    std::ostringstream oss;
	    oss << newPrice;
	    std::string numberY = oss.str();	
	    GraphLabel(width - margin_right, y, numberY, 480, 100, 100, true, indicatorColors[indicator], RGBA(0xFF, 0xFF, 0xFF, 0xFF));
	}


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

void PNGPlotter::addDataPointsPCA(const std::vector<std::vector<double> >& data, const RGBA& pointColor)
{
    // Calculate the effective plotting area considering the margins
    int effectiveWidth = width - margin_left - margin_right;
    int effectiveHeight = height - margin_top - margin_bottom;

    // If no data or empty data, return
    if (data.empty() || data[0].empty()) {
        return;
    }

    // Add a white background
    for (int x = margin_left; x < width - margin_right; ++x)
    {
        for (int y = margin_top; y < height - margin_bottom; ++y) {
            image.SetPixel(x, y, RGBA(0xFF, 0xFF, 0xFF, 0xFF));
        }
    }

    // Determine number of features (columns) in the dataset
    size_t numFeatures = data[0].size();
    
    // Create color variations for each feature
    std::vector<RGBA> featureColors;
    
    // Generate colors for each feature
    for (size_t i = 0; i < numFeatures; i++) {
        // Create a distinct color for each feature
        unsigned char r = (pointColor.r + i * 40) % 256;
        unsigned char g = (pointColor.g + i * 60) % 256;
        unsigned char b = (pointColor.b + i * 80) % 256;
        
        featureColors.push_back(RGBA(r, g, b, pointColor.a));
    }

    // Find min and max values for each feature for proper scaling
    std::vector<double> minValues(numFeatures, std::numeric_limits<double>::max());
    std::vector<double> maxValues(numFeatures, -std::numeric_limits<double>::max());

    for (size_t i = 0; i < data.size(); ++i) {
        for (size_t j = 0; j < numFeatures && j < data[i].size(); ++j) {
            minValues[j] = std::min(minValues[j], data[i][j]);
            maxValues[j] = std::max(maxValues[j], data[i][j]);
        }
    }

    // Calculate the center (origin) of the plot
    int centerX = margin_left + effectiveWidth / 2;
    int centerY = margin_top + effectiveHeight / 2;

    // Calculate point thickness based on data density
    int pointThickness = std::max(6, std::min(16, static_cast<int>(8.0 * (effectiveWidth / 800.0))));

    // Calculate plot area padding (percentage of effective dimensions)
    double paddingRatio = 0.1; // 10% padding
    int paddingX = static_cast<int>(effectiveWidth * paddingRatio);
    int paddingY = static_cast<int>(effectiveHeight * paddingRatio);

    // Usable plotting area after padding
    int usableWidth = effectiveWidth - 2 * paddingX;
    int usableHeight = effectiveHeight - 2 * paddingY;

    // Determine if we should calculate scales separately for each dimension
    // or use a common scale to maintain aspect ratio
    bool maintainAspectRatio = true;
    
    // For each pair of features, create a plot
    for (size_t i = 0; i < numFeatures; ++i) {
        for (size_t j = i + 1; j < numFeatures; ++j) {
            // Calculate scales for this feature pair
            double rangeX = maxValues[i] - minValues[i];
            double rangeY = maxValues[j] - minValues[j];

            // Avoid division by zero
            if (rangeX < 1e-10) rangeX = 1.0;
            if (rangeY < 1e-10) rangeY = 1.0;

            double scaleX, scaleY;
            if (maintainAspectRatio) {
                // Use the same scale for both axes to maintain aspect ratio
                double scaleByX = usableWidth / rangeX;
                double scaleByY = usableHeight / rangeY;
                double commonScale = std::min(scaleByX, scaleByY);
                
                scaleX = commonScale;
                scaleY = commonScale;
            } else {
                // Scale independently for each axis
                scaleX = usableWidth / rangeX;
                scaleY = usableHeight / rangeY;
            }

            // Draw each data point for this feature pair
            for (size_t k = 0; k < data.size(); ++k) {
                if (data[k].size() <= std::max(i, j)) continue; // Skip if not enough dimensions
                
                // Map data coordinates to screen coordinates
                double valueX = data[k][i];
                double valueY = data[k][j];
                
                // Scale relative to center
                int screenX = centerX + static_cast<int>((valueX - (minValues[i] + maxValues[i]) / 2) * scaleX);
                int screenY = centerY - static_cast<int>((valueY - (minValues[j] + maxValues[j]) / 2) * scaleY);
                
                // Ensure the point is within the plotting area
                screenX = clamp(screenX, margin_left + paddingX, width - margin_right - paddingX);
                screenY = clamp(screenY, margin_top + paddingY, height - margin_bottom - paddingY);
                
                // Use a color that combines the colors of both features
                RGBA combinedColor(
                    (featureColors[i].r + featureColors[j].r) / 2,
                    (featureColors[i].g + featureColors[j].g) / 2,
                    (featureColors[i].b + featureColors[j].b) / 2,
                    pointColor.a
                );
                
                // Draw the point with proper bounds checking
                drawPoint(screenX, screenY, pointThickness, combinedColor);
            }
        }
    }

    // Draw axes at the center of the plot
    RGBA axisColor(100, 100, 100, 200);
    // X-axis
    drawLine(margin_left, centerY, width - margin_right, centerY, axisColor);
    // Y-axis
    drawLine(centerX, margin_top, centerX, height - margin_bottom, axisColor);
}

void PNGPlotter::addArrow(const std::vector<std::vector<double> >& sorted_eig_vecs, const RGBA& arrowColor, int arrowSize)
{
    // Calculate the effective plotting area considering the margins
    int effectiveWidth = width - margin_left - margin_right;
    int effectiveHeight = height - margin_top - margin_bottom;

    // Calculate the center (origin) of the plot
    int centerX = margin_left + effectiveWidth / 2;
    int centerY = margin_top + effectiveHeight / 2;

    // Determine the scale factor based on the plot size
    // Using a more moderate scale to ensure arrows stay on the plot
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

void PNGPlotter::drawPoint(int x, int y, int thickness, const RGBA& pointColor)
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

void PNGPlotter::drawLine(int x1, int y1, int x2, int y2, const RGBA& lineColor, int lineWidth)
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
            for (int j = -lineWidth / 2; i <= lineWidth / 2; ++i) {
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
void PNGPlotter::drawArrow(int x1, int y1, int x2, int y2, const RGBA& arrowColor, int arrowSize = 10)
{

    // Draw the main line
    drawLine(x1, y1, x2, y2, arrowColor);

    // Add line thickness
    drawLine(x1 - 1, y1, x2 - 1, y2, arrowColor);
    drawLine(x1 + 1, y1, x2 + 1, y2, arrowColor);
    drawLine(x1, y1 - 1, x2, y2 - 1, arrowColor);
    drawLine(x1, y1 + 1, x2, y2 + 1, arrowColor);

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
    drawLine(x2, y2 + 1, arrowX1, arrowY1 + 1, arrowColor);

    // Draw the arrowhead lines
    drawLine(x2, y2, arrowX2, arrowY2, arrowColor);

    // Add line thickness
    drawLine(x2 - 1, y2, arrowX2 - 1, arrowY2, arrowColor);
    drawLine(x2 + 1, y2, arrowX2 + 1, arrowY2, arrowColor);
    drawLine(x2, y2 - 1, arrowX2, arrowY2 - 1, arrowColor);
    drawLine(x2, y2 + 1, arrowX2, arrowY2 + 1, arrowColor);
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

void PNGPlotter::GraphLabel(unsigned int penX, unsigned int penY, const std::string& text, unsigned int fontSize, unsigned int xOffset, unsigned int yOffset, bool hasBox, RGBA labelColor, RGBA labelColorText) {
    if (FT_Set_Pixel_Sizes(face, 0, fontSize)) 
    {
        printf("Error: Could not set pixel sizes\n");
        return;
    }
    penX += xOffset;
    penY -= yOffset;
   
    // Compute baseline using font metrics
    int ascender = face->size->metrics.ascender / 64; // Convert from 26.6 fixed-point to pixels
    int descender = face->size->metrics.descender / 64; // Convert to pixels
    float heightScale = 0.3;
    // Compute a common baseline using font metrics
    int baseline = face->size->metrics.ascender / 64; // Convert from 26.6 fixed-point to pixels
    unsigned int extraSpacing = fontSize / 3;

    // Calculate the bounding box for the text
    unsigned int boxWidth = 0;
    unsigned int boxHeight = static_cast<unsigned int>((ascender - descender) * heightScale); // Total scaled height of the text block

    // Measure the total width of the text
    for (char c : text)
    {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            printf("Warning: Could not load character %c\n", c);
            continue;
        }

        FT_GlyphSlot glyph = face->glyph;

        // Add glyph width and extra spacing
        boxWidth += (glyph->advance.x >> 6) + extraSpacing;

        // Adjust boxHeight if a taller glyph is found (scaled height)
        unsigned int glyphHeight = static_cast<unsigned int>(glyph->bitmap.rows * heightScale);
        if (glyphHeight > boxHeight)
        {
            boxHeight = glyphHeight;
        }
    }

    // Add padding to the box
    unsigned int padding = fontSize / 6;

    // Draw the box if necessary
    if (hasBox) {
        unsigned int boxX = penX; // Adjust box start position
        unsigned int boxY = penY - boxHeight; // Adjust for baseline alignment
        for (unsigned int y = 0; y < boxHeight; ++y) {
            for (unsigned int x = 0; x < boxWidth; ++x) {
                unsigned imgX = boxX + x;
                unsigned imgY = boxY + y + (boxHeight);

                if (imgX < width && imgY < height) {
                    image.SetPixel(imgX, imgY, labelColor); // Draw the background box
                }
            }
        }
    }


    for (char c : text) 
    {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) 
	{
            printf("Warning: Could not load character %c\n", c);
            continue;
        }

        FT_GlyphSlot glyph = face->glyph;

        unsigned glyphWidth = glyph->bitmap.width;
        unsigned glyphHeight = glyph->bitmap.rows;

        unsigned int x0 = penX + glyph->bitmap_left;
        unsigned int y0 = penY + static_cast<unsigned int>(baseline * heightScale) - static_cast<unsigned int>(glyph->bitmap_top * heightScale);
    

        // Draw the glyph bitmap as solid black
        for (unsigned int y = 0; y < glyphHeight; ++y) 
	{
            for (unsigned int x = 0; x < glyphWidth; ++x) 
	    {
                unsigned imgX = x0 + x;
                unsigned imgY = y0 + static_cast<unsigned int>(y * heightScale);

                if (imgX < width && imgY < height) 
		{
                    unsigned char value = glyph->bitmap.buffer[y * glyphWidth + x];
                    if (value > 0) 
		    { // Only draw if the glyph pixel is not empty
			image.SetPixel(imgX, imgY, labelColorText); 
                    }
                }
            }
        }

        // Advance cursor position
        penX += (glyph->advance.x >> 6) + extraSpacing;
    }
}

/*
 * Header PNG parameters:
 *  text: text to be displayed
 *  fontSize: size of the text
 *  headerPos: which row on the header it will be on (i.e. 0 will be the highest)
 *  rePositionY: If the fontSize is different you can choose to reposition the text, this value will be divided by that rows y-axis
 */ 
//TODO: headerPenY is still not perfect, when changing to smaller or bigger fonts the spacing gets messed up, I currenlty just edit the headerYSpacing, but I want to find the best Y value programmatically
void PNGPlotter::HeaderPNG(const std::string& text, unsigned int fontSize, unsigned int headerPos, unsigned int rePositionY, RGBA headerTextColor)
{

    unsigned int headerPenX = 0;
    unsigned int headerPenY = (headerPenYStarting * (headerPos + 1)) + (headerYSpacing * (headerPos));

    if(rePositionY != 0)
    {
	headerPenY += (headerPenY / rePositionY);
    }


    std::vector<unsigned int> initializeHeaderPosition;

    if(headerPos + 1 > headerSpacings.size())
    {
	std::vector<unsigned int> newVector;
	newVector.push_back(headerPenXStarting);
	headerSpacings.push_back(newVector);
    }

    headerPenX = headerSpacings[headerPos][headerSpacings[headerPos].size() - 1];
    //set font size
    FT_Set_Pixel_Sizes(face, 0, fontSize);

    unsigned int extraSpacing = fontSize / 4;

    float heightScale = 0.3;
    // Compute a common baseline using font metrics
    int baseline = face->size->metrics.ascender / 64; // Convert from 26.6 fixed-point to pixels

    for (char c : text) 
    {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) 
	{
            printf("Warning: Could not load character %c\n", c);
            continue;
        }

        FT_GlyphSlot glyph = face->glyph;

        unsigned glyphWidth = glyph->bitmap.width;
        unsigned glyphHeight = glyph->bitmap.rows;

        unsigned int x0 = headerPenX + glyph->bitmap_left;
        unsigned int y0 = headerPenY + static_cast<unsigned int>(baseline * heightScale) - static_cast<unsigned int>(glyph->bitmap_top * heightScale);
    

        // Draw the glyph bitmap as solid black
        for (unsigned int y = 0; y < glyphHeight; ++y) 
	{
            for (unsigned int x = 0; x < glyphWidth; ++x) 
	    {
                unsigned imgX = x0 + x;
                unsigned imgY = y0 + static_cast<unsigned int>(y * heightScale);

                if (imgX < width && imgY < height) 
		{
                    unsigned char value = glyph->bitmap.buffer[y * glyphWidth + x];
                    if (value > 0) 
		    { // Only draw if the glyph pixel is not empty
                        image.SetPixel(imgX, imgY, headerTextColor); // Solid black
                    }
                }
            }
        }

        // Advance cursor position
        headerPenX += (glyph->advance.x >> 6) + extraSpacing;
    }
    headerSpacings[headerPos].push_back(headerPenX + headerXSpacing);
}

std::string PNGPlotter::aggString(int aggSize)
{
	if(AGG_SIZE.find(aggSize) != AGG_SIZE.end())
	{
		return AGG_SIZE[aggSize];
	}
	return "";
}

void PNGPlotter::SavePNG(const std::string& filename, const std::string& folder)
{

	Image downsampleImage = downsampleToTargetSize();

	std::string full_path = folder;
	full_path.append("/");
	full_path.append(filename);
//	image.SavePNG(full_path.c_str());
	downsampleImage.SavePNG(full_path.c_str());

	FT_Done_Face(face);
        FT_Done_FreeType(ft);
}
