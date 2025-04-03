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
    
    // Seed random number generator
    std::srand(std::time(NULL));
    
    // Create a Cluster10 instance with larger dimensions for better visualization
    Cluster10 plotter(1600, 900, 80, 80, 80, 80);
    
    // Set parameters
    plotter.setShowGrid(true);
    plotter.setShowAxes(true);
    plotter.setCornerRadius(15);
    
    // Add title and axis labels
    plotter.addTitle("Cluster10 Demo - C++03 Compatible", 42);
    
    // Create data sets for visualization
    
    // 1. Create a sine wave line
    std::vector<Cluster10::Point> lineData;
    for (int i = 0; i < 40; ++i) {
        Cluster10::Point p;
        p.x = i * 0.25;
        // Sine wave with increasing amplitude
        p.y = std::sin(i * 0.2) * (5 + i*0.1) + 10;
        lineData.push_back(p);
    }
    
    // Plot line with blue color
    plotter.plotLine(lineData, RGBA(0x00, 0x9E, 0xFF, 0xFF), 3);
    
    // 2. Create random scatter points that follow the line
    std::vector<Cluster10::Point> scatterData;
    for (int i = 0; i < 25; ++i) {
        Cluster10::Point p;
        int idx = std::rand() % lineData.size();
        // Add some random variation around the line points
        p.x = lineData[idx].x + (std::rand() % 200 - 100) / 100.0;
        p.y = lineData[idx].y + (std::rand() % 200 - 100) / 100.0;
        scatterData.push_back(p);
    }
    
    // Plot scatter points with orange color
    plotter.plotPoints(scatterData, RGBA(0xFF, 0x6B, 0x00, 0xFF), 8);
    
    // 3. Create histogram with more interesting data
    std::vector<int> histogramData;
    // Create a pattern with one very tall bar and decreasing heights
    histogramData.push_back(60);
    histogramData.push_back(120);
    histogramData.push_back(90);
    histogramData.push_back(145);
    histogramData.push_back(70);
    histogramData.push_back(110);
    histogramData.push_back(55);
    histogramData.push_back(80);
    histogramData.push_back(125);
    histogramData.push_back(100);
    
    // Plot histogram with green color
    plotter.plotHistogram(histogramData, RGBA(0x03, 0xC0, 0x3C, 0xFF));
    
    // 4. Create more distinct cluster data
    std::vector<std::vector<double> > clusterData;
    std::vector<int> clusterLabels;
    
    // Create 3 well-separated clusters
    
    // Cluster 0 - bottom left
    for (int i = 0; i < 40; ++i) {
        std::vector<double> point;
        point.push_back(std::rand() % 200 / 100.0 + 2.0); // x range: 2-4
        point.push_back(std::rand() % 200 / 100.0 + 2.0); // y range: 2-4
        clusterData.push_back(point);
        clusterLabels.push_back(0);
    }
    
    // Cluster 1 - top middle
    for (int i = 0; i < 40; ++i) {
        std::vector<double> point;
        point.push_back(std::rand() % 200 / 100.0 + 5.0); // x range: 5-7
        point.push_back(std::rand() % 200 / 100.0 + 7.0); // y range: 7-9
        clusterData.push_back(point);
        clusterLabels.push_back(1);
    }
    
    // Cluster 2 - right middle
    for (int i = 0; i < 40; ++i) {
        std::vector<double> point;
        point.push_back(std::rand() % 200 / 100.0 + 8.0); // x range: 8-10
        point.push_back(std::rand() % 200 / 100.0 + 4.0); // y range: 4-6
        clusterData.push_back(point);
        clusterLabels.push_back(2);
    }
    
    // Create distinct centroids
    std::vector<std::vector<double> > centroids;
    
    // Centroid for cluster 0
    std::vector<double> centroid1;
    centroid1.push_back(3.0);
    centroid1.push_back(3.0);
    centroids.push_back(centroid1);
    
    // Centroid for cluster 1
    std::vector<double> centroid2;
    centroid2.push_back(6.0);
    centroid2.push_back(8.0);
    centroids.push_back(centroid2);
    
    // Centroid for cluster 2
    std::vector<double> centroid3;
    centroid3.push_back(9.0);
    centroid3.push_back(5.0);
    centroids.push_back(centroid3);
    
    // Plot clusters
    plotter.plotClusters(clusterData, clusterLabels, centroids);
    
    // Save the result
    plotter.saveAsPNG("cluster10_test_output.png", ".");
    
    printf("Cluster10 test completed. Output saved as 'cluster10_test_output.png'.\n");
} 