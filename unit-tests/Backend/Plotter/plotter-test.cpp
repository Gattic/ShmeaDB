#include "plotter-test.h"
#include "../../../Backend/Plotter/Plotter.h"
#include "../../../Backend/Plotter/GridRenderer.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <cmath>
#include <iostream>

using namespace shmea;

void shmea::testCluster() {
    printf("Testing Cluster visualization with the new fluent API...\n");
    
    // Seed random number generator with a fixed value for consistent output
    std::srand(42);
    
    // Create a Plotter instance
    Plotter plotter(1800, 1000, 4);
    
    // Create 3 well-defined clusters that match the image
    std::vector<std::vector<double> > clusterData;
    std::vector<int> clusterLabels;
    
    // Cluster 0 - bottom left (orange)
    for (int i = 0; i < 40; ++i) {
        std::vector<double> point;
        // Create a more concentrated cluster
        point.push_back(2.0 + (std::rand() % 150) / 100.0);  // x: 2.0-3.5
        point.push_back(2.0 + (std::rand() % 150) / 100.0);  // y: 2.0-3.5
        clusterData.push_back(point);
        clusterLabels.push_back(0);
    }
    
    // Cluster 1 - middle right (blue)
    for (int i = 0; i < 40; ++i) {
        std::vector<double> point;
        // Position this cluster in the middle right
        point.push_back(6.0 + (std::rand() % 200) / 100.0);  // x: 6.0-8.0
        point.push_back(5.0 + (std::rand() % 200) / 100.0);  // y: 5.0-7.0
        clusterData.push_back(point);
        clusterLabels.push_back(1);
    }
    
    // Cluster 2 - right (green) - smaller and more concentrated
    for (int i = 0; i < 40; ++i) {
        std::vector<double> point;
        // Position this to match the green cluster
        point.push_back(8.5 + (std::rand() % 150) / 100.0);  // x: 8.5-10.0
        point.push_back(3.0 + (std::rand() % 150) / 100.0);  // y: 3.0-4.5
        clusterData.push_back(point);
        clusterLabels.push_back(2);
    }
    
    // Define centroids at exact positions matching the image
    std::vector<std::vector<double> > centroids;
    
    // Centroid for cluster 0 (orange)
    std::vector<double> centroid0;
    centroid0.push_back(2.75);
    centroid0.push_back(2.75);
    centroids.push_back(centroid0);
    
    // Centroid for cluster 1 (blue)
    std::vector<double> centroid1;
    centroid1.push_back(7.0);
    centroid1.push_back(6.0);
    centroids.push_back(centroid1);
    
    // Centroid for cluster 2 (green)
    std::vector<double> centroid2;
    centroid2.push_back(9.25);
    centroid2.push_back(3.75);
    centroids.push_back(centroid2);
    
    // Use the new fluent API to create and save the cluster visualization
    plotter.chart()
        .title("Cluster Analysis", 36)
        .grid(true)
        .axes(false)
        .cornerRadius(15)
        .logo("logo.png")
        .axisLabels("Feature X", "Feature Y", 32)
        .autoMargins(CHART_CLUSTER)
        .addClusterData(clusterData, clusterLabels, centroids)
        .saveAs("cluster_test_output.png", ".");
    
    printf("Cluster test with fluent API completed. Output saved as 'cluster_test_output.png'.\n");
}

