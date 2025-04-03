#include "Cluster10-test.h"
#include "../../../Backend/Database/Cluster10.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cstdio>

using namespace shmea;

void shmea::testCluster10() {
    printf("Testing Cluster10 visualization...\n");
    
    // Seed random number generator
    std::srand(std::time(NULL));
    
    // Create a Cluster10 instance
    Cluster10 plotter(1200, 800, 80, 80, 80, 80);
    
    // Set parameters
    plotter.setShowGrid(true);
    plotter.setShowAxes(true);
    plotter.setCornerRadius(15);
    
    // Add title and axis labels
    plotter.addTitle("Cluster10 Demo - C++03 Compatible", 36);
    plotter.addAxisLabels("X Axis", "Y Axis", 24);
    
    // Create test data for line plot
    std::vector<Cluster10::Point> lineData;
    for (int i = 0; i < 50; ++i) {
        Cluster10::Point p;
        p.x = i * 0.2;
        p.y = std::sin(i * 0.2) * 5 + 10;
        lineData.push_back(p);
    }
    
    // Plot line with blue color
    plotter.plotLine(lineData, RGBA(0x00, 0x9E, 0xFF, 0xFF), 3);
    
    // Create test data for scatter plot
    std::vector<Cluster10::Point> scatterData;
    for (int i = 0; i < 40; ++i) {
        Cluster10::Point p;
        p.x = (std::rand() % 1000) / 100.0;
        p.y = (std::rand() % 1000) / 100.0 + 15;
        scatterData.push_back(p);
    }
    
    // Plot scatter points with orange color
    plotter.plotPoints(scatterData, RGBA(0xFF, 0x6B, 0x00, 0xFF), 8);
    
    // Create test data for histogram
    std::vector<int> histogramData;
    for (int i = 0; i < 10; ++i) {
        histogramData.push_back(std::rand() % 100 + 50);
    }
    
    // Plot histogram with green color
    plotter.plotHistogram(histogramData, RGBA(0x03, 0xC0, 0x3C, 0xFF));
    
    // Create test data for clusters
    std::vector<std::vector<double> > clusterData;
    std::vector<int> clusterLabels;
    
    // Create 3 clusters
    for (int i = 0; i < 150; ++i) {
        std::vector<double> point;
        int cluster = i / 50;  // 0, 1, or 2
        
        // Generate point based on cluster
        if (cluster == 0) {
            point.push_back(std::rand() % 200 / 100.0 + 2.0);
            point.push_back(std::rand() % 200 / 100.0 + 2.0);
        } else if (cluster == 1) {
            point.push_back(std::rand() % 200 / 100.0 + 5.0);
            point.push_back(std::rand() % 200 / 100.0 + 7.0);
        } else {
            point.push_back(std::rand() % 200 / 100.0 + 8.0);
            point.push_back(std::rand() % 200 / 100.0 + 3.0);
        }
        
        clusterData.push_back(point);
        clusterLabels.push_back(cluster);
    }
    
    // Create centroids
    std::vector<std::vector<double> > centroids;
    
    std::vector<double> centroid1;
    centroid1.push_back(3.0);
    centroid1.push_back(3.0);
    centroids.push_back(centroid1);
    
    std::vector<double> centroid2;
    centroid2.push_back(6.0);
    centroid2.push_back(8.0);
    centroids.push_back(centroid2);
    
    std::vector<double> centroid3;
    centroid3.push_back(9.0);
    centroid3.push_back(4.0);
    centroids.push_back(centroid3);
    
    // Plot clusters
    plotter.plotClusters(clusterData, clusterLabels, centroids);
    
    // Save the result
    plotter.saveAsPNG("cluster10_test_output.png", ".");
    
    printf("Cluster10 test completed. Output saved as 'cluster10_test_output.png'.\n");
} 