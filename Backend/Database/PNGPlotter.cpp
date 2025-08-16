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
	color_bullish(0x03, 0xC0, 0x3C, 0xFF), // Modern green from cluster10.fig theme
	color_bearish(0xFF, 0x47, 0x45, 0xFF), // Modern red from cluster10.fig theme
	headerPenXStarting(500),
	headerPenYStarting(75),
	headerXSpacing(25),
	headerYSpacing(150)
{
	image.Allocate(width, height);
	RGBA DarkNavy(0x18, 0x1B, 0x2C, 0xFF);  // Dark navy blue top from cluster10.fig theme
	RGBA DarkerNavy(0x0A, 0x0C, 0x16, 0xFF); // Darker navy bottom from cluster10.fig theme
	image.drawVerticalGradient(0, 0, DarkNavy, DarkerNavy, 0);

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
    RGBA lineColor(0x3A, 0x41, 0x5A, 0xDD); // Subtle blue-gray lines from cluster10.fig theme

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
    RGBA gridColor(0x3A, 0x41, 0x5A, 0x99);  // Subtle blue-gray for grid lines

    std::vector<float> horizontalLines = get_axis_ticks(max_price, min_price);
    float adjusted_max = max_price - min_price;
    // Draw horizontal grid lines and y-axis labels
    for (size_t i = 0; i < horizontalLines.size(); ++i) {
	float adjusted_tick = horizontalLines[i] - min_price;
	int y = height - margin_bottom - static_cast<int>(adjusted_tick / adjusted_max * (height - margin_top - margin_bottom));
	y = clamp(y, margin_top, height - margin_bottom);

        drawLine(margin_left, y, width - margin_right, y, gridColor);

	std::ostringstream oss;
	oss << horizontalLines[i];
	std::string numberY = oss.str();	
	if(i != 0)
	{
	    GraphLabel(width - margin_right, y, numberY, 600, 100, 100, false, RGBA(), RGBA(0xFF, 0xFF, 0xFF, 0xFF));
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
    if (std::strftime(buffer, sizeof(buffer), format, std::gmtime(&time))) 
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
    RGBA gridColor(0x3A, 0x41, 0x5A, 0x99);  // Subtle blue-gray for grid lines

    std::vector<std::string> verticalLines = get_date_labels(start, end, graphSize);
    
    // Calculate step size for x-axis grid
    int step = (width - margin_left - margin_right) / (verticalLines.size() - 1);

    // Draw vertical grid lines and x-axis labels
    for (size_t i = 0; i < verticalLines.size(); ++i) 
    {
        int x = margin_left + i * step;
        drawLine(x, margin_top, x, height - margin_bottom, gridColor);

	GraphLabel(x, height - margin_top - margin_bottom + 30, verticalLines[i], 600, -(600/4), 600/8, false, RGBA(), RGBA(0xFF, 0xFF, 0xFF, 0xFF));
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
    // Modern color palette inspired by cluster10.fig theme
    lines_colors.push_back(RGBA(0x00, 0x9E, 0xFF, 0xFF));    // Bright Blue
    lines_colors_name.push_back("Bright Blue");

    lines_colors.push_back(RGBA(0xFF, 0x6B, 0x00, 0xFF));    // Vivid Orange
    lines_colors_name.push_back("Vivid Orange");

    lines_colors.push_back(RGBA(0x9D, 0x02, 0xFF, 0xFF));    // Rich Purple
    lines_colors_name.push_back("Rich Purple");

    lines_colors.push_back(RGBA(0x00, 0xC2, 0xC7, 0xFF));    // Teal
    lines_colors_name.push_back("Teal");

    lines_colors.push_back(RGBA(0xFF, 0xD4, 0x00, 0xFF));    // Golden Yellow
    lines_colors_name.push_back("Golden Yellow");

    lines_colors.push_back(RGBA(0xFF, 0x0C, 0xC0, 0xFF));    // Magenta
    lines_colors_name.push_back("Magenta");

    lines_colors.push_back(RGBA(0x03, 0xC0, 0x3C, 0xFF));    // Deep Green
    lines_colors_name.push_back("Deep Green");

    lines_colors.push_back(RGBA(0xFF, 0x47, 0x45, 0xFF));    // Coral Red
    lines_colors_name.push_back("Coral Red");

    lines_colors.push_back(RGBA(0x29, 0xA2, 0xC6, 0xFF));    // Steel Blue
    lines_colors_name.push_back("Steel Blue");

    lines_colors.push_back(RGBA(0x85, 0x8A, 0xFF, 0xFF));    // Periwinkle
    lines_colors_name.push_back("Periwinkle");

    // Update indicator colors to match the new theme
    indicatorColors["BBLow"] = RGBA(0x00, 0x9E, 0xFF, 0xFF);  // Bright Blue
    indicatorColors["BBHigh"] = RGBA(0x00, 0x9E, 0xFF, 0xFF); // Bright Blue
    indicatorTextColor["BBHigh"] = RGBA(0x00, 0x4A, 0x80, 0xFF);
    indicatorTextColor["BBLow"] = RGBA(0x00, 0x4A, 0x80, 0xFF);

    indicatorColors["EMA"] = RGBA(0xFF, 0xD4, 0x00, 0xFF);    // Golden Yellow
    indicatorTextColor["EMA"] = RGBA(0x80, 0x6A, 0x00, 0xFF);

    indicatorColors["SMA"] = RGBA(0xFF, 0x6B, 0x00, 0xFF);    // Vivid Orange
    indicatorTextColor["SMA"] = RGBA(0x80, 0x35, 0x00, 0xFF);
    
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

	addDataPoint(newPrice, portIndex, true, &indicatorColors[indicator], 25); 
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

    // Add a modern dark gradient background instead of white
    RGBA gradientTop(0x18, 0x1B, 0x2C, 0xFF);    // Dark navy blue at top
    RGBA gradientBottom(0x0A, 0x0C, 0x16, 0xFF); // Darker navy at bottom
    for (int y = margin_top; y < static_cast<int>(height - margin_bottom); ++y) {
        float ratio = static_cast<float>(y - margin_top) / (height - margin_top - margin_bottom);
        RGBA gradientColor(
            static_cast<unsigned char>(gradientTop.r * (1 - ratio) + gradientBottom.r * ratio),
            static_cast<unsigned char>(gradientTop.g * (1 - ratio) + gradientBottom.g * ratio),
            static_cast<unsigned char>(gradientTop.b * (1 - ratio) + gradientBottom.b * ratio),
            0xFF
        );
        
        for (int x = margin_left; x < static_cast<int>(width - margin_right); ++x) {
            image.SetPixel(x, y, gradientColor);
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
    
    // Draw grid lines
    RGBA majorGridColor(0x3A, 0x41, 0x5A, 0x99);  // Subtle blue-gray for major grid lines, semi-transparent
    RGBA minorGridColor(0x2A, 0x30, 0x45, 0x55);  // Darker blue-gray for minor grid lines, more transparent
    
    // Number of grid divisions
    int majorGridDivisions = 4;   // Number of major grid divisions (quadrants)
    int minorGridDivisions = 16;  // Number of minor grid divisions
    
    // Draw minor grid lines first (so they appear behind major lines)
    for (int i = 0; i <= minorGridDivisions; i++) {
        float percentage = static_cast<float>(i) / minorGridDivisions;
        int x = margin_left + static_cast<int>(percentage * effectiveWidth);
        int y = margin_top + static_cast<int>(percentage * effectiveHeight);
        
        // Draw vertical minor grid line
        if (i % (minorGridDivisions / majorGridDivisions) != 0) { // Skip where major lines will be
            drawLine(x, margin_top, x, height - margin_bottom, minorGridColor);
        }
        
        // Draw horizontal minor grid line
        if (i % (minorGridDivisions / majorGridDivisions) != 0) { // Skip where major lines will be
            drawLine(margin_left, y, width - margin_right, y, minorGridColor);
        }
    }
    
    // Draw major grid lines
    for (int i = 0; i <= majorGridDivisions; i++) {
        float percentage = static_cast<float>(i) / majorGridDivisions;
        int x = margin_left + static_cast<int>(percentage * effectiveWidth);
        int y = margin_top + static_cast<int>(percentage * effectiveHeight);
        
        // Draw vertical major grid line
        drawLine(x, margin_top, x, height - margin_bottom, majorGridColor);
        
        // Draw horizontal major grid line
        drawLine(margin_left, y, width - margin_right, y, majorGridColor);
    }
    
    // Draw axes at the center of the plot with thicker lines
    RGBA axisColor(0x50, 0x50, 0x50, 0xFF);  // Darker gray for axes
    
    // X-axis (thicker line)
    for (int offset = -1; offset <= 1; offset++) {
        drawLine(margin_left, centerY + offset, width - margin_right, centerY + offset, axisColor);
    }
    
    // Y-axis (thicker line)
    for (int offset = -1; offset <= 1; offset++) {
        drawLine(centerX + offset, margin_top, centerX + offset, height - margin_bottom, axisColor);
    }

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
}

void PNGPlotter::addDataPointsKMeans(const std::string& graphName, const std::vector<std::vector<double> >& data, const std::vector<int>& labels, const std::vector<std::vector<float> >& centroids)
{
    // Calculate the effective plotting area considering the margins
    int effectiveWidth = width - margin_left - margin_right;
    int effectiveHeight = height - margin_top - margin_bottom;

    // If no data or empty data, return
    if (data.empty() || data[0].empty()) {
        return;
    }

    // Add a gradient background
    RGBA gradientTop(0x18, 0x1B, 0x2C, 0xFF);    // Dark navy blue at top
    RGBA gradientBottom(0x0A, 0x0C, 0x16, 0xFF); // Darker navy at bottom
    for (int y = margin_top; y < static_cast<int>(height - margin_bottom); ++y) {
        float ratio = static_cast<float>(y - margin_top) / effectiveHeight;
        RGBA gradientColor(
            static_cast<unsigned char>(gradientTop.r * (1 - ratio) + gradientBottom.r * ratio),
            static_cast<unsigned char>(gradientTop.g * (1 - ratio) + gradientBottom.g * ratio),
            static_cast<unsigned char>(gradientTop.b * (1 - ratio) + gradientBottom.b * ratio),
            0xFF
        );
        
        for (int x = margin_left; x < static_cast<int>(width - margin_right); ++x) {
            image.SetPixel(x, y, gradientColor);
        }
    }

    // Determine number of features (columns) in the dataset
    size_t numFeatures = data[0].size();
    
    // Find the number of unique clusters - make sure we're accurately counting clusters
    int maxCluster = -1;
    if (!labels.empty()) {
        for (size_t i = 0; i < labels.size(); i++) {
            if (labels[i] > maxCluster) {
                maxCluster = labels[i];
            }
        }
    }
    
    // Generate colors for each cluster - using modern, vibrant colors with better contrast
    std::vector<RGBA> clusterColors;
    
    // Pre-defined cluster10 theme modern colors for better visibility
    std::vector<RGBA> distinctColors;
    distinctColors.push_back(RGBA(255, 107, 107, 255));  // Coral
    distinctColors.push_back(RGBA(48, 173, 255, 255));   // Azure Blue
    distinctColors.push_back(RGBA(67, 233, 123, 255));   // Mint Green
    distinctColors.push_back(RGBA(255, 222, 89, 255));   // Sunflower Yellow
    distinctColors.push_back(RGBA(190, 75, 255, 255));   // Bright Purple
    distinctColors.push_back(RGBA(58, 227, 217, 255));   // Turquoise
    distinctColors.push_back(RGBA(255, 165, 48, 255));   // Amber
    distinctColors.push_back(RGBA(168, 130, 255, 255));  // Lavender
    distinctColors.push_back(RGBA(65, 222, 176, 255));   // Sea Green
    distinctColors.push_back(RGBA(255, 119, 168, 255));  // Pink
    
    // Use pre-defined colors for the first few clusters, then generate additional colors if needed
    for (int i = 0; i <= maxCluster; i++) {
        if (i < static_cast<int>(distinctColors.size())) {
            clusterColors.push_back(distinctColors[i]);
        } else {
            // Generate additional colors with high contrast for clusters beyond our predefined list
            unsigned char r = (73 * (i + 1)) % 256;
            unsigned char g = (121 * (i + 1)) % 256;
            unsigned char b = (167 * (i + 1)) % 256;
            clusterColors.push_back(RGBA(r, g, b, 255));
        }
    }
    
    // Print out debugging info
    std::cout << "Number of data points: " << data.size() << std::endl;
    std::cout << "Number of labels: " << labels.size() << std::endl;
    std::cout << "Maximum cluster ID: " << maxCluster << std::endl;
    std::cout << "Number of cluster colors: " << clusterColors.size() << std::endl;
    std::cout << "Number of centroids: " << centroids.size() << std::endl;

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
    
    // Draw border around the plot area
    RGBA borderColor(0x3A, 0x41, 0x5A, 0xFF);  // Subtle blue-gray border that matches theme
    
    // Top border
    for (int x = margin_left - 2; x <= static_cast<int>(width - margin_right + 2); x++) {
        for (int t = 0; t < 2; t++) {
            int y = margin_top - 2 + t;
            if (y >= 0 && y < static_cast<int>(height) && x >= 0 && x < static_cast<int>(width)) {
                image.SetPixel(x, y, borderColor);
            }
        }
    }
    
    // Bottom border
    for (int x = margin_left - 2; x <= static_cast<int>(width - margin_right + 2); x++) {
        for (int t = 0; t < 2; t++) {
            int y = height - margin_bottom + t;
            if (y >= 0 && y < static_cast<int>(height) && x >= 0 && x < static_cast<int>(width)) {
                image.SetPixel(x, y, borderColor);
            }
        }
    }
    
    // Left border
    for (int y = margin_top - 2; y <= static_cast<int>(height - margin_bottom + 2); y++) {
        for (int t = 0; t < 2; t++) {
            int x = margin_left - 2 + t;
            if (y >= 0 && y < static_cast<int>(height) && x >= 0 && x < static_cast<int>(width)) {
                image.SetPixel(x, y, borderColor);
            }
        }
    }
    
    // Right border
    for (int y = margin_top - 2; y <= static_cast<int>(height - margin_bottom + 2); y++) {
        for (int t = 0; t < 2; t++) {
            int x = width - margin_right + t;
            if (y >= 0 && y < static_cast<int>(height) && x >= 0 && x < static_cast<int>(width)) {
                image.SetPixel(x, y, borderColor);
            }
        }
    }
    
    // Draw grid lines and axes with improved styling
    
    // Draw minor grid lines first (so they appear behind major lines)
    RGBA majorGridColor(0x3A, 0x41, 0x5A, 0x99);  // Subtle blue-gray for major grid lines, semi-transparent
    RGBA minorGridColor(0x2A, 0x30, 0x45, 0x55);  // Darker blue-gray for minor grid lines, more transparent
    
    // Number of grid divisions
    int majorGridDivisions = 4;   // Number of major grid divisions (quadrants)
    int minorGridDivisions = 16;  // Number of minor grid divisions
    
    for (int i = 0; i <= minorGridDivisions; i++) {
        float percentage = static_cast<float>(i) / minorGridDivisions;
        int x = margin_left + static_cast<int>(percentage * effectiveWidth);
        int y = margin_top + static_cast<int>(percentage * effectiveHeight);
        
        // Draw vertical minor grid line
        if (i % (minorGridDivisions / majorGridDivisions) != 0) { // Skip where major lines will be
            drawLine(x, margin_top, x, height - margin_bottom, minorGridColor);
        }
        
        // Draw horizontal minor grid line
        if (i % (minorGridDivisions / majorGridDivisions) != 0) { // Skip where major lines will be
            drawLine(margin_left, y, width - margin_right, y, minorGridColor);
        }
    }
    
    // Draw major grid lines
    for (int i = 0; i <= majorGridDivisions; i++) {
        float percentage = static_cast<float>(i) / majorGridDivisions;
        int x = margin_left + static_cast<int>(percentage * effectiveWidth);
        int y = margin_top + static_cast<int>(percentage * effectiveHeight);
        
        // Draw vertical major grid line
        drawLine(x, margin_top, x, height - margin_bottom, majorGridColor);
        
        // Draw horizontal major grid line
        drawLine(margin_left, y, width - margin_right, y, majorGridColor);
    }
    
    // Draw axes at the center of the plot with thicker lines
    RGBA axisColor(0xF0, 0xF0, 0xF0, 0xFF);  // Brighter gray for axes
    
    // X-axis (thicker line)
    for (int offset = -2; offset <= 2; offset++) {
        drawLine(margin_left, centerY + offset, width - margin_right, centerY + offset, axisColor);
    }
    
    // Y-axis (thicker line)
    for (int offset = -2; offset <= 2; offset++) {
        drawLine(centerX + offset, margin_top, centerX + offset, height - margin_bottom, axisColor);
    }
    
    // Add title for the K-Means plot with improved styling
    GraphLabel(margin_left + effectiveWidth / 2, margin_top - 30, graphName + " Dataset",
               360, 0, 0, false, RGBA(), RGBA(0xFF, 0xFF, 0xFF, 0xFF));

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
            
            // Structure to store mapped screen coordinates for each cluster
            std::vector<std::vector<std::pair<int, int> > > clusterPoints(maxCluster + 1);
            
            // Draw each data point for this feature pair and collect cluster points
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
                
                // Store the point in its cluster collection
                if (k < labels.size() && labels[k] >= 0 && labels[k] <= maxCluster) {
                    clusterPoints[labels[k]].push_back(std::make_pair(screenX, screenY));
                }
                
                // Select color based on the cluster label
                RGBA pointColorToUse;
                if (k < labels.size() && labels[k] >= 0 && labels[k] < static_cast<int>(clusterColors.size())) {
                    pointColorToUse = clusterColors[labels[k]];
                } else {
                    // Default color if label is invalid - make this visibly different
                    pointColorToUse = RGBA(128, 128, 128, 255); // Gray for invalid labels
                }
                
                // Draw the point with proper bounds checking
                drawPoint(screenX, screenY, pointThickness, pointColorToUse);
            }
            
            // Draw circles around each cluster with improved styling
            for (int clusterID = 0; clusterID <= maxCluster; ++clusterID) {
                if (clusterPoints[clusterID].empty()) continue;
                
                // Get the color for this cluster
                RGBA clusterColor = clusterID < static_cast<int>(clusterColors.size()) ? 
                                    clusterColors[clusterID] : 
                                    RGBA(128, 128, 128, 255);
                
                // Make the outline semi-transparent
                RGBA outlineColor = clusterColor;
                outlineColor.a = 150; // Semi-transparent
                
                // Find the centroid of this cluster in screen coordinates
                int sumX = 0, sumY = 0;
                for (size_t p = 0; p < clusterPoints[clusterID].size(); ++p) {
                    sumX += clusterPoints[clusterID][p].first;
                    sumY += clusterPoints[clusterID][p].second;
                }
                int centroidX = sumX / clusterPoints[clusterID].size();
                int centroidY = sumY / clusterPoints[clusterID].size();
                
                // Find the maximum distance from centroid to any point in the cluster
                int maxDist = 0;
                for (size_t p = 0; p < clusterPoints[clusterID].size(); ++p) {
                    int dx = clusterPoints[clusterID][p].first - centroidX;
                    int dy = clusterPoints[clusterID][p].second - centroidY;
                    int dist = static_cast<int>(std::sqrt(static_cast<double>(dx*dx + dy*dy)));
                    maxDist = std::max(maxDist, dist);
                }
                
                // Add some padding to the radius
                int radius = maxDist + pointThickness * 2;
                
                // Draw a circle to encompass all points in the cluster
                drawClusterCircle(centroidX, centroidY, radius, outlineColor);
                
                // Draw cluster label with improved styling
                std::ostringstream clusterLabel;
                clusterLabel << "Cluster " << clusterID;
                
                // Create a subtle background for the label
                RGBA labelBgColor = clusterColor;
                labelBgColor.a = 180; // Semi-transparent
                
                GraphLabel(centroidX + radius + 10, centroidY, clusterLabel.str(), 
                           200, 0, 0, true, labelBgColor, RGBA(0xFF, 0xFF, 0xFF, 0xFF));
                
                // Draw actual centroids with a special marker (if available)
                if (static_cast<size_t>(clusterID) < centroids.size() && centroids[clusterID].size() > j) {
                    // Map the actual centroid coordinates to screen coordinates
                    double centValueX = centroids[clusterID][i];
                    double centValueY = centroids[clusterID][j];
                    
                    int centScreenX = centerX + static_cast<int>((centValueX - (minValues[i] + maxValues[i]) / 2) * scaleX);
                    int centScreenY = centerY - static_cast<int>((centValueY - (minValues[j] + maxValues[j]) / 2) * scaleY);
                    
                    // Ensure the centroid is within the plotting area
                    centScreenX = clamp(centScreenX, margin_left + paddingX, width - margin_right - paddingX);
                    centScreenY = clamp(centScreenY, margin_top + paddingY, height - margin_bottom - paddingY);
                    
                    // Draw a special marker for the actual centroid
                    RGBA centroidMarkerColor = RGBA(0xFF, 0xFF, 0xFF, 0xFF); // White
                    
                    // Draw a white cross inside a colored circle with increased size for visibility
                    drawPoint(centScreenX, centScreenY, pointThickness * 1.8, clusterColor);
                    
                    // Cross lines with increased thickness for better visibility
                    int crossSize = pointThickness * 1.2;
                    for (int lineOffset = -crossSize; lineOffset <= crossSize; lineOffset++) {
                        for (int thickness = -1; thickness <= 1; thickness++) {
                            if (centScreenX + lineOffset >= static_cast<int>(margin_left) && 
                                centScreenX + lineOffset < static_cast<int>(width - margin_right) &&
                                centScreenY + thickness >= static_cast<int>(margin_top) && 
                                centScreenY + thickness < static_cast<int>(height - margin_bottom)) {
                                image.SetPixel(centScreenX + lineOffset, centScreenY + thickness, centroidMarkerColor);
                            }
                            
                            if (centScreenX + thickness >= static_cast<int>(margin_left) && 
                                centScreenX + thickness < static_cast<int>(width - margin_right) &&
                                centScreenY + lineOffset >= static_cast<int>(margin_top) && 
                                centScreenY + lineOffset < static_cast<int>(height - margin_bottom)) {
                                image.SetPixel(centScreenX + thickness, centScreenY + lineOffset, centroidMarkerColor);
                            }
                        }
                    }
                }
            }
            
            // Add a legend at the bottom of the plot with improved styling
            int legendStartX = margin_left + 50;
            int legendStartY = height - margin_bottom + 50;
            int legendItemWidth = 150; // Increased spacing for better readability
            
            // Add a subtle background for the legend
            RGBA legendBgColor(0x1A, 0x1D, 0x2F, 0xCC); // Semi-transparent dark background
            int legendWidth = (maxCluster + 1) * legendItemWidth + 100;
            int legendHeight = 80;
            int legendX = margin_left + (effectiveWidth - legendWidth) / 2;
            int legendY = legendStartY - 15;
            
            // Draw legend background
            for (int x = legendX; x < legendX + legendWidth; x++) {
                for (int y = legendY; y < legendY + legendHeight; y++) {
                    if (x >= 0 && x < static_cast<int>(width) && y >= 0 && y < static_cast<int>(height)) {
                        image.SetPixel(x, y, legendBgColor);
                    }
                }
            }
            
            // Draw a thin border around the legend
            RGBA legendBorderColor(0x3A, 0x41, 0x5A, 0xFF);
            for (int x = legendX; x < legendX + legendWidth; x++) {
                if (x >= 0 && x < static_cast<int>(width)) {
                    if (legendY >= 0 && legendY < static_cast<int>(height)) {
                        image.SetPixel(x, legendY, legendBorderColor);
                    }
                    if (legendY + legendHeight - 1 >= 0 && legendY + legendHeight - 1 < static_cast<int>(height)) {
                        image.SetPixel(x, legendY + legendHeight - 1, legendBorderColor);
                    }
                }
            }
            for (int y = legendY; y < legendY + legendHeight; y++) {
                if (y >= 0 && y < static_cast<int>(height)) {
                    if (legendX >= 0 && legendX < static_cast<int>(width)) {
                        image.SetPixel(legendX, y, legendBorderColor);
                    }
                    if (legendX + legendWidth - 1 >= 0 && legendX + legendWidth - 1 < static_cast<int>(width)) {
                        image.SetPixel(legendX + legendWidth - 1, y, legendBorderColor);
                    }
                }
            }
            
            for (int clusterID = 0; clusterID <= maxCluster; ++clusterID) {
                RGBA clusterColor = clusterID < static_cast<int>(clusterColors.size()) ? 
                                   clusterColors[clusterID] : 
                                   RGBA(128, 128, 128, 255);
                
                int itemX = legendX + 50 + clusterID * legendItemWidth;
                int itemY = legendY + legendHeight/2;
                
                // Draw color circle instead of square for a more modern look
                int circleRadius = 8;
                for (int dx = -circleRadius; dx <= circleRadius; dx++) {
                    for (int dy = -circleRadius; dy <= circleRadius; dy++) {
                        if (dx*dx + dy*dy <= circleRadius*circleRadius) {
                            int x = itemX + dx;
                            int y = itemY + dy;
                            if (x >= 0 && x < static_cast<int>(width) && y >= 0 && y < static_cast<int>(height)) {
                                image.SetPixel(x, y, clusterColor);
                            }
                        }
                    }
                }
                
                // Draw label with improved font
                std::ostringstream labelText;
                labelText << "Cluster " << clusterID;
                GraphLabel(itemX + 20, itemY, labelText.str(), 
                          180, 0, 0, false, RGBA(), RGBA(0xFF, 0xFF, 0xFF, 0xFF));
            }
        }
    }
}

void PNGPlotter::addArrow(const std::vector<std::vector<double> >& sorted_eig_vecs, const std::vector<double>& variance_explained, const RGBA& arrowColor)
{
    int arrowSize = 100;

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
        //double arrowX2 = arrowX1 + vecX * scaleFactor * variance_explained[i];
        //double arrowY2 = arrowY1 - vecY * scaleFactor * variance_explained[i]; // Note the minus sign for Y coordinate
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
	for(int y = y_start; y < static_cast<int>(height - margin_bottom); ++y)
	{
	    image.SetPixel(x, y, barColor);
	}
    }
}

void PNGPlotter::drawPoint(int x, int y, int thickness, const RGBA& pointColor)
{
 // Check if the main point is within the plotting area bounds
    if (x >= static_cast<int>(margin_left) && x < static_cast<int>(width - margin_right) &&
        y >= static_cast<int>(margin_top) && y < static_cast<int>(height - margin_bottom)) {
        
        // Draw the main point
        image.SetPixel(x, y, pointColor);

    	// Draw additional pixels for thickness with bounds checking to make it round
	for (int dx = -thickness; dx <= thickness; ++dx) {
	    for (int dy = -thickness; dy <= thickness; ++dy) {
		// Check if the pixel falls within the circle defined by the thickness
		if (dx * dx + dy * dy <= thickness * thickness) {
		    int newX = x + dx;
		    int newY = y + dy;
		    if (newX >= static_cast<int>(margin_left) && newX < static_cast<int>(width - margin_right) &&
			newY >= static_cast<int>(margin_top) && newY < static_cast<int>(height - margin_bottom)) {
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

                if (newX >= static_cast<int>(margin_left) && newX < static_cast<int>(width - margin_right) &&
                    newY >= static_cast<int>(margin_top) && newY < static_cast<int>(height - margin_bottom)) {
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
            if (x + i >= static_cast<int>(margin_left) && 
                x + i < static_cast<int>(img.getWidth()) - static_cast<int>(margin_right) && 
                y >= static_cast<int>(margin_top) && 
                y < static_cast<int>(img.getHeight()) - static_cast<int>(margin_bottom)) 
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
            if (x + dx >= static_cast<int>(margin_left) && 
                x + dx < static_cast<int>(img.getWidth()) - static_cast<int>(margin_right) && 
                y >= static_cast<int>(margin_top) && 
                y < static_cast<int>(img.getHeight()) - static_cast<int>(margin_bottom)) {
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
    drawLine(x1, y1 + 1, x2 + 1, y2 + 1, arrowColor);

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
    drawLine(x2, y2 + 1, arrowX1 + 1, arrowY1 + 1, arrowColor);

    // Draw the arrowhead lines
    drawLine(x2, y2, arrowX2, arrowY2, arrowColor);

    // Add line thickness
    drawLine(x2 - 1, y2, arrowX2 - 1, arrowY2, arrowColor);
    drawLine(x2 + 1, y2, arrowX2 + 1, arrowY2, arrowColor);
    drawLine(x2, y2 - 1, arrowX2, arrowY2 - 1, arrowColor);
    drawLine(x2, y2 + 1, arrowX2 + 1, arrowY2 + 1, arrowColor);
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
    
    float heightScale = 0.5;
    
    // Compute a common baseline using font metrics
    int baseline = face->size->metrics.ascender / 64; 
    
    // Adjusted spacing for better readability
    unsigned int extraSpacing = fontSize / 6; // Adjusted from /8 to /6

    // Calculate the bounding box for the text
    unsigned int boxWidth = 0;
    unsigned int boxHeight = static_cast<unsigned int>((ascender - descender) * heightScale); // Total scaled height of the text block

    // Measure the total width of the text with reduced spacing
    for (size_t i = 0; i < text.length(); ++i)
    {
        char c = text[i];
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            printf("Warning: Could not load character %c\n", c);
            continue;
        }

        FT_GlyphSlot glyph = face->glyph;

        // Add glyph width and reduced extra spacing
        boxWidth += (glyph->advance.x >> 6) + extraSpacing;

        // Adjust boxHeight if a taller glyph is found (with reduced scaled height)
        unsigned int glyphHeight = static_cast<unsigned int>(glyph->bitmap.rows * heightScale);
        if (glyphHeight > boxHeight)
        {
            boxHeight = glyphHeight;
        }
    }

    // Add padding to the box
    unsigned int padding = fontSize / 10; // Reduced padding

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

    for (size_t i = 0; i < text.length(); ++i) 
    {
        char c = text[i];
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
    
        // Draw the glyph bitmap with improved aspect ratio
        for (unsigned int y = 0; y < glyphHeight; ++y) 
        {
            for (unsigned int x = 0; x < glyphWidth; ++x) 
            {
                // Calculate adjusted position with improved aspect ratio
                unsigned imgX = x0 + x;
                
                // Use heightScale to vertically compress the text for better aspect ratio
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

        // Advance cursor position with reduced spacing
        penX += (glyph->advance.x >> 6) + extraSpacing;
    }
}

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

    for (size_t i = 0; i < text.length(); ++i) 
    {
        char c = text[i];
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

void PNGPlotter::drawClusterCircle(int x, int y, int radius, const RGBA& color) {
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

void PNGPlotter::drawCirclePoints(int x, int y, int x0, int y0, const RGBA& color) {
    // Draw points with additional pixels for thickness
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            // Check if points are within the plotting area bounds
            if (x + x0 + dx >= static_cast<int>(margin_left) && 
                x + x0 + dx < static_cast<int>(width - margin_right) &&
                y + y0 + dy >= static_cast<int>(margin_top) && 
                y + y0 + dy < static_cast<int>(height - margin_bottom)) {
                image.SetPixel(x + x0 + dx, y + y0 + dy, color);
            }
            if (x - x0 + dx >= static_cast<int>(margin_left) && 
                x - x0 + dx < static_cast<int>(width - margin_right) &&
                y + y0 + dy >= static_cast<int>(margin_top) && 
                y + y0 + dy < static_cast<int>(height - margin_bottom)) {
                image.SetPixel(x - x0 + dx, y + y0 + dy, color);
            }
            if (x + x0 + dx >= static_cast<int>(margin_left) && 
                x + x0 + dx < static_cast<int>(width - margin_right) &&
                y - y0 + dy >= static_cast<int>(margin_top) && 
                y - y0 + dy < static_cast<int>(height - margin_bottom)) {
                image.SetPixel(x + x0 + dx, y - y0 + dy, color);
            }
            if (x - x0 + dx >= static_cast<int>(margin_left) && 
                x - x0 + dx < static_cast<int>(width - margin_right) &&
                y - y0 + dy >= static_cast<int>(margin_top) && 
                y - y0 + dy < static_cast<int>(height - margin_bottom)) {
                image.SetPixel(x - x0 + dx, y - y0 + dy, color);
            }
            if (x + y0 + dx >= static_cast<int>(margin_left) && 
                x + y0 + dx < static_cast<int>(width - margin_right) &&
                y + x0 + dy >= static_cast<int>(margin_top) && 
                y + x0 + dy < static_cast<int>(height - margin_bottom)) {
                image.SetPixel(x + y0 + dx, y + x0 + dy, color);
            }
            if (x - y0 + dx >= static_cast<int>(margin_left) && 
                x - y0 + dx < static_cast<int>(width - margin_right) &&
                y + x0 + dy >= static_cast<int>(margin_top) && 
                y + x0 + dy < static_cast<int>(height - margin_bottom)) {
                image.SetPixel(x - y0 + dx, y + x0 + dy, color);
            }
            if (x + y0 + dx >= static_cast<int>(margin_left) && 
                x + y0 + dx < static_cast<int>(width - margin_right) &&
                y - x0 + dy >= static_cast<int>(margin_top) && 
                y - x0 + dy < static_cast<int>(height - margin_bottom)) {
                image.SetPixel(x + y0 + dx, y - x0 + dy, color);
            }
            if (x - y0 + dx >= static_cast<int>(margin_left) && 
                x - y0 + dx < static_cast<int>(width - margin_right) &&
                y - x0 + dy >= static_cast<int>(margin_top) && 
                y - x0 + dy < static_cast<int>(height - margin_bottom)) {
                image.SetPixel(x - y0 + dx, y - x0 + dy, color);
            }
        }
    }
}