void shmea::testHistogram() {
    printf("Testing enhanced histogram visualization from histogram.fig with supersampling...\n");
    
    // Create a Plotter instance optimized for histogram display
    // Use 4x supersampling for high quality output
    Plotter plotter(1800, 1000, 120, 100, 150, 120, 4);
    
    // Set parameters
    plotter.setShowGrid(true);
    plotter.setShowAxes(false);
    plotter.setCornerRadius(15);
    
    // Load the logo
    plotter.loadLogo("logo.png");
    
    // Add title with increased font size
    plotter.addTitle("Enhanced Histogram Visualization", 48);
    
    // Create a bell curve-like distribution
    std::vector<int> bellCurve;
    bellCurve.push_back(25);   // Start of distribution
    bellCurve.push_back(48);
    bellCurve.push_back(86);
    bellCurve.push_back(142);
    bellCurve.push_back(198);  // Approaching peak
    bellCurve.push_back(245);  // Peak of distribution - will only reach 80% of plot height
    bellCurve.push_back(215);  // Symmetric decline
    bellCurve.push_back(158);
    bellCurve.push_back(102);
    bellCurve.push_back(65);
    bellCurve.push_back(32);   // End of distribution
    
    // Plot bell curve histogram with blue color and standard X-axis labels
    // Note: Bars will be drawn at 80% of their original height, while Y-axis values are scaled to 125%
    // of the maximum bin value to ensure proper alignment between bars and axis labels
    plotter.plotHistogram(bellCurve, RGBA(0x00, 0x9E, 0xFF, 0xFF), true);
    
    // Save the result
    plotter.saveAsPNG("histogram_test_output.png", ".");
    
    printf("Histogram test completed. Output saved as 'histogram_test_output.png'.\n");
    printf("Note: Bars appear at 80%% of their original height, while Y-axis shows values up to 125%% of the maximum bin value.\n");
    
    // Create a second test with skewed distribution
    // Use the new constructor with automatic margin calculation
    printf("Creating second histogram with the new fluent API...\n");
    Plotter plotter2(1800, 1000, 4);
    
    // Create a right-skewed distribution
    std::vector<int> skewedDist;
    skewedDist.push_back(180);  // High values at beginning
    skewedDist.push_back(210);
    skewedDist.push_back(235);  // Peak - will only reach 80% of plot height
    skewedDist.push_back(175);
    skewedDist.push_back(120);
    skewedDist.push_back(85);
    skewedDist.push_back(65);
    skewedDist.push_back(45);
    skewedDist.push_back(30);
    skewedDist.push_back(20);   // Tapering to low values
    
    // Use the new fluent API to create and save the histogram
    plotter2.chart()
        .title("Skewed Distribution Histogram", 48)
        .grid(true)
        .axes(false)
        .cornerRadius(15)
        .logo("logo.png")
        .axisLabels("Value", "Frequency", 32)
        .autoMargins(CHART_HISTOGRAM)
        .addHistogramData(skewedDist, RGBA(0xFF, 0x6B, 0x00, 0xFF), false)
        .saveAs("histogram_skewed_output.png", ".");
    
    printf("Skewed histogram test with fluent API completed. Output saved as 'histogram_skewed_output.png'.\n");
    printf("Note: Bars appear at 80%% of their original height, while Y-axis shows values up to 125%% of the maximum bin value.\n");
}

