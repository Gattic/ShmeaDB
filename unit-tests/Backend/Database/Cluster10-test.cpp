#include "Cluster10-test.h"
#include "../../../Backend/Database/Cluster10.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <cmath>

using namespace shmea;

void shmea::testCluster10() {
    printf("Testing Cluster10 visualization...\n");
    
    // Seed random number generator with a fixed value for consistent output
    std::srand(42);
    
    // Create a Cluster10 instance with dimensions matching the image
    Cluster10 plotter(1800, 1000, 80, 80, 80, 80);
    
    // Set parameters
    plotter.setShowGrid(true);
    plotter.setShowAxes(true);
    plotter.setCornerRadius(15);
    
    // Add title and axis labels
    plotter.addTitle("Cluster10 Demo - C++03 Compatible", 42);
    
    // Create a sine wave line with specific parameters to match the image
    std::vector<Cluster10::Point> lineData;
    for (int i = 0; i < 30; ++i) {
        Cluster10::Point p;
        p.x = i * 0.3;
        p.y = std::sin(i * 0.3) * 4 + 13; // Position the sine wave in the upper section
        lineData.push_back(p);
    }
    
    // Plot line with blue color
    plotter.plotLine(lineData, RGBA(0x00, 0x9E, 0xFF, 0xFF), 3);
    
    // Create scatter points along the line with blue color
    std::vector<Cluster10::Point> scatterData;
    for (int i = 0; i < 20; ++i) {
        Cluster10::Point p;
        int idx = (std::rand() % lineData.size());
        // Add slight variation
        p.x = lineData[idx].x + (std::rand() % 100 - 50) / 100.0;
        p.y = lineData[idx].y + (std::rand() % 100 - 50) / 100.0;
        scatterData.push_back(p);
    }
    
    // Plot scatter points with blue color to match the sine wave
    plotter.plotPoints(scatterData, RGBA(0x00, 0x9E, 0xFF, 0xFF), 8);
    
    // Create histogram data that matches the pattern in the image
    // Tall bars alternating with shorter bars
    std::vector<int> histogramData;
    histogramData.push_back(42);   // Lower frequency
    histogramData.push_back(78);   // Medium frequency
    histogramData.push_back(128);  // Higher - approaching peak
    histogramData.push_back(165);  // High point
    histogramData.push_back(187);  // Peak 
    histogramData.push_back(145);  // Declining from peak
    histogramData.push_back(110);  // Continuing decline
    histogramData.push_back(75);   // Approaching baseline
    histogramData.push_back(53);   // Return to baseline
    histogramData.push_back(37);   // Final low value
    
    // Plot histogram with green color
    plotter.plotHistogram(histogramData, RGBA(0x03, 0xC0, 0x3C, 0xFF));
    
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
    plotter.saveAsPNG("cluster10_test_output.png", ".");
    
    printf("Cluster10 test completed. Output saved as 'cluster10_test_output.png'.\n");
}

void shmea::testHistogram() {
    printf("Testing enhanced histogram visualization from histogram.fig...\n");
    
    // Create a Cluster10 instance optimized for histogram display
    Cluster10 plotter(1800, 1000, 120, 100, 150, 120);
    
    // Set parameters
    plotter.setShowGrid(true);
    plotter.setShowAxes(true);
    plotter.setCornerRadius(15);
    
    // Add title
    plotter.addTitle("Enhanced Histogram Visualization", 42);
    
    // Create a bell curve-like distribution
    std::vector<int> bellCurve;
    bellCurve.push_back(25);   // Start of distribution
    bellCurve.push_back(48);
    bellCurve.push_back(86);
    bellCurve.push_back(142);
    bellCurve.push_back(198);  // Approaching peak
    bellCurve.push_back(245);  // Peak of distribution
    bellCurve.push_back(215);  // Symmetric decline
    bellCurve.push_back(158);
    bellCurve.push_back(102);
    bellCurve.push_back(65);
    bellCurve.push_back(32);   // End of distribution
    
    // Plot bell curve histogram with blue color
    plotter.plotHistogram(bellCurve, RGBA(0x00, 0x9E, 0xFF, 0xFF));
    
    // Save the result
    plotter.saveAsPNG("histogram_test_output.png", ".");
    
    printf("Histogram test completed. Output saved as 'histogram_test_output.png'.\n");
    
    // Create a second test with skewed distribution
    Cluster10 plotter2(1800, 1000, 120, 100, 150, 120);
    
    // Set parameters
    plotter2.setShowGrid(true);
    plotter2.setShowAxes(true);
    plotter2.setCornerRadius(15);
    
    // Add title
    plotter2.addTitle("Skewed Distribution Histogram", 42);
    
    // Create a right-skewed distribution
    std::vector<int> skewedDist;
    skewedDist.push_back(180);  // High values at beginning
    skewedDist.push_back(210);
    skewedDist.push_back(235);  // Peak
    skewedDist.push_back(175);
    skewedDist.push_back(120);
    skewedDist.push_back(85);
    skewedDist.push_back(65);
    skewedDist.push_back(45);
    skewedDist.push_back(30);
    skewedDist.push_back(20);   // Tapering to low values
    
    // Plot skewed histogram with a different color
    plotter2.plotHistogram(skewedDist, RGBA(0xFF, 0x6B, 0x00, 0xFF));  // Orange color
    
    // Save the result
    plotter2.saveAsPNG("histogram_skewed_output.png", ".");
    
    printf("Skewed histogram test completed. Output saved as 'histogram_skewed_output.png'.\n");
} 