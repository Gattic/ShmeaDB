#include "SuperSamplingManager.h"
#include <algorithm>

namespace shmea {

SuperSamplingManager::SuperSamplingManager(unsigned int width, unsigned int height, unsigned int factor)
    : originalWidth(width),
      originalHeight(height),
      ssaaFactor(factor > 0 ? factor : 1)
{
    // Calculate supersampled dimensions
    ssaaWidth = originalWidth * ssaaFactor;
    ssaaHeight = originalHeight * ssaaFactor;
    
    // Initialize the buffer
    initializeBuffer();
}

SuperSamplingManager::~SuperSamplingManager() {
    // Image destructor will handle cleanup
}

void SuperSamplingManager::initializeBuffer() {
    // Allocate larger buffer for supersampled rendering
    ssaaImage.Allocate(ssaaWidth, ssaaHeight);
    
    // Fill with transparent black to start
    for (unsigned int y = 0; y < ssaaHeight; ++y) {
        for (unsigned int x = 0; x < ssaaWidth; ++x) {
            ssaaImage.SetPixel(x, y, RGBA(0, 0, 0, 0xFF));
        }
    }
}

void SuperSamplingManager::downsampleToOutput(Image& outputImage) {
    // For each pixel in the output image
    for (unsigned int y = 0; y < originalHeight; ++y) {
        for (unsigned int x = 0; x < originalWidth; ++x) {
            // Accumulate color values from the supersampled region
            unsigned int r = 0, g = 0, b = 0, a = 0;
            
            // Sample the NxN region from the supersampled image
            for (unsigned int sy = 0; sy < ssaaFactor; ++sy) {
                for (unsigned int sx = 0; sx < ssaaFactor; ++sx) {
                    unsigned int ssaaX = x * ssaaFactor + sx;
                    unsigned int ssaaY = y * ssaaFactor + sy;
                    
                    // Ensure we're within bounds of the supersampled image
                    if (ssaaX < ssaaWidth && ssaaY < ssaaHeight) {
                        RGBA pixel = ssaaImage.GetPixel(ssaaX, ssaaY);
                        
                        // Pre-multiply alpha for more accurate blending
                        float alphaFactor = pixel.a / 255.0f;
                        r += static_cast<unsigned int>(pixel.r * alphaFactor);
                        g += static_cast<unsigned int>(pixel.g * alphaFactor);
                        b += static_cast<unsigned int>(pixel.b * alphaFactor);
                        a += pixel.a;
                    }
                }
            }
            
            // Calculate average color
            unsigned int totalSamples = ssaaFactor * ssaaFactor;
            
            // Handle case where alpha is zero to avoid division by zero
            if (a == 0) {
                outputImage.SetPixel(x, y, RGBA(0, 0, 0, 0));
                continue;
            }
            
            // Normalize alpha channel
            float avgAlpha = a / static_cast<float>(totalSamples);
            
            // For better accuracy with very transparent areas, ensure we don't divide by zero
            float avgAlphaFactor = avgAlpha / 255.0f;
            if (avgAlphaFactor < 0.001f) {
                avgAlphaFactor = 0.001f;
            }
            
            // Unpremultiply alpha
            unsigned char finalR = static_cast<unsigned char>(std::min(255.0f, r / static_cast<float>(totalSamples) / avgAlphaFactor));
            unsigned char finalG = static_cast<unsigned char>(std::min(255.0f, g / static_cast<float>(totalSamples) / avgAlphaFactor));
            unsigned char finalB = static_cast<unsigned char>(std::min(255.0f, b / static_cast<float>(totalSamples) / avgAlphaFactor));
            unsigned char finalA = static_cast<unsigned char>(avgAlpha);
            
            // Set the downsampled pixel in the output image
            outputImage.SetPixel(x, y, RGBA(finalR, finalG, finalB, finalA));
        }
    }
}

void SuperSamplingManager::setSuperSamplingFactor(unsigned int factor) {
    if (factor < 1) factor = 1; // Ensure factor is at least 1
    
    // Only reinitialize if the factor has changed
    if (factor != ssaaFactor) {
        ssaaFactor = factor;
        ssaaWidth = originalWidth * ssaaFactor;
        ssaaHeight = originalHeight * ssaaFactor;
        
        // Reinitialize supersampling buffer
        initializeBuffer();
    }
}

unsigned int SuperSamplingManager::getWidth() const {
    return ssaaWidth;
}

unsigned int SuperSamplingManager::getHeight() const {
    return ssaaHeight;
}

unsigned int SuperSamplingManager::getSamplingFactor() const {
    return ssaaFactor;
}

int SuperSamplingManager::scaleX(int x) const {
    return x * ssaaFactor;
}

int SuperSamplingManager::scaleY(int y) const {
    return y * ssaaFactor;
}

int SuperSamplingManager::scaleSize(int size) const {
    return size * ssaaFactor;
}

Image& SuperSamplingManager::getImage() {
    return ssaaImage;
}

const Image& SuperSamplingManager::getImage() const {
    return ssaaImage;
}

} // namespace shmea 