void shmea::testCandlestickChart() {
    printf("Testing candlestick chart visualization with the new fluent API...\n");
    
    // Create a Plotter instance
    Plotter plotter(1800, 1000, 4);
    
    // Create sample candlestick data with a realistic price pattern
    std::vector<CandleData> candleData;
    
    // Starting price and timestamp
    double basePrice = 142.50;
    double timestamp = 1680000000; // Example timestamp
    
    // Create a series of candles with realistic price movements
    // Day 1
    candleData.push_back(CandleData(timestamp, 142.50, 143.80, 144.20, 142.10));
    timestamp += 86400; // add a day
    
    // Day 2 - uptrend
    candleData.push_back(CandleData(timestamp, 143.90, 145.20, 145.60, 143.70));
    timestamp += 86400;
    
    // Day 3 - continued uptrend
    candleData.push_back(CandleData(timestamp, 145.30, 147.80, 148.10, 144.90));
    timestamp += 86400;
    
    // Day 4 - reversal day (bearish)
    candleData.push_back(CandleData(timestamp, 148.00, 146.30, 149.20, 146.00));
    timestamp += 86400;
    
    // Day 5 - continued downtrend
    candleData.push_back(CandleData(timestamp, 146.20, 144.50, 146.40, 144.30));
    timestamp += 86400;
    
    // Day 6 - stabilizing
    candleData.push_back(CandleData(timestamp, 144.60, 145.10, 145.70, 144.10));
    timestamp += 86400;
    
    // Day 7 - small bullish candle
    candleData.push_back(CandleData(timestamp, 145.20, 146.40, 146.70, 144.90));
    timestamp += 86400;
    
    // Day 8 - gap up
    candleData.push_back(CandleData(timestamp, 147.10, 149.30, 149.80, 146.80));
    timestamp += 86400;
    
    // Day 9 - high volume bullish candle
    candleData.push_back(CandleData(timestamp, 149.40, 153.20, 153.80, 149.00));
    timestamp += 86400;
    
    // Day 10 - profit taking (bearish)
    candleData.push_back(CandleData(timestamp, 153.30, 151.80, 154.00, 151.20));
    timestamp += 86400;
    
    // Day 11 - consolidation (small candle)
    candleData.push_back(CandleData(timestamp, 151.90, 152.20, 152.90, 151.40));
    timestamp += 86400;
    
    // Day 12 - breakdown (large bearish)
    candleData.push_back(CandleData(timestamp, 152.10, 148.70, 152.30, 148.20));
    timestamp += 86400;
    
    // Day 13 - continued selling
    candleData.push_back(CandleData(timestamp, 148.60, 146.90, 149.10, 146.50));
    timestamp += 86400;
    
    // Day 14 - bottoming (hammer candle)
    candleData.push_back(CandleData(timestamp, 146.80, 147.50, 147.70, 144.30));
    timestamp += 86400;
    
    // Day 15 - reversal confirmation
    candleData.push_back(CandleData(timestamp, 147.60, 149.80, 150.20, 147.30));
    
    // Use the new fluent API to create and save the candlestick chart
    plotter.chart()
        .title("Stock Price Candlestick Chart", 48)
        .grid(true)
        .axes(false)
        .cornerRadius(15)
        .logo("logo.png")
        .axisLabels("Date", "Price", 32)
        .autoMargins(CHART_CANDLESTICK)
        .addCandlestickData(candleData, RGBA(0x03, 0xC0, 0x3C, 0xFF), RGBA(0xFF, 0x47, 0x45, 0xFF))
        .saveAs("candlestick_chart_output.png", ".");
    
    printf("Candlestick chart test with fluent API completed. Output saved as 'candlestick_chart_output.png'.\n");
}

// New function to test the line and scatter plots separately
void shmea::testLineScatter() {
    printf("Testing line and scatter plots with the new fluent API...\n");
    
    // Seed random number generator
    std::srand(43);  // Different seed than other tests
    
    // Create a Plotter instance
    Plotter plotter(1800, 800, 4);
    
    // Create a sine wave line
    std::vector<Point> lineData;
    for (int i = 0; i < 50; ++i) {
        Point p;
        p.x = i * 0.2;  // X values from 0 to 10
        p.y = std::sin(i * 0.2) * 3 + 6;  // Sine wave oscillating around y=6
        lineData.push_back(p);
    }
    
    // Create a second dataset for a cosine wave
    std::vector<Point> cosineData;
    for (int i = 0; i < 50; ++i) {
        Point p;
        p.x = i * 0.2;  // X values from 0 to 10
        p.y = std::cos(i * 0.2) * 3 + 12;  // Cosine wave oscillating around y=12
        cosineData.push_back(p);
    }
    
    // Create scatter points around the lines
    std::vector<Point> scatterData;
    for (int i = 0; i < 20; ++i) {
        Point p;
        
        // Create random scatter points between the two waves
        p.x = (std::rand() % 1000) / 100.0;  // X values from 0 to 10
        p.y = 8 + (std::rand() % 400 - 200) / 100.0;  // Y values around 8 ±2
        
        scatterData.push_back(p);
    }
    
    // Use the new fluent API to create and save the chart
    plotter.chart()
        .title("Line & Scatter Plot Visualization", 36)
        .grid(true)
        .axes(true)
        .cornerRadius(15)
        .logo("logo.png")
        .axisLabels("X Value", "Y Value", 28)
        .autoMargins(CHART_LINE)
        .addSeries("Sine Wave", lineData, RGBA(0x00, 0x9E, 0xFF, 0xFF), SERIES_LINE, 3)
        .addSeries("Cosine Wave", cosineData, RGBA(0xFF, 0x6B, 0x00, 0xFF), SERIES_LINE, 3)
        .addSeries("Random Points", scatterData, RGBA(0x33, 0xFF, 0x33, 0xFF), SERIES_SCATTER, 2, 10)
        .saveAs("line_scatter_test_output.png", ".");
    
    printf("Line and scatter plot test with fluent API completed. Output saved as 'line_scatter_test_output.png'.\n");
}

