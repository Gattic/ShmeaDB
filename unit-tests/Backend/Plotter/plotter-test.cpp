#include "plotter-test.h"
#include "../../../Backend/Plotter/Plotter.h"
#include "../../../Backend/Plotter/GridRenderer.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <cmath>

using namespace shmea;

void shmea::testCluster() {
    printf("Testing Cluster visualization with supersampling and auto margins...\n");
    
    // Seed random number generator with a fixed value for consistent output
    std::srand(42);
    
    // Create a Plotter instance specifically for cluster visualization
    // Use 4x supersampling for high quality output
    // Use the new constructor with automatic margin calculation
    Plotter plotter(1800, 1000, 4);
    
    // Set parameters
    plotter.setShowGrid(true);
    plotter.setShowAxes(false);
    plotter.setCornerRadius(15);
    
    // Load the logo
    plotter.loadLogo("logo.png");
    
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
    
    // Plot clusters
    plotter.plotClusters(clusterData, clusterLabels, centroids);
    
    // Save the result
    plotter.saveAsPNG("cluster_test_output.png", ".");
    
    printf("Cluster test with auto-calculated margins completed. Output saved as 'cluster_test_output.png'.\n");
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
    printf("Creating second histogram with automatic margin calculation...\n");
    Plotter plotter2(1800, 1000, 4);
    
    // Set parameters
    plotter2.setShowGrid(true);
    plotter2.setShowAxes(false);
    plotter2.setCornerRadius(15);
    
    // Load the logo
    plotter2.loadLogo("logo.png");
    
    // Add title with increased font size
    plotter2.addTitle("Skewed Distribution Histogram", 48);
    
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
    
    // Plot skewed histogram with orange color and NO X-axis labels
    // Note: Bars will be drawn at 80% of their original height, while Y-axis values are scaled to 125%
    // of the maximum bin value to ensure proper alignment between bars and axis labels
    plotter2.plotHistogram(skewedDist, RGBA(0xFF, 0x6B, 0x00, 0xFF), false);
    
    // Save the result
    plotter2.saveAsPNG("histogram_skewed_output.png", ".");
    
    printf("Skewed histogram test with auto-calculated margins completed. Output saved as 'histogram_skewed_output.png'.\n");
    printf("Note: Bars appear at 80%% of their original height, while Y-axis shows values up to 125%% of the maximum bin value.\n");
}

void shmea::testCandlestickChart() {
    printf("Testing candlestick chart visualization from candechart.fig with supersampling and auto margins...\n");
    
    // Create a Plotter instance optimized for candlestick chart display
    // Use 4x supersampling for high quality output
    // Use the new constructor with automatic margin calculation
    Plotter plotter(1800, 1000, 4);
    
    // Set parameters
    plotter.setShowGrid(true);
    plotter.setShowAxes(false);
    plotter.setCornerRadius(15);
    
    // Load the logo
    plotter.loadLogo("logo.png");
    
    // Add title with increased font size
    plotter.addTitle("Stock Price Candlestick Chart", 48);
    
    // Create sample candlestick data with a realistic price pattern
    std::vector<Plotter::CandleData> candleData;
    
    // Starting price and timestamp
    double basePrice = 142.50;
    double timestamp = 1680000000; // Example timestamp
    
    // Create a series of candles with realistic price movements
    // Day 1
    candleData.push_back(Plotter::CandleData(timestamp, 142.50, 143.80, 144.20, 142.10));
    timestamp += 86400; // add a day
    
    // Day 2 - uptrend
    candleData.push_back(Plotter::CandleData(timestamp, 143.90, 145.20, 145.60, 143.70));
    timestamp += 86400;
    
    // Day 3 - continued uptrend
    candleData.push_back(Plotter::CandleData(timestamp, 145.30, 147.80, 148.10, 144.90));
    timestamp += 86400;
    
    // Day 4 - reversal day (bearish)
    candleData.push_back(Plotter::CandleData(timestamp, 148.00, 146.30, 149.20, 146.00));
    timestamp += 86400;
    
    // Day 5 - continued downtrend
    candleData.push_back(Plotter::CandleData(timestamp, 146.20, 144.50, 146.40, 144.30));
    timestamp += 86400;
    
    // Day 6 - stabilizing
    candleData.push_back(Plotter::CandleData(timestamp, 144.60, 145.10, 145.70, 144.10));
    timestamp += 86400;
    
    // Day 7 - small bullish candle
    candleData.push_back(Plotter::CandleData(timestamp, 145.20, 146.40, 146.70, 144.90));
    timestamp += 86400;
    
    // Day 8 - gap up
    candleData.push_back(Plotter::CandleData(timestamp, 147.10, 149.30, 149.80, 146.80));
    timestamp += 86400;
    
    // Day 9 - high volume bullish candle
    candleData.push_back(Plotter::CandleData(timestamp, 149.40, 153.20, 153.80, 149.00));
    timestamp += 86400;
    
    // Day 10 - profit taking (bearish)
    candleData.push_back(Plotter::CandleData(timestamp, 153.30, 151.80, 154.00, 151.20));
    timestamp += 86400;
    
    // Day 11 - consolidation (small candle)
    candleData.push_back(Plotter::CandleData(timestamp, 151.90, 152.20, 152.90, 151.40));
    timestamp += 86400;
    
    // Day 12 - breakdown (large bearish)
    candleData.push_back(Plotter::CandleData(timestamp, 152.10, 148.70, 152.30, 148.20));
    timestamp += 86400;
    
    // Day 13 - continued selling
    candleData.push_back(Plotter::CandleData(timestamp, 148.60, 146.90, 149.10, 146.50));
    timestamp += 86400;
    
    // Day 14 - bottoming (hammer candle)
    candleData.push_back(Plotter::CandleData(timestamp, 146.80, 147.50, 147.70, 144.30));
    timestamp += 86400;
    
    // Day 15 - reversal confirmation
    candleData.push_back(Plotter::CandleData(timestamp, 147.60, 149.80, 150.20, 147.30));
    
    // Plot the candlestick chart
    // Use modern green for bullish and red for bearish
    plotter.plotCandlestickChart(candleData, RGBA(0x03, 0xC0, 0x3C, 0xFF), RGBA(0xFF, 0x47, 0x45, 0xFF));
    
    // Save the result
    plotter.saveAsPNG("candlestick_chart_output.png", ".");
    
    printf("Candlestick chart test with auto-calculated margins completed. Output saved as 'candlestick_chart_output.png'.\n");
}

