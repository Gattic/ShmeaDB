#include "DataMapper.h"
#include <algorithm>

namespace shmea {

DataMapper::DataMapper(ChartLayout& chartLayout)
    : layout(chartLayout),
      currentXRange(0.0, 1.0, 0.05),
      currentYRange(0.0, 1.0, 0.05)
{
}

DataMapper::~DataMapper() {
    // Nothing to clean up
}

// Implement the functor methods for Point X values
bool DataMapper::PointXValueFunctor::isEmptyData(const std::vector<Point>& data) const {
    return data.empty();
}

size_t DataMapper::PointXValueFunctor::getSize(const std::vector<Point>& data) const {
    return data.size();
}

bool DataMapper::PointXValueFunctor::isValidIndex(const std::vector<Point>& data, size_t i) const {
    return i < data.size(); // Always true for vector
}

double DataMapper::PointXValueFunctor::getValue(const std::vector<Point>& data, size_t i) const {
    return data[i].x;
}

// Implement the functor methods for Point Y values
bool DataMapper::PointYValueFunctor::isEmptyData(const std::vector<Point>& data) const {
    return data.empty();
}

size_t DataMapper::PointYValueFunctor::getSize(const std::vector<Point>& data) const {
    return data.size();
}

bool DataMapper::PointYValueFunctor::isValidIndex(const std::vector<Point>& data, size_t i) const {
    return i < data.size(); // Always true for vector
}

double DataMapper::PointYValueFunctor::getValue(const std::vector<Point>& data, size_t i) const {
    return data[i].y;
}

// Implement the functor methods for Matrix X values (first column)
bool DataMapper::MatrixXValueFunctor::isEmptyData(const std::vector<std::vector<double> >& data) const {
    return data.empty() || data[0].empty();
}

size_t DataMapper::MatrixXValueFunctor::getSize(const std::vector<std::vector<double> >& data) const {
    return data.size();
}

bool DataMapper::MatrixXValueFunctor::isValidIndex(const std::vector<std::vector<double> >& data, size_t i) const {
    return i < data.size() && !data[i].empty();
}

double DataMapper::MatrixXValueFunctor::getValue(const std::vector<std::vector<double> >& data, size_t i) const {
    return data[i][0];
}

// Implement the functor methods for Matrix Y values (second column)
bool DataMapper::MatrixYValueFunctor::isEmptyData(const std::vector<std::vector<double> >& data) const {
    return data.empty() || data[0].size() < 2;
}

size_t DataMapper::MatrixYValueFunctor::getSize(const std::vector<std::vector<double> >& data) const {
    return data.size();
}

bool DataMapper::MatrixYValueFunctor::isValidIndex(const std::vector<std::vector<double> >& data, size_t i) const {
    return i < data.size() && data[i].size() > 1;
}

double DataMapper::MatrixYValueFunctor::getValue(const std::vector<std::vector<double> >& data, size_t i) const {
    return data[i][1];
}

// Generic range calculation template
template<typename DataType, typename ValueFunction>
DataMapper::AxisRange DataMapper::calculateRange(const DataType& data, ValueFunction valueFunc) {
    AxisRange range;
    
    // Return default range if data is empty or doesn't meet conditions
    if (valueFunc.isEmptyData(data)) {
        return range;
    }
    
    // Initialize min and max values from first element
    range.min = range.max = valueFunc.getValue(data, 0);
    
    // Find min and max values in the data
    for (size_t i = 1; i < valueFunc.getSize(data); ++i) {
        if (valueFunc.isValidIndex(data, i)) {
            double value = valueFunc.getValue(data, i);
            range.min = std::min(range.min, value);
            range.max = std::max(range.max, value);
        }
    }
    
    // Add padding
    double padding = (range.max - range.min) * range.padding;
    if (padding < 1e-10) {
        padding = 1.0; // Minimum padding to avoid division by zero
    }
    
    range.min -= padding;
    range.max += padding;
    
    return range;
}

DataMapper::AxisRange DataMapper::calculateXRange(const std::vector<Point>& points) {
    // Use the class-level functor
    PointXValueFunctor func;
    AxisRange range = calculateRange(points, func);
    
    // Update current X range
    currentXRange = range;
    
    return range;
}

DataMapper::AxisRange DataMapper::calculateYRange(const std::vector<Point>& points) {
    // Use the class-level functor
    PointYValueFunctor func;
    AxisRange range = calculateRange(points, func);
    
    // Update current Y range
    currentYRange = range;
    
    return range;
}

DataMapper::AxisRange DataMapper::calculateXRange(const std::vector<std::vector<double> >& data) {
    // Use the class-level functor
    MatrixXValueFunctor func;
    AxisRange range = calculateRange(data, func);
    
    // Update current X range
    currentXRange = range;
    
    return range;
}

DataMapper::AxisRange DataMapper::calculateYRange(const std::vector<std::vector<double> >& data) {
    // Use the class-level functor
    MatrixYValueFunctor func;
    AxisRange range = calculateRange(data, func);
    
    // Update current Y range
    currentYRange = range;
    
    return range;
}

DataMapper::Point DataMapper::mapDataToScreen(double x, double y, const AxisRange& xRange, const AxisRange& yRange) {
    // Calculate the effective plotting area
    int plotWidth = layout.getPlotWidth();
    int plotHeight = layout.getPlotHeight();
    
    // Map X coordinate from data space to screen space
    double xRatio = (x - xRange.min) / (xRange.max - xRange.min);
    int screenX = layout.getMarginLeft() + static_cast<int>(xRatio * plotWidth);
    
    // Map Y coordinate from data space to screen space (Y-axis is inverted in screen coordinates)
    double yRatio = (y - yRange.min) / (yRange.max - yRange.min);
    int screenY = layout.getHeight() - layout.getMarginBottom() - static_cast<int>(yRatio * plotHeight);
    
    // Ensure the point is within the plot bounds
    screenX = ChartLayout::clamp(screenX, layout.getMarginLeft(), layout.getWidth() - layout.getMarginRight());
    screenY = ChartLayout::clamp(screenY, layout.getMarginTop(), layout.getHeight() - layout.getMarginBottom());
    
    return Point(screenX, screenY);
}

// Explicit template instantiations required for C++03
template DataMapper::AxisRange DataMapper::calculateRange<std::vector<DataMapper::Point>, DataMapper::PointXValueFunctor>(
    const std::vector<DataMapper::Point>&, DataMapper::PointXValueFunctor);
    
template DataMapper::AxisRange DataMapper::calculateRange<std::vector<DataMapper::Point>, DataMapper::PointYValueFunctor>(
    const std::vector<DataMapper::Point>&, DataMapper::PointYValueFunctor);
    
template DataMapper::AxisRange DataMapper::calculateRange<std::vector<std::vector<double> >, DataMapper::MatrixXValueFunctor>(
    const std::vector<std::vector<double> >&, DataMapper::MatrixXValueFunctor);
    
template DataMapper::AxisRange DataMapper::calculateRange<std::vector<std::vector<double> >, DataMapper::MatrixYValueFunctor>(
    const std::vector<std::vector<double> >&, DataMapper::MatrixYValueFunctor);

} // namespace shmea 