// Test function to visualize 10 cluster circles
void shmea::testMultiCluster() {
    printf("Testing multi-cluster visualization with the new fluent API...\n");
    
    // Seed random number generator with a fixed value for consistent output
    std::srand(123);
    
    // Create a Plotter instance
    Plotter plotter(2400, 1400, 4);
    
    // Initialize the 10-cluster color scheme (uses colors from css-output.css)
    plotter.use10ClusterColorScheme();
    
    // Create data structure for 10 clusters
    std::vector<std::vector<double> > clusterData;
    std::vector<int> clusterLabels;
    std::vector<std::vector<double> > centroids;
    
    // Define cluster centers in a grid-like pattern (5x2 grid)
    // This creates a visually appealing arrangement of clusters
    std::vector<std::pair<double, double> > clusterCenters;
    // Initialize cluster centers manually (C++98 compatible)
    clusterCenters.push_back(std::make_pair(2.0, 2.0));   // Cluster 0: top-left
    clusterCenters.push_back(std::make_pair(6.0, 2.0));   // Cluster 1: top
    clusterCenters.push_back(std::make_pair(10.0, 2.0));  // Cluster 2: top
    clusterCenters.push_back(std::make_pair(14.0, 2.0));  // Cluster 3: top
    clusterCenters.push_back(std::make_pair(18.0, 2.0));  // Cluster 4: top-right
    clusterCenters.push_back(std::make_pair(2.0, 6.0));   // Cluster 5: bottom-left
    clusterCenters.push_back(std::make_pair(6.0, 6.0));   // Cluster 6: bottom
    clusterCenters.push_back(std::make_pair(10.0, 6.0));  // Cluster 7: bottom
    clusterCenters.push_back(std::make_pair(14.0, 6.0));  // Cluster 8: bottom
    clusterCenters.push_back(std::make_pair(18.0, 6.0));  // Cluster 9: bottom-right
    
    // Number of points per cluster
    int pointsPerCluster = 30;
    
    // Generate data for each cluster
    for (int clusterId = 0; clusterId < 10; clusterId++) {
        // Get center coordinates for this cluster
        double centerX = clusterCenters[clusterId].first;
        double centerY = clusterCenters[clusterId].second;
        
        // Create centroid vector
        std::vector<double> centroid;
        centroid.push_back(centerX);
        centroid.push_back(centerY);
        centroids.push_back(centroid);
        
        // Create points for this cluster with random variation around the center
        for (int i = 0; i < pointsPerCluster; i++) {
            std::vector<double> point;
            
            // Create variation radius (different for each cluster to create visual diversity)
            double spreadFactor = 0.5 + (clusterId % 3) * 0.25; // Values: 0.5, 0.75, 1.0
            
            // Generate point with random variation from center
            double xVar = ((std::rand() % 200) - 100) / 100.0 * spreadFactor;
            double yVar = ((std::rand() % 200) - 100) / 100.0 * spreadFactor;
            
            point.push_back(centerX + xVar);  // x coordinate
            point.push_back(centerY + yVar);  // y coordinate
            
            // Add point to dataset with appropriate label
            clusterData.push_back(point);
            clusterLabels.push_back(clusterId);
        }
    }
    
    // Use the new fluent API to create and save the multi-cluster visualization
    plotter.chart()
        .title("Multi-Cluster Analysis - 10 Clusters", 42)
        .grid(true)
        .axes(false)
        .cornerRadius(15)
        .logo("logo.png")
        .autoMargins(CHART_CLUSTER)
        .addClusterData(clusterData, clusterLabels, centroids)
        .saveAs("multi_cluster_test_output.png", ".");
    
    printf("Multi-cluster test with fluent API completed. Output saved as 'multi_cluster_test_output.png'.\n");
}

