#!/bin/bash

# Create a build directory if it doesn't exist
mkdir -p cluster10_test_build
cd cluster10_test_build

# Copy the CMake file to the build directory
cp ../test_cluster10_cmake.txt CMakeLists.txt

# Run CMake
cmake .

# Build the test
make -j$(nproc)

# Run the test
./test_cluster10

# Display the generated images
echo "Tests completed. The generated PNG files are in: $(pwd)"
ls -lh *.png 