// New function to test the line and scatter plots separately
void shmea::testLineScatter() {
    printf("Testing line and scatter plots with supersampling and auto margins...\n");
    
    // Seed random number generator
    std::srand(43);  // Different seed than other tests
    
    // Create a Plotter instance for line and scatter plots with better proportions
    // Use 4x supersampling for high quality output
    Plotter plotter(1800, 800, 4);
    
    // Set parameters
    plotter.setShowGrid(true);
    plotter.setShowAxes(true);
    plotter.setCornerRadius(15);
    
    // Load the logo
    plotter.loadLogo("logo.png");
    
    // Create a sine wave line
    std::vector<Plotter::Point> lineData;
    for (int i = 0; i < 50; ++i) {
        Plotter::Point p;
        p.x = i * 0.2;  // X values from 0 to 10
        p.y = std::sin(i * 0.2) * 3 + 6;  // Sine wave oscillating around y=6
        lineData.push_back(p);
    }
    
    // Create a second dataset for a cosine wave
    std::vector<Plotter::Point> cosineData;
    for (int i = 0; i < 50; ++i) {
        Plotter::Point p;
        p.x = i * 0.2;  // X values from 0 to 10
        p.y = std::cos(i * 0.2) * 3 + 12;  // Cosine wave oscillating around y=12
        cosineData.push_back(p);
    }
    
    // Create scatter points around the lines
    std::vector<Plotter::Point> scatterData;
    for (int i = 0; i < 20; ++i) {
        Plotter::Point p;
        
        // Create random scatter points between the two waves
        p.x = (std::rand() % 1000) / 100.0;  // X values from 0 to 10
        p.y = 8 + (std::rand() % 400 - 200) / 100.0;  // Y values around 8 ±2
        
        scatterData.push_back(p);
    }
    
    // Set up data series
    std::vector<std::vector<Plotter::Point> > seriesData;
    seriesData.push_back(lineData);
    seriesData.push_back(cosineData);
    seriesData.push_back(scatterData);
    
    // Create legend labels
    std::vector<std::string> legendLabels;
    legendLabels.push_back("Sine Wave");
    legendLabels.push_back("Cosine Wave");
    legendLabels.push_back("Random Points");
    
    // Series colors
    std::vector<RGBA> seriesColors;
    seriesColors.push_back(RGBA(0x00, 0x9E, 0xFF, 0xFF)); // Blue
    seriesColors.push_back(RGBA(0xFF, 0x6B, 0x00, 0xFF)); // Orange
    seriesColors.push_back(RGBA(0x33, 0xFF, 0x33, 0xFF)); // Green
    
    // Define which series should be drawn as lines
    std::vector<bool> isLineStyleSeries;
    isLineStyleSeries.push_back(true);  // Sine = line
    isLineStyleSeries.push_back(true);  // Cosine = line
    isLineStyleSeries.push_back(false); // Scatter = points
    
    // Plot all series with a single call - no manual setup needed
    plotter.plotMultiSeries(
        seriesData,
        legendLabels,
        seriesColors,
        isLineStyleSeries,
        "Line & Scatter Plot Visualization",
        "X Value", 
        "Y Value"
    );
    
    // Save the result
    plotter.saveAsPNG("line_scatter_test_output.png", ".");
    
    printf("Line and scatter plot test with auto-calculated margins completed. Output saved as 'line_scatter_test_output.png'.\n");
}

// Test function to visualize 10 cluster circles
void shmea::testMultiCluster() {
    printf("Testing multi-cluster visualization with 10 clusters and automatic margins...\n");
    
    // Seed random number generator with a fixed value for consistent output
    std::srand(123);
    
    // Create a Plotter instance with larger dimensions to handle many clusters
    // Use 4x supersampling for high quality output
    // Use the new constructor with auto-margin calculation
    Plotter plotter(2400, 1400, 4);
    
    // Set parameters
    plotter.setShowGrid(true);
    plotter.setShowAxes(false);
    plotter.setCornerRadius(15);
    
    // Initialize the 10-cluster color scheme (uses colors from css-output.css)
    plotter.use10ClusterColorScheme();
    
    // Load the logo
    plotter.loadLogo("logo.png");
    
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
    
    // Add title with increased font size
    plotter.addTitle("Multi-Cluster Analysis - 10 Clusters", 42);
    
    // Plot the 10 clusters
    plotter.plotClusters(clusterData, clusterLabels, centroids);
    
    // Save the result
    plotter.saveAsPNG("multi_cluster_test_output.png", ".");
    
    printf("Multi-cluster test completed with auto-calculated margins. Output saved as 'multi_cluster_test_output.png'.\n");
} 