// Test function to visualize arrows on charts
void shmea::testArrows() {
    printf("Testing arrow visualization with the new fluent API...\n");
    
    // Test 1: Arrows on a scatter plot
    // ===============================
    
    printf("Test 1: Adding arrows to a scatter plot...\n");
    
    // Seed random number generator
    std::srand(42);
    
    // Create a Plotter instance
    Plotter plotter(1800, 1000, 4);
    
    // Create random scatter points
    std::vector<Point> scatterPoints;
    for (int i = 0; i < 30; ++i) {
        Point p;
        p.x = (std::rand() % 1000) / 100.0; // X values from 0 to 10
        p.y = (std::rand() % 1000) / 100.0; // Y values from 0 to 10
        scatterPoints.push_back(p);
    }
    
    // Create arrows that highlight specific points or trends
    std::vector<Arrow> arrows;
    
    // Arrow 1: Pointing to an interesting point (in red)
    arrows.push_back(Arrow(5.0, 9.0, 3.0, 5.0, RGBA(0xFF, 0x47, 0x45, 0xFF), 3, 12));
    
    // Arrow 2: Showing direction of trend (in green)
    arrows.push_back(Arrow(2.0, 2.0, 8.0, 8.0, RGBA(0x03, 0xC0, 0x3C, 0xFF), 3, 15));
    
    // Arrow 3: Horizontal arrow (in blue)
    arrows.push_back(Arrow(1.0, 7.0, 9.0, 7.0, RGBA(0x00, 0x9E, 0xFF, 0xFF), 3, 12));
    
    // Use the new fluent API to create a scatter plot with arrows
    plotter.chart()
        .title("Scatter Plot with Arrows", 36)
        .grid(true)
        .axes(true)
        .cornerRadius(15)
        .logo("logo.png")
        .axisLabels("X Value", "Y Value", 28)
        .autoMargins(CHART_SCATTER)
        .addSeries("Data Points", scatterPoints, RGBA(0x99, 0x99, 0x99, 0xFF), SERIES_SCATTER, 2, 8)
        .addArrows(arrows)
        .saveAs("scatter_with_arrows_test.png", ".");
    
    printf("Scatter plot with arrows test completed. Output saved as 'scatter_with_arrows_test.png'.\n");
    
    // Test 2: Arrows on a line chart
    // ==============================
    
    printf("Test 2: Adding arrows to a line chart...\n");
    
    // Create a new Plotter instance
    Plotter plotter2(1800, 1000, 4);
    
    // Create a sine wave line
    std::vector<Point> lineData;
    for (int i = 0; i < 50; ++i) {
        Point p;
        p.x = i * 0.2;  // X values from 0 to 10
        p.y = std::sin(i * 0.2) * 3 + 5;  // Sine wave oscillating around y=5
        lineData.push_back(p);
    }
    
    // Create arrows that highlight features of the sine wave
    std::vector<Arrow> waveArrows;
    
    // Arrow 1: Pointing to a maximum (in red)
    waveArrows.push_back(Arrow(8.0, 8.5, 7.9, 8.0, RGBA(0xFF, 0x47, 0x45, 0xFF), 2, 10));
    
    // Arrow 2: Pointing to a minimum (in blue)
    waveArrows.push_back(Arrow(3.0, 1.5, 3.8, 2.0, RGBA(0x00, 0x9E, 0xFF, 0xFF), 2, 10));
    
    // Arrow 3: Showing overall wave direction (in green)
    waveArrows.push_back(Arrow(1.0, 3.0, 3.0, 5.0, RGBA(0x03, 0xC0, 0x3C, 0xFF), 2, 12));
    
    // Arrow 4: Annotation arrow (in purple)
    waveArrows.push_back(Arrow(9.0, 3.0, 6.0, 2.5, RGBA(0xA0, 0x20, 0xF0, 0xFF), 2, 12));

    // Arrow 5: Annotation arrow (in yellow)
    waveArrows.push_back(Arrow(0.0, 0.0, 2.0, 2.0, RGBA(0xFF, 0xD7, 0x00, 0xFF), 2, 12));
    
    // Use the new fluent API to create a line chart with arrows
    plotter2.chart()
        .title("Sine Wave with Annotation Arrows", 36)
        .grid(true)
        .axes(true)
        .cornerRadius(15)
        .logo("logo.png")
        .axisLabels("X Value", "Y Value", 28)
        .autoMargins(CHART_LINE)
        .addSeries("Sine Wave", lineData, RGBA(0x00, 0x00, 0x00, 0xFF), SERIES_LINE, 3)
        .addArrows(waveArrows)
        .saveAs("line_chart_with_arrows_test.png", ".");
    
    printf("Line chart with arrows test completed. Output saved as 'line_chart_with_arrows_test.png'.\n");
    
    // Test 3: Standalone arrows (no other chart elements)
    // =================================================
    
    printf("Test 3: Creating a chart with just arrows...\n");
    
    // Create a new Plotter instance
    Plotter plotter3(1200, 800, 2);
    
    // Create a set of arrows forming a simple diagram
    std::vector<Arrow> diagramArrows;
    
    // Arrow 1: Central arrow (in black)
    diagramArrows.push_back(Arrow(2.0, 5.0, 8.0, 5.0, RGBA(0x00, 0x00, 0x00, 0xFF), 3, 15));
    
    // Arrow 2: Top branch (in red)
    diagramArrows.push_back(Arrow(5.0, 5.0, 7.0, 8.0, RGBA(0xFF, 0x47, 0x45, 0xFF), 3, 12));
    
    // Arrow 3: Bottom branch (in blue)
    diagramArrows.push_back(Arrow(5.0, 5.0, 7.0, 2.0, RGBA(0x00, 0x9E, 0xFF, 0xFF), 3, 12));
    
    // Use the new fluent API to create a chart with only arrows
    plotter3.chart()
        .title("Arrow Diagram", 36)
        .grid(false)
        .axes(false)
        .cornerRadius(15)
        .axisLabels("", "")
        .autoMargins(CHART_DEFAULT)
        .addArrows(diagramArrows)
        .saveAs("arrow_diagram_test.png", ".");
    
    printf("Arrow diagram test completed. Output saved as 'arrow_diagram_test.png'.\n");
}

// Test function for the origin axes feature
void shmea::testOriginAxes() {
    std::cout << "=== Testing Origin Axes (Four Quadrants) ===" << std::endl;
    
    // Create data points in all four quadrants
    std::vector<Point> points;
    
    // Add points in all four quadrants (spiral pattern)
    for (double t = 0; t < 10.0; t += 0.1) {
        double r = t * 0.5;
        double x = r * cos(t);
        double y = r * sin(t);
        points.push_back(Point(x, y));
    }
    
    // Create a plotter with appropriate dimensions
    Plotter plotter(800, 600);
    
    // Build and save the chart with origin axes enabled
    plotter.chart()
        .title("Four Quadrant Chart Demo")
        .axisLabels("X-Axis", "Y-Axis")
        .originAxes(true)  // Enable four quadrant origin axes
        .grid(true)
        .addSeries("Spiral", points, RGBA(0xFF, 0x47, 0x45, 0xFF), SERIES_LINE, 2, 6)
        .saveAs("origin_axes_test.png", "test-output");
    
    std::cout << "Origin axes test completed. Output image: test-output/origin_axes_test.png" << std::endl;
    
    // Test 2: Create asymmetric data that spans across quadrants
    std::vector<Point> asymmetricPoints;
    
    // Create points primarily in quadrants I and IV with some points in quadrants II and III
    asymmetricPoints.push_back(Point(-2, 5));  // Quadrant II
    asymmetricPoints.push_back(Point(-1, 3));  // Quadrant II
    asymmetricPoints.push_back(Point(0, 0));   // Origin
    asymmetricPoints.push_back(Point(1, 1));   // Quadrant I
    asymmetricPoints.push_back(Point(3, 2));   // Quadrant I
    asymmetricPoints.push_back(Point(5, 8));   // Quadrant I
    asymmetricPoints.push_back(Point(8, 12));  // Quadrant I - far point
    asymmetricPoints.push_back(Point(7, -3));  // Quadrant IV
    asymmetricPoints.push_back(Point(4, -5));  // Quadrant IV
    asymmetricPoints.push_back(Point(-2, -4)); // Quadrant III
    
    // Create a second plotter
    Plotter plotter2(800, 600);
    
    // Build and save the chart - the origin axes should automatically adjust to the data range
    plotter2.chart()
        .title("Auto-Aligned Origin Axes")
        .axisLabels("X-Axis", "Y-Axis")
        .originAxes(true)  // Enable four quadrant origin axes
        .grid(true)
        .addSeries("Asymmetric Data", asymmetricPoints, RGBA(0x00, 0x9E, 0xFF, 0xFF), SERIES_SCATTER, 2, 8)
        .saveAs("origin_axes_auto_aligned.png", "test-output");
    
    std::cout << "Auto-aligned origin axes test completed. Output image: test-output/origin_axes_auto_aligned.png" << std::endl;
    
    // Test 3: Compare auto-alignment with manual range specification
    Plotter plotter3(800, 600);
    
    // Create a new set of points with specific range characteristics
    std::vector<Point> offsetPoints;
    
    // Create points that are mostly in the positive X and Y region, but with some negative values
    for (int i = 0; i < 20; i++) {
        double x = i * 0.5 - 2.0;  // Values from -2 to 8
        double y = sin(x) * 3;     // Values from -3 to 3
        offsetPoints.push_back(Point(x, y));
    }
    
    // Build and save the chart with origin axes enabled and let the system auto-align
    plotter3.chart()
        .title("Auto vs Manual Range Comparison")
        .axisLabels("X-Axis", "Y-Axis")
        .originAxes(true)  // Enable four quadrant origin axes
        .grid(true)
        .addSeries("Offset Sine Wave", offsetPoints, RGBA(0x03, 0xC0, 0x3C, 0xFF), SERIES_LINE, 2, 6)
        .saveAs("origin_axes_comparison.png", "test-output");
    
    // Save a version with manually specified data ranges
    plotter3.drawOriginAxes(-3.0, 9.0, -4.0, 4.0);
    plotter3.saveAsPNG("origin_axes_manual_range.png", "test-output");
    
    std::cout << "Comparison test completed. Output images:" << std::endl
              << "  - Auto-aligned: test-output/origin_axes_comparison.png" << std::endl
              << "  - Manual range: test-output/origin_axes_manual_range.png" << std::endl;
} 
